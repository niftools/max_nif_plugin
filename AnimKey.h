
#pragma once

const float FramesPerSecond = 30.0f;
const float FramesIncrement = 1.0f / 30.0f;
const int TicksPerFrame = GetTicksPerFrame( );

inline TimeValue TimeToFrame( float t )
{
	return TimeValue( t * FramesPerSecond * TicksPerFrame );
}

inline float FrameToTime( TimeValue t )
{
	return float( t ) / ( FramesPerSecond * TicksPerFrame );
}

typedef Niflib::Key<float> FloatKey;
typedef Niflib::Key<Niflib::Quaternion> QuatKey;
typedef Niflib::Key<Niflib::Vector3> Vector3Key;
typedef Niflib::Key<std::string> StringKey;

template<typename T, typename U> T MapKey( U& key, float time );

// Specialized Linear Mappings
template<> ILinFloatKey MapKey<ILinFloatKey, FloatKey>( FloatKey& key, float time );
template<> ILinRotKey MapKey<ILinRotKey, QuatKey>( QuatKey& key, float time );
template<> ILinScaleKey MapKey<ILinScaleKey, FloatKey>( FloatKey& key, float time );
template<> ILinPoint3Key MapKey<ILinPoint3Key, Vector3Key>( Vector3Key& key, float time );

template<> FloatKey MapKey<FloatKey, ILinFloatKey>( ILinFloatKey& key, float time );
template<> QuatKey MapKey<QuatKey, ILinRotKey>( ILinRotKey& key, float time );
template<> FloatKey MapKey<FloatKey, ILinScaleKey>( ILinScaleKey& key, float time );
template<> Vector3Key MapKey<Vector3Key, ILinPoint3Key>( ILinPoint3Key& key, float time );

// Specialized Bezier/Hybrid mappings
template<> IBezFloatKey MapKey<IBezFloatKey, FloatKey>(FloatKey& key, float time);
template<> IBezPoint3Key MapKey<IBezPoint3Key, Vector3Key>(Vector3Key& key, float time);
template<> IBezQuatKey MapKey<IBezQuatKey, QuatKey>(QuatKey& key, float time);
template<> IBezScaleKey MapKey<IBezScaleKey, FloatKey>(FloatKey& key, float time);

template<> FloatKey MapKey<FloatKey, IBezFloatKey>(IBezFloatKey& key, float time);
template<> QuatKey MapKey<QuatKey, IBezQuatKey>(IBezQuatKey& key, float time);
template<> FloatKey MapKey<FloatKey, IBezScaleKey>(IBezScaleKey& key, float time);
template<> Vector3Key MapKey<Vector3Key, IBezPoint3Key>(IBezPoint3Key& key, float time);

// Specialized TCB mappings
template<> ITCBFloatKey MapKey<ITCBFloatKey, FloatKey>(FloatKey& key, float time);
template<> ITCBRotKey MapKey<ITCBRotKey, QuatKey>(QuatKey& key, float time);
template<> ITCBPoint3Key MapKey<ITCBPoint3Key, Vector3Key>(Vector3Key& key, float time);
template<> ITCBScaleKey MapKey<ITCBScaleKey, FloatKey>(FloatKey& key, float time);

template<> FloatKey MapKey<FloatKey, ITCBFloatKey>(ITCBFloatKey& key, float time);
template<> QuatKey MapKey<QuatKey, ITCBRotKey>(ITCBRotKey& key, float time);
template<> FloatKey MapKey<FloatKey, ITCBScaleKey>(ITCBScaleKey& key, float time);
template<> Vector3Key MapKey<Vector3Key, ITCBPoint3Key>(ITCBPoint3Key& key, float time);

// Interpolated Keys
template<typename T> T InterpKey( Control* subCtrl, TimeValue time, float timeOff );
template<> FloatKey InterpKey( Control* subCtrl, TimeValue time, float timeOff );
template<> QuatKey InterpKey( Control* subCtrl, TimeValue time, float timeOff );
template<> Vector3Key InterpKey( Control* subCtrl, TimeValue time, float timeOff );

template<typename T> void MergeKey(T& lhs, T& rhs) {
   lhs.val += rhs.val;
}

template<> void MergeKey<ILinRotKey>(ILinRotKey& lhs, ILinRotKey& rhs);
template<> void MergeKey<IBezQuatKey>(IBezQuatKey& lhs, IBezQuatKey& rhs);
template<> void MergeKey<ITCBRotKey>(ITCBRotKey& lhs, ITCBRotKey& rhs);

