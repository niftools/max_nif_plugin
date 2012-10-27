#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"

#define REGPATH "Software\\NifTools\\MaxPlugins"

static LPCTSTR NifExportSection = TEXT("MaxNifExport");
static LPCTSTR KfExportSection = TEXT("KfExport");
static LPCTSTR CollisionSection = TEXT("Collision");


void regSet(HKEY hKey, const char *value, float f);
void regSet(HKEY hKey, const char *value, bool b);
void regSet(HKEY hKey, const char *value, DWORD dw);
void regSet(HKEY hKey, const char *value, int i);
void regSet(HKEY hKey, const char *value, const string &str);
void regGet(HKEY hKey, const char *value, float &f);
void regGet(HKEY hKey, const char *value, bool &b);
void regGet(HKEY hKey, const char *value, DWORD &v);
void regGet(HKEY hKey, const char *value, int &i);
void regGet(HKEY hKey, const char *value, string &str);

void Exporter::writeConfig(INode *node)
{
}

void Exporter::writeConfig(Interface *i)
{
   if (mUseRegistry)
   {
      HKEY hKey;
      if (RegCreateKey(HKEY_CURRENT_USER, REGPATH, &hKey) != ERROR_SUCCESS)
         return;

      regSet(hKey, "npx_ver", mVersion);
      regSet(hKey, "npx_tristrips", mTriStrips);
      regSet(hKey, "npx_hidden", mExportHidden);
      regSet(hKey, "npx_furn", mExportFurn);
      regSet(hKey, "npx_lights", mExportLights);
      regSet(hKey, "npx_vcolors", mVertexColors);
      //	regSet(hKey, "npx_wthresh", mWeldThresh);
      regSet(hKey, "npx_tprefix", mTexPrefix);
      regSet(hKey, "npx_coll", mExportCollision);
      regSet(hKey, "npx_remap", mRemapIndices);

      RegCloseKey(hKey);
   }
   else
   {
      TCHAR iniName[MAX_PATH];
      GetIniFileName(iniName);

      SetIniValue(NifExportSection, "GenerateStrips", mTriStrips, iniName);    
      SetIniValue(NifExportSection, "IncludeHidden", mExportHidden, iniName);
      SetIniValue(NifExportSection, "FurnatureMarkers", mExportFurn, iniName);
      SetIniValue(NifExportSection, "Lights", mExportLights, iniName);
      SetIniValue(NifExportSection, "VertexColors", mVertexColors, iniName);
      SetIniValue(NifExportSection, "TexturePrefix", mTexPrefix, iniName);
      SetIniValue(NifExportSection, "ExportCollision", mExportCollision, iniName);
      SetIniValue(NifExportSection, "RemapIndices", mRemapIndices, iniName);

      SetIniValue(NifExportSection, "ExportExtraNodes", mExportExtraNodes, iniName);
      SetIniValue(NifExportSection, "ExportSkin", mExportSkin, iniName);
      SetIniValue(NifExportSection, "UserPropBuffer", mUserPropBuffer, iniName);
      SetIniValue(NifExportSection, "FlattenHierarchy", mFlattenHierarchy, iniName);
      SetIniValue(NifExportSection, "RemoveUnreferencedBones", mRemoveUnreferencedBones, iniName);
      SetIniValue(NifExportSection, "SortNodesToEnd", mSortNodesToEnd, iniName);
      SetIniValue(NifExportSection, "SkeletonOnly", mSkeletonOnly, iniName);
      SetIniValue(NifExportSection, "Cameras", mExportCameras, iniName);
      SetIniValue(NifExportSection, "GenerateBoneCollision", mGenerateBoneCollision, iniName);

      SetIniValue(NifExportSection, "ExportTransforms", mExportTransforms, iniName);
      SetIniValue<int>(NifExportSection, "ExportType", mExportType, iniName);     
      SetIniValue<float>(KfExportSection, "Priority", mDefaultPriority, iniName);

      SetIniValue(NifExportSection, "MultiplePartitions", mMultiplePartitions, iniName);
      SetIniValue<int>(NifExportSection, "BonesPerVertex", mBonesPerVertex, iniName);     
      SetIniValue<int>(KfExportSection, "BonesPerPartition", mBonesPerPartition, iniName);
      //SetIniValue(NifExportSection, "UseTimeTags", mUseTimeTags, iniName);

      SetIniValue(NifExportSection, "AllowAccum", mAllowAccum, iniName);
      SetIniValue(NifExportSection, "CollapseTransforms", mCollapseTransforms, iniName);
      SetIniValue(NifExportSection, "ZeroTransforms", mZeroTransforms, iniName);
      SetIniValue(NifExportSection, "FixNormals", mFixNormals, iniName);
      SetIniValue(NifExportSection, "TangentAndBinormalExtraData", mTangentAndBinormalExtraData, iniName);
      SetIniValue(NifExportSection, "UseAlternateStripper", mUseAlternateStripper, iniName);
      SetIniValue(NifExportSection, "TangentAndBinormalMethod", mTangentAndBinormalMethod, iniName);
      SetIniValue(NifExportSection, "StartNifskopeAfterStart", mStartNifskopeAfterStart, iniName);
      //SetIniValue(CollisionSection, "bhkScaleFactor", bhkScaleFactor, iniName);

      SetIniValue<string>(NifExportSection, "Creator", mCreatorName, iniName);
      SetIniValue(NifExportSection, "GeneratePartitionStrips", mTriPartStrips, iniName);

      SetIniValue(NifExportSection, "WeldVertexThresh", mWeldThresh, iniName);
      SetIniValue(NifExportSection, "WeldVertexThresh", mNormThresh, iniName);
      SetIniValue(NifExportSection, "WeldUVWThresh", mUVWThresh, iniName);

	  SetIniValue<string>(NifExportSection, "RootType", mRootType, iniName);
   }
}

