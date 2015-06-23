
#include "stdafx.h"

#define NifExport_CLASS_ID	Class_ID( 0xa57ff0a4, 0xa0374ffb )

TSTR NifExportSection = _T( "MaxNifExport" );
TSTR shortDescription;

// --------------------------------------------------------------------
// NifExport
// --------------------------------------------------------------------

class NifExport : public SceneExport
{
public:
	static HWND		hParams;
	int				mDlgResult;
	TSTR			iniFileName;
	TSTR			fileVersion;
	TSTR			webSite;
	TSTR			wikiSite;

	int				ExtCount( );
	const TCHAR*	Ext( int n );
	const TCHAR*	LongDesc( );
	const TCHAR*	ShortDesc( );
	const TCHAR*	AuthorName( );
	const TCHAR*	CopyrightMessage( );
	const TCHAR*	OtherMessage1( );
	const TCHAR*	OtherMessage2( );
	unsigned int	Version( );
	void			ShowAbout( HWND hWnd );

	BOOL			SupportsOptions( int ext, DWORD options );
	int				DoExport( const TCHAR* name, ExpInterface* ei, Interface* i, BOOL supressPrompts=FALSE, DWORD options=0 );
	int				DoExportInternal( const TCHAR* name, ExpInterface* ei, Interface* i, BOOL supressPrompts, DWORD options );

	NifExport( );
	~NifExport( );
};

// --------------------------------------------------------------------
// NifExportClassDesc
// --------------------------------------------------------------------

class NifExportClassDesc : public ClassDesc2
{
public:
	int				IsPublic( ) { return TRUE; }
	void*			Create( BOOL loading=FALSE ) { return new NifExport( ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_NIF_CLASS_NAME ); }
	SClass_ID		SuperClassID( ) { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID( ) { return NifExport_CLASS_ID; }
	const TCHAR*	Category( ) { return GetString( IDS_CATEGORY ); }

	const TCHAR*	InternalName( ) { return _T( "NifExport" ); }
	HINSTANCE		HInstance( ) { return hInstance; }
};

// --------------------------------------------------------------------
// Global Functions
// --------------------------------------------------------------------

static NifExportClassDesc NifExportDesc;
ClassDesc2* GetNifExportDesc( ) { return &NifExportDesc; }

INT_PTR CALLBACK NifExportOptionsDlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static NifExport* imp = NULL;
	wchar_t buffer[ 256 ];
	static bool setCursor = false;

