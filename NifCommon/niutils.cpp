/**********************************************************************
*<
FILE: NIUtils.cpp

DESCRIPTION:	NifImporter Utilities

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
#include "niutils.h"
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <malloc.h>
#include <sstream>
#include <modstack.h>
#include <iparamb2.h>
#include <iskin.h>
#include "../NifProps/bhkRigidBodyInterface.h"

#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#  include <cs/OurExp.h> 
#endif
#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
#include "maxscrpt\Strings.h"
#include "maxscrpt\Parser.h"
#else
#include <maxscript/maxscript.h>
#include <maxscript/compiler/parser.h>
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


// routine for parsing white space separated lines.  Handled like command line parameters w.r.t quotes.
static void parse_line (
                        const char *start,
                        char **argv,
                        char *args,
                        int *numargs,
                        int *numchars
                        )
{
   const char NULCHAR    = '\0';
   const char SPACECHAR  = ' ';
   const char TABCHAR    = '\t';
   const char RETURNCHAR = '\r';
   const char LINEFEEDCHAR = '\n';
   const char DQUOTECHAR = '\"';
   const char SLASHCHAR  = '\\';
   const char *p;
   int inquote;                    /* 1 = inside quotes */
   int copychar;                   /* 1 = copy char to *args */
   unsigned numslash;              /* num of backslashes seen */

   *numchars = 0;
   *numargs = 0;                   /* the program name at least */

   p = start;

   inquote = 0;

   /* loop on each argument */
   for(;;) 
   {
      if ( *p ) { while (*p == SPACECHAR || *p == TABCHAR || *p == RETURNCHAR || *p == LINEFEEDCHAR) ++p; }

      if (*p == NULCHAR) break; /* end of args */

      /* scan an argument */
      if (argv)
         *argv++ = args;     /* store ptr to arg */
      ++*numargs;

      /* loop through scanning one argument */
      for (;;) 
      {
         copychar = 1;
         /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
         2N+1 backslashes + " ==> N backslashes + literal "
         N backslashes ==> N backslashes */
         numslash = 0;
         while (*p == SLASHCHAR) 
         {
            /* count number of backslashes for use below */
            ++p;
            ++numslash;
         }
         if (*p == DQUOTECHAR) 
         {
            /* if 2N backslashes before, start/end quote, otherwise copy literally */
            if (numslash % 2 == 0) {
               if (inquote) {
                  if (p[1] == DQUOTECHAR)
                     p++;    /* Double quote inside quoted string */
                  else        /* skip first quote char and copy second */
                     copychar = 0;
               } else
                  copychar = 0;       /* don't copy quote */

               inquote = !inquote;
            }
            numslash /= 2;          /* divide numslash by two */
         }

         /* copy slashes */
         while (numslash--) {
            if (args)
               *args++ = SLASHCHAR;
            ++*numchars;
         }

         /* if at end of arg, break loop */
         if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR || *p == RETURNCHAR || *p == LINEFEEDCHAR)))
            break;

         /* copy character into argument */
         if (copychar) 
         {
            if (args)
               *args++ = *p;
            ++*numchars;
         }
         ++p;
      }

      /* null-terminate the argument */
      if (args)
         *args++ = NULCHAR;          /* terminate string */
      ++*numchars;
   }
   /* We put one last argument in -- a null ptr */
   if (argv)
      *argv++ = NULL;
   ++*numargs;
}

// Tokenize a string using strtok and return it as a stringlist
stringlist TokenizeString(LPCTSTR str, LPCTSTR delims, bool trim)
{
   stringlist values;
   LPTSTR buf = STRDUPA(str);
   for (LPTSTR p = _tcstok(buf, delims); p && *p; p = _tcstok(NULL, delims)){
      values.push_back(string((trim) ? Trim(p) : p));
   }
   return values;
}

