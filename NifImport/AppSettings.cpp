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
         AppSettings* setting = FindAppSetting(*appstr);
         if (NULL == setting){
            AppSettingsMap::iterator itr = TheAppSettings.insert(TheAppSettings.end(), AppSettings(*appstr));
            (*itr).ReadSettings(iniName);
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
   rootPaths = TokenizeString(GetSetting<string>("RootPaths").c_str(), ";");
   searchPaths = TokenizeString(GetSetting<string>("TextureSearchPaths").c_str(), ";");
   extensions = TokenizeString(GetSetting<string>("TextureExtensions").c_str(), ";");
   textureRootPaths = TokenizeString(GetSetting<string>("TextureRootPaths").c_str(), ";");

   Skeleton = GetSetting<string>("Skeleton");
   useSkeleton = GetSetting<bool>("UseSkeleton", useSkeleton);
   goToSkeletonBindPosition = GetSetting<bool>("GoToSkeletonBindPosition", goToSkeletonBindPosition);
}

string AppSettings::FindImage(const string& fname){
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (PathIsRoot(fname.c_str())) {
      if (-1 != _taccess(fname.c_str(), 0))
         return fname;
   }

   // Test if its relative and in one of the specified root paths
   for (stringlist::iterator itr = textureRootPaths.begin(), end = textureRootPaths.end(); itr != end; ++itr ){
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


// Check whether the given file is a child of the root paths
bool AppSettings::IsFileInRootPaths(const std::string& fname)
{
   TCHAR root[MAX_PATH];
   TCHAR file[MAX_PATH];
   GetFullPathName(fname.c_str(), _countof(file), file, NULL);
   PathMakePretty(file);

   for (stringlist::iterator itr = rootPaths.begin(), end = rootPaths.end(); itr != end; ++itr ){
      GetFullPathName((*itr).c_str(), _countof(root), root, NULL);
      PathAddBackslash(root);
      PathMakePretty(root);
      if (-1 != _taccess(root,0)) {
         int len = _tcslen(root);
         if (0 == _tcsncmp(root, file, len))
            return true;
      }
   }
   return false;
}
