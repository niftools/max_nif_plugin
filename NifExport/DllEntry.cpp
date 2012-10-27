#include "pch.h"
#include "niutils.h"

extern ClassDesc2* GetNifExportDesc();
extern ClassDesc2* GetKfExportDesc();

static void InitializeLibSettings();
HINSTANCE hInstance;
int controlsInit = FALSE;
int libVersion = VERSION_3DSMAX;

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	hInstance = hinstDLL;				// Hang on to this DLL's instance handle.

	if (!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);	// Initialize MAX's custom controls
		InitCommonControls();			// Initialize Win95 controls
	}
   if (fdwReason == DLL_PROCESS_ATTACH)
      InitializeLibSettings();
	return (TRUE);
}

void InitializeLibSettings()
{
   Interface *gi = GetCOREInterface();
   TCHAR iniName[MAX_PATH];
   if (gi) {
      LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
      PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   } else {
      GetModuleFileName(NULL, iniName, _countof(iniName));
      if (LPTSTR fname = PathFindFileName(iniName))
         fname = NULL;
      PathAddBackslash(iniName);
      PathAppend(iniName, "plugcfg");
      PathAppend(iniName, "MaxNifTools.ini");
   }
   libVersion = GetIniValue("MaxNifExport", "MaxSDKVersion", libVersion, iniName);
   if (libVersion == 0)
      libVersion = VERSION_3DSMAX;
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
   return 1;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
		case 0: return GetNifExportDesc();
      case 1: return GetKfExportDesc();
		default: return 0;
	}
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
	return libVersion;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}