void Exporter::readConfig(INode *node)
{

}

void Exporter::readConfig(Interface *i)
{
   if (mUseRegistry)
   {
      HKEY hKey;
      if (RegCreateKey(HKEY_CURRENT_USER, REGPATH, &hKey) != ERROR_SUCCESS)
         return;

      DWORD ver;
      regGet(hKey, "npx_ver", ver);
      regGet(hKey, "npx_tristrips", mTriStrips);
      regGet(hKey, "npx_hidden", mExportHidden);
      regGet(hKey, "npx_furn", mExportFurn);
      regGet(hKey, "npx_lights", mExportLights);
      regGet(hKey, "npx_vcolors", mVertexColors);
      //	regGet(hKey, "npx_wthresh", mWeldThresh);
      regGet(hKey, "npx_tprefix", mTexPrefix);
      regGet(hKey, "npx_coll", mExportCollision);
      regGet(hKey, "npx_remap", mRemapIndices);
   }
   else
   {
      TCHAR iniName[MAX_PATH];
      GetIniFileName(iniName);

      //mVersion = GetIniValue<int>(NifExportSection, "Version", 013, iniName);
      mTriStrips = GetIniValue<bool>(NifExportSection, "GenerateStrips", true, iniName);
      mExportHidden = GetIniValue<bool>(NifExportSection, "IncludeHidden", false, iniName);
      mExportFurn = GetIniValue<bool>(NifExportSection, "FurnatureMarkers", true, iniName);
      mExportLights = GetIniValue<bool>(NifExportSection, "Lights", false, iniName);
      mVertexColors = GetIniValue<bool>(NifExportSection, "VertexColors", true, iniName);
      mWeldThresh = GetIniValue<float>(NifExportSection, "WeldVertexThresh", 0.01f, iniName);
      mNormThresh = GetIniValue<float>(NifExportSection, "WeldNormThresh", 0.01f, iniName);
      mUVWThresh = GetIniValue<float>(NifExportSection, "WeldUVWThresh", 0.01f, iniName);

      mTexPrefix = GetIniValue<string>(NifExportSection, "TexturePrefix", "textures", iniName);
      mExportCollision = GetIniValue<bool>(NifExportSection, "ExportCollision", true, iniName);
      mRemapIndices = GetIniValue(NifExportSection, "RemapIndices", true, iniName);

      mExportExtraNodes = GetIniValue(NifExportSection, "ExportExtraNodes", false, iniName);
      mExportSkin = GetIniValue(NifExportSection, "ExportSkin", false, iniName);
      mUserPropBuffer = GetIniValue(NifExportSection, "UserPropBuffer", false, iniName);
      mFlattenHierarchy = GetIniValue(NifExportSection, "FlattenHierarchy", false, iniName);
      mRemoveUnreferencedBones = GetIniValue(NifExportSection, "RemoveUnreferencedBones", false, iniName);
      mSortNodesToEnd = GetIniValue(NifExportSection, "SortNodesToEnd", false, iniName);
      mSkeletonOnly = GetIniValue(NifExportSection, "SkeletonOnly", false, iniName);
      mExportCameras = GetIniValue(NifExportSection, "Cameras", false, iniName);
      mGenerateBoneCollision = GetIniValue(NifExportSection, "GenerateBoneCollision", false, iniName);

      mExportTransforms = GetIniValue(KfExportSection, "Transforms", true, iniName);
      mDefaultPriority = GetIniValue<float>(KfExportSection, "Priority", 0.0f, iniName);
      mExportType = ExportType(GetIniValue<int>(NifExportSection, "ExportType", NIF_WO_ANIM, iniName));

      mMultiplePartitions = GetIniValue(NifExportSection, "MultiplePartitions", false, iniName);
      mBonesPerVertex = GetIniValue<int>(NifExportSection, "BonesPerVertex", 4, iniName);     
      mBonesPerPartition = GetIniValue<int>(NifExportSection, "BonesPerPartition", 20, iniName);

      //mUseTimeTags = GetIniValue(NifExportSection, "UseTimeTags", false, iniName);
      mAllowAccum = GetIniValue(NifExportSection, "AllowAccum", true, iniName);
      mCollapseTransforms = GetIniValue(NifExportSection, "CollapseTransforms", false, iniName);
      mZeroTransforms = GetIniValue(NifExportSection, "ZeroTransforms", false, iniName);
      mFixNormals = GetIniValue(NifExportSection, "FixNormals", false, iniName);
      mTangentAndBinormalExtraData = GetIniValue(NifExportSection, "TangentAndBinormalExtraData", false, iniName);
      mTangentAndBinormalMethod = GetIniValue<int>(NifExportSection, "TangentAndBinormalMethod", 0, iniName);

      mUseAlternateStripper = GetIniValue(NifExportSection, "UseAlternateStripper", false, iniName);
      mCreatorName = GetIniValue<string>(NifExportSection, "Creator", "", iniName);

      bhkScaleFactor = GetIniValue<float>(CollisionSection, "bhkScaleFactor", 7.0f, iniName);

      mStartNifskopeAfterStart = GetIniValue(NifExportSection, "StartNifskopeAfterStart", false, iniName);
      mNifskopeDir = ExpandEnvironment(GetIndirectValue(GetIniValue<string>("System", "NifskopeDir", "", iniName).c_str()));
      mTriPartStrips = GetIniValue<bool>(NifExportSection, "GeneratePartitionStrips", true, iniName);

	  mRootType = GetIniValue<string>(NifExportSection, "RootType", "NiNode", iniName);
	  mRootTypes = TokenizeString(GetIniValue<string>(NifExportSection, "RootTypes", "NiNode;BSFadeNode", iniName).c_str(), ";");
  }
}


