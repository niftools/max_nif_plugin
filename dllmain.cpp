// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern void DoNotifyNodeHide( void* param, NotifyInfo* info );
extern void DoNotifyNodeUnHide( void* param, NotifyInfo* info );
extern Class_ID BHKLISTOBJECT_CLASS_ID;

extern ClassDesc2* GetNifExportDesc( );
extern ClassDesc2* GetBSDSModifierDesc( );
extern ClassDesc2* GetbhkRigidBodyModifierDesc( );
extern ClassDesc2* GetbhkBoxDesc( );
extern ClassDesc2* GetbhkSphereDesc( );
extern ClassDesc2* GetbhkCapsuleDesc( );
extern ClassDesc2* GetbhkListDesc( );
extern ClassDesc2* GetbhkProxyObjDesc( );
extern ClassDesc2* GetNifFurnitureDesc( );
extern ClassDesc2* GetNifShaderDesc( );
extern ClassDesc2* GetNifPropsDesc( );
extern ClassDesc2* GetMaxNifImportDesc( );

// NEW
extern ClassDesc2* GetSwitchNodeClassDesc( );

enum ClassDescType
{
	CD_Import,
    CD_Export,
    CD_Props,
    CD_Furniture,
    CD_KFExport,
    CD_Count
};

static void InitializeLibSettings( );
static void InitializeHavok( );

HINSTANCE hInstance = 0;

static int controlsInit = FALSE;
static int libVersion = VERSION_3DSMAX;
static int foundOlderReleaseConflict = -1;
static int nClasses = 0;
static ClassDesc2* classDescriptions[ 20 ];
static bool classDescEnabled[ CD_Count ];

TCHAR* GetString( int id )
{
	static TCHAR Buffer[ 256 ];

	if( hInstance )
		return LoadString( hInstance, id, Buffer, _countof( Buffer ) ) ? Buffer : NULL;

	return NULL;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	hInstance = hModule;

	if( !controlsInit )
	{
		controlsInit = TRUE;

#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
		InitCustomControls( hInstance );
#endif

		InitCommonControls( );
		RegisterNotification( DoNotifyNodeHide, NULL, NOTIFY_NODE_HIDE );
		RegisterNotification( DoNotifyNodeUnHide, NULL, NOTIFY_NODE_UNHIDE );
	}

	if( ul_reason_for_call == DLL_PROCESS_ATTACH )
		InitializeLibSettings( );

	return TRUE;
}

void InitializeLibSettings( )
{
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );

	libVersion = GetIniValue( _T( "System" ), _T( "MaxSDKVersion" ), libVersion, iniName );
	if( libVersion == 0 )
		libVersion = VERSION_3DSMAX;

	nClasses = 0;
	if( GetIniValue<bool>( _T( "MaxNifExport" ), _T( "Enable" ), true, iniName ) )
	{
		classDescEnabled[ CD_Export ] = true;
		classDescriptions[ nClasses++ ] = GetNifExportDesc( );
	}

	if( GetIniValue<bool>( _T( "MaxNifImport" ), _T( "Enable" ), true, iniName ) )
	{
		classDescEnabled[ CD_Import ] = true;
		classDescriptions[ nClasses++ ] = GetMaxNifImportDesc( );
	}

	if( GetIniValue<bool>( _T( "NifProps" ), _T( "Enable" ), true, iniName ) )
	{
		classDescEnabled[ CD_Props ] = true;
		classDescriptions[ nClasses++ ] = GetNifPropsDesc( );
		classDescriptions[ nClasses++ ] = GetbhkListDesc( );
		classDescriptions[ nClasses++ ] = GetbhkProxyObjDesc( );
		classDescriptions[ nClasses++ ] = GetbhkRigidBodyModifierDesc( );
		classDescriptions[ nClasses++ ] = GetbhkSphereDesc( );
		classDescriptions[ nClasses++ ] = GetbhkCapsuleDesc( );
		classDescriptions[ nClasses++ ] = GetbhkBoxDesc( );
		classDescriptions[ nClasses++ ] = GetBSDSModifierDesc( );
		classDescriptions[ nClasses++ ] = GetNifShaderDesc( );

		// NEW
		classDescriptions[ nClasses++ ] = GetSwitchNodeClassDesc( );
	}

	if( GetIniValue<bool>( _T( "NifFurniture" ), _T( "Enable" ), true, iniName ) )
	{
		classDescEnabled[ CD_Furniture ] = true;
		classDescriptions[ nClasses++ ] = GetNifFurnitureDesc( );
	}

	InitializeHavok( );
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription( )
{
	static TCHAR Buffer[ 256 ];

	if( hInstance )
		return LoadString( hInstance, IDS_LIBDESCRIPTION, Buffer, sizeof( Buffer ) ) ? Buffer : NULL;

	return NULL;
}