// Tokenize a string using strtok and return it as a stringlist
stringlist TokenizeCommandLine(LPCTSTR str, bool trim)
{
   stringlist values;
   int nargs = 0, nchars = 0;
   parse_line( str, NULL, NULL, &nargs, &nchars);
   char **largv = (char **)_alloca(nargs * sizeof(TCHAR *) + nchars * sizeof(TCHAR));
   parse_line( str, largv, ((TCHAR*)largv) + nargs * sizeof(TCHAR*), &nargs, &nchars);
   for (int i=0; i<nargs; ++i) {
      LPTSTR p = largv[i];
      if (p == NULL) continue;
      values.push_back(string((trim) ? Trim(p) : p));
   }
   return values;
}

string JoinCommandLine(stringlist args)
{
   std::stringstream str;
   for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
      if (itr != args.begin()) str << ' ';
      str << (*itr);
   }
   return str.str();
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
		 if (strlen(valueName) == 0)
			 valueName = NULL;

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

bool wildmatch(const TCHAR* match, const TCHAR* value)
{
   return (wildcmpi(match, value)) ? true : false;
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

Point3 TOEULER(const Matrix3 &m)
{
   Point3 rv(0.0f, 0.0f, 0.0f);
   if ( m.GetRow(2)[0] < 1.0 )
   {
      if ( m.GetRow(2)[0] > - 1.0 )
      {
         rv[2] = atan2( - m.GetRow(1)[0], m.GetRow(0)[0] );
         rv[1] = asin( m.GetRow(2)[0] );
         rv[0] = atan2( - m.GetRow(2)[1], m.GetRow(2)[2] );
      }
      else
      {
         rv[2] = - atan2( - m.GetRow(1)[2], m.GetRow(1)[1] );
         rv[1] = - PI / 2;
         rv[0] = 0.0;
      }
   }
   else
   {
      rv[2] = atan2( m.GetRow(1)[2], m.GetRow(1)[1] );
      rv[1] = PI / 2;
      rv[0] = 0.0;
   }
   return rv;
}

inline Point3 TODEG(const Point3& p){
   return Point3(TODEG(p[0]), TODEG(p[1]), TODEG(p[2]));
}

inline Point3 TORAD(const Point3& p){
   return Point3(TORAD(p[0]), TORAD(p[1]), TORAD(p[2]));
}

inline TSTR TOSTRING(const Point3& p) {
   return FormatText("[%g,%g,%g]", p[0], p[1], p[2]);
}

inline TSTR TOSTRING(float p) {
   return FormatText("%g", p);
}

inline TSTR TOSTRING(const Matrix3& m) {
   return TOSTRING( TODEG( TOEULER(m) ) );
}

inline TSTR TOSTRING(const Quat& q) {
   Matrix3 m; q.MakeMatrix(m);
   return TOSTRING( m );
}

void PosRotScaleNode(INode *n, Matrix3& m3, PosRotScale prs, TimeValue t)
{
   Point3 p = m3.GetTrans();
   Quat q = m3;
   Matrix3 stm = m3 * Inverse(m3);
   float s = (sqrt(stm.GetRow(0)[0]) + sqrt(stm.GetRow(1)[1]) + sqrt(stm.GetRow(1)[1])) / 3.0f;
   PosRotScaleNode(n, p, q, s, prs, t);
}

// Set Position and Rotation on a standard controller will need to handle bipeds
//   Always in World Transform coordinates
void PosRotScaleNode(INode *n, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t)
{
   if (Control *c = n->GetTMController()) {
      if (prs & prsRot && q.w == FloatNegINF) prs = PosRotScale(prs & ~prsRot);
      if (prs & prsPos && p.x == FloatNegINF) prs = PosRotScale(prs & ~prsPos);
      if (prs & prsScale && s == FloatNegINF) prs = PosRotScale(prs & ~prsScale);
#ifdef USE_BIPED
      // Bipeds are special.  And will crash if you dont treat them with care
      if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
         ||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
         ||(c->ClassID() == FOOTPRINT_CLASS_ID))
      {
         ScaleValue sv(Point3(s,s,s));
         // Get the Biped Export Interface from the controller 
         //IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);
         IOurBipExport *BipIface = (IOurBipExport *) c->GetInterface(I_OURINTERFACE);
         if (prs & prsScale)
            BipIface->SetBipedScale(sv, t, n);
         if (prs & prsRot)
            BipIface->SetBipedRotation(q, t, n, 0/*???*/);
         if (prs & prsPos)
            BipIface->SetBipedPosition(p, t, n);
         return;
      }
#endif
      PosRotScaleNode(c, p, q, s, prs, t);

//#ifdef _DEBUG
//      static TSTR sEmpty = "<Empty>";
//      TSTR spos = (prs & prsPos) ? TOSTRING(p) : sEmpty;
//      TSTR srot = (prs & prsRot) ? TOSTRING(q) : sEmpty;
//      TSTR sscl = (prs & prsScale) ? TOSTRING(s) : sEmpty;
//      OutputDebugString(FormatText("Transform(%s, %s, %s, %s)\n", n->GetName(), spos.data(), srot.data(), sscl.data()));
//#endif
   }
}

