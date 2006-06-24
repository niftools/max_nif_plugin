#include "stdafx.h"
#include <io.h>
#include <string.h>
#include <tchar.h>
#include "AppSettings.h"

AppSettingsMap TheAppSettings;

void AppSettings::Initialize(Interface *gi) 
{
   TCHAR iniName[MAX_PATH];
   LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
   PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   if (-1 != _taccess(iniName, 0)) {
      bool reparse = GetIniValue<bool>("System", "Reparse", false, iniName);
      if (reparse || TheAppSettings.empty()){
         TheAppSettings.clear();
      }

      string Applications = GetIniValue<string>("System", "KnownApplications", "", iniName);
      stringlist apps = TokenizeString(Applications.c_str(), ";");
      for (stringlist::iterator appstr=apps.begin(); appstr != apps.end(); ++appstr){
         AppSettingsMap::iterator itr = TheAppSettings.find(*appstr);
         if (itr == TheAppSettings.end()){
            itr = TheAppSettings.insert(TheAppSettings.end(), AppSettingsMap::value_type(*appstr, AppSettings(*appstr)) );
            (*itr).second.ReadSettings(iniName);
         }
      }
   }
}

void AppSettings::ReadSettings(string iniFile)
{
   NameValueCollection settings = ReadIniSection(Name.c_str(), iniFile.c_str());

   // expand indirect values first
   for (NameValueCollection::iterator itr = settings.begin(), end = settings.end(); itr != end; ++itr)
      itr->second = GetIndirectValue(itr->second.c_str());

   // next expand qualifiers
   for (NameValueCollection::iterator itr = settings.begin(), end = settings.end(); itr != end; ++itr)
      itr->second = ExpandQualifiers(itr->second.c_str(), settings);

   // finally expand environment variables, last because it clobbers my custom qualifier expansion
   for (NameValueCollection::iterator itr = settings.begin(), end = settings.end(); itr != end; ++itr)
      itr->second = ExpandEnvironment(itr->second);

   std::swap(Environment, settings);

   rootPath = GetSetting<string>("RootPath");

   string searchPathString = GetSetting<string>("TextureSearchPaths");
   searchPaths = TokenizeString(searchPathString.c_str(), ";");

   string extensionString = GetSetting<string>("TextureExtensions");
   extensions = TokenizeString(extensionString.c_str(), ";");

   string texRootPathString = GetSetting<string>("TextureRootPaths");
   rootPaths = TokenizeString(texRootPathString.c_str(), ";");

   Skeleton = GetSetting<string>("Skeleton");
}

string AppSettings::FindImage(const string& fname){
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (PathIsRoot(fname.c_str())) {
      if (-1 != _taccess(fname.c_str(), 0))
         return fname;
   }

   // Test if its relative and in one of the specified root paths
   for (stringlist::iterator itr = rootPaths.begin(), end = rootPaths.end(); itr != end; ++itr ){
      PathCombine(buffer, itr->c_str(), fname.c_str());
      if (-1 != _taccess(buffer, 0)){
         return string(buffer);
      }
   }

   // Hit the directories to find out whats out there
   if (!parsedImages){
      FindImages(imgTable, rootPath, searchPaths, extensions);
      parsedImages = true;
   }
   
   // Search my filename for our texture
   _tcscpy(buffer, PathFindFileName(fname.c_str()));
   PathRemoveExtension(buffer);
   NameValueCollection::iterator itr = imgTable.find(buffer);
   if (itr != imgTable.end()){
      if (!rootPath.empty()) {
         _tcscpy(buffer, rootPath.c_str());
         PathCombine(buffer, rootPath.c_str(), ((*itr).second).c_str());
         return string(buffer);
      } else {
         return (*itr).second;
      }
   }
   return fname;
}

