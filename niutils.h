
#pragma once

const unsigned int IntegerInf = 0x7f7fffff;
const unsigned int IntegerNegInf = 0xff7fffff;
const float FloatINF = *(float*)&IntegerInf;
const float FloatNegINF = *(float*)&IntegerNegInf;

inline TCHAR* Trim( TCHAR* &p )
{
	while( _istspace( *p ) ) *p++ = 0;
	TCHAR *e = p + _tcslen( p ) - 1;
	while( e > p && _istspace( *e ) ) *e-- = 0;

	return p;
}

inline std::wstring Trim( std::wstring& p )
{
    std::wstring str = p;
    size_t found;
    found = str.find_last_not_of( ' ' );

    if( found != std::wstring::npos )
    	str.erase( found+1 );
    else
    	str.clear( ); // str is all whitespace

    return str;
}

struct ltstr
{
	bool operator()( const wchar_t* s1, const wchar_t* s2 ) const { return _wcsicmp( s1, s2 ) < 0; }
	bool operator()( const std::wstring& s1, const std::wstring& s2 ) const { return _wcsicmp( s1.c_str( ), s2.c_str( ) ) < 0; }
	bool operator()( const std::wstring& s1, const wchar_t* s2 ) const { return _wcsicmp( s1.c_str( ), s2 ) < 0; }
	bool operator()( const wchar_t* s1, const std::wstring& s2 ) const { return _wcsicmp( s1, s2.c_str( ) ) < 0; }
};

enum PosRotScale
{
	prsPos = 0x01,
	prsRot = 0x02,
	prsScale = 0x04,
	prsDefault = prsPos | prsRot | prsScale,
};

static inline float Average( const Point3& val ) 
{
   return ( val[ 0 ] + val[ 1 ] + val[ 2 ] ) / 3.0f;
}

static inline float Average( const Niflib::Vector3& val ) 
{
   return ( val.x + val.y + val.z ) / 3.0f;
}

// Common collections
typedef std::map<std::wstring, std::wstring, ltstr> NameValueCollection;
typedef std::pair<std::wstring, std::wstring> KeyValuePair;
typedef std::vector<std::wstring> stringlist;

// Simple conversion helpers
static inline float TODEG( float x ) { return x * 180.0f / PI; }
static inline float TORAD( float x ) { return x * PI / 180.0f; }

inline std::string narrow( std::wstring const& text )
{
    std::locale loc("");
    wchar_t const* from = text.c_str();
    std::size_t const len = text.size();
    std::vector<char> buffer(len + 1);
    std::use_facet<std::ctype<wchar_t> >(loc).narrow(from, from + len, '_', &buffer[0]);
    return std::string(&buffer[0], &buffer[len]);
}

inline std::wstring wide( std::string const& text )
{
    std::locale loc("");
    char const* from = text.c_str();
    std::size_t const len = text.size();
    std::vector<char> buffer(len + 1);
	std::use_facet<std::ctype<char> >(loc).widen( from, from + len, &buffer[0] );
    return std::wstring(&buffer[0], &buffer[len]);
}

extern int wildcmp( const TCHAR* wild, const TCHAR* string );
extern int wildcmpi( const TCHAR* wild, const TCHAR* string );

inline bool strmatch( const std::wstring& lhs, const std::wstring& rhs )
{
	return( 0 == _wcsicmp( lhs.c_str( ), rhs.c_str( ) ) );
}

inline bool strmatch( const TCHAR* lhs, const std::wstring& rhs )
{
	return( 0 == _wcsicmp( lhs, rhs.c_str( ) ) );
}

inline bool strmatch( const std::wstring& lhs, const TCHAR* rhs )
{
	return( 0 == _wcsicmp( lhs.c_str( ), rhs ) );
}

inline bool strmatch( const TCHAR* lhs, const TCHAR* rhs )
{
	return( 0 == _wcsicmp( lhs, rhs ) );
}

bool wildmatch( const TCHAR* match, const TCHAR* value );
bool wildmatch( const std::wstring& match, const std::wstring& value );
bool wildmatch( const stringlist& matches, const std::wstring& value );

