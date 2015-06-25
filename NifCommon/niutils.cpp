
#include "stdafx.h"

#define STRDUPA( p ) ( _tcscpy( (TCHAR*) alloca( ( _tcslen( p ) + 1 ) * sizeof( *p ) ), p ) )

Point3 TOEULER(const Matrix3 &m)
{
   Point3 rv(0.0f, 0.0f, 0.0f);
   if ( m.GetRow(2)[0] < 1.0 )
   {
      if ( m.GetRow(2)[0] > - 1.0 )
      {
         rv[2] = atan2( - m.GetRow(1)[0], m.GetRow(0)[0] );
         rv[1] = asin( m.GetRow(2)[0] );
         rv[0] = atan2( - m.GetRow(2)[1], m.GetRow(2)[2] );
      }
      else
      {
         rv[2] = - atan2( - m.GetRow(1)[2], m.GetRow(1)[1] );
         rv[1] = - PI / 2;
         rv[0] = 0.0;
      }
   }
   else
   {
      rv[2] = atan2( m.GetRow(1)[2], m.GetRow(1)[1] );
      rv[1] = PI / 2;
      rv[0] = 0.0;
   }
   return rv;
}

// routine for parsing white space separated lines.  Handled like command line parameters w.r.t quotes.
static void parse_line (
                        const char *start,
                        char **argv,
                        char *args,
                        int *numargs,
                        int *numchars
                        )
{
   const char NULCHAR    = '\0';
   const char SPACECHAR  = ' ';
   const char TABCHAR    = '\t';
   const char RETURNCHAR = '\r';
   const char LINEFEEDCHAR = '\n';
   const char DQUOTECHAR = '\"';
   const char SLASHCHAR  = '\\';
   const char *p;
   int inquote;                    /* 1 = inside quotes */
   int copychar;                   /* 1 = copy char to *args */
   unsigned numslash;              /* num of backslashes seen */

   *numchars = 0;
   *numargs = 0;                   /* the program name at least */

   p = start;

   inquote = 0;

   /* loop on each argument */
   for(;;) 
   {
      if ( *p ) { while (*p == SPACECHAR || *p == TABCHAR || *p == RETURNCHAR || *p == LINEFEEDCHAR) ++p; }

      if (*p == NULCHAR) break; /* end of args */

      /* scan an argument */
      if (argv)
         *argv++ = args;     /* store ptr to arg */
      ++*numargs;

      /* loop through scanning one argument */
      for (;;) 
      {
         copychar = 1;
         /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
         2N+1 backslashes + " ==> N backslashes + literal "
         N backslashes ==> N backslashes */
         numslash = 0;
         while (*p == SLASHCHAR) 
         {
            /* count number of backslashes for use below */
            ++p;
            ++numslash;
         }
         if (*p == DQUOTECHAR) 
         {
            /* if 2N backslashes before, start/end quote, otherwise copy literally */
            if (numslash % 2 == 0) {
               if (inquote) {
                  if (p[1] == DQUOTECHAR)
                     p++;    /* Double quote inside quoted string */
                  else        /* skip first quote char and copy second */
                     copychar = 0;
               } else
                  copychar = 0;       /* don't copy quote */

               inquote = !inquote;
            }
            numslash /= 2;          /* divide numslash by two */
         }

         /* copy slashes */
         while (numslash--) {
            if (args)
               *args++ = SLASHCHAR;
            ++*numchars;
         }

         /* if at end of arg, break loop */
         if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR || *p == RETURNCHAR || *p == LINEFEEDCHAR)))
            break;

         /* copy character into argument */
         if (copychar) 
         {
            if (args)
               *args++ = *p;
            ++*numchars;
         }
         ++p;
      }

      /* null-terminate the argument */
      if (args)
         *args++ = NULCHAR;          /* terminate string */
      ++*numchars;
   }
   /* We put one last argument in -- a null ptr */
   if (argv)
      *argv++ = NULL;
   ++*numargs;
}

