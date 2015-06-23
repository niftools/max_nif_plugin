
#include "stdafx.h"

#define REGPATH _T( "Software\\NifTools\\MaxPlugins" )

static LPCWSTR NifExportSection = _T( "MaxNifExport" );
static LPCWSTR KfExportSection = _T( "KfExport" );
static LPCWSTR CollisionSection = _T( "Collision" );

void regSet( HKEY hKey, const wchar_t* value, float f );
void regSet( HKEY hKey, const wchar_t* value, bool b );
void regSet( HKEY hKey, const wchar_t* value, DWORD v );
void regSet( HKEY hKey, const wchar_t* value, int i );
void regSet( HKEY hKey, const wchar_t* value, const std::wstring& str );

void regGet( HKEY hKey, const wchar_t* value, float& f );
void regGet( HKEY hKey, const wchar_t* value, bool& b );
void regGet( HKEY hKey, const wchar_t* value, DWORD& v );
void regGet( HKEY hKey, const wchar_t* value, int& i );
void regGet( HKEY hKey, const wchar_t* value, std::wstring& str );

void Exporter::writeConfig( INode* node )
{
	// Nothing
}

void Exporter::writeConfig( Interface* i )
{
	if( mUseRegistry )
	{
		HKEY hKey;
		if( RegCreateKey( HKEY_CURRENT_USER, REGPATH, &hKey ) != ERROR_SUCCESS )
			return;

		regSet( hKey, _T( "npx_ver" ), mVersion );
		regSet( hKey, _T( "npx_tristrips" ), mTriStrips );
		regSet( hKey, _T( "npx_hidden" ), mExportHidden );
		regSet( hKey, _T( "npx_furn" ), mExportFurn );
		regSet( hKey, _T( "npx_lights" ), mExportLights );
		regSet( hKey, _T( "npx_vcolors" ), mVertexColors );
		regSet( hKey, _T( "npx_tprefix" ), mTexPrefix );
		regSet( hKey, _T( "npx_coll" ), mExportCollision );
		regSet( hKey, _T( "npx_remap" ), mRemapIndices );

		RegCloseKey( hKey );
	}
	else
	{
		TCHAR iniName[ MAX_PATH ];
		GetIniFileName( iniName );

		SetIniValue( NifExportSection, _T( "GenerateStrips" ), mTriStrips, iniName );
		SetIniValue( NifExportSection, _T( "IncludeHidden" ), mExportHidden, iniName );
		SetIniValue( NifExportSection, _T( "FurnatureMarkers" ), mExportFurn, iniName );
		SetIniValue( NifExportSection, _T( "Lights" ), mExportLights, iniName );
		SetIniValue( NifExportSection, _T( "VertexColors" ), mVertexColors, iniName );
		SetIniValue( NifExportSection, _T( "TexturePrefix" ), mTexPrefix, iniName );
		SetIniValue( NifExportSection, _T( "ExportCollision" ), mExportCollision, iniName );
		SetIniValue( NifExportSection, _T( "RemapIndices" ), mRemapIndices, iniName );

		SetIniValue( NifExportSection, _T( "ExportExtraNodes" ), mExportExtraNodes, iniName );
		SetIniValue( NifExportSection, _T( "ExportSkin" ), mExportSkin, iniName );
		SetIniValue( NifExportSection, _T( "UserPropBuffer" ), mUserPropBuffer, iniName );
		SetIniValue( NifExportSection, _T( "FlattenHierarchy" ), mFlattenHierarchy, iniName );
		SetIniValue( NifExportSection, _T( "RemoveUnreferencedBones" ), mRemoveUnreferencedBones, iniName );
		SetIniValue( NifExportSection, _T( "SortNodesToEnd" ), mSortNodesToEnd, iniName );
		SetIniValue( NifExportSection, _T( "SkeletonOnly" ), mSkeletonOnly, iniName );
		SetIniValue( NifExportSection, _T( "Cameras" ), mExportCameras, iniName );
		SetIniValue( NifExportSection, _T( "GenerateBoneCollision" ), mGenerateBoneCollision, iniName );

		SetIniValue( NifExportSection, _T( "ExportTransforms" ), mExportTransforms, iniName );
		SetIniValue<int>( NifExportSection, _T( "ExportType" ), mExportType, iniName );
		SetIniValue<float>( KfExportSection, _T( "Priority" ), mDefaultPriority, iniName );

		SetIniValue( NifExportSection, _T( "MultiplePartitions" ), mMultiplePartitions, iniName );
		SetIniValue<int>( NifExportSection, _T( "BonesPerVertex" ), mBonesPerVertex, iniName );
		SetIniValue<int>( KfExportSection, _T( "BonesPerPartition" ), mBonesPerPartition, iniName );

		SetIniValue( NifExportSection, _T( "AllowAccum" ), mAllowAccum, iniName );
		SetIniValue( NifExportSection, _T( "CollapseTransforms" ), mCollapseTransforms, iniName );
		SetIniValue( NifExportSection, _T( "ZeroTransforms" ), mZeroTransforms, iniName );
		SetIniValue( NifExportSection, _T( "FixNormals" ), mFixNormals, iniName );
		SetIniValue( NifExportSection, _T( "TangentAndBinormalExtraData" ), mTangentAndBinormalExtraData, iniName );
		SetIniValue( NifExportSection, _T( "UseAlternateStripper" ), mUseAlternateStripper, iniName );
		SetIniValue( NifExportSection, _T( "TangentAndBinormalMethod" ), mTangentAndBinormalMethod, iniName );
		SetIniValue( NifExportSection, _T( "StartNifskopeAfterStart" ), mStartNifskopeAfterStart, iniName );

		SetIniValue<std::wstring>( NifExportSection, _T( "Creator" ), mCreatorName, iniName );
		SetIniValue( NifExportSection, _T( "GeneratePartitionStrips" ), mTriPartStrips, iniName );

		SetIniValue( NifExportSection, _T( "WeldVertexThresh" ), mWeldThresh, iniName );
		SetIniValue( NifExportSection, _T( "WeldNormThresh" ), mNormThresh, iniName );
		SetIniValue( NifExportSection, _T( "WeldUVWThresh" ), mUVWThresh, iniName );

		SetIniValue<std::wstring>( NifExportSection, _T( "RootType" ), mRootType, iniName );
	}
}