void PosRotScaleNode(Control *c, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t)
{
   if (c) {
      if (prs & prsRot && q.w == FloatNegINF) prs = PosRotScale(prs & ~prsRot);
      if (prs & prsPos && p.x == FloatNegINF) prs = PosRotScale(prs & ~prsPos);
      if (prs & prsScale && s == FloatNegINF) prs = PosRotScale(prs & ~prsScale);

      ScaleValue sv(Point3(s,s,s));
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
                  if (PathRelativePathTo(buffer2, root, FILE_ATTRIBUTE_DIRECTORY, buffer, FILE_ATTRIBUTE_NORMAL))
				  {
					  TCHAR *p = buffer2; while (*p == '\\') ++p;
					  collection.insert(KeyValuePair(FindFileData.cFileName, p));					
				  }
				  else
				  {
					  collection.insert(KeyValuePair(FindFileData.cFileName, buffer));
				  }
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

INode* FindINode(Interface *i, const string& name)
{
   return i->GetINodeByName(name.c_str());
}

INode* FindINode(Interface *i, NiObjectNETRef node)
{
   if (node != NULL)
   {
      return FindINode(i, node->GetName());
   }
   return NULL;
}

Matrix3 GetNodeLocalTM(INode *n, TimeValue t)
{
   Matrix3 m3 = n->GetNodeTM(t);
   Matrix3 m3p = n->GetParentTM(t);
   m3p.Invert();
   return m3 * m3p;
}

// Locate a TriObject in an Object if it exists
TriObject* GetTriObject(Object *o)
{
   if (o && o->CanConvertToType(triObjectClassID))
      return (TriObject *)o->ConvertToType(0, triObjectClassID);
   while (o->SuperClassID() == GEN_DERIVOB_CLASS_ID && o)
   {
      IDerivedObject* dobj = (IDerivedObject *)(o);
      o = dobj->GetObjRef();
      if (o && o->CanConvertToType(triObjectClassID))
         return (TriObject *)o->ConvertToType(0, triObjectClassID);
   }
   return NULL;
}

// Get or Create the Skin Modifier
Modifier *GetOrCreateSkin(INode *node)
{
   Modifier *skinMod = GetSkin(node);
   if (skinMod)
      return skinMod;   
   
   IDerivedObject *dobj = CreateDerivedObject(node->GetObjectRef());
   //create a skin modifier and add it
   skinMod = (Modifier*) CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
   dobj->SetAFlag(A_LOCK_TARGET);
   dobj->AddModifier(skinMod);
   dobj->ClearAFlag(A_LOCK_TARGET);
   node->SetObjectRef(dobj);
   return skinMod;
}

Modifier *GetSkin(INode *node)
{
   Object* pObj = node->GetObjectRef();
   if (!pObj) return NULL;
   while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
   {
      IDerivedObject* pDerObj = (IDerivedObject *)(pObj);
      int Idx = 0;
      while (Idx < pDerObj->NumModifiers())
      {
         // Get the modifier. 
         Modifier* mod = pDerObj->GetModifier(Idx);
         if (mod->ClassID() == SKIN_CLASSID)
         {
            // is this the correct Physique Modifier based on index?
            return mod;
         }
         Idx++;
      }
      pObj = pDerObj->GetObjRef();
   }
   return NULL;
}


TSTR GetFileVersion(const char *fileName)
{
   TSTR retval;
   char fileVersion[MAX_PATH];
   if (fileName == NULL)
   {
      GetModuleFileName(hInstance, fileVersion, MAX_PATH);
      fileName = fileVersion;
   }
   HMODULE ver = GetModuleHandle("version.dll");
   if (!ver) ver = LoadLibrary("version.dll");
   if (ver != NULL)
   {
      DWORD (APIENTRY *GetFileVersionInfoSize)(LPCTSTR, LPDWORD) = NULL;
      BOOL (APIENTRY *GetFileVersionInfo)(LPCTSTR, DWORD, DWORD, LPVOID) = NULL;
      BOOL (APIENTRY *VerQueryValue)(const LPVOID, LPTSTR, LPVOID *, PUINT) = NULL;
      *(FARPROC*)&GetFileVersionInfoSize = GetProcAddress(ver, "GetFileVersionInfoSizeA");
      *(FARPROC*)&GetFileVersionInfo = GetProcAddress(ver, "GetFileVersionInfoA");
      *(FARPROC*)&VerQueryValue = GetProcAddress(ver, "VerQueryValueA");
      if (GetFileVersionInfoSize && GetFileVersionInfo && VerQueryValue)
      {
         DWORD vLen = 0;
         DWORD vSize = GetFileVersionInfoSize(fileName,&vLen);
         if (vSize) 
         {
            LPVOID versionInfo = malloc(vSize+1);
            if (GetFileVersionInfo(fileName,vLen,vSize,versionInfo))
            {            
               LPVOID version=NULL;
               if (VerQueryValue(versionInfo,"\\VarFileInfo\\Translation",&version,(UINT *)&vLen) && vLen==4) 
               {
                  DWORD langD = *(DWORD*)version;
                  sprintf(fileVersion, "\\StringFileInfo\\%02X%02X%02X%02X\\ProductVersion",
                     (langD & 0xff00)>>8,langD & 0xff,(langD & 0xff000000)>>24, (langD & 0xff0000)>>16);            
               }
               else 
               {
                  sprintf(fileVersion, "\\StringFileInfo\\%04X04B0\\ProductVersion", GetUserDefaultLangID());
               }
               LPCTSTR value = NULL;
               if (VerQueryValue(versionInfo,fileVersion,&version,(UINT *)&vLen))
                  value = LPCTSTR(version);
               else if (VerQueryValue(versionInfo,"\\StringFileInfo\\040904B0\\ProductVersion",&version,(UINT *)&vLen))
                  value = LPCTSTR(version);
               if (value != NULL)
               {
                  stringlist val = TokenizeString(value, ",", true);
                  if (val.size() >= 4){
                     retval = FormatText("%s.%s.%s.%s", val[0].c_str(), val[1].c_str(), val[2].c_str(), val[3].c_str());
                  }
               }
               free(versionInfo);
            }
         }
      }
   }
   return retval;
}

// Calculate bounding sphere using minimum-volume axis-align bounding box.  Its fast but not a very good fit.
void CalcAxisAlignedSphere(const vector<Vector3>& vertices, Vector3& center, float& radius)
{
   //--Calculate center & radius--//

   //Set lows and highs to first vertex
   Vector3 lows = vertices[ 0 ];
   Vector3 highs = vertices[ 0 ];

   //Iterate through the vertices, adjusting the stored values
   //if a vertex with lower or higher values is found
   for ( unsigned int i = 0; i < vertices.size(); ++i ) {
      const Vector3 & v = vertices[ i ];

      if ( v.x > highs.x ) highs.x = v.x;
      else if ( v.x < lows.x ) lows.x = v.x;

      if ( v.y > highs.y ) highs.y = v.y;
      else if ( v.y < lows.y ) lows.y = v.y;

      if ( v.z > highs.z ) highs.z = v.z;
      else if ( v.z < lows.z ) lows.z = v.z;
   }

   //Now we know the extent of the shape, so the center will be the average
   //of the lows and highs
   center = (highs + lows) / 2.0f;

   //The radius will be the largest distance from the center
   Vector3 diff;
   float dist2(0.0f), maxdist2(0.0f);
   for ( unsigned int i = 0; i < vertices.size(); ++i ) {
      const Vector3 & v = vertices[ i ];

      diff = center - v;
      dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
      if ( dist2 > maxdist2 ) maxdist2 = dist2;
   };
   radius = sqrt(maxdist2);
}

// Calculate bounding sphere using average position of the points.  Better fit but slower.
void CalcCenteredSphere(const vector<Vector3>& vertices, Vector3& center, float& radius)
{
   size_t nv = vertices.size();
   Vector3 sum;
   for (size_t i=0; i<nv; ++i)
      sum += vertices[ i ];
   center = sum / float(nv);
   radius = 0.0f;
   for (size_t i=0; i<nv; ++i){
      Vector3 diff = vertices[ i ] - center;
      float mag = diff.Magnitude();
      radius = max(radius, mag);
   }
}



static void TransformVector3(Matrix44& tm, vector<Vector3>& pts)
{
   for (vector<Vector3>::iterator itr = pts.begin(); itr != pts.end(); ++itr)
   {    
      Matrix44 m4(*itr, Matrix33::IDENTITY, 1.0f);
      Matrix44 ntm = m4 * tm;
      Vector3 v  = ntm.GetTranslation();
      (*itr) = v;
   }
}

void CollapseGeomTransform(NiNode node) {
   
}

void CollapseGeomTransform(NiTriBasedGeomRef shape)
{
   NiTriBasedGeomDataRef data = shape->GetData();
   vector<Vector3> verts = data->GetVertices();
   vector<Vector3> norms = data->GetNormals();
   int nuvsets = data->GetUVSetCount();
   vector< vector<TexCoord> > uvSets;
   uvSets.resize(nuvsets);
   for (int i=0; i<nuvsets; ++i)
      uvSets[i] = data->GetUVSet(i);

   Matrix44 ltm = shape->GetLocalTransform();
   Matrix44 invtm = ltm.Inverse();
   Matrix44 tm = ltm * invtm;
   shape->SetLocalTransform(tm);

   TransformVector3(ltm, verts);

   data->SetVertices(verts);
   data->SetNormals(norms);
   data->SetUVSetCount(nuvsets);
   for (int i=0; i<nuvsets; ++i)
      data->SetUVSet(i, uvSets[i]);
}

void CollapseGeomTransforms(vector<NiTriBasedGeomRef>& shapes)
{
   for (vector<NiTriBasedGeomRef>::iterator itr = shapes.begin(); itr != shapes.end(); ++itr) {
      CollapseGeomTransform(*itr);
   }
}

void FixNormals(vector<Triangle>& tris, vector<Vector3>& verts, vector<Vector3>& norms)
{
   if (tris.size() != norms.size())
      return;

   int n = tris.size();
   for (int i=0; i < n; ++i)
   {
      Triangle& tri = tris[i];
      Vector3 v1 = verts[tri.v1];
      Vector3 v2 = verts[tri.v2];
      Vector3 v3 = verts[tri.v3];
      Vector3 n1 = (v2-v1).CrossProduct(v3-v1).Normalized();
      Vector3 n2 = norms[i];
      float dp = n1.DotProduct(n2);
      if ( dp < 0.0f ) {
         std::swap(tri.v3, tri.v2);
      }
   }
}


static Value* LocalExecuteScript(CharStream* source, bool *res) {

   *res = true;

   init_thread_locals();
   push_alloc_frame();
   three_typed_value_locals(Parser* parser, Value* code, Value* result);
   CharStream* out = thread_local(current_stdout);
   vl.parser = new Parser (out);

   try	{

      source->flush_whitespace();
      while (!source->at_eos()) {
         vl.code		= vl.parser->compile(source);
         vl.result	= vl.code->eval()->get_heap_ptr();
         source->flush_whitespace();
      }
      source->close();

   } catch (...) {
      *res = false;
   }

   if (vl.result == NULL)
      vl.result = &ok;

   pop_alloc_frame();
   return_value(vl.result);
}

// CallMaxscript
// Send the string to maxscript 
//
void CallMaxscript(const TCHAR *s)
{
   static bool script_initialized = false;
   if (!script_initialized) {
      init_MAXScript();
      script_initialized = TRUE;
   }
   init_thread_locals();

   push_alloc_frame();
   two_typed_value_locals(StringStream* ss, Value* result);

   vl.ss = new StringStream ( const_cast<TCHAR*>(s) );
   bool res = false;
   try	{
      vl.result = LocalExecuteScript(vl.ss,&res);
   } catch (...) {
      res = false;
   }
   thread_local(current_result) = vl.result;
   thread_local(current_locals_frame) = vl.link;
   pop_alloc_frame();
}

void GetIniFileName(char *iniName)
{
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
	Interface *gi = GetCOREInterface();
#else
	Interface *gi = NULL;
#endif
	if (gi) {
		LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
		PathCombine(iniName, pluginDir, "MaxNifTools.ini");

		int forcePlugcfg = GetIniValue("System", "ForcePlugcfg", 0, iniName);

		if (forcePlugcfg == 1 || _access(iniName, 06) != 0) {
			TCHAR iniPath[MAX_PATH];
			GetModuleFileName(NULL, iniPath, MAX_PATH);
			if (LPTSTR fname = PathFindFileName(iniPath))
				*fname = 0;
			PathAddBackslash(iniPath);
			PathAppend(iniPath, "plugcfg");
			PathAppend(iniPath, "MaxNifTools.ini");

			// Use plugcfg directory ini 
			if (_access(iniPath, 06) == 0) {
				strcpy(iniName, iniPath);
			}
		}
	} else {
		GetModuleFileName(NULL, iniName, MAX_PATH);
		if (LPTSTR fname = PathFindFileName(iniName))
			*fname = 0;
		PathAddBackslash(iniName);
		PathAppend(iniName, "plugcfg");
		PathAppend(iniName, "MaxNifTools.ini");
	}

	if (_access(iniName, 06) != 0) {
		MessageBox(NULL, "MaxNifTools could not find a valid INI.  The plugin may not work correctly.\nPlease check for proper installation.", 
			"MaxNifTools", MB_OK|MB_ICONWARNING);
	}
}

Modifier *GetbhkCollisionModifier(INode* node)
{
	extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;

	Object* pObj = node->GetObjectRef();
	if (!pObj) return NULL;
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* pDerObj = (IDerivedObject *)(pObj);
		int Idx = 0;
		while (Idx < pDerObj->NumModifiers())
		{
			// Get the modifier. 
			Modifier* mod = pDerObj->GetModifier(Idx);
			if (mod->ClassID() == BHKRIGIDBODYMODIFIER_CLASS_ID)
			{
				return mod;
			}
			Idx++;
		}
		pObj = pDerObj->GetObjRef();
	}
	return NULL;
}

Modifier *CreatebhkCollisionModifier(
		  INode* node
		, int type
		, HavokMaterial material /*= HAV_MAT_STONE*/
		, OblivionLayer layer /*= OL_UNIDENTIFIED */
		, unsigned char filter /*= 0*/
		)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, PB_LAYER, PB_FILTER, };
	extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;

	Modifier *rbMod = GetbhkCollisionModifier(node);
	if (rbMod == NULL)
	{
		IDerivedObject *dobj = CreateDerivedObject(node->GetObjectRef());
		rbMod = (Modifier*) CreateInstance(OSM_CLASS_ID, BHKRIGIDBODYMODIFIER_CLASS_ID);
		dobj->SetAFlag(A_LOCK_TARGET);
		dobj->AddModifier(rbMod);
		dobj->ClearAFlag(A_LOCK_TARGET);
		node->SetObjectRef(dobj);
	}

	if (IParamBlock2* pblock2 = rbMod->GetParamBlockByID(havok_params))
	{
		pblock2->SetValue(PB_BOUND_TYPE, 0, type, 0);
		pblock2->SetValue(PB_MATERIAL, 0, material, 0);
		pblock2->SetValue(PB_LAYER, 0, layer, 0);
		pblock2->SetValue(PB_FILTER, 0, filter, 0);
	}
	return rbMod;
}