static LPWSTR PathMerge(LPWSTR base, LPCWSTR file)
{
   PathRemoveFileSpec( base );
   PathAppend( base, file );

   return base;
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses( )
{
	// Has conflict exit now.
	if( foundOlderReleaseConflict > 0 )
		return 0;

	else if( foundOlderReleaseConflict < 0 )
	{
		foundOlderReleaseConflict = 0;

		// Check for older releases
		if( ( classDescEnabled[ CD_Import ]    && NULL != GetModuleHandle( _T( "MaxNifImport.dli" ) ) )
		 || ( classDescEnabled[ CD_Export ]    && NULL != GetModuleHandle( _T( "NifExport.dle" ) ) )
		 || ( classDescEnabled[ CD_Furniture ] && NULL != GetModuleHandle( _T( "NifFurniture.dlo" ) ) )
		 || ( classDescEnabled[ CD_Props ]     && NULL != GetModuleHandle( _T( "NifProps.dlu" ) ) )
		 )
		{
			foundOlderReleaseConflict = 1;
		}
		else
		{
			// do more aggressive access search now in case we are loaded after them
			TCHAR filename[ MAX_PATH ];
			GetModuleFileName( hInstance, filename, MAX_PATH );

			if( classDescEnabled[ CD_Import ] && -1 != _taccess( PathMerge( filename, _T( "MaxNifImport.dli" ) ), 0 ) )
				foundOlderReleaseConflict = 1;

			if( classDescEnabled[ CD_Export ] && -1 != _taccess( PathMerge( filename, _T( "NifExport.dle" ) ), 0 ) )
				foundOlderReleaseConflict = 1;

			if( classDescEnabled[ CD_Furniture ] && -1 != _taccess( PathMerge( filename, _T( "NifFurniture.dlo" ) ), 0 ) )
				foundOlderReleaseConflict = 1;

			if( classDescEnabled[ CD_Props ] && -1 != _taccess( PathMerge( filename, _T( "NifProps.dlu" ) ), 0 ) )
				foundOlderReleaseConflict = 1;
		}

		if( foundOlderReleaseConflict > 0 )
		{
			TCHAR buffer[ 512 ];
			wsprintf( buffer, 
				_T( "An older release of the Niftools Max Plugins was found.\n\n" )
				_T( "Please remove the following files from your 3dsmax\\plugins directory:\n" )
				_T( "%s%s%s%s" )
				_T( "The current version will be disabled." )
				, classDescEnabled[ CD_Import ] ? _T( "\tMaxNifImport.dli\n" ) : _T( "" )
				, classDescEnabled[ CD_Export ] ? _T( "\tNifExport.dle\n" ) : _T( "" )
				, classDescEnabled[ CD_Furniture ] ? _T( "\tNifFurniture.dlo\n" ) : _T( "" )
				, classDescEnabled[ CD_Props ] ? _T( "\tNifProps.dlu\n" ) : _T( "" )
				);

			MessageBox( NULL, buffer, _T( "Niftools Max Plugins" ), MB_ICONSTOP | MB_OK );

			return 0;
		}
	}

	return nClasses;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc( int i )
{
	return classDescriptions[ i ];
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion( )
{
	return ULONG( libVersion );
}

__declspec( dllexport ) int LibInitialize( )
{
	// Nothing
	return TRUE;
}

__declspec( dllexport ) int LibShutdown( )
{
	// Nothing
	return TRUE;
}

static void DoNotifyNodeHide( void* param, NotifyInfo* info )
{
	int code = info->intcode;
	INode* node = (INode*) info->callParam;

	if( Object* obj = node->GetObjectRef( ) )
	{
		// Look for messages in network\Max.log
		// MAXScript_interface->Log()->LogEntry(SYSLOG_DEBUG, NO_DIALOG, "NifTools Max Plugin", 
		// 	"Entered DoNotifyNodeHide; node is -%s- and class ID is %ld\n", node->GetName(), obj->ClassID().PartA());

		if( obj->ClassID( ) == BHKLISTOBJECT_CLASS_ID )
		{
			const int PB_MESHLIST = 1;

			IParamBlock2* pblock2 = obj->GetParamBlockByID( 0 );
			int nBlocks = pblock2->Count( PB_MESHLIST );

			for( int i = 0; i < nBlocks; i++ )
			{
				INode* tnode = NULL;
				pblock2->GetValue( PB_MESHLIST, 0, tnode, FOREVER, i );

				if( tnode != NULL )
					tnode->Hide( TRUE );
			}
		}
	}
}

static void DoNotifyNodeUnHide( void* param, NotifyInfo* info )
{
	int code = info->intcode;
	INode* node = (INode*) info->callParam;

	if( Object* obj = node->GetObjectRef( ) )
	{
		// Look for messages in network\Max.log
		// MAXScript_interface->Log()->LogEntry(SYSLOG_DEBUG, NO_DIALOG, "NifTools Max Plugin", 
		// 	"Entered DoNotifyNodeUnHide; node is -%s- and class ID is %ld\n", node->GetName(), obj->ClassID().PartA());

		if( obj->ClassID( ) == BHKLISTOBJECT_CLASS_ID )
		{
			const int PB_MESHLIST = 1;

			IParamBlock2* pblock2 = obj->GetParamBlockByID( 0 );
			int nBlocks = pblock2->Count( PB_MESHLIST );

			for( int i = 0; i < nBlocks; i++ )
			{
				INode* tnode = NULL;
				pblock2->GetValue( PB_MESHLIST, 0, tnode, FOREVER, i );

				if( tnode != NULL )
					tnode->Hide( FALSE );
			}
		}
	}
}

static void InitializeHavok( )
{
	wchar_t curfile[ _MAX_PATH ];
	
	GetModuleFileName( hInstance, curfile, MAX_PATH );
	PathRemoveFileSpec( curfile );
	PathAppend( curfile, _T( "NifMopp.dll" ) );

	HMODULE hNifHavok = LoadLibrary( curfile );
	if( hNifHavok == NULL )
		hNifHavok = LoadLibrary( _T( "NifMopp.dll" ) );

	if( hNifHavok != NULL )
	{
		Niflib::Inertia::SetCalcMassPropertiesBox(
			(Niflib::Inertia::fnCalcMassPropertiesBox) GetProcAddress( hNifHavok, "CalcMassPropertiesBox" ) );

		Niflib::Inertia::SetCalcMassPropertiesSphere(
			(Niflib::Inertia::fnCalcMassPropertiesSphere) GetProcAddress( hNifHavok, "CalcMassPropertiesSphere" ) );

		Niflib::Inertia::SetCalcMassPropertiesPolyhedron(
			(Niflib::Inertia::fnCalcMassPropertiesPolyhedron) GetProcAddress( hNifHavok, "CalcMassPropertiesPolyhedron" ) );

		Niflib::Inertia::SetCombineMassProperties(
			(Niflib::Inertia::fnCombineMassProperties) GetProcAddress( hNifHavok, "CombineMassProperties" ) );
	}
}
