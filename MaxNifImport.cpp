
#include "stdafx.h"

#define MaxNifImport_CLASS_ID Class_ID( 0x794ac1c1, 0x8b4c64c7 )

class MaxNifImport : public SceneImport
{
public:

	static HWND				hParams;
	TSTR					shortDescription;
	TSTR					webSite;
	TSTR					wikiSite;

	int						ExtCount( ) { return 2; }
	const TCHAR*			Ext( int n );
	const TCHAR*			LongDesc( );
	const TCHAR*			ShortDesc( );
	const TCHAR*			AuthorName( );
	const TCHAR*			CopyrightMessage( );
	const TCHAR*			OtherMessage1( );
	const TCHAR*			OtherMessage2( ) { return _T( "" ); }
	unsigned int			Version( ) { return 1 * 100; }
	void					ShowAbout( HWND hWnd ) { }
	int						DoImport( const TCHAR* name, ImpInterface* i, Interface* gi, BOOL suppressPrompts = FALSE );

	MaxNifImport( );
	~MaxNifImport( );

	std::wstring iniFileName;
};

class MaxNifImportClassDesc : public ClassDesc2
{
public:
	int				IsPublic( ) { return 1; }
	void*			Create( BOOL loading = FALSE ) {  return new MaxNifImport( ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_CLASS_NAME_NI ); }
	SClass_ID		SuperClassID( ) { return SCENE_IMPORT_CLASS_ID; }
	Class_ID		ClassID( ) { return MaxNifImport_CLASS_ID; }
	const TCHAR*	Category( ) { return GetString( IDS_CATEGORY_NI ); }

	// Returns fixed parsable name (scripter-visible name )
	const TCHAR*	InternalName( ) { return _T( "MaxNifImport" ); }
	// Returns owning module handle
	HINSTANCE		HInstance( ) { return hInstance; }
};

static MaxNifImportClassDesc MaxNifImportDesc;
ClassDesc2* GetMaxNifImportDesc( ) { return &MaxNifImportDesc; }

// =============================================================
// MaxNifImport
// =============================================================

MaxNifImport::MaxNifImport( )
{
	Interface* gi = GetCOREInterface( );
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );
	iniFileName = iniName;

	shortDescription = GetIniValue<TSTR>( _T( "System" ), _T( "ShortDescription" ), _T( "Netimmerse/Gamebryo" ), iniFileName.c_str( ) );
	webSite = GetIniValue<TSTR>( _T( "System" ), _T( "Website" ), _T( "http://niftools.sourceforge.net" ), iniFileName.c_str( ) );
	wikiSite = GetIniValue<TSTR>( _T( "System" ), _T( "Wiki" ), _T( "http://niftools.sourceforge.net/wiki/3ds_Max" ), iniFileName.c_str( ) );
}

MaxNifImport::~MaxNifImport( )
{
}

const TCHAR* MaxNifImport::Ext( int n )
{
	switch( n )
	{
	case 0: return _T( "NIF" );
	case 1: return _T( "KF" );
	case 2: return _T( "KFM" );
	}

	return _T( "" );
}

const TCHAR* MaxNifImport::LongDesc( )
{
	return _T( "Netimmerse/Gamebryo" );
}

const TCHAR* MaxNifImport::ShortDesc( )
{
	return shortDescription;
}

const TCHAR* MaxNifImport::AuthorName( )
{
	return _T( "Theo. Modified by Ehamloptiran." );
}

const TCHAR* MaxNifImport::CopyrightMessage( )
{
	return (
      _T( "Copyright (c) 2006, NIF File Format Library and Tools\n" )
      _T( "All rights reserved." )
	  );
}

const TCHAR* MaxNifImport::OtherMessage1( )
{
	return (
      _T( "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n" )
      _T( "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n" )
      _T( "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n" )
      _T( "FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n" )
      _T( "COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n" )
      _T( "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n" )
      _T( "BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n" )
      _T( "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n" )
      _T( "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n" )
      _T( "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN\n" )
      _T( "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n" )
      _T( "POSSIBILITY OF SUCH DAMAGE.\n" )
	  );
}

int MaxNifImport::DoImport( const TCHAR* filename, ImpInterface* i, Interface* gi, BOOL suppressPrompts )
{
	bool ok = true;
	try
	{
		HoldSuspend myHold( TRUE );
		AppSettings::Initialize( gi );
		
		std::wstring current_file = filename;
		LPCWSTR ext = PathFindExtension( filename );

		if( _wcsicmp( ext, _T( ".NIF" ) ) == 0 )
		{
			NifImporter importer( filename, i, gi, suppressPrompts );
			if( !importer.isValid( ) )
				return FALSE;

			ok = importer.DoImport( );
		}
		else if( _wcsicmp( ext, _T( ".KFM" ) ) == 0 )
		{
			// TODO: ...
		}
		else if( _wcsicmp( ext, _T( ".KF" ) ) == 0 )
		{
			// TODO: ...
		}
	}
	catch( std::exception& e )
	{
		MessageBox( NULL, wide( e.what( ) ).c_str( ), _T( "Import Error" ), MB_OK );
	}
	catch( RuntimeError& e )
	{
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+
		MessageBox( NULL, e.desc1, _T( "Import Error" ), MB_OK );
#endif
		return TRUE;
	}
	catch( ... )
	{
		MessageBox( NULL, _T( "Unknown error." ), _T( "Import Error" ), MB_OK );
		return TRUE;
	}

	return ok ? TRUE : FALSE;
}