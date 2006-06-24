/**********************************************************************
*<
FILE: AppSettings.h

DESCRIPTION:	AppSetting helper class for managing game specific
               file settings.

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef _APPSETTINGS_H_
#define _APPSETTINGS_H_

#include "niutils.h"

class AppSettings
{
public:
   AppSettings(const std::string& name) : Name(name), parsedImages(false) {}

   std::string Name;
   std::string rootPath;
   bool parsedImages;
   stringlist searchPaths;
   stringlist rootPaths;
   stringlist extensions;
   std::string Skeleton;
   NameValueCollection Environment;
   NameValueCollection imgTable;

   static void Initialize(Interface *gi);
   void ReadSettings(std::string iniFile);
   std::string FindImage(const std::string& fname);

   template<typename T>
   inline T GetSetting(std::string setting){
      T v;
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end()){
         stringstream sstr((*itr).second);
         sstr >> v;
      }
      return v;
   }
   template<>
   inline std::string GetSetting(std::string setting){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end())
         return (*itr).second;
      return std::string();
   }

   template<typename T>
   inline T GetSetting(std::string setting, T Default){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end()){
         T v;
         stringstream sstr((*itr).second);
         sstr >> v;
         return v;
      }
      return Default;
   }
   template<>
   inline std::string GetSetting(std::string setting, std::string Default){
      NameValueCollection::iterator itr = Environment.find(setting);
      if (itr != Environment.end())
         return (*itr).second;
      return Default;
   }
};

typedef std::map<std::string, AppSettings, ltstr> AppSettingsMap;

// The Global Map
//  Global so that I dont have to parse the texture directories on every import
extern AppSettingsMap TheAppSettings;

#endif //_APPSETTINGS_H_