void Exporter::readKfConfig(Interface *i)
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);

   mExportHidden = GetIniValue(KfExportSection, "IncludeHidden", false, iniName);
   mExportLights = GetIniValue(KfExportSection, "Lights", false, iniName);
   mExportCameras = GetIniValue(KfExportSection, "Cameras", false, iniName);
   mExportTransforms = GetIniValue(KfExportSection, "Transforms", true, iniName);
   mDefaultPriority = GetIniValue<float>(KfExportSection, "Priority", 0.0f, iniName);
}

void Exporter::writeKfConfig(Interface *i)
{
   TCHAR iniName[MAX_PATH];
   LPCTSTR pluginDir = i->GetDir(APP_PLUGCFG_DIR);
   PathCombine(iniName, pluginDir, "MaxNifTools.ini");

   SetIniValue(KfExportSection, "IncludeHidden", mExportHidden, iniName);
   SetIniValue(KfExportSection, "Lights", mExportLights, iniName);
   SetIniValue(KfExportSection, "Cameras", mExportCameras, iniName);
   SetIniValue(KfExportSection, "Transforms", mExportTransforms, iniName);
   SetIniValue<float>(KfExportSection, "Priority", mDefaultPriority, iniName);
}


AppSettings * Exporter::exportAppSettings()
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);

   // Update the current app version
   AppSettings * appSettings = FindAppSetting(Exporter::mGameName);
   if (appSettings == NULL && !TheAppSettings.empty())
      appSettings = &TheAppSettings.front();

   if (Exporter::mAutoDetect){
      SetIniValue<string>(NifExportSection, "CurrentApp", "AUTO", iniName);
   } else {
      SetIniValue<string>(NifExportSection, "CurrentApp", appSettings->Name, iniName);
   }

   appSettings->NiVersion = Exporter::mNifVersion;
   appSettings->NiUserVersion = Exporter::mNifUserVersion;
	appSettings->NiUserVersion2 = Exporter::mNifUserVersion2;
   appSettings->rotate90Degrees = Exporter::mRotate90Degrees;
   appSettings->supportPrnStrings = Exporter::mSupportPrnStrings;

   return appSettings;
}