template<typename T>
inline T GetIniValue( LPCTSTR Section, LPCTSTR Setting, T Default, LPCTSTR iniFileName )
{
	T v;
	TCHAR buffer[ 1024 ];
	wstringstream sstr;
	sstr << Default;
	buffer[ 0 ] = 0;

	if( 0 < GetPrivateProfileString( Section, Setting, sstr.str( ).c_str( ), buffer, sizeof( buffer ), iniFileName ) )
	{
		wstringstream sstr( buffer );
		sstr >> v;
		return v;
	}

	return Default;
}

template<>
inline std::wstring GetIniValue<std::wstring>( LPCTSTR Section, LPCTSTR Setting, std::wstring Default, LPCTSTR iniFileName )
{
	TCHAR buffer[ 1024 ];
	buffer[ 0 ] = 0;

	if( 0 < GetPrivateProfileString( Section, Setting, Default.c_str( ), buffer, sizeof( buffer ), iniFileName ) )
		return std::wstring( buffer );

	return Default;
}

template<>
inline TSTR GetIniValue<TSTR>( LPCTSTR Section, LPCTSTR Setting, TSTR Default, LPCTSTR iniFileName )
{
	TCHAR buffer[ 1024 ];
	buffer[ 0 ] = 0;

	if( 0 < GetPrivateProfileString( Section, Setting, Default.data( ), buffer, sizeof( buffer ), iniFileName ) )
		return TSTR( buffer );

	return Default;
}

// Generic IniFile writing routine
template<typename T>
inline void SetIniValue( LPCTSTR Section, LPCTSTR Setting, T value, LPCTSTR iniFileName )
{
	std::wstringstream sstr;
	sstr << value;

	WritePrivateProfileString( Section, Setting, sstr.str( ).c_str( ), iniFileName );
}

// Specific override for wstring values
template<>
inline void SetIniValue( LPCTSTR Section, LPCTSTR Setting, std::wstring value, LPCTSTR iniFileName )
{
	WritePrivateProfileString( Section, Setting, value.c_str( ), iniFileName );
}

// Specific override for TSTR values
template<>
inline void SetIniValue( LPCTSTR Section, LPCTSTR Setting, TSTR value, LPCTSTR iniFileName )
{
	WritePrivateProfileString( Section, Setting, value.data( ), iniFileName );
}

static inline Color TOCOLOR( const Niflib::Color3& c3 )
{
	return Color( c3.r, c3.g, c3.b );
}

static inline Niflib::Color3 TOCOLOR3( const Color& c3 )
{
	return Niflib::Color3( c3.r, c3.g, c3.b );
}

static inline Niflib::Color3 TOCOLOR3( const Point3& c3 )
{
	return Niflib::Color3( c3.x, c3.y, c3.z );
}

static inline Point3 TOPOINT3( const Niflib::Color3& c3 )
{
   return Point3( c3.r, c3.g, c3.b );
}

static inline Point3 TOPOINT3( const Niflib::Vector3& v )
{
   return Point3( v.x, v.y, v.z );
}

static inline Point3 TOPOINT3( const Niflib::Vector4& v )
{
	return Point3( v.x, v.y, v.z );
}

static inline Niflib::Vector3 TOVECTOR3( const Point3& v )
{
	return Niflib::Vector3( v.x, v.y, v.z );
}

static inline Niflib::Vector3 TOVECTOR3( const Niflib::Vector4& v )
{
	return Niflib::Vector3( v.x, v.y, v.z );
}

static inline Niflib::Vector4 TOVECTOR4( const Point3& v, float w = 0.0f )
{
	return Niflib::Vector4( v.x, v.y, v.z, w );
}

static inline Niflib::QuaternionXYZW TOQUATXYZW( const Niflib::Quaternion& q )
{
	Niflib::QuaternionXYZW qt;
	
	qt.x = q.x;
	qt.y = q.y;
	qt.z = q.z;
	qt.w = q.w;

	return qt;
}

static inline Niflib::QuaternionXYZW TOQUATXYZW( const Quat& q )
{
	Niflib::QuaternionXYZW qt;
	
	qt.x = q.x;
	qt.y = q.y;
	qt.z = q.z;
	qt.w = q.w;

	return qt;
}

static inline Quat TOQUAT( const Niflib::Quaternion& q, bool inverse = false )
{
	Quat qt( q.x, q.y, q.z, q.w );
	return ( inverse && q.w != FloatNegINF ) ? qt.Inverse( ) : qt;
}