TSTR GetNodeName(INode* node)
{
	return node->GetName();
}

Matrix3 GetLocalTM(INode *node)
{
	if (INode *parent = node->GetParentNode())
	{
		Matrix3 parentTM, nodeTM;
		nodeTM = node->GetNodeTM(0);
		parent = node->GetParentNode();
		parentTM = parent->GetNodeTM(0);
		return nodeTM*Inverse(parentTM);
	}
	else
	{
		return node->GetNodeTM(0);
	}
}


// Enumeration Support
TSTR EnumToString(int value, const EnumLookupType *table) {
   for (const EnumLookupType *itr = table; itr->name != NULL; ++itr) {
      if (itr->value == value) return TSTR(itr->name);
   }
   return FormatText("%x", value);
}

int EnumToIndex(int value, const EnumLookupType *table) {
   int i = 0;
   for (const EnumLookupType *itr = table; itr->name != NULL; ++itr, ++i) {
      if (itr->value == value) return i;
   }
   return -1;
}

int StringToEnum(TSTR value, const EnumLookupType *table) {
   //Trim(value);
   if (value.isNull()) return 0;

   for (const EnumLookupType *itr = table; itr->name != NULL; ++itr) {
      if (0 == _tcsicmp(value, itr->name)) return itr->value;
   }
   char *end = NULL;
   return (int)strtol(value, &end, 0);
}

TSTR FlagsToString(int value, const EnumLookupType *table) {
   TSTR sstr;
   for (const EnumLookupType *itr = table; itr->name != NULL; ++itr) {
      if (itr->value && (itr->value & value) == itr->value) {
         if (!sstr.isNull()) sstr += " | ";
         sstr += itr->name;
         value ^= itr->value;
      }
   }
   if (value == 0 && sstr.isNull()) {
      return EnumToString(value, table);
   }
   if (value != 0) {
      if (!sstr.isNull()) sstr += "|";
      sstr += EnumToString(value, table);
   }
   return sstr;
}

int StringToFlags(TSTR value, const EnumLookupType *table) {
   int retval = 0;
   LPCTSTR start = value.data();
   LPCTSTR end = value.data() + value.Length();
   while(start < end) {
      LPCTSTR bar = _tcschr(start, '|');
      int len = (bar != NULL) ?  bar-start : end-start;
      TSTR subval = value.Substr(start-value.data(), len);
      retval |= StringToEnum(subval, table);
      start += (len + 1);
   }
   return retval;
}