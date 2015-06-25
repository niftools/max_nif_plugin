
#pragma once

class AppSettings
{
public:
	AppSettings( const std::wstring& name )
		: Name( name )
		, parsedImages( false )
		, useSkeleton( false )
		, goToSkeletonBindPosition( true )
		, disableCreateNubsForBones( false )
		, textureUseFullPath( 0 )
		, supportPrnStrings( false )
		, doNotReuseExistingBones( false )
	{
	}

	std::wstring Name;
	std::wstring rootPath;
	bool parsedImages;
	stringlist searchPaths;
	stringlist textureRootPaths;
	stringlist rootPaths;
	stringlist extensions;
	std::wstring Skeleton;
	bool useSkeleton;
	bool goToSkeletonBindPosition;
	bool disableCreateNubsForBones;
	int textureUseFullPath;
	NameValueCollection Environment;
	NameValueCollection imgTable;
	stringlist dummyNodeMatches;
	int applyOverallTransformToSkinAndBones;
	std::wstring NiVersion;
	int NiUserVersion;
	int NiUserVersion2;
	stringlist rotate90Degrees;
	bool supportPrnStrings;
	bool doNotReuseExistingBones;
	std::wstring skeletonCheck;

	static void Initialize( Interface* gi );
	void ReadSettings( std::wstring iniFile );
	void WriteSettings( Interface* gi );

	std::wstring FindImage( const std::wstring& fname );

	// Check whether the given file is a child of the root paths
	bool IsFileInRootPaths( const std::wstring& fname );

	// Return the relative texture path for filename or empty
	std::wstring GetRelativeTexPath( const std::wstring& fname, const std::wstring& prefix );

	template<typename T>
	inline T GetSetting( std::wstring setting )
	{
		T v;
		NameValueCollection::iterator itr = Environment.find( setting );

		if( itr != Environment.end( ) )
		{
			std::wstringstream sstr( (*itr).second );
			sstr >> v;
		}

		return v;
	}

	template<>
	inline std::wstring GetSetting( std::wstring setting )
	{
		NameValueCollection::iterator itr = Environment.find( setting );
		if( itr != Environment.end( ) )
			return (*itr).second;

		return std::wstring( );
	}

	template<typename T>
	inline T GetSetting( std::wstring setting, T Default )
	{
		NameValueCollection::iterator itr = Environment.find( setting );
		if( itr != Environment.end( ) )
		{
			T v;
			std::wstringstream sstr( (*itr).second );
			sstr >> v;

			return v;
		}

		return Default;
	}

	template<> std::wstring GetSetting( std::wstring setting, std::wstring Default )
	{
		NameValueCollection::iterator itr = Environment.find( setting );
		if( itr != Environment.end( ) )
			return (*itr).second;

		return Default;
	}
};

typedef std::list<AppSettings> AppSettingsMap;

struct AppSettingsNameEquivalence : public ltstr
{
	bool operator()( const AppSettings& n1, const AppSettings& n2 ) const { return ltstr::operator()( n1.Name, n2.Name ); }
	bool operator()( const std::wstring& n1, const AppSettings& n2 ) const { return ltstr::operator()( n1, n2.Name ); }
	bool operator()( const AppSettings& n1, const std::wstring& n2 ) const { return ltstr::operator()( n1.Name, n2 ); }
};

// ...

// The Global Map
//  Global so that I dont have to parse the texture directories on every import
extern AppSettingsMap TheAppSettings;

inline AppSettings* FindAppSetting( const std::wstring& name )
{
	AppSettingsNameEquivalence equiv;
	for( AppSettingsMap::iterator itr = TheAppSettings.begin( ), end = TheAppSettings.end( ); itr != end; ++itr )
		if( !equiv( *itr, name ) && !equiv( name, *itr ) )
			return &( *itr );

	return NULL;
}