static inline Quat TOQUAT( const Niflib::QuaternionXYZW& q, bool inverse = false )
{
	Quat qt( q.x, q.y, q.z, q.w );
	return ( inverse && q.w != FloatNegINF ) ? qt.Inverse( ): qt;
}

static inline Niflib::Quaternion TOQUAT( const Quat& q, bool inverse = false )
{
	return( inverse && q.w != FloatNegINF ) ? TOQUAT( q.Inverse( ), false ) : Niflib::Quaternion( q.w, q.x, q.y, q.z );
}


static inline Matrix3 TOMATRIX3( const Niflib::Matrix44& tm, bool invert = false )
{
	Niflib::Vector3 pos;
	Niflib::Matrix33 rot;
	float scale;

	tm.Decompose( pos, rot, scale );

	Matrix3 m( rot.rows[ 0 ].data, rot.rows[ 1 ].data, rot.rows[ 2 ].data, Point3( ) );
	if( invert )
		m.Invert( );

	m.Scale( Point3( scale, scale, scale ) );
	m.SetTrans( Point3( pos.x, pos.y, pos.z ) );

	return m;
}

static inline Niflib::Matrix33 TOMATRIX33( const Matrix3& tm, bool invert = false )
{
	Niflib::Matrix33 m3( tm.GetRow( 0 )[ 0 ], tm.GetRow( 0 )[ 1 ], tm.GetRow( 0 )[ 2 ],
		tm.GetRow( 1 )[ 0 ], tm.GetRow( 1 )[ 1 ], tm.GetRow( 1 )[ 2 ],
		tm.GetRow( 2 )[ 0 ], tm.GetRow( 2 )[ 1 ], tm.GetRow( 2 )[ 2 ] );

	return m3;
}

static inline Matrix3 TOMATRIX3( Niflib::Vector3& trans, Niflib::QuaternionXYZW quat, float scale )
{
	Matrix3 tm( true ), qm;
	Quat q( quat.x, quat.y, quat.z, quat.w );

	q.MakeMatrix( qm );
	tm.SetTranslate( TOPOINT3( trans ) );
	tm *= qm;
	tm *= ScaleMatrix( Point3( scale, scale, scale ) );

	return tm;
}

static inline Matrix3 TOMATRIX3( Niflib::Vector3& trans, Niflib::Quaternion quat, float scale )
{
	Matrix3 tm, qm;
	Quat q( quat.x, quat.y, quat.z, quat.w );

	q.MakeMatrix( qm );
	tm.SetTranslate( TOPOINT3( trans ) );
	tm *= qm;
	tm *= ScaleMatrix( Point3( scale, scale, scale ) );

	return tm;
}

static inline Niflib::Matrix44 TOMATRIX4( const Matrix3 &tm, bool invert = false )
{
   Niflib::Matrix33 m3( tm.GetRow(0)[0], tm.GetRow(0)[1], tm.GetRow(0)[2],
                        tm.GetRow(1)[0], tm.GetRow(1)[1], tm.GetRow(1)[2],
                        tm.GetRow(2)[0], tm.GetRow(2)[1], tm.GetRow(2)[2] );

   Niflib::Matrix44 m4( TOVECTOR3( tm.GetTrans( ) ), m3, 1.0f );

   return m4;
}

static inline Point3 GetScale( const Matrix3& mtx )
{
	return Point3( fabs( mtx.GetRow( 0 )[ 0 ] ), fabs( mtx.GetRow( 1 )[ 1 ] ), fabs( mtx.GetRow( 2 )[ 2 ] ) );
}

template<typename T>
inline Niflib::Ref<T> CreateNiObject( )
{
	return Niflib::StaticCast<T>( T::Create( ) );
}

extern stringlist TokenizeString( LPCWSTR str, LPCWSTR delims, bool trim=false );
extern stringlist TokenizeCommandLine( LPCWSTR str, bool trim );
extern std::wstring JoinCommandLine( stringlist args );

extern TSTR FormatText( const TCHAR* format, ... );
extern std::wstring FormatString( const TCHAR* format, ... );

extern TSTR GetFileVersion( const wchar_t* fileName );

