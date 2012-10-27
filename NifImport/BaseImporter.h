/**********************************************************************
*<
FILE: BaseImporter.h

DESCRIPTION:	Base Importer class

CREATED BY: tazpn (Theo)

HISTORY:

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/

#ifndef __BASEIMPORTER_H__
#define __BASEIMPORTER_H__

class IBipMaster;
extern IBipMaster * (_cdecl * Max8CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int,int,int,int,int);
extern IBipMaster * (_cdecl * Max7CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int);

// Importer Base
class BaseImporter
{
public:
   string name;
   string path;
   ImpInterface *i;
   Interface *gi;
   BOOL suppressPrompts;
   bool iniFileValid;
   string iniFileName;
   AppSettings *appSettings;
   TSTR webSite;
   TSTR wikiSite;
   int nifVersion;
	int userVersion;

	bool hasErrors;
	string errorMsg;

   Niflib::NiObjectRef root;

   BaseImporter(){}

   void BaseInit(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
   {
      name = Name;
      i=I;
      gi=GI;
      suppressPrompts = SuppressPrompts;
      
      char buffer[MAX_PATH] = {0}, *p = NULL;
      GetFullPathName(Name, _countof(buffer), buffer, &p);

      if (-1 == _access(buffer, 0)) {
         throw RuntimeError(FormatText("File does not exist: %s", buffer));
      }

      if (p) *p = 0;
      path = buffer;
      iniFileValid = false;

      HMODULE hBiped = GetModuleHandle("biped.dlc");
      if (NULL != hBiped)
      {
         if (0 == Max8CreateNewBiped && 0 == Max7CreateNewBiped){
            LPCTSTR Max8CreateNewBipedName = TEXT("?CreateNewBiped@@YAPAVIBipMaster@@MMABVPoint3@@HHHHHHHHHHHHMHHHHHHHH@Z");
            *(FARPROC*)&Max8CreateNewBiped = GetProcAddress(hBiped, Max8CreateNewBipedName);
         }
         if (0 == Max8CreateNewBiped && 0 == Max7CreateNewBiped) {
            LPCTSTR Max7CreateNewBipedName = TEXT("?CreateNewBiped@@YAPAVIBipMaster@@MMABVPoint3@@HHHHHHHHHHHHMHHHH@Z");
            *(FARPROC*)&Max7CreateNewBiped = GetProcAddress(hBiped, Max7CreateNewBipedName);
         }
      }

      webSite = GetIniValue<TSTR>("System", "Website", "http://niftools.sourceforge.net");
      wikiSite = GetIniValue<TSTR>("System", "Wiki", "http://niftools.sourceforge.net/wiki/3ds_Max");

	  nifVersion = Niflib::GetNifVersion(this->name);

	  hasErrors = false;

      // Load ini settings
      iniFileValid = false;
      LoadIniSettings();

      ReadBlocks();

      Initialize();
   }

   virtual void LoadIniSettings() = 0;
   virtual void ReadBlocks() = 0;
   virtual void Initialize() = 0;

   virtual bool isValid() const { return (NULL != root); }

   // Generic IniFile reading routine
   template<typename T>
   T GetIniValue(LPCTSTR Section, LPCTSTR Setting, T Default){
      if (!iniFileValid) 
         return Default;
      return ::GetIniValue<T>(Section, Setting, Default, iniFileName.c_str());
   }
   // Generic IniFile reading routine
   template<typename T>
   void SetIniValue(LPCTSTR Section, LPCTSTR Setting, T value){
      if (!iniFileValid) 
         return;
      ::SetIniValue<T>(Section, Setting, value, iniFileName.c_str());
   }
};

extern void GoToSkeletonBindPosition(std::vector<Niflib::NiNodeRef>& blocks);

#endif