void GetIniFileName( wchar_t* iniName )
{
	Interface* gi = GetCOREInterface( );

	if( gi )
	{
		LPCTSTR pluginDir = gi->GetDir( APP_PLUGCFG_DIR );
		PathCombine( iniName, pluginDir, _T( "MaxNifTools.ini" ) );

		int forcePlugcfg = GetIniValue( _T( "System" ), _T( "ForcePlugcfg" ), 0, iniName );

		if( forcePlugcfg == 1 || _waccess( iniName, 06 ) != 0 )
		{
			TCHAR iniPath[ MAX_PATH ];
			GetModuleFileName( NULL, iniPath, MAX_PATH );

			if( LPTSTR fname = PathFindFileName( iniPath ) )
				*fname = 0;

			PathAddBackslash( iniPath );
			PathAppend( iniPath, _T( "plugcfg" ) );
			PathAppend( iniPath, _T( "MaxNifTools.ini" ) );

			if( _waccess( iniPath, 06 ) == 0 )
				wcscpy( iniName, iniPath );
		}
	}
	else
	{
		GetModuleFileName( NULL, iniName, MAX_PATH );

		if( LPTSTR fname = PathFindFileName( iniName ) )
			*fname = 0;

		PathAddBackslash( iniName );
		PathAppend( iniName, _T( "plugcfg" ) );
		PathAppend( iniName, _T( "MaxNifTools.ini" ) );
	}

	if( _waccess( iniName, 06 ) != 0 )
		MessageBox( NULL, _T( "MaxNifTools could not find a valid INI. The plugin may not work correctly.\nPlease check for proper installation." ), _T( "MaxNifTools" ), MB_OK|MB_ICONWARNING );
}

stringlist TokenizeString( LPCWSTR str, LPCWSTR delims, bool trim )
{
	stringlist values;
	LPWSTR buf = STRDUPA( str );

	for( LPWSTR p = _tcstok( buf, delims ); p && *p; p = _tcstok( NULL, delims ) )
		values.push_back( std::wstring( ( trim ) ? Trim( p ) : p ) );

	return values;
}

// Tokenize a string using strtok and return it as a stringlist
stringlist TokenizeCommandLine( LPCWSTR str, bool trim )
{
	stringlist values;
	int nargs = 0;
	int nchars = 0;

	parse_line( narrow( str ).c_str( ), NULL, NULL, &nargs, &nchars );
	char** largv = (char**) _alloca( nargs * sizeof( TCHAR* ) + nchars * sizeof( TCHAR ) );
	parse_line( narrow( str ).c_str( ), largv, ((char*)largv) + nargs * sizeof( char* ), &nargs, &nchars );

	for( int i = 0; i < nargs; ++i )
	{
		LPSTR p = largv[ i ];
		if( p == NULL )
			continue;

		std::wstring val = wide( std::string( p ) );
		values.push_back( ( trim ) ? Trim( val ) : val );
	}

	return values;
}

// sprintf for TSTR without having to worry about buffer size.
TSTR FormatText( const TCHAR* format, ... )
{
	TCHAR buffer[ 512 ];
	TSTR text;
	va_list args;
	va_start( args, format );

	int nChars = _vsnwprintf( buffer, _countof( buffer ), format, args );
	if( nChars != -1 )
		text = buffer;
	else
	{
		size_t Size = _vscwprintf( format, args );
		text.Resize( Size );
		nChars = _vsnwprintf( (wchar_t*) text.data( ), Size, format, args );
	}

	va_end( args );

	return text;
}

// sprintf for std::wstring without having to worry about buffer size.
std::wstring FormatString( const TCHAR* format, ... )
{
	TCHAR buffer[ 512 ];
	std::wstring text;
	va_list args;
	va_start( args, format );

	int nChars = _vsnwprintf( buffer, _countof( buffer ), format, args );
	if( nChars != -1 )
		text = buffer;
	else
	{
		size_t Size = _vscwprintf( format, args );
		TCHAR* pbuf = (TCHAR*) _alloca( Size );
		nChars = _vsnwprintf( pbuf, Size, format, args );
		text = pbuf;
	}

	va_end( args );

	return text;
}

