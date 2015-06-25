
#include "stdafx.h"

AppSettingsMap TheAppSettings;

void AppSettings::Initialize( Interface* gi )
{
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );

	if( -1 != _waccess( iniName, 0 ) ) 
	{
		bool reparse = GetIniValue<bool>( _T( "System" ), _T( "Reparse" ), false, iniName );
		if( reparse || TheAppSettings.empty( ) )
			TheAppSettings.clear( );

		std::wstring Applications = GetIniValue<std::wstring>( _T( "System" ), _T( "KnownApplications" ), _T( "" ), iniName );
		stringlist apps = TokenizeString( Applications.c_str( ), _T( ";" ) );
		apps.push_back( std::wstring( _T( "User" ) ) ); // always ensure that user is present

		for( stringlist::iterator appstr = apps.begin( ); appstr != apps.end( ); ++appstr )
		{
			AppSettings* setting = FindAppSetting( *appstr );
			if( NULL == setting )
			{
				AppSettingsMap::iterator itr = TheAppSettings.insert( TheAppSettings.end( ), AppSettings( *appstr ) );
				(*itr).ReadSettings( iniName );
			}
		}
	}
}

void AppSettings::ReadSettings( std::wstring iniFile )
{
	NameValueCollection settings = ReadIniSection( Name.c_str( ), iniFile.c_str( ) );

	// expand indirect values first
	for( NameValueCollection::iterator itr = settings.begin( ), end = settings.end( ); itr != end; ++itr )
		itr->second = GetIndirectValue( itr->second.c_str( ) );

	// next expand qualifers
	for( NameValueCollection::iterator itr = settings.begin( ), end = settings.end( ); itr != end; ++itr )
		itr->second = ExpandQualifiers( itr->second.c_str( ), settings );

	// finally expand environment variables
	for( NameValueCollection::iterator itr = settings.begin( ), end = settings.end( ); itr != end; ++itr )
		itr->second = ExpandEnvironment( itr->second );

	std::swap( Environment, settings );

	NiVersion = GetSetting<std::wstring>( _T( "NiVersion" ), _T( "20.0.0.5" ) );
	NiUserVersion = GetSetting<int>( _T( "NiUserVersion" ), 0 );
	NiUserVersion2 = GetSetting<int>( _T( "NiUserVersion2" ), 0 );

	rootPath = GetSetting<std::wstring>( _T( "RootPath" ) );
	rootPaths = TokenizeString( GetSetting<std::wstring>( _T( "RootPath" ) ).c_str( ), _T( ";" ) );
	searchPaths = TokenizeString( GetSetting<std::wstring>( _T( "TextureSearchPaths" ) ).c_str( ), _T( ";" ) );
	extensions = TokenizeString( GetSetting<std::wstring>( _T( "TextureExtensions" ) ).c_str( ), _T( ";" ) );
	textureRootPaths = TokenizeString( GetSetting<std::wstring>( _T( "TextureRootPaths" ) ).c_str( ), _T( ";" ) );

	Skeleton = GetSetting<std::wstring>( _T( "Skeleton" ) );
	useSkeleton = GetSetting<bool>( _T( "UseSkeleton" ), useSkeleton );
	goToSkeletonBindPosition = GetSetting<bool>( _T( "GoToSkeletonBindPosition" ), goToSkeletonBindPosition );
	disableCreateNubsForBones = GetSetting<bool>( _T( "DisableCreateNubsForBones" ), disableCreateNubsForBones );
	applyOverallTransformToSkinAndBones = GetSetting<int>( _T( "ApplyOverallTransformToSkinAndBones" ), -1 );
	textureUseFullPath = GetSetting<int>( _T( "TextureUseFullPath" ), textureUseFullPath );

	dummyNodeMatches = TokenizeString( GetSetting<std::wstring>( _T( "DummyNodeMatches" ) ).c_str( ), _T( ";" ) );
	rotate90Degrees = TokenizeString( GetSetting<std::wstring>( _T( "Rotate90Degrees" ) ).c_str( ), _T( ";" ) );
	supportPrnStrings = GetSetting<bool>( _T( "SupportPrnStrings" ), supportPrnStrings );
	doNotReuseExistingBones = GetSetting<bool>( _T( "DoNotReuseExistingBones" ), doNotReuseExistingBones );

	skeletonCheck = GetSetting<std::wstring>( _T( "SkeletonCheck" ) );
}

