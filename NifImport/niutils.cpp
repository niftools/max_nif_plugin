/**********************************************************************
*<
FILE: NIUtils.cpp

DESCRIPTION:	NifImporter Utilities

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "niutils.h"
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <malloc.h>
#include <sstream>
#include <modstack.h>
#include <iskin.h>
#ifdef USE_BIPED
#  include <cs/Biped8Api.h>
#  include <cs/OurExp.h> 
#endif
using namespace std;
using namespace Niflib;

// Macro to create a dynamically allocated strdup on the stack
#define STRDUPA(p) (_tcscpy((TCHAR*)alloca((_tcslen(p)+1)*sizeof(*p)),p))

// sprintf for TSTR without having to worry about buffer size.
TSTR FormatText(const TCHAR* format,...)
{
   TCHAR buffer[512];
   TSTR text;
   va_list args;
   va_start(args, format);
   int nChars = _vsntprintf(buffer, _countof(buffer), format, args);
   if (nChars != -1) {
      text = buffer;
   } else {
      size_t Size = _vsctprintf(format, args);
      text.Resize(Size);
      nChars = _vsntprintf(text.data(), Size, format, args);
   }
   va_end(args);
   return text;
}

// sprintf for std::string without having to worry about buffer size.
std::string FormatString(const TCHAR* format,...)
{
   TCHAR buffer[512];
   std::string text;
   va_list args;
   va_start(args, format);
   int nChars = _vsntprintf(buffer, _countof(buffer), format, args);
   if (nChars != -1) {
      text = buffer;
   } else {
      size_t Size = _vsctprintf(format, args);
      TCHAR* pbuf = (TCHAR*)_alloca(Size);
      nChars = _vsntprintf(pbuf, Size, format, args);
      text = pbuf;
   }
   va_end(args);
   return text;
}

// Tokenize a string using strtok and return it as a stringlist
stringlist TokenizeString(LPCTSTR str, LPCTSTR delims)
{
   stringlist values;
   LPTSTR buf = STRDUPA(str);
   for (LPTSTR p = _tcstok(buf, delims); p && *p; p = _tcstok(NULL, delims)){
      values.push_back(string(p));
   }
   return values;
}

// Parse and ini file section and return the results as s NameValueCollection.
NameValueCollection ReadIniSection(LPCTSTR Section, LPCTSTR iniFileName )
{
   NameValueCollection map;
   DWORD len = 2048 * sizeof(TCHAR);
   LPTSTR buf = (LPTSTR)calloc(len+2, 1);
   while(NULL != buf) {
      DWORD rlen = GetPrivateProfileSection(Section, buf, len, iniFileName);
      if (rlen != (len-2)) break;
      len += 2;
      buf = (LPTSTR)realloc(buf, len);
   }
   if (NULL != buf) {
      for (LPTSTR line = buf, next = line + strlen(line) + 1; *line; line = next, next = line + strlen(line) + 1){
         Trim(line);
         if (line[0] == ';' || line[0] == 0) 
            continue;
         if (LPTSTR equals = _tcschr(line, TEXT('='))){
            *equals++ = 0;
            Trim(line), Trim(equals);
            map[string(line)] = string(equals);
         }
      }
   }
   return map;
}

// Expand Qualifiers in string using a ${Name} syntax.  Name will be looked up in the
//    NameValueCollection and expand in place.  Missing names will expand to empty.
//    - Please dont give self-referential strings
string ExpandQualifiers(const string& src, const NameValueCollection& map)
{
   string value;
   bool hasPercent = false;
   string::size_type end = src.length();
   value.reserve(src.length());
   for (string::size_type i = 0; i < end; ++i) {
      if (src[i] == TEXT('$')) {
         if (++i < end) {
            if (src[i] == TEXT('$')){
               value.append(1, src[i]);
            } else if (src[i] == TEXT('{')) {
               string::size_type term = src.find_first_of(TEXT('}'), i);
               if (term == string::npos) {
                  i = end;
               } else {
                  string key = src.substr(i+1, term-i-1);
                  NameValueCollection::const_iterator kvp = map.find(key);
                  if (kvp != map.end()) {
                     value.append(ExpandQualifiers(kvp->second, map));
                  } 
                  i = term;
               }
            } else if (src[i] == TEXT('(')) {
               string::size_type term = src.find_first_of(TEXT(')'), i);
               if (term == string::npos) {
                  i = end;
               } else {
                  string key = src.substr(i+1, term-i-1);
                  NameValueCollection::const_iterator kvp = map.find(key);
                  if (kvp != map.end()) {
                     value.append(ExpandQualifiers(kvp->second, map));
                  } 
                  i = term;
               }
            }
         } else {
            value.append(1, TEXT('$'));
         }
      } else {
         value.append(1, src[i]);
      }
   }
   return value;
}

// Call ExpandEnvironmentStrings but with std string instead
string ExpandEnvironment(const string& src)
{
   DWORD Size = ExpandEnvironmentStrings(src.c_str(), NULL, 0);
   if (TCHAR* pbuf = (TCHAR*)_alloca(Size+2)) {
      pbuf[0] = 0;
      ExpandEnvironmentStrings(src.c_str(), pbuf, Size+2);
      return string(pbuf);
   }
   return src;
}


// Helper struct and method for dealing with standard registry handles
struct str2hdl
{
   const TCHAR *str;
   HANDLE key;
};
const static struct str2hdl RegKeyMap[] = {
   {TEXT("HKLM"), HKEY_LOCAL_MACHINE},
   {TEXT("HKCU"), HKEY_CURRENT_USER},
   {TEXT("HKCR"), HKEY_CLASSES_ROOT},
   {TEXT("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
   {TEXT("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
   {TEXT("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
};
static HANDLE GetRegKey(LPCTSTR key) {
   for (int i=0; i<_countof(RegKeyMap); ++i)
      if (0 == _tcscmp(RegKeyMap[i].str, key))
         return RegKeyMap[i].key;
   return 0;
}

// Returns value from indirect source
//  Currently only supports registry string values using '[HKEY\Key]=@"Value"' 
//  where  HKEY is HKLM,HKCU,HKCR  
//         Key is the registry key to lookup
//         Value is the data value to lookup.  
string GetIndirectValue(LPCSTR path)
{
   if (!path || !*path)
      return string();
   string value;
   LPTSTR p = STRDUPA(path);
   Trim(p);
   if (*p == '['){
      LPTSTR end = _tcschr(++p, ']');
      if (end != NULL) {
         *end++ = 0;
         // Advance unsafely past unnecessary qualifiers
         LPTSTR valueName = end;
         end = valueName + strlen(end) - 1;
         if (*valueName == '=') ++valueName;
         if (*valueName == '@') ++valueName;
         if (*valueName == '\"' || *valueName == '\'') ++valueName;
         if (*end == '\"' || *end == '\'') *end-- = 0;
         Trim(valueName);

         LPTSTR keyEnd = _tcschr(p, '\\');
         if (keyEnd != NULL) {
            *keyEnd++ = 0;
            HANDLE hRoot = GetRegKey(p);
            if (hRoot != 0) {
               HKEY hKey = NULL;
               if (ERROR_SUCCESS == RegOpenKeyEx((HKEY)hRoot, keyEnd, 0, KEY_READ, &hKey)) {
                  BYTE buffer[MAX_PATH*sizeof(*path)];
                  DWORD dwLen = _countof(buffer);
                  if (ERROR_SUCCESS == RegQueryValueEx(hKey, valueName, NULL, NULL, (LPBYTE)buffer, &dwLen) && dwLen > 0) {
                     value = (TCHAR*)buffer;
                  }
                  RegCloseKey(hKey);
               }
            }
         }
      }
   } else {
      value = path;
   }
   return value;
}

// Original Source: Jack Handy www.codeproject.com
int wildcmp(const TCHAR *wild, const TCHAR *string) {
   const TCHAR *cp, *mp;

   while ((*string) && (*wild != '*')) {
      if ((*wild != *string) && (*wild != '?')) {
         return 0;
      }
      wild++;
      string++;
   }

   while (*string) {
      if (*wild == '*') {
         if (!*++wild) {
            return 1;
         }
         mp = wild;
         cp = string+1;
      } else if ((*wild == *string) || (*wild == '?')) {
         wild++;
         string++;
      } else {
         wild = mp;
         string = cp++;
      }
   }

   while (*wild == '*') {
      wild++;
   }
   return !*wild;
}

// Same as above but case insensitive
int wildcmpi(const TCHAR *wild, const TCHAR *string) {
   const TCHAR *cp, *mp;
   int f,l;
   while ((*string) && (*wild != '*')) {
      f = _totlower( (_TUCHAR)(*string) );
      l = _totlower( (_TUCHAR)(*wild) );
      if ((f != l) && (l != '?')) {
         return 0;
      }
      wild++, string++;
   }
   while (*string) {
      if (*wild == '*') {
         if (!*++wild) return 1;
         mp = wild, cp = string+1;
      } else {
         f = _totlower( (_TUCHAR)(*string) );
         l = _totlower( (_TUCHAR)(*wild) );
         if ((f == l) || (l == '?')) {
            wild++, string++;
         } else {
            wild = mp, string = cp++;
         }
      }
   }
   while (*wild == '*') wild++;
   return !*wild;
}

bool wildmatch(const string& match, const std::string& value) 
{
   return (wildcmpi(match.c_str(), value.c_str())) ? true : false;
}

bool wildmatch(const stringlist& matches, const std::string& value)
{
   for (stringlist::const_iterator itr=matches.begin(), end=matches.end(); itr != end; ++itr){
      if (wildcmpi((*itr).c_str(), value.c_str()))
         return true;
   }
   return false;
}

//! Renames Max Node if it exists
void RenameNode(Interface *gi, LPCTSTR SrcName, LPCTSTR DstName)
{
   INode *node = gi->GetINodeByName(SrcName);
   if (node != NULL) node->SetName(const_cast<LPTSTR>(DstName));
}

void PosRotScaleNode(INode *n, Matrix3& m3, PosRotScale prs, TimeValue t)
{
   Point3 p = m3.GetTrans();
   Quat q = m3;
   PosRotScaleNode(n, p, q, 1.0f, prs, t);
}

// Set Position and Rotation on a standard controller will need to handle bipeds
//   Always in World Transform coordinates
void PosRotScaleNode(INode *n, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t)
{
   if (Control *c = n->GetTMController()) {
      ScaleValue sv(Point3(s,s,s));
#ifdef USE_BIPED
      // Bipeds are special.  And will crash if you dont treat them with care
      if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
         ||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
         ||(c->ClassID() == FOOTPRINT_CLASS_ID))
      {
         // Get the Biped Export Interface from the controller 
         //IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);
         IOurBipExport *BipIface = (IOurBipExport *) c->GetInterface(I_OURINTERFACE);
         if (prs & prsScale)
            BipIface->SetBipedScale(sv, t, n);
         if (prs & prsRot)
            BipIface->SetBipedRotation(q, t, n, 0/*???*/);
         if (prs & prsPos)
            BipIface->SetBipedPosition(p, t, n);
      }
      else
