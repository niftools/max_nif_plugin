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
   AppSettings(const std::string& name) 
      : Name(name)
      , parsedImages(false) 
      , useSkeleton(false)
      , goToSkeletonBindPosition(true)
      , disableCreateNubsForBones(false)
      , textureUseFullPath(0)
      , supportPrnStrings(false)
	  , doNotReuseExistingBones(false)
   {}

   std::string Name;
   std::string rootPath;
   bool parsedImages;
   stringlist searchPaths;
   stringlist textureRootPaths;
   stringlist rootPaths;
   stringlist extensions;
   std::string Skeleton;
   bool useSkeleton;
   bool goToSkeletonBindPosition;
   bool disableCreateNubsForBones;
   int textureUseFullPath;
   NameValueCollection Environment;
   NameValueCollection imgTable;
   stringlist dummyNodeMatches;
   int applyOverallTransformToSkinAndBones;
   std::string NiVersion;
   int NiUserVersion;
	int NiUserVersion2;
   stringlist rotate90Degrees;
   bool supportPrnStrings;
   bool doNotReuseExistingBones;
   string skeletonCheck;

   static void Initialize(Interface *gi);
   void ReadSettings(std::string iniFile);
   void WriteSettings(Interface *gi);
   std::string FindImage(const std::string& fname);

   // Check whether the given file is a child of the root paths
   bool IsFileInRootPaths(const std::string& fname);

   // Return the Relative Texture Path for filename or empty
   std::string GetRelativeTexPath(const std::string& fname, const std::string& prefix);

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

typedef std::list<AppSettings> AppSettingsMap;

struct AppSettingsNameEquivalence : public ltstr
{
   bool operator()(const AppSettings& n1, const AppSettings& n2) const { 
      return ltstr::operator()(n1.Name, n2.Name);
   }
   bool operator()(const string& n1, const AppSettings& n2) const { 
      return ltstr::operator()(n1, n2.Name);
   }
   bool operator()(const AppSettings& n1, const string& n2) const { 
      return ltstr::operator()(n1.Name, n2);
   }
};

// The Global Map
//  Global so that I dont have to parse the texture directories on every import
extern AppSettingsMap TheAppSettings;

inline AppSettings* FindAppSetting(const std::string& name){
   AppSettingsNameEquivalence equiv;
   for (AppSettingsMap::iterator itr=TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
      if (!equiv(*itr, name) && !equiv(name, *itr))
         return &(*itr);
   }
   return NULL;
}

#endif //_APPSETTINGS_H_