template<typename T, typename U>
inline int GetKeys( Control* subCtrl, std::vector<T>& keys, float time )
{
	if( IKeyControl* ikeys = GetKeyControlInterface( subCtrl ) )
	{
		int n = ikeys->GetNumKeys( );
		keys.reserve( n );

		for( int i = 0; i < n; ++i )
		{
			AnyKey buf;
			U* key = reinterpret_cast<U*>( (IKey*) buf );

			ikeys->GetKey( i, key );
			keys.push_back( MapKey<T>( *key, time ) );
		}

		return keys.size( );
	}

	return 0;
}

template<typename T, typename U>
inline int GetKeys( Control* subCtrl, std::vector<T>& keys, Interval range )
{
	if( IKeyControl* ikeys = GetKeyControlInterface( subCtrl ) )
	{
		float timeOffset = -FrameToTime( range.Start( ) );
		int n = ikeys->GetNumKeys( );
		keys.reserve( n );

		bool hasStart = false;
		bool hasEnd = false;

		for( int i = 0; i < n; ++i )
		{
			AnyKey buf;
			U* key = reinterpret_cast<U*>( (IKey*) buf );

			ikeys->GetKey( i, key );
			if( range.InInterval( key->time ) )
			{
				hasStart |= ( range.Start( ) == key->time );
				hasEnd   |= ( range.End( ) == key->time );
				keys.push_back( MapKey<T>( *key, timeOffset ) );
			}
		}

		if( keys.size( ) > 0 )
		{
			if( !hasStart )
				keys.insert( keys.begin( ), InterpKey<T>( subCtrl, range.Start( ), timeOffset ) );

			if( !hasEnd )
				keys.push_back( InterpKey<T>( subCtrl, range.End( ), timeOffset ) );
		}

		return (int) keys.size( );
	}

	return 0;
}

template<typename T> void MergeKey(IKeyControl *keys, T& key)
{
   for( int i = 0, n = keys->GetNumKeys( ); i < n; ++i )
   {
      AnyKey buf; 
	  T *tmp = reinterpret_cast<T*>( (IKey*) buf );
      keys->GetKey( i, tmp );

      if( tmp->time == key.time ) 
	  {
         MergeKey<T>( *(T*) tmp, key );
         keys->SetKey( i, tmp );
         return;
      }
   }

   keys->AppendKey( &key );
}

template<typename T, typename U>
inline void SetKeys( Control *subCtrl, vector<U>& keys, float time )
{
   if( subCtrl && !keys.empty( ) )
   {
      if( IKeyControl *ikeys = GetKeyControlInterface( subCtrl ) )
	  {
         ikeys->SetNumKeys( (int) keys.size( ) );
         for( size_t i = 0, n = keys.size( ); i < n; ++i )
            ikeys->SetKey( (int) i, &MapKey<T>( keys[ i ], time ) );

         ikeys->SortKeys( );
      }
   }
}

template<typename T, typename U>
inline void MergeKeys( Control *subCtrl, vector<U>& keys, float time )
{
   if( subCtrl && !keys.empty( ) )
   {
      if( IKeyControl *ikeys = GetKeyControlInterface( subCtrl ) ) 
	  {
         if( ikeys->GetNumKeys( ) == 0 )
            SetKeys<T,U>( subCtrl, keys, time );

         else
		 {
            for( int i = 0, n = keys.size( ); i < n; ++i )
               MergeKey( ikeys, MapKey<T>( keys[ i ], time ) );

            ikeys->SortKeys( );
         }        
      }
   }
}

enum V3T
{
   V3T_X, 
   V3T_Y, 
   V3T_Z
};

extern float GetValue( Niflib::Vector3& value, V3T type );
extern void SetValue( Niflib::Vector3& v, float value, V3T type );
extern FloatKey& CopyKey( FloatKey& dst, Vector3Key& src, V3T type );
extern Vector3Key& CopyKey( Vector3Key& dst, FloatKey& src, V3T type );

extern void SplitKeys( std::vector<Vector3Key>& keys, std::vector<FloatKey>& xkeys, std::vector<FloatKey>& ykeys, std::vector<FloatKey>& zkeys );
extern void JoinKeys( std::vector<Vector3Key>& keys, std::vector<FloatKey>& xkeys, std::vector<FloatKey>& ykeys, std::vector<FloatKey>& zkeys );

typedef Niflib::Key<string> KeyTextValue;