#endif
      {
         if (prs & prsScale)
            if (Control *sclCtrl = c->GetScaleController())
               sclCtrl->SetValue(t, &sv, 1, CTRL_ABSOLUTE);
         if (prs & prsRot)
            if (Control *rotCtrl = c->GetRotationController())
               rotCtrl->SetValue(t, &q, 1, CTRL_ABSOLUTE);
         if (prs & prsPos)
            if (Control *posCtrl = c->GetPositionController())
               posCtrl->SetValue(t, &p, 1, CTRL_ABSOLUTE);
      }
   }
}

// Search NiNode collection for a specific name
NiNodeRef FindNodeByName( const vector<NiNodeRef>& blocks, const string& name )
{
   for (vector<NiNodeRef>::const_iterator itr = blocks.begin(), end = blocks.end(); itr != end; ++itr)
   {
      const NiNodeRef& block = (*itr);
      if (name == block->GetName())
         return block;
   }
   return NiNodeRef();
}

// Search NiNode collection names that match a wildcard 
vector<NiNodeRef> SelectNodesByName( const vector<NiNodeRef>& blocks, LPCTSTR match)
{
   vector<NiNodeRef> nodes;
   for (vector<NiNodeRef>::const_iterator itr = blocks.begin(), end = blocks.end(); itr != end; ++itr)
   {
      const NiNodeRef& block = (*itr);
      if (wildcmpi(match, block->GetName().c_str()))
         nodes.insert(nodes.end(), block);
   }
   return nodes;
}

