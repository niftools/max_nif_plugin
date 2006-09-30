/**********************************************************************
 *<
	FILE: DllEntry.cpp

	DESCRIPTION: Contains the Dll Entry stuff

	CREATED BY: tazpn (Theo)

	HISTORY: 

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

extern ClassDesc2* GetMaxNifImportDesc();
extern ClassDesc2* GetNifExportDesc();
extern ClassDesc2* GetNifPropsDesc();
extern ClassDesc2* GetNifFurnitureDesc();
extern ClassDesc2* GetKfExportDesc();

enum ClassDescType
{
   CD_Import,
   CD_Export,
   CD_Props,
   CD_Furniture,
   CD_KFExport,
   CD_Count
};

static void InitializeLibSettings();

HINSTANCE hInstance;
static int controlsInit = FALSE;
static int libVersion = VERSION_3DSMAX;
static int foundOlderReleaseConflict = -1;
static int nClasses = 0;
static ClassDesc2* classDescriptions[CD_Count];
static bool classDescEnabled[CD_Count];



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
   libVersion = GetIniValue("System", "MaxSDKVersion", libVersion, iniName);
   if (libVersion == 0)
      libVersion = VERSION_3DSMAX;

   nClasses = 0;
   if ( GetIniValue<bool>("MaxNifExport", "Enable", true, iniName) ) {
      classDescEnabled[CD_Export] = true;
      classDescriptions[nClasses++] = GetNifExportDesc();
   }
   if ( GetIniValue<bool>("MaxNifImport", "Enable", true, iniName) ) {
      classDescEnabled[CD_Import] = true;
      classDescriptions[nClasses++] = GetMaxNifImportDesc();
   }
   if ( GetIniValue<bool>("NifProps", "Enable", true, iniName) ) {
      classDescEnabled[CD_Props] = true;
      classDescriptions[nClasses++] = GetNifPropsDesc();
   }
   if ( GetIniValue<bool>("NifFurniture", "Enable", true, iniName) ) {
      classDescEnabled[CD_Furniture] = true;
      classDescriptions[nClasses++] = GetNifFurnitureDesc();
   }
   if ( GetIniValue<bool>("KFExport", "Enable", false, iniName) ) {
      classDescEnabled[CD_KFExport] = true;
      classDescriptions[nClasses++] = GetKfExportDesc();
   }
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
   return TEXT("Niftools Max Plugins");
	//return GetString(IDS_LIBDESCRIPTION);
}

static LPTSTR PathMerge(LPTSTR base, LPCTSTR file)
{
   PathRemoveFileSpec(base);
   PathAppend(base, file);
   return base;
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
   // Has conflict exit now.
   if (foundOlderReleaseConflict > 0) {
      return 0;
   } else if ( foundOlderReleaseConflict < 0 ) { 
      foundOlderReleaseConflict = 0;

      // Check for older releases
      if (  (classDescEnabled[CD_Import] && NULL != GetModuleHandle("MaxNifImport.dli"))
         || (classDescEnabled[CD_Export] && NULL != GetModuleHandle("NifExport.dle"))
         || (classDescEnabled[CD_Furniture] && NULL != GetModuleHandle("NifFurniture.dlo"))
         || (classDescEnabled[CD_Props]  && NULL != GetModuleHandle("NifProps.dlu"))
         )
      {
         foundOlderReleaseConflict = 1;
      }
      else
      {
         // do more aggressive access search now in case we are loaded after them
         TCHAR filename[MAX_PATH];
         GetModuleFileName(hInstance, filename, MAX_PATH);

         if (classDescEnabled[CD_Import] && -1 != _taccess(PathMerge(filename, TEXT("MaxNifImport.dli")), 0))
            foundOlderReleaseConflict = 1;
         else if (classDescEnabled[CD_Export] && -1 != _taccess(PathMerge(filename, TEXT("NifExport.dle")), 0))
            foundOlderReleaseConflict = 1;
         else if (classDescEnabled[CD_Furniture] && -1 != _taccess(PathMerge(filename, TEXT("NifFurniture.dlo")), 0))
            foundOlderReleaseConflict = 1;
         else if (classDescEnabled[CD_Props] && -1 != _taccess(PathMerge(filename, TEXT("NifProps.dlu")), 0))
            foundOlderReleaseConflict = 1;
      }
      if (foundOlderReleaseConflict > 0)
      {
         TCHAR buffer[512];
         sprintf(buffer,
            "An older release of the Niftools Max Plugins was found.\n\n"
            "Please remove the following files from your 3dsmax\\plugins directory:\n"
            "%s%s%s%s"
            "The current version will be disabled."
            , classDescEnabled[CD_Import] ? "\tMaxNifImport.dli\n" : ""
            , classDescEnabled[CD_Export] ? "\tNifExport.dle\n" : ""
            , classDescEnabled[CD_Furniture] ? "\tNifFurniture.dlo\n" : ""
            , classDescEnabled[CD_Props] ? "\tNifProps.dlu\n\n" : ""
            );
         ::MessageBox( NULL
            , buffer
            , "Niftools Max Plugins"
            , MB_ICONSTOP|MB_OK
            );
         return 0;
      }
   }
	return nClasses;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
   return classDescriptions[i];
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
   return ULONG(libVersion);
}
