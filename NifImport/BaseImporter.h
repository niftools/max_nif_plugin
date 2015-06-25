
#pragma once

class IBipMaster;
extern IBipMaster * (_cdecl * Max8CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int,int,int,int,int);
extern IBipMaster * (_cdecl * Max7CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int);

class BaseImporter
{
public:
	std::wstring			name;
	std::wstring			path;
	ImpInterface*			i;
	Interface*				gi;
	BOOL					suppressPrompts;
	bool					iniFileValid;
	std::wstring			iniFileName;
	AppSettings*			appSettings;

	TSTR					webSite;
	TSTR					wikiSite;

	int						nifVersion;
	int						userVersion;

	bool					hasErrors;
	std::wstring			errorMsg;

	Niflib::NiObjectRef		root;

	BaseImporter( ) { }

	void BaseInit( const TCHAR* Name, ImpInterface* I, Interface* GI, BOOL SuppressPrompts )
	{
		name = Name;
		i = I;
		gi = GI;
		suppressPrompts = SuppressPrompts;

		wchar_t buffer[ MAX_PATH ] = {0};
		wchar_t* p = NULL;

		GetFullPathName( Name, _countof( buffer ), buffer, &p );

		if( -1 == _waccess( buffer, 0 ) )
			throw RuntimeError( FormatText( _T( "File does not exist: %s" ), buffer ) );

		if( p )
			*p = 0;

		path = buffer;
		iniFileValid = false;

		HMODULE hBiped = GetModuleHandle( _T( "biped.dlc" ) );
		if( NULL != hBiped )
		{
			if( 0 == Max8CreateNewBiped && 0 == Max7CreateNewBiped )
			{
				LPCTSTR Max8CreateNewBipedName = _T( "?CreateNewBiped@@YAPAVIBipMaster@@MMABVPoint3@@HHHHHHHHHHHHMHHHHHHHH@Z" );
				*(FARPROC*) &Max8CreateNewBiped = GetProcAddress( hBiped, narrow( Max8CreateNewBipedName ).c_str( ) );
			}

			if( 0 == Max8CreateNewBiped && 0 == Max7CreateNewBiped )
			{
				LPCTSTR Max7CreateNewBipedName = _T( "?CreateNewBiped@@YAPAVIBipMaster@@MMABVPoint3@@HHHHHHHHHHHHMHHHH@Z" );
				*(FARPROC*) &Max7CreateNewBiped = GetProcAddress( hBiped, narrow( Max7CreateNewBipedName ).c_str( ) );
			}
		}

		webSite = GetIniValue<TSTR>( _T( "System" ), _T( "Website" ), _T( "http://niftools.sourceforge.net" ) );
		wikiSite = GetIniValue<TSTR>( _T( "System" ), _T( "Wiki" ), _T( "http://niftools.sourceforge.net/wiki/3ds_Max" ) );

		nifVersion = Niflib::GetNifVersion( narrow( this->name ) );

		hasErrors = false;

		// Load ini settings
		iniFileValid = false;
		LoadIniSettings( );

		ReadBlocks( );

		Initialize( );
	}

	virtual void LoadIniSettings( ) = 0;
	virtual void ReadBlocks( ) = 0;
	virtual void Initialize( ) = 0;

	virtual bool isValid( ) const { return ( NULL != root ); }

	// Generic iniFile reading routine
	template<typename T>
	T GetIniValue( LPCTSTR Section, LPCTSTR Setting, T Default )
	{
		if( !iniFileValid )
			return Default;

		return ::GetIniValue<T>( Section, Setting, Default, iniFileName.c_str( ) );
	}

	// Generic iniFile writing routine
	template<typename T>
	void SetIniValue( LPCTSTR Section, LPCTSTR Setting, T value )
	{
		if( !iniFileValid )
			return;

		::SetIniValue<T>( Section, Setting, value, iniFileName.c_str( ) );
	}
};

extern void GoToSkeletonBindPosition( std::vector<Niflib::NiNodeRef>& blocks );