// Count number of NiNodes that match a wildcard 
int CountNodesByName( const vector<NiNodeRef>& blocks, LPCTSTR match )
{
   int count = 0;
   for (vector<NiNodeRef>::const_iterator itr = blocks.begin(), end = blocks.end(); itr != end; ++itr) {
      const NiNodeRef& block = (*itr);
      if (wildcmpi(match, block->GetName().c_str()))
         ++count;
   }
   return count;
}

// Get a vector of names from an NiNode vector
vector<string> GetNamesOfNodes( const vector<Niflib::NiNodeRef>& nodes )
{
   vector<string> slist;
   for (vector<NiNodeRef>::const_iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr) {
      const NiNodeRef& block = (*itr);
      slist.push_back(block->GetName());
   }
   return slist;
}

// Recursively search through directories applying a filter on what to return
template <typename FileMatch>
void BuildFileNameMap(NameValueCollection & collection, const TCHAR *root, const TCHAR *path, FileMatch pred)
{
   TCHAR buffer[MAX_PATH], buffer2[MAX_PATH], search[MAX_PATH];
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   ZeroMemory(&FindFileData, sizeof(FindFileData));
   if (path == NULL || path[0] == 0)
      return;
   PathCanonicalize(search, path);
   PathAddBackslash(search);
   _tcscat(search, "*");

   hFind = FindFirstFile(search, &FindFileData);
   if (hFind != INVALID_HANDLE_VALUE) {
      stringlist list;
      for (BOOL ok = TRUE ; ok ; ok = FindNextFile(hFind, &FindFileData)) {
         if (FindFileData.cFileName[0] == '.' || (FindFileData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)))
            continue;
         if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            PathCombine(buffer, path, FindFileData.cFileName);
            PathAddBackslash(buffer);
            list.push_back(buffer);
         } else {
            if (pred(FindFileData.cFileName)) {
               if (collection.find(FindFileData.cFileName) == collection.end()) {
                  PathCombine(buffer, path, FindFileData.cFileName);
                  GetLongPathName(buffer, buffer, MAX_PATH);
                  PathRemoveExtension(FindFileData.cFileName);
                  PathRelativePathTo(buffer2, root, FILE_ATTRIBUTE_DIRECTORY, buffer, FILE_ATTRIBUTE_NORMAL);
                  TCHAR *p = buffer2; while (*p == '\\') ++p;
                  collection.insert(KeyValuePair(FindFileData.cFileName, p));					
               }
            }
         }
      }
      FindClose(hFind);
      for (stringlist::iterator itr = list.begin(), end = list.end(); itr != end; ++itr) {
         BuildFileNameMap(collection, root, (*itr).c_str(), pred);
      }
   }
}
// Implementation for BuildFileNameMap which will search for a specific set of extensions
struct ExtensionMatch : public std::unary_function<LPCTSTR, bool>
{
   stringlist extns;
   ExtensionMatch(string extnlist) {
      extns = TokenizeString(extnlist.c_str(), ";");
   }
   ExtensionMatch(const stringlist& extnlist) : extns(extnlist) {
   }
   bool operator()(LPCTSTR name) const {
      LPCSTR ext = PathFindExtension(name);
      for (stringlist::const_iterator itr = extns.begin(), end = extns.end(); itr != end; ++itr) {
         if (0 == _tcsicmp(ext, (*itr).c_str()))
            return true;
      }
      return false;      
   }
};