void Exporter::readConfig( INode* node )
{
	// Nothing
}

void Exporter::readConfig( Interface* i )
{
	if( mUseRegistry )
	{
		HKEY hKey;
		if( RegCreateKey( HKEY_CURRENT_USER, REGPATH, &hKey ) != ERROR_SUCCESS )
			return;

		DWORD ver;
		regGet( hKey, _T( "npx_ver" ), ver );
		regGet( hKey, _T( "npx_tristrips" ), mTriStrips );
		regGet( hKey, _T( "npx_hidden" ), mExportHidden );
		regGet( hKey, _T( "npx_furn" ), mExportFurn );
		regGet( hKey, _T( "npx_lights" ), mExportLights );
		regGet( hKey, _T( "npx_vcolors" ), mVertexColors );
		
		regGet( hKey, _T( "npx_tprefix" ), mTexPrefix );
		regGet( hKey, _T( "npx_coll" ), mExportCollision );
		regGet( hKey, _T( "npx_remap" ), mRemapIndices );
	}
	else
	{
		TCHAR iniName[ MAX_PATH ];
		GetIniFileName( iniName );

		mTriStrips = GetIniValue<bool>( NifExportSection, _T( "GenerateStrips" ), true, iniName );
		mExportHidden = GetIniValue<bool>( NifExportSection, _T( "IncludeHidden" ), false, iniName );
		mExportFurn = GetIniValue<bool>( NifExportSection, _T( "FurnatureMarkers" ), true, iniName );
		mExportLights = GetIniValue<bool>( NifExportSection, _T( "Lights" ), false, iniName );
		mVertexColors = GetIniValue<bool>( NifExportSection, _T( "VertexColors" ), true, iniName );
		mWeldThresh = GetIniValue<float>( NifExportSection, _T( "WeldVertexThresh" ), 0.01f, iniName );
		mNormThresh = GetIniValue<float>( NifExportSection, _T( "WeldNormThresh" ), 0.01f, iniName );
		mUVWThresh = GetIniValue<float>( NifExportSection, _T( "WeldUVWThresh" ), 0.01f, iniName );

		mTexPrefix = GetIniValue<std::wstring>( NifExportSection, _T( "TexturePrefix" ), _T( "textures" ), iniName );
		mExportCollision = GetIniValue<bool>( NifExportSection, _T( "ExportCollision" ), true, iniName );
		mRemapIndices = GetIniValue( NifExportSection, _T( "RemapIndices" ), true, iniName );

		mExportExtraNodes = GetIniValue( NifExportSection, _T( "ExportExtraNodes" ), false, iniName );
		mExportSkin = GetIniValue( NifExportSection, _T( "ExportSkin" ), false, iniName );
		mUserPropBuffer = GetIniValue( NifExportSection, _T( "UserPropBuffer" ), false, iniName );
		mFlattenHierarchy = GetIniValue( NifExportSection, _T( "FlattenHierarchy" ), false, iniName );
		mRemoveUnreferencedBones = GetIniValue( NifExportSection, _T( "RemoveUnreferencedBones" ), false, iniName );
		mSortNodesToEnd = GetIniValue( NifExportSection, _T( "SortNodesToEnd" ), false, iniName );
		mSkeletonOnly = GetIniValue( NifExportSection, _T( "SkeletonOnly" ), false, iniName );
		mExportCameras = GetIniValue( NifExportSection, _T( "Cameras" ), false, iniName );
		mGenerateBoneCollision = GetIniValue( NifExportSection, _T( "GenerateBoneCollision" ), false, iniName );

		mExportTransforms = GetIniValue( NifExportSection, _T( "Transforms" ), true, iniName );
		mDefaultPriority = GetIniValue<float>( NifExportSection, _T( "Priority" ), 0.0f, iniName );
		mExportType = ExportType( GetIniValue<int>( NifExportSection, _T( "ExportType" ), NIF_WO_ANIM, iniName ) );

		mMultiplePartitions = GetIniValue( NifExportSection, _T( "MultiplePartitions" ), false, iniName );
		mBonesPerVertex = GetIniValue<int>( NifExportSection, _T( "BonesPerVertex" ), 4, iniName );
		mBonesPerPartition = GetIniValue<int>( NifExportSection, _T( "BonesPerPartition" ), 20, iniName );

		mAllowAccum = GetIniValue( NifExportSection, _T( "AllowAccum" ), true, iniName );
		mCollapseTransforms = GetIniValue( NifExportSection, _T( "CollapseTransforms" ), false, iniName );
		mZeroTransforms = GetIniValue( NifExportSection, _T( "ZeroTransforms" ), false, iniName );
		mFixNormals = GetIniValue( NifExportSection, _T( "FixNormals" ), false, iniName );
		mTangentAndBinormalExtraData = GetIniValue( NifExportSection, _T( "TangentAndBinormalExtraData" ), false, iniName );
		mTangentAndBinormalMethod = GetIniValue<int>( NifExportSection, _T( "TangentAndBinormalMethod" ), 0, iniName );

		mUseAlternateStripper = GetIniValue( NifExportSection, _T( "UseAlternateStripper" ), false, iniName );
		mCreatorName = GetIniValue<std::wstring>( NifExportSection, _T( "Creator" ), _T( "" ), iniName );

		bhkScaleFactor = GetIniValue<float>( NifExportSection, _T( "bhkScaleFactor" ), 7.0f, iniName );

		mStartNifskopeAfterStart = GetIniValue( NifExportSection, _T( "StartNifskopeAfterStart" ), false, iniName );
		mNifskopeDir = ExpandEnvironment( GetIndirectValue( GetIniValue<std::wstring>( _T( "System" ), _T( "NifskopeDir" ), _T( "" ), iniName ).c_str( ) ) );
		mTriPartStrips = GetIniValue<bool>( NifExportSection, _T( "GeneratePartitionStrips" ), true, iniName );

		mRootType = GetIniValue<std::wstring>( NifExportSection, _T( "RootType" ), _T( "NiNode" ), iniName );
		mRootTypes = TokenizeString( GetIniValue<std::wstring>( NifExportSection, _T( "RootTypes" ), _T( "NiNode;BSFadeNode" ), iniName ).c_str( ), _T( ";" ) );
	}
}