TSTR GetFileVersion( const wchar_t* fileName )
{
	TSTR retval;
	wchar_t fileVersion[ MAX_PATH ];

	if( fileName == NULL )
	{
		GetModuleFileName( hInstance, fileVersion, MAX_PATH );
		fileName = fileVersion;
	}

	HMODULE ver = GetModuleHandle( _T( "version.dll" ) );
	
	if( !ver )
		ver = LoadLibrary( _T( "version.dll" ) );

	if( ver != NULL )
	{
		DWORD ( APIENTRY* GetFileVersionInfoSize )( LPCWSTR, LPDWORD ) = NULL;
		BOOL ( APIENTRY* GetFileVersionInfo )( LPCWSTR, DWORD, DWORD, LPVOID ) = NULL;
		BOOL ( APIENTRY* VerQueryValue )( const LPVOID, LPCWSTR, LPVOID*, PUINT ) = NULL;
		*(FARPROC*) &GetFileVersionInfoSize = GetProcAddress( ver, "GetFileVersionInfoSizeW" );
		*(FARPROC*) &GetFileVersionInfo = GetProcAddress( ver, "GetFileVersionInfoW" );
		*(FARPROC*) &VerQueryValue = GetProcAddress( ver, "VerQueryValueW" );

		if( GetFileVersionInfoSize && GetFileVersionInfo && VerQueryValue )
		{
			DWORD vLen = 0;
			DWORD vSize = GetFileVersionInfoSize( fileName, &vLen );
			
			if( vSize )
			{
				LPVOID versionInfo = malloc( vSize + 1 );
				if( GetFileVersionInfo( fileName, vLen, vSize, versionInfo ) )
				{
					LPVOID version = NULL;
					if( VerQueryValue( versionInfo, _T( "\\VarFileInfo\\Translation" ), &version, (UINT*) &vLen ) && vLen == 4 )
					{
						DWORD langD = *(DWORD*) version;
						swprintf( fileVersion, _T( "\\StringFileInfo\\%02X%02X%02X%02X\\ProductVersion" ), 
							( langD & 0xff00 ) >> 8, langD & 0xff, ( langD & 0xff000000 ) >> 24, ( langD & 0xff0000 ) >> 16 );
					}
					else
					{
						swprintf( fileVersion, _T( "\\StringFileInfo\\%04X04B0\\ProductVersion" ), GetUserDefaultLangID( ) );
					}

					LPCWSTR value = NULL;
					if( VerQueryValue( versionInfo, fileVersion, &version, (UINT*) &vLen ) )
						value = LPCWSTR( version );

					else if( VerQueryValue( versionInfo, _T( "\\StringFileInfo\\040904B0\\ProductVersion" ), &version, (UINT*) &vLen ) )
						value = LPCWSTR( version );

					if( value != NULL )
					{
						stringlist val = TokenizeString( value, _T( "," ), true );
						if( val.size( ) >= 4 )
							retval = FormatText( _T( "%s.%s.%s.%s" ), val[0].c_str( ), val[1].c_str( ), val[2].c_str( ), val[3].c_str( ) );
					}
					free( versionInfo );
				}
			}
		}
	}

	return retval;
}

// Parse an ini file section and return the results as a NameValueCollection
NameValueCollection ReadIniSection( LPCWSTR Section, LPCWSTR iniFileName )
{
	NameValueCollection map;
	DWORD len = 2048 * sizeof( TCHAR );
	LPWSTR buf = (LPWSTR) calloc( len + 2, 1 );

	while( NULL != buf )
	{
		DWORD rlen = GetPrivateProfileSection( Section, buf, len, iniFileName );
		if( rlen != ( len - 2 ) )
			break;

		len += 2;
		buf = (LPWSTR) realloc( buf, len );
	}

	if( NULL != buf )
	{
		for( LPWSTR line = buf, next = line + wcslen( line ) + 1; *line; line = next, next = line + wcslen( line ) + 1 )
		{
			Trim( line );
			if( line[ 0 ] == _T( ';' ) || line[ 0 ] == 0 )
				continue;

			if( LPWSTR equals = wcschr( line, _T( '=' ) ) )
			{
				*equals++ = 0;
				Trim( line );
				Trim( equals );
				map[ std::wstring( line ) ] = std::wstring( equals );
			}
		}
	}

	return map;
}

struct str2hdl
{
	const TCHAR* str;
	HANDLE key;
};

const static struct str2hdl RegKeyMap[] = 
{
	{ _T( "HKLM" ), HKEY_LOCAL_MACHINE },
	{ _T( "HKCU" ), HKEY_CURRENT_USER },
	{ _T( "HKCR" ), HKEY_CLASSES_ROOT },

	{ _T( "HKEY_LOCAL_MACHINE" ), HKEY_LOCAL_MACHINE },
	{ _T( "HKEY_CURRENT_USER" ), HKEY_CURRENT_USER },
	{ _T( "HKEY_CLASSES_ROOT" ), HKEY_CLASSES_ROOT },
};