	switch( message )
	{
	case WM_INITDIALOG:
		{
			imp = (NifExport*) lParam;

			// Append file version to dialog
			if( !imp->fileVersion.isNull( ) )
			{
				GetWindowText( hWnd, buffer, _countof( buffer ) );
				wcscat( buffer, _T( " " ) );
				wcscat( buffer, imp->fileVersion );
				SetWindowText( hWnd, buffer );
			}

			CenterWindow( hWnd, GetParent( hWnd ) );
			CheckDlgButton( hWnd, IDC_CHK_STRIPS, Exporter::mTriStrips );
			CheckDlgButton( hWnd, IDC_CHK_HIDDEN, Exporter::mExportHidden );
			CheckDlgButton( hWnd, IDC_CHK_FURN, Exporter::mExportFurn );
			CheckDlgButton( hWnd, IDC_CHK_LIGHTS, Exporter::mExportLights );
			CheckDlgButton( hWnd, IDC_CHK_VCOLORS, Exporter::mVertexColors );
			SetDlgItemText( hWnd, IDC_ED_TEXPREFIX, Exporter::mTexPrefix.c_str( ) );
			CheckDlgButton( hWnd, IDC_CHK_COLL, Exporter::mExportCollision );
			CheckDlgButton( hWnd, IDC_CHK_REMAP, Exporter::mRemapIndices );

			CheckDlgButton( hWnd, IDC_CHK_EXTRA, Exporter::mExportExtraNodes );
			CheckDlgButton( hWnd, IDC_CHK_UPB, Exporter::mUserPropBuffer );
			CheckDlgButton( hWnd, IDC_CHK_HIER, Exporter::mFlattenHierarchy );
			CheckDlgButton( hWnd, IDC_CHK_REM_BONES, Exporter::mRemoveUnreferencedBones );
			CheckDlgButton( hWnd, IDC_CHK_SORTNODES, Exporter::mSortNodesToEnd );
			CheckDlgButton( hWnd, IDC_CHK_SKEL_ONLY, Exporter::mSkeletonOnly );
			CheckDlgButton( hWnd, IDC_CHK_CAMERA, Exporter::mExportCameras );
			CheckDlgButton( hWnd, IDC_CHK_BONE_COLL, Exporter::mGenerateBoneCollision );            
			CheckDlgButton( hWnd, IDC_CHK_TANGENTS, Exporter::mTangentAndBinormalExtraData );
			CheckDlgButton( hWnd, IDC_CHK_COLLAPSE_TRANS, Exporter::mCollapseTransforms );
			CheckDlgButton( hWnd, IDC_CHK_ZERO_TRANS, Exporter::mZeroTransforms );

			std::wstring selection = Exporter::mGameName;
			std::wstring version = Exporter::mNifVersion;
			std::wstring userVer = FormatString( _T( "%d" ), Exporter::mNifUserVersion );
			std::wstring userVer2 = FormatString( _T( "%d" ), Exporter::mNifUserVersion2 );

			for( AppSettingsMap::iterator itr = TheAppSettings.begin( ), end = TheAppSettings.end( ); itr != end; ++itr )
				SendDlgItemMessage( hWnd, IDC_CB_GAME, CB_ADDSTRING, 0, LPARAM( itr->Name.c_str( ) ) );

			SendDlgItemMessage( hWnd, IDC_CB_GAME, CB_SELECTSTRING, WPARAM( -1 ), LPARAM( selection.c_str( ) ) );
			SendDlgItemMessage( hWnd, IDC_CB_VERSION, WM_SETTEXT, 0, LPARAM( version.c_str( ) ) );
			SendDlgItemMessage( hWnd, IDC_CB_USER_VERSION, WM_SETTEXT, 0, LPARAM( userVer.c_str( ) ) );
			SendDlgItemMessage( hWnd, IDC_CB_USER_VERSION2, WM_SETTEXT, 0, LPARAM( userVer2.c_str( ) ) );
			CheckDlgButton( hWnd, IDC_CHK_AUTO_DETECT, Exporter::mAutoDetect );

			// Populate Type options
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WO_ANIM, LPARAM( _T( "NIF w/o Animation" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WO_KF, LPARAM( _T( "NIF with Animation" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::SINGLE_KF_WITH_NIF, LPARAM( _T( "Single KF with NIF" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::SINGLE_KF_WO_NIF, LPARAM( _T( "Single KF w/o NIF" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::MULTI_KF_WITH_NIF, LPARAM( _T( "Multi KF with NIF" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::MULTI_KF_WO_NIF, LPARAM( _T( "Multi KF w/o NIF" ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WITH_MGR, LPARAM( _T( "NIF w/ Manager" ) ) );

			SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_SETCURSEL, WPARAM( Exporter::mExportType ), 0 );

			CheckDlgButton( hWnd, IDC_CHK_TRANSFORMS2, Exporter::mExportTransforms );
			SetDlgItemText( hWnd, IDC_ED_PRIORITY2, FormatText( _T( "%.1f", Exporter::mDefaultPriority ) ) );

			// Skin
			CheckDlgButton( hWnd, IDC_CHK_SKIN, Exporter::mExportSkin );
			CheckDlgButton( hWnd, IDC_CHK_SKINPART, Exporter::mMultiplePartitions );
			SetDlgItemText( hWnd, IDC_ED_BONES_PART, FormatText( _T( "%d" ), Exporter::mBonesPerPartition ) );
			SetDlgItemText( hWnd, IDC_ED_BONES_VERTEX, FormatText( _T( "%d" ), Exporter::mBonesPerVertex ) );

			CheckDlgButton( hWnd, IDC_CHK_ALLOW_ACCUM, Exporter::mAllowAccum );

			TSTR tmp;
			tmp.printf( _T( "%.4f" ), Exporter::mWeldThresh );
			SetDlgItemText( hWnd, IDC_ED_WELDTHRESH2, tmp );

			CheckDlgButton( hWnd, IDC_CHK_START_NIFSKOPE, Exporter::mStartNifskopeAfterStart );

			// ConvertStaticToHyperlink( hWnd, IDC_LBL_LINK );
			// ConvertStaticToHyperlink( hWnd, IDC_LBL_WIKI );

			CheckDlgButton( hWnd, IDC_CHK_PARTSTRIPS, Exporter::mTriPartStrips );

			// Populate Type options
			int i = 0;
			for( stringlist::iterator itr = Exporter::mRootTypes.begin( ); itr != Exporter::mRootTypes.end( ); ++itr )
				SendDlgItemMessage( hWnd, IDC_CBO_ROOT_TYPE, CB_ADDSTRING, i++, LPARAM( (*itr).c_str( ) ) );

			LRESULT lIndex = SendDlgItemMessage( hWnd, IDC_CBO_ROOT_TYPE, CB_FINDSTRINGEXACT, -1, LPARAM( Exporter::mRootType.c_str( ) ) );
			SendDlgItemMessage( hWnd, IDC_CBO_ROOT_TYPE, CB_SETCURSEL, WPARAM( lIndex ), 0 );

			imp->mDlgResult = IDCANCEL;
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog( hWnd, imp->mDlgResult );
		return TRUE;

	case WM_COMMAND:
		if( HIWORD( wParam ) == BN_CLICKED )
		{
			wchar_t tmp[ MAX_PATH ], *end;
			bool close = false;

			switch( LOWORD( wParam ) )
			{
			case IDOK:
				// Validity Check
				GetDlgItemText( hWnd, IDC_CB_VERSION, tmp, MAX_PATH );
				if( tmp[ 0 ] != 0 )
				{
					int nifVersion = Niflib::ParseVersionString( narrow( tmp ) );
					if( !Niflib::IsSupportedVersion( nifVersion ) )
					{
						if( IDNO == MessageBox( hWnd, FormatString( _T( "Version '%s' is not a known version. Do you wish to continue?" ), tmp ).c_str( ), _T( "NifExport" ), MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP ) )
							return FALSE;
					}
				}

				Exporter::mTriStrips = IsDlgButtonChecked( hWnd, IDC_CHK_STRIPS );
				Exporter::mExportHidden = IsDlgButtonChecked( hWnd, IDC_CHK_HIDDEN );
				Exporter::mExportFurn = IsDlgButtonChecked( hWnd, IDC_CHK_FURN );
				Exporter::mExportLights = IsDlgButtonChecked( hWnd, IDC_CHK_LIGHTS );
				Exporter::mVertexColors = IsDlgButtonChecked( hWnd, IDC_CHK_VCOLORS );
				Exporter::mExportCollision = IsDlgButtonChecked( hWnd, IDC_CHK_COLL );
				Exporter::mRemapIndices = IsDlgButtonChecked( hWnd, IDC_CHK_REMAP );

				Exporter::mExportExtraNodes = IsDlgButtonChecked( hWnd, IDC_CHK_EXTRA );
				Exporter::mUserPropBuffer = IsDlgButtonChecked( hWnd, IDC_CHK_UPB );
				Exporter::mFlattenHierarchy = IsDlgButtonChecked( hWnd, IDC_CHK_HIER );
				Exporter::mRemoveUnreferencedBones = IsDlgButtonChecked( hWnd, IDC_CHK_REM_BONES );
				Exporter::mSortNodesToEnd = IsDlgButtonChecked( hWnd, IDC_CHK_SORTNODES );
				Exporter::mSkeletonOnly = IsDlgButtonChecked( hWnd, IDC_CHK_SKEL_ONLY );
				Exporter::mExportCameras = IsDlgButtonChecked( hWnd, IDC_CHK_CAMERA );
				Exporter::mGenerateBoneCollision = IsDlgButtonChecked( hWnd, IDC_CHK_BONE_COLL );
				Exporter::mTangentAndBinormalExtraData = IsDlgButtonChecked( hWnd, IDC_CHK_TANGENTS );
				Exporter::mCollapseTransforms = IsDlgButtonChecked( hWnd, IDC_CHK_COLLAPSE_TRANS );
				Exporter::mZeroTransforms = IsDlgButtonChecked( hWnd, IDC_CHK_ZERO_TRANS );
				Exporter::mExportTransforms = IsDlgButtonChecked( hWnd, IDC_CHK_TRANSFORMS2 );

				Exporter::mExportType = Exporter::ExportType( SendDlgItemMessage( hWnd, IDC_CBO_ANIM_TYPE, CB_GETCURSEL, 0, 0 ) );
				GetDlgItemText( hWnd, IDC_ED_PRIORITY2, tmp, MAX_PATH );
				Exporter::mDefaultPriority = (float) _wtof( tmp );

				GetDlgItemText( hWnd, IDC_ED_TEXPREFIX, tmp, MAX_PATH );
				Exporter::mTexPrefix = tmp;

				GetDlgItemText( hWnd, IDC_ED_WELDTHRESH2, tmp, MAX_PATH );
				Exporter::mWeldThresh = (float) _wtof( tmp );

				Exporter::mAllowAccum = IsDlgButtonChecked( hWnd, IDC_CHK_ALLOW_ACCUM );

				// Skin
				Exporter::mExportSkin = IsDlgButtonChecked( hWnd, IDC_CHK_SKIN );
				Exporter::mMultiplePartitions = IsDlgButtonChecked( hWnd, IDC_CHK_SKINPART );
				GetDlgItemText( hWnd, IDC_ED_BONES_PART, tmp, MAX_PATH );
				Exporter::mBonesPerPartition = _wtoi( tmp );
				GetDlgItemText( hWnd, IDC_ED_BONES_VERTEX, tmp, MAX_PATH );
				Exporter::mBonesPerVertex = _wtoi( tmp );

				GetDlgItemText( hWnd, IDC_CB_GAME, tmp, MAX_PATH );
				if( AppSettings *appSettings = FindAppSetting( tmp ) )
				{
				   Exporter::mGameName = appSettings->Name;
				   GetDlgItemText( hWnd, IDC_CB_VERSION, tmp, MAX_PATH );
				   Exporter::mNifVersion = tmp;
				   GetDlgItemText( hWnd, IDC_CB_USER_VERSION, tmp, MAX_PATH );
				   Exporter::mNifUserVersion = wcstol( tmp, &end, 0 );
				   GetDlgItemText( hWnd, IDC_CB_USER_VERSION2, tmp, MAX_PATH );
				   Exporter::mNifUserVersion2 = wcstol( tmp, &end, 0 );
				}

				Exporter::mAutoDetect = IsDlgButtonChecked( hWnd, IDC_CHK_AUTO_DETECT );
				Exporter::mStartNifskopeAfterStart = IsDlgButtonChecked( hWnd, IDC_CHK_START_NIFSKOPE );
				Exporter::mTriPartStrips = IsDlgButtonChecked( hWnd, IDC_CHK_PARTSTRIPS );

				{
					int idx = SendDlgItemMessage( hWnd, IDC_CBO_ROOT_TYPE, CB_GETCURSEL, 0, 0 );
					Exporter::mRootType = ( idx >= 0 ) ? Exporter::mRootTypes[ idx ] : _T( "NiNode" );
				}

				EndDialog( hWnd, imp->mDlgResult = IDOK );
				close = true;
				break;

			case IDCANCEL:
				EndDialog( hWnd, imp->mDlgResult = IDCANCEL );
				close = true;
				break;

			case IDC_LBL_LINK:
				// ShellExecute( hWnd, _T( "open" ), imp->webSite, NULL, NULL, SW_SHOWNORMAL );
				break;

			case IDC_LBL_WIKI:
				// ShellExecute( hWnd, _T( "open" ), imp->wikiSite, NULL, NULL, SW_SHOWNORMAL );
				break;
			}

			if( close )
				SendMessage( hWnd, WM_CLOSE, 0, 0 );
		}
		else if( HIWORD( wParam ) == CBN_SELCHANGE )
		{
			if( LOWORD( wParam ) == IDC_CB_GAME )
			{
				wchar_t tmp[ MAX_PATH ];
				GetDlgItemText( hWnd, IDC_CB_GAME, tmp, MAX_PATH );

				if( AppSettings* appSettings = FindAppSetting( tmp ) )
				{
					std::wstring version = appSettings->NiVersion;
					std::wstring userVer = FormatString( _T( "%d" ), appSettings->NiUserVersion );
					std::wstring userVer2 = FormatString( _T( "%d" ), appSettings->NiUserVersion2 );

					SendDlgItemMessage( hWnd, IDC_CB_VERSION, WM_SETTEXT, 0, LPARAM( version.c_str( ) ) );
					SendDlgItemMessage( hWnd, IDC_CB_USER_VERSION, WM_SETTEXT, 0, LPARAM( userVer.c_str( ) ) );
					SendDlgItemMessage( hWnd, IDC_CB_USER_VERSION2, WM_SETTEXT, 0, LPARAM( userVer2.c_str( ) ) );
				}
			}
		}
		break;
	}

	return FALSE;
}

// --------------------------------------------------------------------
// NifExport Functions
// --------------------------------------------------------------------

NifExport::NifExport( )
{
	Interface* gi = GetCOREInterface( );
	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );
	iniFileName = iniName;
	shortDescription = GetIniValue<TSTR>( _T( "System" ), _T( "ShortDescription" ), _T( "NetImmerse/Gamebryo" ), iniFileName );
	fileVersion = GetFileVersion( NULL );

	webSite = GetIniValue<TSTR>( _T( "System" ), _T( "Website" ), _T( "http://niftools/sourceforge.net" ), iniFileName );
	wikiSite = GetIniValue<TSTR>( _T( "System" ), _T( "Wiki" ), _T( "http://niftools.sourceforge.net/wiki/3ds_Max" ), iniFileName );
}

NifExport::~NifExport( )
{
}

int NifExport::ExtCount( )
{
	return 2;
}

const TCHAR* NifExport::Ext( int n )
{
	switch( n )
	{
	case 0: return _T( "KF" );
	case 1: return _T( "NIF" );
	}

	return NULL;
}

const TCHAR* NifExport::LongDesc( )
{
	return _T( "Gamebryo File" );
}

const TCHAR* NifExport::ShortDesc( )
{
	return shortDescription;
}

const TCHAR* NifExport::AuthorName( )
{
	return _T( "Alexander \"Gundalf\" Stillich / The Niftools Team / Ehamloptiran" );
}

const TCHAR* NifExport::CopyrightMessage( )
{
	return _T( "http://niftools.sourceforge.net" );
}

const TCHAR* NifExport::OtherMessage1( )
{
	return _T( "http://niftools.sourceforge.net" );
}

const TCHAR* NifExport::OtherMessage2( )
{
	return _T( "http://niftools.sourceforge.net" );
}

unsigned int NifExport::Version( )
{
	return Exporter::mVersion;
}

void NifExport::ShowAbout( HWND hWnd )
{
}

BOOL NifExport::SupportsOptions( int ext, DWORD options )
{
	return TRUE;
}

static DWORD WINAPI dummyProgress( LPVOID arg )
{
	return(0);
}

int NifExport::DoExport( const TCHAR* name, ExpInterface* ei, Interface* i, BOOL suppressPrompts, DWORD options )
{
	try
	{
		TSTR title = FormatText( _T( "Exporting '%s'..." ), PathFindFileName( name ) );
		i->PushPrompt( title );

		if( !suppressPrompts )
			i->ProgressStart( title, TRUE, dummyProgress, NULL );

		DoExportInternal( name, ei, i, suppressPrompts, options );
	}
	catch( Exporter::CancelExporterException& )
	{
		// Special user cancellation exception
	}
	catch( exception& e )
	{
		if( !suppressPrompts )
		{
			wchar_t buffer[ 256 ];
			wsprintf( buffer, _T( "%s" ), e.what( ) );
			MessageBox( NULL, buffer, _T( "Export Error" ), MB_OK );
		}
	}
	catch( ... )
	{
		if( !suppressPrompts )
			MessageBox( NULL, _T( "Unknown error." ), _T( "Export error" ), MB_OK );
	}

	try
	{
		i->PopPrompt( );

		if( !suppressPrompts )
			i->ProgressEnd( );
	}
	catch( ... )
	{
	}

	return TRUE;
}

int NifExport::DoExportInternal( const TCHAR* name, ExpInterface* ei, Interface* i, BOOL suppressPrompts, DWORD options )
{
	TCHAR path[ MAX_PATH ];
	GetFullPathName( name, MAX_PATH, path, NULL );
	PathRenameExtension( path, _T( ".nif" ) );
	
	// Register Objects
	Niflib::RegisterNiObjects( );

	Exporter::mSelectedOnly = ( options & SCENE_EXPORT_SELECTED ) != 0;

	// read application settings
	AppSettings::Initialize( i );

	TCHAR iniName[ MAX_PATH ];
	GetIniFileName( iniName );
	bool iniNameIsValid = ( -1 != _waccess( iniName, 0 ) );

	// Set whether Config should use registry or not
	Exporter::mUseRegistry = !iniNameIsValid || GetIniValue<bool>( NifExportSection, _T( "UseRegistry" ), false, iniName );

	// read config from registry
	Exporter::readConfig( i );

	// read config from root node
	Exporter::readConfig( i->GetRootNode( ) );

	// locate the "default" app setting
	std::wstring fname = path;
	AppSettings* appSettings = Exporter::importAppSettings( fname );

	Exporter::mSuppressPrompts = suppressPrompts;
	if( !suppressPrompts )
	{
		if( DialogBoxParam( hInstance, MAKEINTRESOURCE( IDD_NIF_PANEL ), GetActiveWindow( ), NifExportOptionsDlgProc, (LPARAM) this ) != IDOK )
			return true;

		// Write config to registry
		Exporter::writeConfig( i );

		// Write config to root node
		Exporter::writeConfig( i->GetRootNode( ) );

		appSettings = Exporter::exportAppSettings( );
		appSettings->WriteSettings( i );
	}

	int nifVersion = Niflib::VER_20_0_0_5;
	int nifUserVer = Exporter::mNifUserVersion;
	int nifUserVer2 = Exporter::mNifUserVersion2;

	if( !Exporter::mNifVersion.empty( ) )
	{
		nifVersion = Niflib::ParseVersionString( narrow( Exporter::mNifVersion ) );
		if( !Niflib::IsSupportedVersion( nifVersion ) )
		{
			std::wstring tmp = wide( Niflib::FormatVersionString( nifVersion ) );
			if( IDNO == MessageBox( GetActiveWindow( ), FormatString( _T( "Version '%s' is not a known version. Do you wish to continue?" ), tmp.c_str( ) ).c_str( ), _T( "NifExport" ), MB_YESNO|MB_DEFBUTTON2|MB_ICONSTOP ) )
				return FALSE;
		}
	}

	Exporter::mNifVersionInt = nifVersion;

	Exporter::ExportType exportType = Exporter::mExportType;

	// Hack so MW exports cleaner. Basically write tree without NiControllerManager
	if( nifVersion <= Niflib::VER_10_0_1_0 && ( Exporter::mExportType != Exporter::NIF_WO_ANIM && Exporter::mExportType != Exporter::NIF_WITH_MGR ) )
		Exporter::mExportType = Exporter::NIF_WO_KF;

	Niflib::NifInfo info( nifVersion, nifUserVer, nifUserVer2 );
	info.endian = Niflib::ENDIAN_LITTLE;
	info.creator = narrow( Exporter::mCreatorName );
	info.exportInfo1 = narrow( std::wstring( FormatText( _T( "Niftools 3ds Max Plugins %s" ), fileVersion.data( ) ) ) );

	wchar_t buf[ MAX_PATH ];
	wsprintf( buf, _T( "%s" ), path );
	PathRemoveFileSpec( buf );
	Exporter::mExportPath = std::wstring( buf );

	Exporter exp( i, appSettings );

	Niflib::Ref<Niflib::NiNode> root = Niflib::DynamicCast<Niflib::NiNode>( Niflib::ObjectRegistry::CreateObject( narrow( Exporter::mRootType ) ) );
	if( root == NULL )
		root = new Niflib::NiNode( );

	if( exp.IsFallout3( ) || exp.IsSkyrim( ) )
		root->SetFlags( 14 );

	Exporter::Result result = exp.doExport( root, i->GetRootNode( ) );

	if( result != Exporter::Ok && result != Exporter::Skip )
		throw exception( "Unknown error." );

	if( exportType == Exporter::NIF_WO_ANIM || exportType == Exporter::NIF_WITH_MGR )
		Niflib::WriteNifTree( narrow( path ), Niflib::NiObjectRef( root ), info );

	else
	{
		Niflib::ExportOptions export_type = Niflib::EXPORT_NIF;
		switch( exportType )
		{
		case Exporter::SINGLE_KF_WITH_NIF:	export_type = Niflib::EXPORT_NIF_KF;			break;
		case Exporter::SINGLE_KF_WO_NIF:	export_type = Niflib::EXPORT_KF;				break;
		case Exporter::MULTI_KF_WITH_NIF:	export_type = Niflib::EXPORT_NIF_KF_MULTI;		break;
		case Exporter::MULTI_KF_WO_NIF:		export_type = Niflib::EXPORT_KF_MULTI;			break;
		}

		Niflib::NifGame game = Niflib::KF_MW;
		if( appSettings->Name == _T( "Dark Age of Camelot" ) )
			game = Niflib::KF_DAOC;

		else if( nifVersion <= Niflib::VER_4_0_0_2 )
			game = Niflib::KF_MW;

		else if( nifVersion <= Niflib::VER_20_0_0_4 )
			game = Niflib::KF_FFVT3R;

		else
			game = Niflib::KF_CIV4;

		Niflib::WriteFileGroup( narrow( path ), Niflib::StaticCast<Niflib::NiObject>( root ), info, export_type, game );

	}

	if( Exporter::mStartNifskopeAfterStart )
	{
		std::wstring nifskope = Exporter::mNifskopeDir;
		std::wstring::size_type idx = nifskope.find( _T( "%1" ) );

		if( idx != std::wstring::npos )
		{
			nifskope.replace( idx, 2, path );
			WinExec( narrow( nifskope ).c_str( ), SW_SHOWNORMAL );
		}
	}

	return TRUE;
}