AppSettings* Exporter::exportAppSettings( )
{
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );

	// Update the current app version
	AppSettings* appSettings = FindAppSetting( Exporter::mGameName );
	if( appSettings == NULL && !TheAppSettings.empty( ) )
		appSettings = &TheAppSettings.front( );

	if( Exporter::mAutoDetect )
		SetIniValue<std::wstring>( NifExportSection, _T( "CurrentApp" ), _T( "AUTO" ), iniName );

	else
		SetIniValue<std::wstring>( NifExportSection, _T( "CurrentApp" ), appSettings->Name, iniName );

	appSettings->NiVersion = Exporter::mNifVersion;
	appSettings->NiUserVersion = Exporter::mNifUserVersion;
	appSettings->NiUserVersion2 = Exporter::mNifUserVersion2;
	appSettings->rotate90Degrees = Exporter::mRotate90Degrees;
	appSettings->supportPrnStrings = Exporter::mSupportPrnStrings;

	return appSettings;
}

AppSettings* Exporter::importAppSettings( std::wstring fname )
{
	AppSettings* appSettings = NULL;
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );

	// Locate which application to use. If Auto, find first app where this file appears in the root path list
	std::wstring curapp = GetIniValue<std::wstring>( NifExportSection, _T( "CurrentApp" ), _T( "AUTO" ), iniName );
	if( 0 == wcscmp( curapp.c_str( ), _T( "AUTO" ) ) )
	{
		Exporter::mAutoDetect = true;

		// Scan Root paths
		for( AppSettingsMap::iterator itr = TheAppSettings.begin( ), end = TheAppSettings.end( ); itr != end; ++itr )
		{
			if( (*itr).IsFileInRootPaths( fname ) )
			{
				appSettings = &(*itr);
				break;
			}
		}
	}
	else
	{
		Exporter::mAutoDetect = false;
		appSettings = FindAppSetting( curapp );
	}

	if( appSettings == NULL && !TheAppSettings.empty( ) )
		appSettings = &TheAppSettings.front( );

	if( !appSettings )
		return NULL;

	Exporter::mGameName = appSettings->Name;
	Exporter::mNifVersion = appSettings->NiVersion;
	Exporter::mNifUserVersion = appSettings->NiUserVersion;
	Exporter::mNifUserVersion2 = appSettings->NiUserVersion2;

	if( !appSettings->rotate90Degrees.empty( ) )
		Exporter::mRotate90Degrees = appSettings->rotate90Degrees;

	Exporter::mSupportPrnStrings = appSettings->supportPrnStrings;

	return appSettings;
}