static HANDLE GetRegKey( LPCWSTR key )
{
	for( int i = 0; i < _countof( RegKeyMap ); ++i )
	{
		if( 0 == wcscmp( RegKeyMap[ i ].str, key ) )
			return RegKeyMap[ i ].key;
	}

	return 0;
}

// Returns value from indirect source
//  Currently only supports registry string values using '[HKEY\Key]=@"Value"' 
//  where  HKEY is HKLM,HKCU,HKCR  
//         Key is the registry key to lookup
//         Value is the data value to lookup.  
std::wstring GetIndirectValue( LPCWSTR path )
{
	if( !path || !*path )
		return std::wstring( );

	std::wstring value;
	LPWSTR p = STRDUPA( path );
	Trim( p );

	if( *p == _T( 'p' ) )
	{
		LPWSTR end = wcschr( ++p, ']' );
		if( end != NULL )
		{
			*end++ = 0;

			// Advance unsafetly past unnecesary qualifiers
			LPWSTR valueName = end;
			end = valueName + wcslen( end ) - 1;

			if( *valueName == '=' ) ++valueName;
			if( *valueName == '@' ) ++valueName;
			if( *valueName == '\"' || *valueName == '\'' ) ++valueName;
			if( *end == '\"' || *end == '\'' ) *end-- = 0;

			Trim( valueName );
			if( wcslen( valueName ) == 0 )
				valueName = NULL;

			LPWSTR keyEnd = wcschr( p, '\\' );
			if( keyEnd != NULL )
			{
				*keyEnd++ = 0;
				HANDLE hRoot = GetRegKey( p );

				if( hRoot != 0 )
				{
					HKEY hKey = NULL;
					if( ERROR_SUCCESS == RegOpenKeyEx( (HKEY) hRoot, keyEnd, 0, KEY_READ, &hKey ) )
					{
						BYTE buffer[ MAX_PATH * sizeof( *path ) ];
						DWORD dwLen = _countof( buffer );

						if( ERROR_SUCCESS == RegQueryValueEx( hKey, valueName, NULL, NULL, (LPBYTE) buffer, &dwLen ) )
							value = (TCHAR*) buffer;

						RegCloseKey( hKey );
					}
				}
			}
		}
	}
	else
		value = path;

	return value;
}

// Expand Qualifiers in string using a ${Name} syntax.  Name will be looked up in the
//    NameValueCollection and expand in place.  Missing names will expand to empty.
//    - Please dont give self-referential strings
std::wstring ExpandQualifiers( const std::wstring& src, const NameValueCollection& map )
{
	std::wstring value;
	bool hasPercent = false;
	std::wstring::size_type end = src.length( );
	
	value.reserve( src.length( ) );

	for( std::wstring::size_type i = 0; i < end; ++i )
	{
		if( src[ i ] == '$' )
		{
			if( ++i < end )
			{
				if( src[ i ] == '$' )
					value.append( 1, src[ i ] );

				else if( src[ i ] == '{' )
				{
					std::wstring::size_type term = src.find_first_of( '}', i );
					if( term == std::wstring::npos )
						i = end;

					else
					{
						std::wstring key = src.substr( i + 1, term - i - 1 );
						NameValueCollection::const_iterator kvp = map.find( key );
						
						if( kvp != map.end( ) )
							value.append( ExpandQualifiers( kvp->second, map ) );

						i = term;
					}
				}
				else if( src[ i ] == '(' )
				{
					std::wstring::size_type term = src.find_first_of( ')', i );
					if( term == std::wstring::npos )
						i = end;

					else
					{
						std::wstring key = src.substr( i + 1, term - i - 1 );
						NameValueCollection::const_iterator kvp = map.find( key );

						if( kvp != map.end( ) )
							value.append( ExpandQualifiers( kvp->second, map ) );

						i = term;
					}
				}
			}
			else
				value.append( 1, '$' );
		}
		else
			value.append( 1, src[ i ] );
	}

	return value;
}