AppSettings *Exporter::importAppSettings(string fname)
{
   AppSettings *appSettings = NULL;
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);

   // Locate which application to use. If Auto, find first app where this file appears in the root path list
   string curapp = GetIniValue<string>(NifExportSection, "CurrentApp", "AUTO", iniName);
   if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
      Exporter::mAutoDetect = true;
      // Scan Root paths
      for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
         if ((*itr).IsFileInRootPaths(fname)) {
            appSettings = &(*itr);
            break;
         }
      }
   } else {
      Exporter::mAutoDetect = false;
      appSettings = FindAppSetting(curapp);
   }
   if (appSettings == NULL && !TheAppSettings.empty())
      appSettings = &TheAppSettings.front();

   if (!appSettings)
      return NULL;

   Exporter::mGameName = appSettings->Name;
   Exporter::mNifVersion = appSettings->NiVersion;
   Exporter::mNifUserVersion = appSettings->NiUserVersion;
	Exporter::mNifUserVersion2 = appSettings->NiUserVersion2;
   if (!appSettings->rotate90Degrees.empty())
      Exporter::mRotate90Degrees = appSettings->rotate90Degrees;
   Exporter::mSupportPrnStrings = appSettings->supportPrnStrings;

   return appSettings;
}

void regSet(HKEY hKey, const char *value, float f)
{
	DWORD dw = *((DWORD*)&f);
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, bool b)
{
	DWORD dw = (DWORD)b;
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, DWORD dw)
{
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, int v)
{
	DWORD dw = (DWORD)v;
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, const std::string &str)
{
	RegSetValueEx(hKey, value, NULL, REG_SZ, (LPBYTE)str.c_str(), str.length()+1);
}

void regGet(HKEY hKey, const char *value, float &f)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		f = *((float*)&dw);
}

void regGet(HKEY hKey, const char *value, bool &b)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		b = (bool)dw;
}

void regGet(HKEY hKey, const char *value, DWORD &v)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		v = dw;
}

void regGet(HKEY hKey, const char *value, int &v)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		v = dw;
}

void regGet(HKEY hKey, const char *value, string &str)
{
	char buff[MAX_PATH];
	DWORD type, cdata = MAX_PATH;
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)buff, &cdata)==ERROR_SUCCESS && type==REG_SZ)
		str = buff;
}
