/**********************************************************************
*<
FILE: NIUtils.h

DESCRIPTION:	NifImporter Utilities

CREATED BY: tazpn (Theo)

HISTORY: 

INFO: See Implementation for minimalist comments

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef _NIUTILS_H_
#define _NIUTILS_H_

#ifndef _WINDOWS_
#  include <windows.h>
#endif
#include <tchar.h>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <map>

// Max Headers
#include <Max.h>
#include <strclass.h>
#include <color.h>

// Niflib Headers
#include <obj\NiNode.h>
#include <nif_math.h>

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

extern int wildcmp(const TCHAR *wild, const TCHAR *string);
extern int wildcmpi(const TCHAR *wild, const TCHAR *string);

// Trim whitespace before and after a string
inline TCHAR *Trim(TCHAR*&p) { 
   while(_istspace(*p)) *p++ = 0; 
   TCHAR *e = p + _tcslen(p) - 1;
   while (e > p && _istspace(*e)) *e-- = 0;
   return p;
}

// Case insensitive string equivalence test for collections
struct ltstr
{
   bool operator()(const TCHAR* s1, const TCHAR* s2) const
   { return _tcsicmp(s1, s2) < 0; }

   bool operator()(const std::string& s1, const std::string& s2) const
   { return s1.compare(s2) < 0; }

   bool operator()(const std::string& s1, const TCHAR * s2) const
   { return s1.compare(s2) < 0; }

   bool operator()(const TCHAR * s1, const std::string& s2) const
   { return s2.compare(s1) >= 0; }
};

// Case insensitive string equivalence but numbers are sorted together
struct NumericStringEquivalence
{
   bool operator()(const TCHAR* s1, const TCHAR* s2) const
   { return numstrcmp(s1, s2) < 0; }

   bool operator()(const std::string& s1, const TCHAR* s2) const
   { return numstrcmp(s1.c_str(), s2) < 0; }

   bool operator()(const TCHAR* s1, const std::string& s2) const
   { return numstrcmp(s1, s2.c_str()) < 0; }

   bool operator()(const std::string& s1, const std::string& s2) const
   { return numstrcmp(s1.c_str(), s2.c_str()) < 0; }

   static int numstrcmp(const TCHAR *str1, const TCHAR *str2)
   {
      TCHAR *p1, *p2;
      int c1, c2, lcmp;
      for(;;)
      {
         c1 = tolower(*str1), c2 = tolower(*str2);
         if ( c1 == 0 || c2 == 0 )
            break;
         else if (isdigit(c1) && isdigit(c2))
         {			
            lcmp = strtol(str1, &p1, 10) - strtol(str2, &p2, 10);
            if ( lcmp == 0 )
               lcmp = (p2 - str2) - (p1 - str1);
            if ( lcmp != 0 )
               return (lcmp > 0 ? 1 : -1);
            str1 = p1, str2 = p2;
         }
         else
         {
            lcmp = (c1 - c2);
            if (lcmp != 0)
               return (lcmp > 0 ? 1 : -1);
            ++str1, ++str2;
         }
      }
      lcmp = (c1 - c2);
      return ( lcmp < 0 ) ? -1 : (lcmp > 0 ? 1 : 0);
   }
};

// Generic IniFile reading routine
template<typename T>
inline T GetIniValue(LPCTSTR Section, LPCTSTR Setting, T Default, LPCTSTR iniFileName){
   T v;
   TCHAR buffer[1024];
   stringstream sstr;
   sstr << Default;
   buffer[0] = 0;
   if (0 < GetPrivateProfileString(Section, Setting, sstr.str().c_str(), buffer, sizeof(buffer), iniFileName)){
      stringstream sstr(buffer);
      sstr >> v;
      return v;
   }
   return Default;
}

// Specific override for int values
template<>
inline int GetIniValue<int>(LPCTSTR Section, LPCTSTR Setting, int Default, LPCTSTR iniFileName){
   return GetPrivateProfileInt(Section, Setting, Default, iniFileName);
}

// Specific override for string values
template<>
inline std::string GetIniValue<std::string>(LPCTSTR Section, LPCTSTR Setting, std::string Default, LPCTSTR iniFileName){
   TCHAR buffer[1024];
   buffer[0] = 0;
   if (0 < GetPrivateProfileString(Section, Setting, Default.c_str(), buffer, sizeof(buffer), iniFileName)){
      return std::string(buffer);
   }
   return Default;
}

// Specific override for TSTR values
template<>
inline TSTR GetIniValue<TSTR>(LPCTSTR Section, LPCTSTR Setting, TSTR Default, LPCTSTR iniFileName){
   TCHAR buffer[1024];
   buffer[0] = 0;
   if (0 < GetPrivateProfileString(Section, Setting, Default.data(), buffer, sizeof(buffer), iniFileName)){
      return TSTR(buffer);
   }
   return Default;
}

// Generic IniFile reading routine
template<typename T>
inline void SetIniValue(LPCTSTR Section, LPCTSTR Setting, T value, LPCTSTR iniFileName){
   stringstream sstr;
   sstr << value;
   WritePrivateProfileString(Section, Setting, sstr.str().c_str(), iniFileName);
}

// Specific override for string values
template<>
inline void SetIniValue<std::string>(LPCTSTR Section, LPCTSTR Setting, std::string value, LPCTSTR iniFileName){
   WritePrivateProfileString(Section, Setting, value.c_str(), iniFileName);
}

// Specific override for TSTR values
template<>
inline void SetIniValue<TSTR>(LPCTSTR Section, LPCTSTR Setting, TSTR value, LPCTSTR iniFileName){
   WritePrivateProfileString(Section, Setting, value.data(), iniFileName);
}

// Common collections that I use
typedef std::map<std::string, std::string, ltstr> NameValueCollection;
typedef std::pair<std::string, std::string> KeyValuePair;
typedef std::list<std::string> stringlist;

extern TSTR FormatText(const TCHAR* format,...);
extern std::string FormatString(const TCHAR* format,...);

extern stringlist TokenizeString(LPCTSTR str, LPCTSTR delims);
extern string GetIndirectValue(LPCSTR path);
extern NameValueCollection ReadIniSection(LPCTSTR Section, LPCTSTR iniFileName );
extern string ExpandQualifiers(const string& src, const NameValueCollection& map);
extern string ExpandEnvironment(const string& src);

extern void FindImages(NameValueCollection& images, const string& rootPath, const stringlist& searchpaths, const stringlist& extensions);

extern void RenameNode(Interface *gi, LPCTSTR SrcName, LPCTSTR DstName);
extern TriObject* GetTriObject(Object *o);
extern Modifier *GetSkin(INode *node);

enum PosRotScale
{
   prsPos = 0x1,
   prsRot = 0x2,
   prsScale = 0x4,
   prsDefault = prsPos | prsRot | prsScale,
};
extern void PositionAndRotateNode(INode *n, Point3 p, Quat& q, PosRotScale prs = prsDefault, TimeValue t = 0);

extern Niflib::NiNodeRef FindNodeByName( const vector<Niflib::NiNodeRef>& blocks, const string& name );
extern std::vector<Niflib::NiNodeRef> SelectNodesByName( const vector<Niflib::NiNodeRef>& blocks, LPCTSTR match);
extern int CountNodesByName( const vector<Niflib::NiNodeRef>& blocks, LPCTSTR match );
extern std::vector<std::string> GetNamesOfNodes( const vector<Niflib::NiNodeRef>& blocks );

// Simple conversion helpers
static inline float TODEG(float x) { return x * 180.0f / PI; }
static inline float TORAD(float x) { return x * PI / 180.0f; }

static inline Color TOCOLOR(const Niflib::Color3& c3) {
   return Color(c3.r, c3.g, c3.b);
}

static inline Matrix3 TOMATRIX3(const Niflib::Matrix44 &tm, bool invert = true){
   Niflib::Vector3 pos; Niflib::Matrix33 rot; float scale;
   tm.Decompose(pos, rot, scale);
   Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3());
   if (invert) m.Invert();
   m.SetTrans(Point3(pos.x, pos.y, pos.z));
   return m;
}

static inline Quat TOQUAT(const Niflib::Quaternion& q){
   return Quat(q.x, q.y, q.z, q.w);
}

#endif // _NIUTILS_H_