// Call ExpandEnvironmentStrings but with std wstring instead
std::wstring ExpandEnvironment( const std::wstring& src )
{
	DWORD Size = ExpandEnvironmentStrings( src.c_str( ), NULL, 0 );
	TCHAR* pbuf = new TCHAR[ Size + 2 ];

	//if( TCHAR* pbuf = (TCHAR*) malloc( Size + 2 ) )
	//{
		pbuf[ 0 ] = 0;
		ExpandEnvironmentStrings( src.c_str( ), pbuf, Size + 2 );

		return std::wstring( pbuf );
	//}

	//return src;
}

// Original Source: Jack Handy www.codeproject.com
int wildcmp( const TCHAR* wild, const TCHAR* string )
{
	const TCHAR *cp, *mp;

	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
			return 1;
			}
			mp = wild;
			cp = string+1;
		} else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}

// Same as above but case insensitive
int wildcmpi( const TCHAR* wild, const TCHAR* string )
{
	const TCHAR *cp, *mp;
	int f,l;
	while ((*string) && (*wild != '*')) {
		f = _totlower( (_TUCHAR)(*string) );
		l = _totlower( (_TUCHAR)(*wild) );
		if ((f != l) && (l != '?')) {
			return 0;
		}
		wild++, string++;
	}
	while (*string) {
		if (*wild == '*') {
			if (!*++wild) return 1;
			mp = wild, cp = string+1;
		} else {
			f = _totlower( (_TUCHAR)(*string) );
			l = _totlower( (_TUCHAR)(*wild) );
			if ((f == l) || (l == '?')) {
			wild++, string++;
			} else {
			wild = mp, string = cp++;
			}
		}
	}
	while (*wild == '*') wild++;
	return !*wild;
}

bool wildmatch( const TCHAR* match, const TCHAR* value )
{
	return ( wildcmpi( match, value ) ) ? true : false;
}

bool wildmatch( const std::wstring& match, const std::wstring& value )
{
	return ( wildcmpi( match.c_str( ), value.c_str( ) ) ) ? true : false;
}

bool wildmatch( const stringlist& matches, const std::wstring& value )
{
	for( stringlist::const_iterator itr = matches.begin( ), end = matches.end( ); itr != end; ++itr )
		if( wildcmpi( (*itr).c_str( ), value.c_str( ) ) )
			return true;

	return false;
}

// Set Position and Rotation on a standard controller will need to handle bipeds
// Always in World Transform coordinates
void PosRotScaleNode( INode* n, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t )
{
	if( Control* c = n->GetTMController( ) )
	{
		if( prs & prsRot && q.w == FloatNegINF ) prs = PosRotScale( prs * ~prsRot );
		if( prs & prsPos && p.x == FloatNegINF ) prs = PosRotScale( prs * ~prsPos );
		if( prs & prsScale && s == FloatNegINF ) prs = PosRotScale( prs * ~prsScale );

#ifdef USE_BIPED
		// Bipeds are special. And will crash if you dont treat them with care
		if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
			||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
			||(c->ClassID() == FOOTPRINT_CLASS_ID))
		{
			ScaleValue sv(Point3(s,s,s));
			// Get the Biped Export Interface from the controller 
			//IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);
			IOurBipExport *BipIface = (IOurBipExport *) c->GetInterface(I_OURINTERFACE);
			if (prs & prsScale)
			BipIface->SetBipedScale(sv, t, n);
			if (prs & prsRot)
			BipIface->SetBipedRotation(q, t, n, 0/*???*/);
			if (prs & prsPos)
			BipIface->SetBipedPosition(p, t, n);
			return;
		}
#endif
		PosRotScaleNode( c, p, q, s, prs, t );

	}
}

void PosRotScaleNode( Control* c, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t )
{
	if( c ) 
	{
		if( prs & prsRot && q.w == FloatNegINF ) prs = PosRotScale( prs & ~prsRot );
		if( prs & prsPos && p.x == FloatNegINF ) prs = PosRotScale( prs & ~prsPos );
		if( prs & prsScale && s == FloatNegINF ) prs = PosRotScale( prs & ~prsScale );

		ScaleValue sv( Point3( s, s, s ) );
		if( prs & prsScale )
			if( Control *sclCtrl = c->GetScaleController( ) )
		sclCtrl->SetValue( t, &sv, 1, CTRL_ABSOLUTE );

		if( prs & prsRot )
			if( Control *rotCtrl = c->GetRotationController( ) )
		rotCtrl->SetValue( t, &q, 1, CTRL_ABSOLUTE );

		if( prs & prsPos )
			if( Control *posCtrl = c->GetPositionController( ) )
		posCtrl->SetValue( t, &p, 1, CTRL_ABSOLUTE );
	}
}