extern std::wstring GetIndirectValue( LPCWSTR path );
extern NameValueCollection ReadIniSection( LPCWSTR Section, LPCWSTR iniFileName );
extern std::wstring ExpandQualifiers( const std::wstring& src, const NameValueCollection& map );
extern std::wstring ExpandEnvironment( const std::wstring& src );

extern void PosRotScaleNode( INode* n, Point3 p, Quat& q, float s, PosRotScale prs = prsDefault, TimeValue t = 0 );
extern void PosRotScaleNode( Control* c, Point3 p, Quat& q, float s, PosRotScale prs = prsDefault, TimeValue t = 0 );
extern void PosRotScaleNode( INode* n, Matrix3& m3, PosRotScale prs = prsDefault, TimeValue t = 0 );
extern void PosRotScaleNode( Control* c, Matrix3& m3, PosRotScale prs = prsDefault, TimeValue t = 0 );
extern Matrix3 GetNodeLocalTM( INode* n, TimeValue t = 0 );

void GetIniFileName( wchar_t* iniName );

// Morph related routines in nimorph.cpp
extern Modifier* GetMorpherModifier( INode* node );
//...
extern TSTR MorpherGetName( Modifier* mod, int index );
extern bool MorpherIsActive( Modifier* mod, int index );
extern bool MorpherHasData( Modifier* mod, int index );
extern int MorpherNumProgMorphs( Modifier* mod, int index );
extern INode* MorpherGetProgMorph( Modifier* mod, int index, int morphIdx );

typedef struct EnumLookupType
{
	int value;
	const wchar_t* name;

} EnumLookupType;

extern TSTR EnumToString( int value, const EnumLookupType* table );
extern int StringToEnum( TSTR value, const EnumLookupType* table );
extern int EnumToIndex( int value, const EnumLookupType* table );

extern void FindImages( NameValueCollection& images, const std::wstring& rootPath, const stringlist& searchpaths, const stringlist& extensions );

extern Modifier* GetOrCreateSkin( INode* node );
extern Modifier* GetSkin( INode* node );
extern TriObject* GetTriObject( Object* o );

Modifier* GetbhkCollisionModifier( INode* node );
Modifier* CreatebhkCollisionModifier( 
	INode* node
	, int type
	, Niflib::HavokMaterial material
	, Niflib::OblivionLayer layer
	, byte filter
	);

struct NodeEquivalence
{
   bool operator()(const Niflib::NiNodeRef& lhs, const Niflib::NiNodeRef& rhs) const{
      return (!lhs || !rhs) ? (lhs < rhs) : (lhs->GetName() < rhs->GetName());
   }
   bool operator()(const Niflib::NiNodeRef& lhs, const std::string& rhs) const{
      return (lhs->GetName() < rhs);
   }
   bool operator()(const std::string& lhs, const Niflib::NiNodeRef& rhs) const{
      return (lhs < rhs->GetName());
   }
};

extern std::vector<std::wstring> GetNamesOfNodes( const std::vector<Niflib::NiNodeRef>& blocks );
extern std::vector<Niflib::NiNodeRef> SelectNodesByName( const std::vector<Niflib::NiNodeRef>& blocks, LPCTSTR match );

template<typename U, typename T>
inline Niflib::Ref<U> SelectFirstObjectOfType( std::vector<Niflib::Ref<T> > const& objs )
{
	for( std::vector<Niflib::Ref<T> >::const_iterator itr = objs.begin( ), end = objs.end( ); itr != end; ++itr )
	{
		Niflib::Ref<U> obj = Niflib::DynamicCast<U>( *itr );
		if( obj )
			return obj;
	}

	return Niflib::Ref<U>( );
}

template<typename U, typename T>
inline Niflib::Ref<U> SelectFirstObjectOfType( std::list<Niflib::Ref<T> > const& objs )
{
	for( std::list<Niflib::Ref<T> >::const_iterator itr = objs.begin( ), end = objs.end( ); itr != end; ++itr )
	{
		Niflib::Ref<U> obj = Niflib::DynamicCast<U>( *itr );
		if( obj )
			return obj;
	}

	return Niflib::Ref<U>( );
}

BOOL ExecuteCommandLine( std::wstring cmdLine, DWORD & exitCode );