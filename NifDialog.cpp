
#include "stdafx.h"

static INT_PTR CALLBACK MaxNifImportOptionsDlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static NifImporter* imp = NULL;
	static DWORD dlgRes = IDCANCEL;

	switch( message )
	{
	case WM_INITDIALOG:
		{
			dlgRes = IDCANCEL;

			// Append file version to dialog
			TSTR version = GetFileVersion( NULL );
			if( !version.isNull( ) )
			{
				wchar_t buffer[ 256 ];

				GetWindowText( hWnd, buffer, _countof( buffer ) );
				wcscat( buffer, _T( " " ) );
				wcscat( buffer, version );

				SetWindowText( hWnd, buffer );
			}

			imp = (NifImporter*) lParam;
			CenterWindow( hWnd, GetParent( hWnd ) );

			CheckDlgButton( hWnd, IDC_CHK_BONES_NI, imp->importBones );
            CheckDlgButton( hWnd, IDC_CHK_SKIN_NI, imp->enableSkinSupport );
            CheckDlgButton( hWnd, IDC_CHK_VCOLORS_NI, imp->vertexColorMode );
            CheckDlgButton( hWnd, IDC_CHK_COLL_NI, imp->enableCollision );
			EnableWindow( GetDlgItem( hWnd, IDC_CHK_COLL_NI ), TRUE );
            CheckDlgButton( hWnd, IDC_CHK_ANIMATION_NI, imp->enableAnimations );
            CheckDlgButton( hWnd, IDC_CHK_LIGHTS_NI, imp->enableLights );
            CheckDlgButton( hWnd, IDC_CHK_CAMERA_NI, imp->enableCameras );

            CheckDlgButton( hWnd, IDC_CHK_FLIP_UV_NI, imp->flipUVTextures );
            CheckDlgButton( hWnd, IDC_CHK_SHOW_TEX_NI, imp->showTextures );
            CheckDlgButton( hWnd, IDC_CHK_AUTOSMOOTH_NI, imp->enableAutoSmooth );
            CheckDlgButton( hWnd, IDC_CHK_ILLEGAL_NI, imp->removeIllegalFaces );
            CheckDlgButton( hWnd, IDC_CHK_REM_BONES_NI, imp->removeUnusedImportedBones );
            CheckDlgButton( hWnd, IDC_CHK_CLEARANIM_NI, imp->clearAnimation );
            CheckDlgButton( hWnd, IDC_CHK_KEYNOTES_NI, imp->addNoteTracks );
            CheckDlgButton( hWnd, IDC_CHK_TIMETAGS_NI, imp->addTimeTags );
            CheckDlgButton( hWnd, IDC_CHK_IGNORE_ROOT_NI, imp->ignoreRootNode );
			CheckDlgButton( hWnd, IDC_CHK_USE_SHADER_NI, imp->useNiftoolsShader );

			CheckDlgButton( hWnd, IDC_CHK_BIPED_NI, imp->useBiped );
            CheckDlgButton( hWnd, IDC_CHK_UPB_NI, !imp->importUPB );

			std::wstring selection = ( imp->appSettings ) ? imp->appSettings->Name : _T( "User" );
			for( AppSettingsMap::iterator itr = TheAppSettings.begin( ), end = TheAppSettings.end( ); itr != end; ++itr )
				SendDlgItemMessage( hWnd, IDC_CB_GAME_NI, CB_ADDSTRING, 0, LPARAM( itr->Name.c_str( ) ) );

			SendDlgItemMessage( hWnd, IDC_CB_GAME_NI, CB_SELECTSTRING, WPARAM( -1 ), LPARAM( selection.c_str( ) ) );
			CheckDlgButton( hWnd, IDC_CHK_AUTO_DETECT_NI, imp->autoDetect );

			SHAutoComplete( GetDlgItem( hWnd, IDC_ED_SKELETON_NI ), SHACF_FILESYSTEM );
			if( imp->HasSkeleton( ) && imp->appSettings && imp->importSkeleton )
				SetDlgItemText( hWnd, IDC_ED_SKELETON_NI, imp->skeleton.c_str( ) );

			else
			{
				EnableWindow( GetDlgItem( hWnd, IDC_STC_SKELETON_NI ), FALSE );
				EnableWindow( GetDlgItem( hWnd, IDC_ED_SKELETON_NI ), FALSE );
				EnableWindow( GetDlgItem( hWnd, IDC_BTN_BROWSE_NI ), FALSE );
				EnableWindow( GetDlgItem( hWnd, IDC_CHK_BIPED_NI ), FALSE );
				EnableWindow( GetDlgItem( hWnd, IDC_CHK_REM_BONES_NI ), FALSE );
			}

			// Weld Threshold
			TSTR weldThresh;
			weldThresh.printf( _T( "%g" ), imp->weldVertexThresh );

			CheckDlgButton( hWnd, IDC_CHK_WELD_NI, imp->weldVertices );
			EnableWindow( GetDlgItem( hWnd, IDC_CHK_WELD_NI ), TRUE );

			// Collision:
			CheckDlgButton( hWnd, IDC_CHK_COLL_NI, imp->enableCollision );
			EnableWindow( GetDlgItem( hWnd, IDC_CHK_COLL_NI ), TRUE );

			SetDlgItemText( hWnd, IDC_EDIT_WELDTHRESH_NI, weldThresh );
			EnableWindow( GetDlgItem( hWnd, IDC_EDIT_WELDTHRESH_NI ), imp->weldVertices );

			//ConvertStaticToHyperlink( hWnd, IDC_LBL_LINK_NI );
			//ConvertStaticToHyperlink( hWnd, IDC_LBL_WIKI_NI );
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog( hWnd, dlgRes );
		return TRUE;

	case WM_COMMAND:
		{
			if( HIWORD( wParam ) == BN_CLICKED )
			{
				wchar_t tmp[ MAX_PATH ];
				switch( LOWORD( wParam ) )
				{
				case IDOK:

					imp->importBones = IsDlgButtonChecked( hWnd, IDC_CHK_BONES_NI ) ? true : false;
					imp->enableSkinSupport = IsDlgButtonChecked( hWnd, IDC_CHK_SKIN_NI ) ? true : false;
					imp->vertexColorMode = (int) IsDlgButtonChecked( hWnd, IDC_CHK_VCOLORS_NI );
					imp->enableCollision = IsDlgButtonChecked( hWnd, IDC_CHK_COLL_NI ) ? true : false;
					imp->enableCameras = IsDlgButtonChecked( hWnd, IDC_CHK_CAMERA_NI ) ? true : false;
					imp->enableLights = IsDlgButtonChecked( hWnd, IDC_CHK_LIGHTS_NI ) ? true : false;
					imp->enableAnimations = IsDlgButtonChecked( hWnd, IDC_CHK_ANIMATION_NI ) ? true : false;

					imp->flipUVTextures = IsDlgButtonChecked( hWnd, IDC_CHK_FLIP_UV_NI ) ? true : false;
					imp->showTextures = IsDlgButtonChecked( hWnd, IDC_CHK_SHOW_TEX_NI ) ? true : false;
					imp->enableAutoSmooth = IsDlgButtonChecked( hWnd, IDC_CHK_AUTOSMOOTH_NI ) ? true : false;
					imp->removeDegenerateFaces =
					imp->removeIllegalFaces = IsDlgButtonChecked( hWnd, IDC_CHK_ILLEGAL_NI ) ? true : false;
					imp->removeUnusedImportedBones = IsDlgButtonChecked( hWnd, IDC_CHK_REM_BONES_NI ) ? true : false;
					imp->clearAnimation = IsDlgButtonChecked( hWnd, IDC_CHK_CLEARANIM_NI ) ? true : false;
					imp->addNoteTracks = IsDlgButtonChecked( hWnd, IDC_CHK_KEYNOTES_NI ) ? true : false;
					imp->addTimeTags = IsDlgButtonChecked( hWnd, IDC_CHK_TIMETAGS_NI ) ? true : false;
					imp->useBiped = IsDlgButtonChecked( hWnd, IDC_CHK_BIPED_NI ) ? true : false;
					imp->importUPB = IsDlgButtonChecked( hWnd, IDC_CHK_UPB_NI ) ? false : true;
					imp->ignoreRootNode = IsDlgButtonChecked( hWnd, IDC_CHK_IGNORE_ROOT_NI ) ? true : false;
					imp->useNiftoolsShader = IsDlgButtonChecked( hWnd, IDC_CHK_USE_SHADER_NI ) ? true : false;
                  
					GetDlgItemText( hWnd, IDC_CB_GAME_NI, tmp, MAX_PATH );
					if( AppSettings *appSettings = FindAppSetting( tmp ) ) 
						imp->appSettings = appSettings;

					imp->autoDetect = IsDlgButtonChecked( hWnd, IDC_CHK_AUTO_DETECT_NI ) ? true : false;
                  
					GetDlgItemText( hWnd, IDC_ED_SKELETON_NI, tmp, MAX_PATH );
					imp->skeleton = tmp;

					// Weld Threshold
					imp->weldVertices = IsDlgButtonChecked( hWnd, IDC_CHK_WELD_NI )? true : false;
					GetDlgItemText( hWnd, IDC_EDIT_WELDTHRESH_NI, tmp, MAX_PATH );
					if( wcslen( tmp ) > 0 ) 
						imp->weldVertexThresh = (float) _wtof( tmp );

					EndDialog( hWnd, dlgRes = IDOK );
					return TRUE;

				case IDCANCEL:
					EndDialog( hWnd, dlgRes = IDCANCEL );
					return TRUE;

				case IDC_BTN_BROWSE_NI:
					{
						// TODO: ...
					}
					break;

				case IDC_CHK_WELD_NI:
					EnableWindow( GetDlgItem( hWnd, IDC_EDIT_WELDTHRESH_NI ), IsDlgButtonChecked( hWnd, IDC_CHK_WELD_NI ) );
					break;

				case IDC_LBL_LINK_NI:
					//ShellExecute( hWnd, _T( "open" ), imp->webSite, NULL, NULL, SW_SHOWNORMAL );
					break;

				case IDC_LBL_WIKI_NI:
					//ShellExecute( hWnd, _T( "open" ), imp->wikiSite, NULL, NULL, SW_SHOWNORMAL );
					break;
				}
			}
			else if( HIWORD( wParam ) == CBN_SELCHANGE )
			{
				if( LOWORD( wParam ) == IDC_CB_GAME_NI )
				{
					wchar_t tmp[ MAX_PATH ];
					GetDlgItemText( hWnd, IDC_CB_GAME_NI, tmp, MAX_PATH );

					if( AppSettings* appSettings = FindAppSetting( tmp ) )
					{
						std::wstring skeleton = imp->GetSkeleton( appSettings );
						BOOL enable = imp->HasSkeleton( ) ? TRUE : FALSE;

						if( enable )
							SetDlgItemText( hWnd, IDC_ED_SKELETON_NI, skeleton.c_str( ) );

						EnableWindow( GetDlgItem( hWnd, IDC_STC_SKELETON_NI ), enable );
						EnableWindow( GetDlgItem( hWnd, IDC_ED_SKELETON_NI ), enable );
						EnableWindow( GetDlgItem( hWnd, IDC_BTN_BROWSE_NI ), enable );
					}
				}
			}
		}
		break;
	}

	return FALSE;
}

bool NifImporter::ShowDialog( )
{
	return ( IDOK == DialogBoxParam( hInstance, MAKEINTRESOURCE( IDD_NIF_PANEL_NI ), GetActiveWindow( ), MaxNifImportOptionsDlgProc, (LPARAM) this ) );
}