void PosRotScaleNode( INode* n, Matrix3& m3, PosRotScale prs, TimeValue t )
{
	Point3 p = m3.GetTrans( );
	Quat q = m3;
	Matrix3 stm = m3 * Inverse( m3 );
	float s = ( sqrt( stm.GetRow( 0 )[ 0 ] ) + sqrt( stm.GetRow( 1 )[ 1 ] ) + sqrt( stm.GetRow( 1 )[ 1 ] ) ) / 3.0f;
	PosRotScaleNode( n, p, q, s, prs, t );
}

void PosRotScaleNode( Control* c, Matrix3& m3, PosRotScale prs, TimeValue t )
{
	// Doesn't exist
}

Matrix3 GetNodeLocalTM( INode* n, TimeValue t )
{
	Matrix3 m3 = n->GetNodeTM( t );
	Matrix3 m3p = n->GetParentTM( t );

	m3p.Invert( );

	return m3 * m3p;
}

TSTR EnumToString( int value, const EnumLookupType* table )
{
	for( const EnumLookupType* itr = table; itr->name != NULL; ++itr )
		if( itr->value == value )
			return TSTR( itr->name );

	return FormatText( _T( "%x" ), value );
}

int StringToEnum( TSTR value, const EnumLookupType* table )
{
	if( value.isNull( ) )
		return 0;

	for( const EnumLookupType* itr = table; itr->name != NULL; ++itr )
		if( 0 == _wcsicmp( value, itr->name ) )
			return itr->value;

	wchar_t* end = NULL;

	return (int) wcstol( value, &end, 0 );
}

int EnumToIndex( int value, const EnumLookupType* table )
{
	int i = 0;
	for( const EnumLookupType* itr = table; itr->name !=  NULL; ++itr, ++i )
		if( itr->value == value )
			return i;

	return -1;
}

// Get or create the Skin Modifier
Modifier* GetOrCreateSkin( INode* node )
{
	Modifier* skinMod = GetSkin( node );
	if( skinMod )
		return skinMod;

	IDerivedObject* dobj = CreateDerivedObject( node->GetObjectRef( ) );

	// Create a skin modifier and add it
	skinMod = (Modifier*) CreateInstance( OSM_CLASS_ID, SKIN_CLASSID );
	dobj->SetAFlag( A_LOCK_TARGET );
	dobj->AddModifier( skinMod );
	dobj->ClearAFlag( A_LOCK_TARGET );
	
	node->SetObjectRef( dobj );

	return skinMod;
}

TriObject* GetTriObject( Object* o )
{
	if( o && o->CanConvertToType( triObjectClassID ) )
		return (TriObject*) o->ConvertToType( 0, triObjectClassID );

	while( o->SuperClassID( ) == GEN_DERIVOB_CLASS_ID && o )
	{
		IDerivedObject* dobj = (IDerivedObject*) ( o );
		o = dobj->GetObjRef( );

		if( o && o->CanConvertToType( triObjectClassID ) )
			return (TriObject*) o->ConvertToType( 0, triObjectClassID );
	}

	return NULL;
}

Modifier* GetSkin( INode* node )
{
	Object* pObj = node->GetObjectRef( );
	if( !pObj )
		return NULL;

	while( pObj->SuperClassID( ) == GEN_DERIVOB_CLASS_ID )
	{
		IDerivedObject* pDerObj = (IDerivedObject*) ( pObj );
		int Idx = 0;

		while( Idx < pDerObj->NumModifiers( ) )
		{
			// Get the modifier
			Modifier* mod = pDerObj->GetModifier( Idx );
			if( mod->ClassID( ) == SKIN_CLASSID )
				// is this the correct Physique Modifier based on index?
				return mod;

			Idx++;
		}

		pObj = pDerObj->GetObjRef( );
	}

	return NULL;
}