// Run through the search paths and add them to the image collection
void FindImages(NameValueCollection& images, const string& rootPath, const stringlist& searchpaths, const stringlist& extensions)
{
   ExtensionMatch ddsMatch(extensions);
   for (stringlist::const_iterator itr = searchpaths.begin(), end = searchpaths.end(); itr != end; ++itr) {
      if (PathIsRelative((*itr).c_str()))
      {
         TCHAR texPath[MAX_PATH];
         PathCombine(texPath, rootPath.c_str(), (*itr).c_str());
         PathAddBackslash(texPath);
         BuildFileNameMap(images, rootPath.c_str(), texPath, ddsMatch);
      }
      else
      {
         BuildFileNameMap(images, rootPath.c_str(), (*itr).c_str(), ddsMatch);
      }
   }
}


// Debugger Trace Window Utilities
TSTR PrintMatrix3(Matrix3& m)
{
   Point3 pt = m.GetTrans();
   float y,p,r;
   m.GetYawPitchRoll(&y,&p,&r);
   return FormatText("Matrix3: [%g,%g,%g] <%g,%g,%g>\n"
      ,pt.x, pt.y, pt.z
      ,TODEG(y), TODEG(p), TODEG(r)
      );
}

void DumpMatrix3(Matrix3& m)
{
   OutputDebugString(PrintMatrix3(m));
}

TSTR PrintMatrix44(Matrix44& m)
{
   Vector3 p; Matrix33 rot; float sc;
   m.Decompose(p, rot, sc);
   Quaternion q = rot.AsQuaternion();
   Float3 f = q.AsEulerYawPitchRoll();

   return FormatText("Matrix3: [%g,%g,%g] <%g,%g,%g> (%g)\n"
      ,p.x, p.y, p.z
      ,TODEG(f[0]), TODEG(f[1]), TODEG(f[2])
      ,sc
      );
}
void DumpMatrix44(Matrix44& m)
{
   OutputDebugString(PrintMatrix44(m));
}