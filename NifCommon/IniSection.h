#pragma once

#include <string>
#include <list>

enum VARIABLETYPE
{
   vtUnknown,
   vtInteger,
   vtFloat,
   vtDouble,
   vtBoolean,
   vtText,
   vtPointer,
   vtHandle,
   vtVector = 0x1000,
   vtFixedVector = vtVector | 0x2000,
   vtMatrix = 0x4000,
};

struct VARIABLE
{
   VARIABLE() : ShortName(NULL), IniName(NULL), MemberAddr(0), Description(NULL), DefaultValue(NULL), ValueSize(0) {
   }

   ~VARIABLE() {
      if (ValueSize && DefaultValue) {
         delete DefaultValue;
         DefaultValue = NULL;
         ValueSize = 0;
      }
   }

   template<typename U>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const U& member, U default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr(NULL), Description(NULL), MemberType(type) {
      SetDefault(default);
   }

   template<>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const std::string& member, std::string default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==vtUnknown?vtText:type; 
      SetDefault(default);
   }

   template<>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const int& member, int default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==vtUnknown?vtInteger:type; 
      SetDefault(default);
   }

   template<>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const float& member, float default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==vtUnknown?vtFloat:type; 
      SetDefault(default);
   }

   template<>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const double& member, double default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
         MemberType = type==vtUnknown?vtDouble:type; 
         SetDefault(default);
   }

   template<>
   VARIABLE(LPCTSTR sName, LPCTSTR iName, const bool& member, bool default, LPCTSTR desc, VARIABLETYPE type) 
      : ShortName(sName), IniName(iName), MemberAddr((DWORD)&member), Description(NULL) { 
      MemberType = type==vtUnknown?vtBoolean:type; 
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
   VARIABLETYPE MemberType;
   LPCTSTR Description;
   LPVOID DefaultValue;
   DWORD ValueSize;
};

#define BEGIN_INI_MAP(name) \
   virtual void *GetMember(const VARIABLE* memptr) const { return (void*)(((char*)this) + memptr->MemberAddr); } \
   virtual const VARIABLE* GetInfDefmap() const { return InternalGetInfDefmap(); } \
   static VARIABLE* InternalGetInfDefmap() { \
   const name* pThis = 0; \
   static VARIABLE map[] = { \

#define BEGIN_INI_MAP_WITH_BASE(name, base) \
   virtual void *GetMember(const VARIABLE* memptr) const { return (void*)(((char*)this) + memptr->MemberAddr); } \
   virtual const VARIABLE* GetInfDefmap() const { return InternalGetInfDefmap(); } \
   static VARIABLE* InternalGetInfDefmap() { \
   const name* pThis = 0; \
   static VARIABLE map[] = { \

#define END_INI_MAP() \
   VARIABLE() };\
   return map;\
}

#define ADDITEM(sName, iName, member, desc) \
   VARIABLE(sName, iName, pThis->member, desc, vtUnknown), \

#define ADDITEMEX(sName, iName, member, type, desc) \
   VARIABLE(sName, iName, pThis->member, desc, type), \

struct IniFileSection
{
   virtual LPCTSTR GetName() const = 0;
   virtual const VARIABLE* GetInfDefmap() const = 0;
   virtual void *GetMember(const VARIABLE* memptr) const = 0;
};

typedef std::list<IniFileSection*>  IniFileSectionList;

inline int GetIniDefSectionSize(IniFileSection *section) {
   int len = 0;

   for (const VARIABLE *inidef = section->GetInfDefmap()
      ; inidef != NULL && inidef->ShortName
      ; ++inidef)
      ++len;
   return len;
}