Modifier* GetbhkCollisionModifier( INode* node )
{
	extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;

	Object* pObj = node->GetObjectRef( );
	if( !pObj )
		return NULL;

	while( pObj->SuperClassID( ) == GEN_DERIVOB_CLASS_ID )
	{
		IDerivedObject* pDerObj = (IDerivedObject*) ( pObj );
		int Idx = 0;

		while( Idx < pDerObj->NumModifiers( ) )
		{
			// Get the modifier
			Modifier* mod = pDerObj->GetModifier( Idx );
			if( mod->ClassID( ) == BHKRIGIDBODYMODIFIER_CLASS_ID )
				return mod;

			Idx++;
		}

		pObj = pDerObj->GetObjRef( );
	}

	return NULL;
}

Modifier* CreatebhkCollisionModifier( INode* node, int type, Niflib::HavokMaterial material, Niflib::OblivionLayer layer, byte filter )
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, PB_LAYER, PB_FILTER, };
	extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;

	Modifier* rbMod = GetbhkCollisionModifier( node );
	if( rbMod == NULL )
	{
		IDerivedObject* dobj = CreateDerivedObject( node->GetObjectRef( ) );
		rbMod = (Modifier*) CreateInstance( OSM_CLASS_ID, BHKRIGIDBODYMODIFIER_CLASS_ID );

		dobj->SetAFlag( A_LOCK_TARGET );
		dobj->AddModifier( rbMod );
		dobj->ClearAFlag( A_LOCK_TARGET );

		node->SetObjectRef( dobj );
	}

	if( IParamBlock2* pblock2 = rbMod->GetParamBlockByID( havok_params ) )
	{
		pblock2->SetValue( PB_BOUND_TYPE, 0, type, 0 );
		pblock2->SetValue( PB_MATERIAL, 0, material, 0 );
		pblock2->SetValue( PB_LAYER, 0, layer, 0 );
		pblock2->SetValue( PB_FILTER, 0, filter, 0 );
	}

	return rbMod;
}

// Search NiNode collection names that match a wildcard
std::vector<Niflib::NiNodeRef> SelectNodesByName( const std::vector<Niflib::NiNodeRef>& blocks, LPCTSTR match )
{
	std::vector<Niflib::NiNodeRef> nodes;
	for( std::vector<Niflib::NiNodeRef>::const_iterator itr = blocks.begin( ), end = blocks.end( ); itr != end; ++itr )
	{
		const Niflib::NiNodeRef& block = (*itr);
		if( wildcmpi( match, wide( block->GetName( ) ).c_str( ) ) )
			nodes.insert( nodes.end( ), block );
	}

	return nodes;
}

// Get a vector of names from NiNode vector
std::vector<std::wstring> GetNamesOfNodes( const std::vector<Niflib::NiNodeRef>& blocks )
{
	std::vector<std::wstring> slist;
	for( std::vector<Niflib::NiNodeRef>::const_iterator itr = blocks.begin( ), end = blocks.end( ); itr != end; ++itr )
	{
		const Niflib::NiNodeRef& block = (*itr);
		slist.push_back( wide( block->GetName( ) ) );
	}

	return slist;
}

// Recursively search through directories applying a filter on what to return
template <typename FileMatch>
void BuildFileNameMap( NameValueCollection& collection, const TCHAR* root, const TCHAR* path, FileMatch pred )
{
	TCHAR buffer[ MAX_PATH ];
	TCHAR buffer2[ MAX_PATH ];
	TCHAR search[ MAX_PATH ];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	ZeroMemory( &FindFileData, sizeof( FindFileData ) );
	if( path == NULL || path[ 0 ] == 0 )
		return;

	PathCanonicalize( search, path );
	PathAddBackslash( search );

	wcscat( search, _T( "*" ) );

	hFind = FindFirstFile( search, &FindFileData );
	if( hFind != INVALID_HANDLE_VALUE )
	{
		stringlist list;
		for( BOOL ok = TRUE; ok; ok = FindNextFile( hFind, &FindFileData ) )
		{
			if( FindFileData.cFileName[ 0 ] == '.' || ( FindFileData.dwFileAttributes & ( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) ) )
				continue;

			if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				PathCombine( buffer, path, FindFileData.cFileName );
				PathAddBackslash( buffer );
				list.push_back( buffer );
			}
			else
			{
				if( pred( FindFileData.cFileName ) )
				{
					if( collection.find( FindFileData.cFileName ) == collection.end( ) )
					{
						PathCombine( buffer, path, FindFileData.cFileName );
						GetLongPathName( buffer, buffer, MAX_PATH );
						PathRemoveExtension( FindFileData.cFileName );

						if( PathRelativePathTo( buffer2, root, FILE_ATTRIBUTE_DIRECTORY, buffer, FILE_ATTRIBUTE_NORMAL ) )
						{
							TCHAR* p = buffer2;
							while( *p == '\\' )
								++p;

							collection.insert( KeyValuePair( FindFileData.cFileName, p ) );
						}
						else
							collection.insert( KeyValuePair( FindFileData.cFileName, buffer ) );
					}
				}
			}
		}

		FindClose( hFind );
		for( stringlist::iterator itr = list.begin( ), end = list.end( ); itr != end; ++itr )
			BuildFileNameMap( collection, root, (*itr).c_str( ), pred );
	}
}

