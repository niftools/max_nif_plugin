/**********************************************************************
 *<
	FILE: DllEntry.cpp

	DESCRIPTION: Contains the Dll Entry stuff

	CREATED BY: tazpn (Theo)

	HISTORY: 

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include <notify.h>


extern void DoNotifyNodeHide(void *param, NotifyInfo *info);
extern void DoNotifyNodeUnHide(void *param, NotifyInfo *info);
extern Class_ID BHKLISTOBJECT_CLASS_ID;

extern ClassDesc2* GetMaxNifImportDesc();
extern ClassDesc2* GetNifExportDesc();
extern ClassDesc2* GetNifPropsDesc();
extern ClassDesc2* GetNifFurnitureDesc();
extern ClassDesc2* GetKfExportDesc();
extern ClassDesc2* GetbhkSphereDesc();
extern ClassDesc2* GetbhkCapsuleDesc();
extern ClassDesc2* GetbhkRigidBodyModifierDesc();
extern ClassDesc2* GetbhkBoxDesc();
extern ClassDesc* GetDDSLibClassDesc();
extern ClassDesc2* GetbhkListObjDesc();
extern ClassDesc2* GetbhkProxyObjDesc();
extern ClassDesc2* GetBSDSModifierDesc();
extern ClassDesc2* GetNifShaderDesc();

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
static void InitializeHavok();

HINSTANCE hInstance;
static int controlsInit = FALSE;
static int libVersion = VERSION_3DSMAX;
static int foundOlderReleaseConflict = -1;
static int nClasses = 0;
static ClassDesc2* classDescriptions[20];
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
#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
		InitCustomControls(hInstance);	// Initialize MAX's custom controls
#endif
		InitCommonControls();			// Initialize Win95 controls
		RegisterNotification(DoNotifyNodeHide, NULL, NOTIFY_NODE_HIDE); 
		RegisterNotification(DoNotifyNodeUnHide, NULL, NOTIFY_NODE_UNHIDE); 
	}
	if (fdwReason == DLL_PROCESS_ATTACH)
		InitializeLibSettings();
	return (TRUE);
}

void InitializeLibSettings()
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);
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
	  classDescriptions[nClasses++] = GetbhkListObjDesc();
	  classDescriptions[nClasses++] = GetbhkProxyObjDesc();
      classDescriptions[nClasses++] = GetbhkRigidBodyModifierDesc();
      classDescriptions[nClasses++] = GetbhkSphereDesc();
      classDescriptions[nClasses++] = GetbhkCapsuleDesc();
      classDescriptions[nClasses++] = GetbhkBoxDesc();
      classDescriptions[nClasses++] = GetBSDSModifierDesc();
      classDescriptions[nClasses++] = GetNifShaderDesc();
   }
   if ( GetIniValue<bool>("NifFurniture", "Enable", true, iniName) ) {
      classDescEnabled[CD_Furniture] = true;
      classDescriptions[nClasses++] = GetNifFurnitureDesc();
   }
   if ( GetIniValue<bool>("KFExport", "Enable", false, iniName) ) {
      classDescEnabled[CD_KFExport] = true;
      classDescriptions[nClasses++] = GetKfExportDesc();
   }
#ifdef GAME_VER
   classDescriptions[nClasses++] = (ClassDesc2 *)GetDDSLibClassDesc();
#endif
   InitializeHavok();
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

static void DoNotifyNodeHide(void *param, NotifyInfo *info)
{
	int code = info->intcode;
	INode *node = (INode*)info->callParam;
	if (Object* obj = node->GetObjectRef())
	{
		// Look for messages in network\Max.log
		// MAXScript_interface->Log()->LogEntry(SYSLOG_DEBUG, NO_DIALOG, "NifTools Max Plugin", 
		// 	"Entered DoNotifyNodeHide; node is -%s- and class ID is %ld\n", node->GetName(), obj->ClassID().PartA());

	   if (obj->ClassID() == BHKLISTOBJECT_CLASS_ID)
	   {
		   const int PB_MESHLIST = 1;
		   IParamBlock2* pblock2 = obj->GetParamBlockByID(0);
		   int nBlocks = pblock2->Count(PB_MESHLIST);
		   for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++)
		   {
			   INode *tnode = NULL;
			   pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
			   if (tnode != NULL)
			   {
				   tnode->Hide(TRUE);
			   }
		   }
	   }
	}
}

static void DoNotifyNodeUnHide(void *param, NotifyInfo *info)
{
	int code = info->intcode;
	INode *node = (INode*)info->callParam;
	if (Object* obj = node->GetObjectRef())
	{
		// Look for messages in network\Max.log
		// MAXScript_interface->Log()->LogEntry(SYSLOG_DEBUG, NO_DIALOG, "NifTools Max Plugin", 
		// 	"Entered DoNotifyNodeUnHide; node is -%s- and class ID is %ld\n", node->GetName(), obj->ClassID().PartA());

	   if (obj->ClassID() == BHKLISTOBJECT_CLASS_ID)
	   {
		   const int PB_MESHLIST = 1;
		   IParamBlock2* pblock2 = obj->GetParamBlockByID(0);
		   int nBlocks = pblock2->Count(PB_MESHLIST);
		   for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++)
		   {
			   INode *tnode = NULL;
			   pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
			   if (tnode != NULL)
			   {
				   tnode->Hide(FALSE);
			   }
		   }
	   }
	}
}

#include "Inertia.h"
static void InitializeHavok()
{
	char curfile[_MAX_PATH];
	GetModuleFileName(hInstance, curfile, MAX_PATH);
	PathRemoveFileSpec(curfile);
	PathAppend(curfile, "NifMopp.dll");
	HMODULE hNifHavok = LoadLibraryA( curfile );
	if (hNifHavok == NULL)
		hNifHavok = LoadLibraryA( "NifMopp.dll" );
	if ( hNifHavok != NULL )
	{
		Niflib::Inertia::SetCalcMassPropertiesBox( 
			(Niflib::Inertia::fnCalcMassPropertiesBox)GetProcAddress(hNifHavok, "CalcMassPropertiesBox") );

		Niflib::Inertia::SetCalcMassPropertiesSphere( 
			(Niflib::Inertia::fnCalcMassPropertiesSphere)GetProcAddress(hNifHavok, "CalcMassPropertiesSphere") );

		Niflib::Inertia::SetCalcMassPropertiesCapsule( 
			(Niflib::Inertia::fnCalcMassPropertiesCapsule)GetProcAddress(hNifHavok, "CalcMassPropertiesCapsule") );

		Niflib::Inertia::SetCalcMassPropertiesPolyhedron( 
			(Niflib::Inertia::fnCalcMassPropertiesPolyhedron)GetProcAddress(hNifHavok, "CalcMassPropertiesPolyhedron") );

		Niflib::Inertia::SetCombineMassProperties( 
			(Niflib::Inertia::fnCombineMassProperties)GetProcAddress(hNifHavok, "CombineMassProperties") );
	}
}