#pragma once

#include <string>
#include <list>
enum INIDEFTYPE
{
   IDT_STRING,
   IDT_INT,
   IDT_FLOAT,
   IDT_BOOL,
   IDT_FILENAME,
   IDT_UNKNOWN,
};

struct INIDEF
{
   INIDEF() : ShortName(NULL), IniName(NULL), MemberAddr(0), Description(NULL), DefaultValue(NULL), ValueSize(0) {
   }

   ~INIDEF() {
      if (ValueSize && DefaultValue) {
         delete DefaultValue;
         DefaultValue = NULL;
         ValueSize = 0;
      }
   }

   template<typename U>
   INIDEF(LPCTSTR sName, LPCTSTR iName, const U& member, U default, LPCTSTR desc, INIDEFTYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr(NULL), Description(NULL), MemberType(type) {
      SetDefault(default);
   }

   template<>
   INIDEF(LPCTSTR sName, LPCTSTR iName, const std::string& member, std::string default, LPCTSTR desc, INIDEFTYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==IDT_UNKNOWN?IDT_STRING:type; 
      SetDefault(default);
   }

   template<>
   INIDEF(LPCTSTR sName, LPCTSTR iName, const int& member, int default, LPCTSTR desc, INIDEFTYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==IDT_UNKNOWN?IDT_INT:type; 
      SetDefault(default);
   }

   template<>
   INIDEF(LPCTSTR sName, LPCTSTR iName, const float& member, float default, LPCTSTR desc, INIDEFTYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==IDT_UNKNOWN?IDT_FLOAT:type; 
      SetDefault(default);
   }

   template<>
   INIDEF(LPCTSTR sName, LPCTSTR iName, const bool& member, bool default, LPCTSTR desc, INIDEFTYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==IDT_UNKNOWN?IDT_BOOL:type; 
      SetDefault(default);
   }

   template<typename U>
   void SetDefault(U default) {
      ValueSize = sizeof(U);
      DefaultValue = new U(default);
   }

   template <typename U>
   const U& GetDefault() const {
      return *(U*)DefaultValue;
   }

   LPCTSTR ShortName;
   LPCTSTR IniName;
   DWORD MemberAddr;
   INIDEFTYPE MemberType;
   LPCTSTR Description;
   LPVOID DefaultValue;
   DWORD ValueSize;
};

#define BEGIN_INI_MAP(name) \
   virtual void *GetMember(const INIDEF* memptr) const { return (void*)(((char*)this) + memptr->MemberAddr); } \
   virtual const INIDEF* GetInfDefmap() const { return InternalGetInfDefmap(); } \
   static INIDEF* InternalGetInfDefmap() { \
   const name* pThis = 0; \
   static INIDEF map[] = { \

#define BEGIN_INI_MAP_WITH_BASE(name, base) \
   virtual void *GetMember(const INIDEF* memptr) const { return (void*)(((char*)this) + memptr->MemberAddr); } \
   virtual const INIDEF* GetInfDefmap() const { return InternalGetInfDefmap(); } \
   static INIDEF* InternalGetInfDefmap() { \
   const name* pThis = 0; \
   static INIDEF map[] = { \

#define END_INI_MAP() \
   INIDEF() };\
   return map;\
}

#define ADDITEM(sName, iName, member, desc) \
   INIDEF(sName, iName, pThis->member, desc, IDT_UNKNOWN), \

#define ADDITEMEX(sName, iName, member, type, desc) \
   INIDEF(sName, iName, pThis->member, desc, type), \

struct IniFileSection
{
   virtual LPCTSTR GetName() const = 0;
   virtual const INIDEF* GetInfDefmap() const = 0;
   virtual void *GetMember(const INIDEF* memptr) const = 0;
};

typedef std::list<IniFileSection*>  IniFileSectionList;

inline int GetIniDefSectionSize(IniFileSection *section) {
   int len = 0;

   for (const INIDEF *inidef = section->GetInfDefmap()
      ; inidef != NULL && inidef->ShortName
      ; ++inidef)
      ++len;
   return len;
}