void AppSettings::WriteSettings( Interface* gi )
{
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );

	if( -1 != _taccess( iniName, 0 ) )
	{
		SetIniValue( Name.c_str( ), _T( "NiVersion" ), NiVersion.c_str( ), iniName );
		SetIniValue( Name.c_str( ), _T( "NiUserVersion" ), FormatString( _T( "%d" ), NiUserVersion ).c_str( ), iniName );
		SetIniValue( Name.c_str( ), _T( "NiUserVersion2" ), FormatString( _T( "%d" ), NiUserVersion2 ).c_str( ), iniName );
	}
}

// Check whether the given file is a child of the root paths
bool AppSettings::IsFileInRootPaths( const std::wstring& fname )
{
	TCHAR root[ MAX_PATH ];
	TCHAR file[ MAX_PATH ];
	GetFullPathName( fname.c_str( ), _countof( file ), file, NULL );
	PathMakePretty( file );

	for( stringlist::iterator itr = rootPaths.begin( ), end = rootPaths.end( ); itr != end; ++itr )
	{
		GetFullPathName( (*itr).c_str( ), _countof( root ), root, NULL );
		PathAddBackslash( root );
		PathMakePretty( root );

		if( -1 != _waccess( root, 0 ) )
		{
			size_t len = wcslen( root );
			if( 0 == wcsncmp( root, file, len ) )
				return true;
		}
	}

	return false;
}

// Return the Relative Texture Path for filename or empty
std::wstring AppSettings::GetRelativeTexPath( const std::wstring& fname, const std::wstring& prefix )
{
	TCHAR buffer[ MAX_PATH ];
	if( textureUseFullPath == 1 ) // full path name
	{
		GetFullPathName( fname.c_str( ), _countof( buffer ), buffer, NULL );
		return std::wstring( buffer );
	}
	else if( textureUseFullPath == -1 ) // only filename
	{
		return std::wstring( PathFindFileName( fname.c_str( ) ) );
	}

	if( !PathIsRelative( fname.c_str( ) ) )
	{
		TCHAR root[ MAX_PATH ];
		TCHAR file[ MAX_PATH ];

		GetFullPathName( fname.c_str( ), _countof( file ), file, NULL );
		PathMakePretty( file );

		for( stringlist::iterator itr = textureRootPaths.begin( ), end = textureRootPaths.end( ); itr != end; ++itr )
		{
			GetFullPathName( (*itr).c_str( ), _countof( root ), root, NULL );
			PathAddBackslash( root );
			PathMakePretty( root );

			if( -1 != _waccess( root, 0 ) )
			{
				size_t len = wcslen( root );
				if( 0 == _wcsnicmp( root, file, len ) )
					return std::wstring( file + len );
			}
		}
	}
	else // Test if its relative to one of the specified root paths just return the texture
	{
		for( stringlist::iterator itr = textureRootPaths.begin( ), end = textureRootPaths.end( ); itr != end; ++itr )
		{
			PathCombine( buffer, itr->c_str( ), fname.c_str( ) );
			if( -1 != _waccess( buffer, 0 ) )
				return fname;
		}
	}

	// check if prefix is in place if so then just return fname as is
	if( wcsnicmp( fname.c_str( ), prefix.c_str( ), prefix.size( ) ) == 0 )
		return fname;

	// Now just combine prefix with the file portion of the name
	PathCombine( buffer, prefix.c_str( ), PathFindFileName( fname.c_str( ) ) );

	return std::wstring( buffer );
}

std::wstring AppSettings::FindImage( const std::wstring& fname )
{
	TCHAR buffer[ MAX_PATH ];

	// Simply check for fully qualified path
	if( !PathIsRelative( fname.c_str( ) ) )
	{
		if( -1 != _waccess( fname.c_str( ), 0 ) )
			return fname;
	}

	// Test if its relative and in one of the specified root paths
	for( stringlist::iterator itr = textureRootPaths.begin( ), end = textureRootPaths.end( ); itr != end; ++itr )
	{
		PathCombine( buffer, itr->c_str( ), fname.c_str( ) );
		if( -1 != _waccess( buffer, 0 ) )
			return std::wstring( buffer );
	}

	// Hit the directories to find out whats out there
	if( !parsedImages )
	{
		FindImages( imgTable, rootPath, searchPaths, extensions );
		parsedImages = true;
	}

	// Search my filename for out texture
	wcscpy( buffer, PathFindFileName( fname.c_str( ) ) );
	PathRemoveExtension( buffer );

	NameValueCollection::iterator nmitr = imgTable.find( buffer );
	if( nmitr != imgTable.end( ) )
	{
		if( !rootPath.empty( ) )
		{
			wcscpy( buffer, rootPath.c_str( ) );
			PathCombine( buffer, rootPath.c_str( ), ( (*nmitr).second ).c_str( ) );

			return std::wstring( buffer );
		}
		else
			return (*nmitr).second;
	}

	return fname;
}