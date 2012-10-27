#include <io.h>
#include <string.h>
#include <tchar.h>
#include "AppSettings.h"
#include "IniSection.h"

AppSettingsMap TheAppSettings;

void AppSettings::Initialize(Interface *gi) 
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);
   if (-1 != _taccess(iniName, 0)) {
      bool reparse = GetIniValue<bool>("System", "Reparse", false, iniName);
      if (reparse || TheAppSettings.empty()){
         TheAppSettings.clear();
      }

      string Applications = GetIniValue<string>("System", "KnownApplications", "", iniName);
      stringlist apps = TokenizeString(Applications.c_str(), ";");
      apps.push_back(string("User")); // always ensure that user is present
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

   NiVersion = GetSetting<string>("NiVersion", "20.0.0.5");
   NiUserVersion = GetSetting<int>("NiUserVersion", 0);
	NiUserVersion2 = GetSetting<int>("NiUserVersion2", 0);

   rootPath = GetSetting<string>("RootPath");
   rootPaths = TokenizeString(GetSetting<string>("RootPaths").c_str(), ";");
   searchPaths = TokenizeString(GetSetting<string>("TextureSearchPaths").c_str(), ";");
   extensions = TokenizeString(GetSetting<string>("TextureExtensions").c_str(), ";");
   textureRootPaths = TokenizeString(GetSetting<string>("TextureRootPaths").c_str(), ";");

   Skeleton = GetSetting<string>("Skeleton");
   useSkeleton = GetSetting<bool>("UseSkeleton", useSkeleton);
   goToSkeletonBindPosition = GetSetting<bool>("GoToSkeletonBindPosition", goToSkeletonBindPosition);
   disableCreateNubsForBones = GetSetting<bool>("DisableCreateNubsForBones", disableCreateNubsForBones);
   applyOverallTransformToSkinAndBones = GetSetting<int>("ApplyOverallTransformToSkinAndBones", -1);
   textureUseFullPath = GetSetting<int>("TextureUseFullPath", textureUseFullPath);

   dummyNodeMatches = TokenizeString(GetSetting<string>("DummyNodeMatches").c_str(), ";");
   rotate90Degrees = TokenizeString(GetSetting<string>("Rotate90Degrees").c_str(), ";");
   supportPrnStrings = GetSetting<bool>("SupportPrnStrings", supportPrnStrings);
   doNotReuseExistingBones = GetSetting<bool>("DoNotReuseExistingBones", doNotReuseExistingBones);

   skeletonCheck = GetSetting<string>("SkeletonCheck");
}

void AppSettings::WriteSettings(Interface *gi)
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);
   if (-1 != _taccess(iniName, 0)) 
   {
      SetIniValue(Name.c_str(), "NiVersion", NiVersion.c_str(), iniName);
      SetIniValue(Name.c_str(), "NiUserVersion", FormatString("%d", NiUserVersion).c_str(), iniName);
		SetIniValue(Name.c_str(), "NiUserVersion2", FormatString("%d", NiUserVersion2).c_str(), iniName);
   }
}


string AppSettings::FindImage(const string& fname){
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (!PathIsRelative(fname.c_str())) {
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
   NameValueCollection::iterator nmitr = imgTable.find(buffer);
   if (nmitr != imgTable.end()){
      if (!rootPath.empty()) {
         _tcscpy(buffer, rootPath.c_str());
         PathCombine(buffer, rootPath.c_str(), ((*nmitr).second).c_str());
         return string(buffer);
      } else {
         return (*nmitr).second;
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
         size_t len = _tcslen(root);
         if (0 == _tcsncmp(root, file, len))
            return true;
      }
   }
   return false;
}

// Return the Relative Texture Path for filename or empty
std::string AppSettings::GetRelativeTexPath(const std::string& fname, const std::string& prefix)
{
   TCHAR buffer[MAX_PATH];
   if (textureUseFullPath == 1) // full path name
   {
      GetFullPathName(fname.c_str(), _countof(buffer), buffer, NULL);
      return string(buffer);
   }
   else if (textureUseFullPath == -1) // only filename
   {
	   return string(PathFindFileName(fname.c_str()));
   }
   if (!PathIsRelative(fname.c_str())) 
   {
      TCHAR root[MAX_PATH];
      TCHAR file[MAX_PATH];
      GetFullPathName(fname.c_str(), _countof(file), file, NULL);
      PathMakePretty(file);

      for (stringlist::iterator itr = textureRootPaths.begin(), end = textureRootPaths.end(); itr != end; ++itr ){
         GetFullPathName((*itr).c_str(), _countof(root), root, NULL);
         PathAddBackslash(root);
         PathMakePretty(root);
         if (-1 != _taccess(root,0)) {
            size_t len = _tcslen(root);
            if (0 == _tcsnicmp(root, file, len))
               return string(file+len);
         }
      }
   }
   else // Test if its relative to one of the specified root paths just return the texture 
   {
      for (stringlist::iterator itr = textureRootPaths.begin(), end = textureRootPaths.end(); itr != end; ++itr ){
         PathCombine(buffer, itr->c_str(), fname.c_str());
         if (-1 != _taccess(buffer, 0)){
            return fname;
         }
      }
   }

	// check if prefix is in place if so then just return fname as is
	if (_tcsnicmp(fname.c_str(), prefix.c_str(), prefix.size()) == 0)
	{
		return fname;
	}

   // Now just combine prefix with file portion of the name
   PathCombine(buffer, prefix.c_str(), PathFindFileName(fname.c_str()));
   return string(buffer);
}