void regSet( HKEY hKey, const wchar_t* value, float f )
{
	DWORD dw = *( (DWORD*) &f );
	RegSetValueEx( hKey, value, NULL, REG_DWORD, (LPBYTE) &dw, sizeof( DWORD ) );
}

void regSet( HKEY hKey, const wchar_t* value, bool b )
{
	DWORD dw = (DWORD) b;
	RegSetValueEx( hKey, value, NULL, REG_DWORD, (LPBYTE) &dw, sizeof( DWORD ) );
}

void regSet( HKEY hKey, const wchar_t* value, DWORD dw )
{
	RegSetValueEx( hKey, value, NULL, REG_DWORD, (LPBYTE) &dw, sizeof( DWORD ) );
}

void regSet( HKEY hKey, const wchar_t* value, int v )
{
	DWORD dw = (DWORD) v;
	RegSetValueEx( hKey, value, NULL, REG_DWORD, (LPBYTE) &dw, sizeof( DWORD ) );
}

void regSet( HKEY hKey, const wchar_t* value, const std::wstring& str )
{
	RegSetValueEx( hKey, value, NULL, REG_SZ, (LPBYTE) str.c_str( ), str.length( ) + 1 );
}

void regGet( HKEY hKey, const wchar_t* value, float& f )
{
	DWORD dw, type, cdata = sizeof( DWORD );
	if( RegQueryValueEx( hKey, value, NULL, &type, (LPBYTE) &dw, &cdata ) == ERROR_SUCCESS && type == REG_DWORD )
		f = *( (float*) &dw );
}

void regGet( HKEY hKey, const wchar_t* value, bool& b )
{
	DWORD dw, type, cdata = sizeof( DWORD );
	if( RegQueryValueEx( hKey, value, NULL, &type, (LPBYTE) &dw, &cdata ) == ERROR_SUCCESS && type == REG_DWORD )
		b = (bool) dw;
}

void regGet( HKEY hKey, const wchar_t* value, DWORD& v )
{
	DWORD dw, type, cdata = sizeof( DWORD );
	if( RegQueryValueEx( hKey, value, NULL, &type, (LPBYTE) &dw, &cdata ) == ERROR_SUCCESS && type == REG_DWORD )
		v = dw;
}

void regGet( HKEY hKey, const wchar_t* value, int& i )
{
	DWORD dw, type, cdata = sizeof( DWORD );
	if( RegQueryValueEx( hKey, value, NULL, &type, (LPBYTE) &dw, &cdata ) == ERROR_SUCCESS && type == REG_DWORD )
		i = dw;
}

void regGet( HKEY hKey, const wchar_t* value, std::wstring& str )
{
	wchar_t buff[ MAX_PATH ];
	DWORD type, cdata = sizeof( DWORD );
	if( RegQueryValueEx( hKey, value, NULL, &type, (LPBYTE) buff, &cdata ) == ERROR_SUCCESS && type == REG_SZ )
		str = buff;
}