// Implementation for BuildFileNameMap which will search for a specific set of extensions
struct ExtensionMatch : public std::unary_function<LPCTSTR, bool>
{
   stringlist extns;
   ExtensionMatch( std::wstring extnlist ) 
   {
      extns = TokenizeString( extnlist.c_str(), _T( ";" ) );
   }

   ExtensionMatch( const stringlist& extnlist ) 
	   : extns( extnlist )
   {
   }

   bool operator()( LPWSTR name ) const 
   {
      LPWSTR ext = PathFindExtension( name );
      for( stringlist::const_iterator itr = extns.begin( ), end = extns.end( ); itr != end; ++itr ) 
	  {
         if( 0 == _tcsicmp( ext, (*itr).c_str( ) ) )
            return true;
      }

      return false;      
   }
};

// Run through the search paths and add them to the image collection
void FindImages( NameValueCollection& images, const std::wstring& rootPath, const stringlist& searchpaths, const stringlist& extensions )
{
	ExtensionMatch ddsMatch( extensions );
	for( stringlist::const_iterator itr = searchpaths.begin( ), end = searchpaths.end( ); itr != end; ++itr )
	{
		if( PathIsRelative( (*itr).c_str( ) ) )
		{
			TCHAR texPath[ MAX_PATH ];
			PathCombine( texPath, rootPath.c_str( ), (*itr).c_str( ) );
			PathAddBackslash( texPath );

			BuildFileNameMap( images, rootPath.c_str( ), texPath, ddsMatch );
		}
		else
			BuildFileNameMap( images, rootPath.c_str( ), (*itr).c_str( ), ddsMatch );
	}
}

std::wstring JoinCommandLine( stringlist args )
{
   std::wstringstream str;
   for( stringlist::iterator itr = args.begin( ); itr != args.end( ); ++itr )
   {
      if( itr != args.begin( ) )
		  str << ' ';

      str << (*itr);
   }
   return str.str();
}

BOOL ExecuteCommandLine( std::wstring cmdLine, DWORD & exitCode )
{
   PROCESS_INFORMATION processInformation = {0};
   STARTUPINFO startupInfo                = {0};
   startupInfo.cb                         = sizeof(startupInfo);
   int nStrBuffer                         = cmdLine.length() + 50;
 

   // Create the process
   BOOL result = CreateProcess(NULL, (LPWSTR) cmdLine.c_str( ), 
                               NULL, NULL, FALSE, 
                               NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 
                               NULL, NULL, &startupInfo, &processInformation);
 

   if (!result)
   {
      // CreateProcess() failed
      // Get the error from the system
      LPVOID lpMsgBuf;
      DWORD dw = GetLastError();
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &lpMsgBuf, 0, NULL);
 
      // We failed.
      return FALSE;
   }
   else
   {
      // Successfully created the process.  Wait for it to finish.
      WaitForSingleObject( processInformation.hProcess, INFINITE );
 
      // Get the exit code.
      result = GetExitCodeProcess(processInformation.hProcess, &exitCode);
 
      // Close the handles.
      CloseHandle( processInformation.hProcess );
      CloseHandle( processInformation.hThread );
 
      if (!result)
      {
         // Could not get exit code.
         return FALSE;
      }
 

      // We succeeded.
      return TRUE;
   }
}