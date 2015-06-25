
#include "stdafx.h"

const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID( 0x118f7e02, 0xffee238a );

static void GetTimeRange( Control* c, Interval& range );
static Interval GetTimeRange( INode* node );

struct AnimationExport
{
	AnimationExport( Exporter& parent )
		: ne( parent )
	{
	}

	INode* findTrackedNode( INode* root );
	
	bool doExport( Niflib::NiControllerSequenceRef seq );
	bool doExport( Niflib::NiControllerManagerRef ctrl, INode* node );
	bool exportController( INode* node, Exporter::AccumType accumType );
	Control* GetTMController( INode* node );
	Niflib::NiTimeControllerRef exportController( INode* node, Interval range, bool setTM );

	bool SampleAnimation( INode* node, Interval& range, PosRotScale prs, Niflib::NiKeyframeDataRef data );
	bool GetTextKeys( INode* node, std::vector<StringKey>& textKeys );
	bool splitAccum( Niflib::NiTransformDataRef base, Niflib::NiTransformDataRef accum, Exporter::AccumType accumType );
	void GetTimeRange( Control* c, Interval& range );
	Interval GetTimeRange( INode* node );

	Exporter& ne;
	Interval range;
	Niflib::NiControllerSequenceRef seq;

	std::set<Niflib::NiAVObjectRef> objRefs;
	std::map<Niflib::NiControllerSequenceRef, Interval> ranges;
};

float QuatDot( const Niflib::Quaternion& q, const Niflib::Quaternion& p )
{
	return q.w * p.w + q.x * p.x + q.y * p.y + q.z * p.z;
}

bool GetTextKeys( INode* node, std::vector<StringKey>& textKeys, Interval range )
{
	// Populate Text keys and Sequence information from note tracks
	if( Exporter::mUseTimeTags )
	{
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7+

		if( IFrameTagManager* tagMgr = (IFrameTagManager*) GetCOREInterface( FRAMETAGMANAGER_INTERFACE ) )
		{
			int n = tagMgr->GetTagCount( );
			for( int i = 0; i < n; i++ )
			{
				UINT id = tagMgr->GetTagID( i );
				TimeValue t = tagMgr->GetTimeByID( id, FALSE );
				TSTR name = tagMgr->GetNameByID( id );

				StringKey strkey;
				strkey.time = FrameToTime( Interval( range.Start( ), t ).Duration( ) - 1 );
				strkey.data = narrow( std::wstring( name ) );

				textKeys.push_back( strkey );
			}
		}

#endif
	}
	else
	{
		for( int i = 0, n = node->NumNoteTracks( ); i < n; ++i )
		{
			if( NoteTrack* nt = node->GetNoteTrack( i ) )
			{
				if( nt->ClassID( ) == Class_ID( NOTETRACK_CLASS_ID, 0 ) )
				{
					DefNoteTrack* defNT = (DefNoteTrack*) nt;
					if( defNT->NumKeys( ) > 0 )
					{
						bool stop = false;
						for( int j = 0, m = defNT->keys.Count( ); j < m && !stop; ++j )
						{
							NoteKey* key = defNT->keys[ j ];
							stringlist args = TokenizeCommandLine( key->note, true );
							
							if( args.empty( ) )
								continue;

							bool isStart = false;
							bool isEnd = false;

							if( wcsicmp( args[ 0 ].c_str( ), _T( "start" ) ) == 0 )
								isStart = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "start" ) ) == 0 )
								isStart = true;

							else if( wcsicmp( args[ 0 ].c_str( ), _T( "end" ) ) == 0 )
								isEnd = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "stop" ) ) == 0 )
								isEnd = true;

							if( range.Empty( ) )
								range.SetInstant( key->time );
							
							else
							{
								if( key->time < range.Start( ) )
									range.SetStart( key->time );

								if( key->time > range.End( ) )
									range.SetEnd( key->time );
							}

							StringKey strkey;
							strkey.time = FrameToTime( Interval( range.Start( ), key->time ).Duration( ) - 1 );
							strkey.data = narrow( key->note.data( ) );

							textKeys.push_back( strkey );
						}
					}
				}
			}
		}
	}

	return !textKeys.empty( );
}

Exporter::Result Exporter::scanForAnimation( INode* node )
{
	if( NULL == node )
		return Exporter::Skip;

	Interval nodeRange = GetTimeRange( node );
	if( !nodeRange.Empty( ) )
	{
		if( globalRange.Empty( ) )
			globalRange = nodeRange;

		if( globalRange.Empty( ) || nodeRange.Start( ) < globalRange.Start( ) )
			globalRange.SetStart( nodeRange.Start( ) );

		if( globalRange.Empty( ) || nodeRange.End( ) > globalRange.End( ) )
			globalRange.SetEnd( nodeRange.End( ) );
	}

	// Ideally check for Morph: targets
#if VERSION_3DSMAX >= ((8000<<16)+(15<<8)+0) // Version 8+

	if( Modifier* mod = GetMorpherModifier( node ) )
	{
		int idx = -1;
		for( int i = 1; i <= 100; ++i )
		{
			if( MorpherIsActive( mod, i ) && MorpherHasData( mod, i ) )
			{
				TSTR str = MorpherGetName( mod, i );
				int nodes = MorpherNumProgMorphs( mod, i );

				for( int j = 1; j <= nodes; j++ )
				{
					if( INode* morph = MorpherGetProgMorph( mod, i, j ) )
						markAsHandled( morph );
				}
			}
		}
	}

#endif

	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		scanForAnimation( node->GetChildNode( i ) );

	return Exporter::Ok;
}

void Exporter::GetTimeRange( Control* c, Interval& range )
{
	if( c->IsKeyable( ) )
	{
		int iNumKeys = c->NumKeys( );
		for( int i = 0; i < iNumKeys; i++ )
		{
			TimeValue t = c->GetKeyTime( i );
			if( range.Empty( ) )
				range.SetInstant( t );

			else
			{
				if( t < range.Start( ) )
					range.SetStart( t );

				if( t > range.End( ) )
					range.SetEnd( t );
			}
		}

		if( range.Empty( ) )
		{
			if( IKeyControl* ikeys = GetKeyControlInterface( c ) )
			{
				int n = ikeys->GetNumKeys( );
				for( int i = 0; i < n; ++i )
				{
					AnyKey buf;
					IKey* key = (IKey*) buf;
					
					ikeys->GetKey( i, key );
					if( range.Empty( ) )
						range.SetInstant( key->time );
					
					else
					{
						if( key->time < range.Start( ) )
							range.SetStart( key->time );

						if( key->time > range.End( ) )
							range.SetEnd( key->time );
					}
				}
			}
		}
	}

	if( range.Empty( ) )
	{
		if( c->IsAnimated( ) )
		{
			if( range.Empty( ) )
				range = globalRange;

			if( !globalRange.Empty( ) && range.Start( ) > globalRange.Start( ) )
				range.SetStart( globalRange.Start( ) );

			if( !globalRange.Empty( ) && range.End( ) < globalRange.End( ) )
				range.SetEnd( globalRange.End( ) );

		}
	}

#if VERSION_3DSMAX >= ((5000<<16)+(9<<8)+0)
	
	if( range.Empty( ) )
	{
		if( IListControl* listc = GetIListControlInterface( c ) )
		{
			if( listc->GetListCount( ) > 0 )
			{
				if( range.Empty( ) )
					range = globalRange;

				if( !globalRange.Empty( ) && range.Start( ) > globalRange.Start( ) )
					range.SetStart( globalRange.Start( ) );

				if( !globalRange.Empty( ) && range.End( ) > globalRange.End( ) )
					range.SetEnd( globalRange.End( ) );
			}
		}
	}

#endif

}

Interval Exporter::GetTimeRange( INode* node )
{
	Interval range;
	range.SetEmpty( );
	int nTracks = node->NumNoteTracks( );

	// Populate Text keys and Sequence information from note tracks
	for( int i = 0; i < nTracks; ++i )
	{
		if( NoteTrack* nt = node->GetNoteTrack( i ) )
		{
			if( nt->ClassID( ) == Class_ID( NOTETRACK_CLASS_ID, 0 ) )
			{
				DefNoteTrack* defNT = (DefNoteTrack*) nt;
				if( defNT->NumKeys( ) > 0 )
				{
					for( int j = 0, m = defNT->keys.Count( ); j < m; ++j )
					{
						NoteKey* key = defNT->keys[ j ];
						if( range.Empty( ) )
							range.SetInstant( key->time );

						else
						{
							if( key->time < range.Start( ) )
								range.SetStart( key->time );

							if( key->time > range.End( ) )
								range.SetEnd( key->time );
						}
					}
				}
			}
		}
	}

	if( Control* c = node->GetTMController( ) )
	{
		GetTimeRange( c, range );

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5

		if( Control* sc = c->GetWController( ) )
		{
			GetTimeRange( sc, range );
			if( sc != c )
				GetTimeRange( sc, range );
		}

#endif

		if( Control* sc = c->GetXController( ) )
			if( sc != c )
				GetTimeRange( sc, range );

		if( Control* sc = c->GetYController( ) )
			if( sc != c )
				GetTimeRange( sc, range );

		if( Control* sc = c->GetZController( ) )
			if( sc != c )
				GetTimeRange( sc, range );

		if( Control* sc = c->GetRotationController( ) )
			if( sc != c )
				GetTimeRange( sc, range );

		if( Control* sc = c->GetPositionController( ) )
			if( sc != c )
				GetTimeRange( sc, range );

		if( Control* sc = c->GetScaleController( ) )
			if( sc != c )
				GetTimeRange( sc, range );
	}

	return range;
}

bool Exporter::isNodeTracked( INode* node )
{
	if( Exporter::mUseTimeTags )
	{
		if( mI->GetRootNode( ) == node->GetParentNode( ) && isNodeKeyed( node ) )
			return true;
	}
	else if( node->HasNoteTracks( ) )
	{
		for( int i = 0, n = node->NumNoteTracks( ); i < n; ++i )
		{
			if( NoteTrack* nt = node->GetNoteTrack( i ) )
			{
				if( nt->ClassID( ) == Class_ID( NOTETRACK_CLASS_ID, 0 ) )
				{
					DefNoteTrack* defNT = (DefNoteTrack*) nt;
					if( defNT->NumKeys( ) > 0 )
					{
						for( int j = 0, m = defNT->keys.Count( ); j < m; ++j )
						{
							NoteKey* key = defNT->keys[ j ];
							
							// Versions less than 20.0.0.4 will always export
							if( wildmatch( _T( "*start*" ), key->note ) )
								return true;
						}
					}
				}
			}
		}
	}

	return false;
}

static bool HasKeys( Control* c )
{
	bool rv = false;
	if( c != NULL )
	{
		if( c->IsColorController( ) )
			return false;

		if( IKeyControl* ikeys = GetKeyControlInterface( c ) )
			if( ikeys->GetNumKeys( ) > 0 )
				return true;

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
		
		if( Control* sc = c->GetWController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

#endif

		if( Control* sc = c->GetXController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

		if( Control* sc = c->GetYController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

		if( Control* sc = c->GetZController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

		if( Control* sc = c->GetRotationController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

		if( Control* sc = c->GetPositionController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;

		if( Control* sc = c->GetScaleController( ) )
			if( sc != c && HasKeys( sc ) )
				return true;
	}

	return false;
}

bool Exporter::isNodeKeyed( INode* node )
{
	if( node->HasNoteTracks( ) )
		return true;

	if( node->NumKeys( ) > 0 )
		return true;

	if( Control* tmCont = node->GetTMController( ) )
		if( HasKeys( tmCont ) )
			return true;

	return false;
}

Niflib::Ref<Niflib::NiTimeController> Exporter::CreateController( INode* node, Interval range )
{
	AnimationExport ae( *this );
	if( Exporter::mExportType == Exporter::NIF_WO_KF && isNodeTracked( node ) )
	{
		Niflib::NiNodeRef ninode = getNode( node );
		std::vector<StringKey> textKeys;

		if( GetTextKeys( node, textKeys, range ) )
		{
			Niflib::NiTextKeyExtraDataRef textKeyData = new Niflib::NiTextKeyExtraData( );
			ninode->AddExtraData( Niflib::StaticCast<Niflib::NiExtraData>( textKeyData ), Exporter::mNifVersionInt );
			textKeyData->SetKeys( textKeys );
		}
	}

	if( Niflib::NiTimeControllerRef tc = ae.exportController( node, range, false ) )
		return tc;

	return Niflib::NiTimeControllerRef( );
}

void Exporter::InitializeTimeController( Niflib::Ref<Niflib::NiTimeController> ctrl, Niflib::NiNodeRef parent )
{
	ctrl->SetFrequency( 1.0f );
	ctrl->SetStartTime( FloatINF );
	ctrl->SetStopTime( FloatNegINF );
	ctrl->SetPhase( 0.0f );
	ctrl->SetFlags( 0x0C );

	parent->AddController( Niflib::DynamicCast<Niflib::NiTimeController>( ctrl ) );
}

Niflib::NiNodeRef Exporter::createAccumNode( Niflib::NiNodeRef parent, INode* node )
{
	//
	// Tweak by veryxRV
	//
	Niflib::NiNodeRef accumNode;
	bool isTracked = isNodeTracked( node );

	if( !Exporter::mAllowAccum || ( !isTracked && !isSkeletonRoot( node ) ) )
		accumNode = parent;

	else
	{
		accumNode = getNode( FormatString( _T( "%s NonAccum" ), node->GetName( ) ) );
		accumNode->SetLocalTransform( Niflib::Matrix44::IDENTITY );

		parent->AddChild( Niflib::DynamicCast<Niflib::NiAVObject>( accumNode ) );
	}

	// add multi target controller to parent if exporting with animation
	if( mExportType == NIF_WO_KF )
	{
		if( Exporter::mAllowAccum && isTracked )
		{
			std::list<Niflib::NiTimeControllerRef> ctlrs = parent->GetControllers( );
			for( list<Niflib::NiTimeControllerRef>::iterator it = ctlrs.begin( ); it != ctlrs.end( ); ++it )
			{
				parent->RemoveController( *it );
				accumNode->AddController( *it );
			}
		}
	}
	else if( Exporter::mExportType != Exporter::NIF_WO_ANIM )
	{
		// NiMultiTargetTransformController causes crashes in old formats
		if( Exporter::mNifVersionInt >= Niflib::VER_10_0_1_0 )
		{
			Niflib::NiMultiTargetTransformControllerRef ctrl = new Niflib::NiMultiTargetTransformController( );
			std::vector<Niflib::NiNodeRef> children;
			getChildNodes( node, children );

			ctrl->SetExtraTargets( Niflib::DynamicCast<Niflib::NiAVObject>( children ) );
			Exporter::InitializeTimeController( ctrl, parent );
		}

		Niflib::NiControllerManagerRef mgr = new Niflib::NiControllerManager( );
		Exporter::InitializeTimeController( mgr, parent );

		// Export Animation now
		doAnimExport( mgr, node );
	}

	return accumNode;
}

Exporter::Result Exporter::doAnimExport( Niflib::NiControllerSequenceRef root )
{
	AnimationExport animExporter( *this );
	return animExporter.doExport( root ) ? Exporter::Ok : Exporter::Abort;
}

Exporter::Result Exporter::doAnimExport( Niflib::NiControllerManagerRef ctrl, INode* node )
{
	AnimationExport animExporter( *this );
	return animExporter.doExport( ctrl, node ) ? Exporter::Ok : Exporter::Abort;
}

Exporter::Result Exporter::exportGeomMorpherControl( Modifier* mod, std::vector<Niflib::Vector3>& verts, std::vector<int>& baseVertIdx, Niflib::NiObjectNETRef owner )
{
	// TODO: Later
	return Ok;
}

// ===============================================
// AnimationExport
// ===============================================

INode* AnimationExport::findTrackedNode( INode* root )
{
	if( ne.isNodeTracked( root ) )
		return root;

	// locate START in note track before assuming all is well
	for( int i = 0; i < root->NumberOfChildren( ); ++i )
		if( INode* node = findTrackedNode( root->GetChildNode( i ) ) )
			return node;

	return NULL;
}

bool AnimationExport::doExport( Niflib::NiControllerSequenceRef seq )
{
	INode* node = findTrackedNode( ne.mI->GetRootNode( ) );
	if( node == NULL )
		throw runtime_error( "No Actor Roots have been selected in the Animation Manager. Cannot continue." );

	this->seq = seq;

	std::vector<StringKey> textKeys;

	this->range.SetInstant( 0 );

	seq->SetStartTime( FloatINF );
	seq->SetStopTime( FloatINF );
	seq->SetFrequency( 1.0f );
	seq->SetCycleType( Niflib::CYCLE_CLAMP );
	seq->SetTargetName( narrow( node->GetName( ) ) );

	Niflib::NiTextKeyExtraDataRef textKeyData = new Niflib::NiTextKeyExtraData( );
	seq->SetTextKey( textKeyData );

	Exporter::AccumType accumType = Exporter::AT_NONE;

	// Populate Text keys and sequence information from note tracks
	if( Exporter::mUseTimeTags )
	{
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7

		if( IFrameTagManager* tagMgr = (IFrameTagManager*) GetCOREInterface( FRAMETAGMANAGER_INTERFACE ) )
		{
			int n = tagMgr->GetTagCount( );
			for( int i = 0; i < n; i++ )
			{
				UINT id = tagMgr->GetTagID( i );
				TimeValue t = tagMgr->GetTimeByID( id, FALSE );
				TSTR name = tagMgr->GetNameByID( id );

				StringKey strkey;
				strkey.time = FrameToTime( Interval( range.Start( ), t ).Duration( ) - 1 );
				strkey.data = narrow( std::wstring( name ) );

				textKeys.push_back( strkey );
			}
		}

#endif
	}
	else
	{
		for( int i = 0, n = node->NumNoteTracks( ); i < n; ++i )
		{
			if( NoteTrack* nt = node->GetNoteTrack( i ) )
			{
				if( nt->ClassID( ) == Class_ID( NOTETRACK_CLASS_ID, 0 ) )
				{
					DefNoteTrack* defNT = (DefNoteTrack*) nt;
					if( defNT->NumKeys( ) > 0 )
					{
						bool stop = false;
						for( int j = 0, m = defNT->keys.Count( ); j < m && !stop; ++j )
						{
							NoteKey* key = defNT->keys[ j ];
							stringlist args = TokenizeCommandLine( key->note, true );

							if( args.empty( ) )
								continue;

							bool isStart = false;
							bool isEnd = false;

							if( wcsicmp( args[ 0 ].c_str( ), _T( "start" ) ) == 0 )
								isStart = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "start" ) ) == 0 )
							{
								isStart = true;
								const wchar_t* s = args[ 0 ].c_str( );
								const wchar_t* p = wcschr( s, ':' );
								std::wstring name( p == NULL ? s : s, p - s );
								
								seq->SetName( narrow( name ) );
							}
							else if( wcsicmp( args[ 0 ].c_str( ), _T( "end" ) ) == 0 )
								isEnd = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "stop" ) ) == 0 )
								isEnd = true;

							if( isStart )
							{
								seq->SetStartTime( 0.0f );
								range.SetStart( key->time );

								for( stringlist::iterator itr = args.begin( ); itr != args.end( ); ++itr )
								{
									if( strmatch( _T( "-name" ), *itr ) )
									{
										if( ++itr == args.end( ) )
											break;

										seq->SetName( narrow( *itr ) );
									}

									else if( strmatch( _T( "-loop" ), *itr ) )
										seq->SetCycleType( Niflib::CYCLE_LOOP );

									else if( strmatch( _T( "-at" ), *itr ) )
									{
										if( ++itr == args.end( ) )
											break;

										std::wstring type = (*itr);
										if( strmatch( type, _T( "none" ) ) )
											accumType = Exporter::AT_NONE;

										else
										{
											for( size_t j = 0; j < type.size( ); ++j )
											{
												if( tolower( type[ j ] ) == 'x' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_X );

												else if( tolower( type[ j ] ) == 'y' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_Y );

												else if( tolower( type[ j ] ) == 'z' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_Z );
											}
										}
									}
								}
							}
							else if( isEnd )
							{
								range.SetEnd( key->time );
								seq->SetStopTime( FrameToTime( range.Duration( ) - 1 ) );
								stop = true;
							}

							StringKey strkey;
							strkey.time = FrameToTime( Interval( range.Start( ), key->time ).Duration( ) - 1 );
							strkey.data = narrow( std::wstring( key->note ) );

							textKeys.push_back( strkey );
						}
					}
				}
			}
		}
	}

	// keys without explicit start/stop will
	if( !textKeys.empty( ) && seq->GetStartTime( ) == FloatINF )
	{
		seq->SetStartTime( 0.0f );
		seq->SetStopTime( textKeys.back( ).time );
	}

	textKeyData->SetKeys( textKeys );

	// Now let the fun begin.

	return exportController( node, accumType );
}

bool AnimationExport::doExport( Niflib::NiControllerManagerRef ctrl, INode* node )
{
	int start = 0;
	Niflib::NiDefaultAVObjectPaletteRef objPal = new Niflib::NiDefaultAVObjectPalette( );
	ctrl->SetObjectPalette( objPal );

	std::vector<Niflib::NiControllerSequenceRef> seqs;
	std::vector<StringKey> textKeys;

	Niflib::NiControllerSequenceRef curSeq;
	Exporter::AccumType accumType = Exporter::AT_FORCE;

	// Populate Text keys and Sequence information from note tracks
	if( Exporter::mUseTimeTags )
	{
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7

		if( IFrameTagManager* tagMgr = (IFrameTagManager*) GetCOREInterface( FRAMETAGMANAGER_INTERFACE ) )
		{
			curSeq = new Niflib::NiControllerSequence( );
			curSeq->SetStartTime( FloatINF );
			curSeq->SetStopTime( FloatINF );
			curSeq->SetFrequency( 1.0f );
			curSeq->SetCycleType( Niflib::CYCLE_CLAMP );
			curSeq->SetTargetName( narrow( node->GetName( ) ) );

			seqs.push_back( curSeq );
			this->range.SetInstant( 0 );
			curSeq->SetStartTime( 0.0f );

			int n = tagMgr->GetTagCount( );
			for( int i = 0; i < n; i++ )
			{
				UINT id = tagMgr->GetTagID( i );
				TimeValue t = tagMgr->GetTimeByID( id, FALSE );
				TSTR name = tagMgr->GetNameByID( id );

				if( t < range.Start( ) )
					range.SetStart( t );

				if( t > range.End( ) )
					range.SetEnd( t );

				StringKey strkey;
				strkey.time = FrameToTime( Interval( range.Start( ), t ).Duration( ) - 1 );
				strkey.data = narrow( std::wstring( name ) );

				textKeys.push_back( strkey );
			}

			Niflib::NiTextKeyExtraDataRef textKeyData = new Niflib::NiTextKeyExtraData( );
			curSeq->SetTextKey( textKeyData );
			textKeyData->SetKeys( textKeys );

			curSeq->SetStopTime( FrameToTime( range.Duration( ) - 1 ) );
			this->ranges[ curSeq ] = range;
			curSeq = NULL;
		}

#endif
	}
	else
	{
		int nTracks = node->NumNoteTracks( );

		for( int i = 0; i < nTracks; ++i )
		{
			if( NoteTrack* nt = node->GetNoteTrack( i ) )
			{
				if( nt->ClassID( ) == Class_ID( NOTETRACK_CLASS_ID, 0 ) )
				{
					DefNoteTrack* defNT = (DefNoteTrack*) nt;
					if( defNT->NumKeys( ) > 0 )
					{
						for( int j = 0, m = defNT->keys.Count( ); j < m; ++j )
						{
							NoteKey* key = defNT->keys[ j ];
							stringlist args = TokenizeCommandLine( key->note, true );

							if( args.empty( ) )
								continue;

							bool isStart = false;
							bool isEnd = false;
							std::wstring startName;

							if( wcsicmp( args[ 0 ].c_str( ), _T( "start" ) ) == 0 )
								isStart = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "start" ) ) == 0 )
							{
								isStart = true;
								const wchar_t* s = args[ 0 ].c_str( );
								const wchar_t* p = wcschr( s, ':' );
								
								startName = std::wstring( p == NULL ? s : s, p - s - 1 );
							}
							else if( wcsicmp( args[ 0 ].c_str( ), _T( "end" ) ) == 0 )
								isEnd = true;

							else if( args.size( ) > 1 && wcsicmp( args[ 1 ].c_str( ), _T( "stop" ) ) == 0 )
								isEnd = true;

							if( isStart )
							{
								textKeys.clear( );

								curSeq = new Niflib::NiControllerSequence( );
								curSeq->SetStartTime( FloatINF );
								curSeq->SetStopTime( FloatINF );
								curSeq->SetFrequency( 1.0f );
								curSeq->SetCycleType( Niflib::CYCLE_CLAMP );
								curSeq->SetTargetName( narrow( node->GetName( ) ) );
								curSeq->SetName( narrow( startName ) );

								seqs.push_back( curSeq );
								this->range.SetInstant( 0 );

								curSeq->SetStartTime( 0.0f );
								range.SetStart( key->time );

								for( stringlist::iterator itr = args.begin( ); itr != args.end( ); ++itr )
								{
									if( strmatch( _T( "-name" ), *itr ) )
									{
										if( ++itr == args.end( ) )
											break;

										curSeq->SetName( narrow( *itr ) );
									}

									else if( strmatch( _T( "-loop" ), *itr ) )
										curSeq->SetCycleType( Niflib::CYCLE_LOOP );

									else if( strmatch( _T( "-at" ), *itr ) )
									{
										if( ++itr == args.end( ) )
											break;

										std::wstring type = (*itr);
										if( strmatch( type, _T( "none" ) ) )
											accumType = Exporter::AT_NONE;

										else
										{
											for( size_t j = 0; j < type.size( ); ++j )
											{
												if( tolower( type[ j ] ) == 'x' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_X );

												else if( tolower( type[ j ] ) == 'y' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_Y );

												else if( tolower( type[ j ] ) == 'z' )
													accumType = Exporter::AccumType( accumType | Exporter::AT_Z );
											}
										}
									}
								}
							}
							
							StringKey strkey;
							strkey.time = FrameToTime( Interval( range.Start( ), key->time ).Duration( ) - 1 );
							strkey.data = narrow( std::wstring( key->note ) );
							textKeys.push_back( strkey );

							if( isEnd )
							{
								range.SetEnd( key->time );

								// add accumulated text keys to sequence
								if( curSeq != NULL )
								{
									curSeq->SetStopTime( key->time );
								
									this->ranges[ curSeq ] = range;

									Niflib::NiTextKeyExtraDataRef textKeyData = new Niflib::NiTextKeyExtraData( );
									curSeq->SetTextKey( textKeyData );
									//curSeq->SetTextKeys( textKeyData );
									//curSeq->SetTextKeysName( curSeq->GetTargetName( ) );

									textKeyData->SetKeys( textKeys );
									textKeys.clear( );

									curSeq = NULL;
								}
							}
						}
					}
				}
			}
		}
	}

	for( std::vector<Niflib::NiControllerSequenceRef>::iterator itr = seqs.begin( ); itr != seqs.end( ); ++itr )
	{
		// Hold temporary value
		this->seq = (*itr);
		this->range = this->ranges[ this->seq ];

		// Now let the fun begin.
		bool ok = exportController( node, accumType );
	}

	// Set objects with animation
	std::vector<Niflib::NiAVObjectRef> objs;
	objs.insert( objs.end( ), objRefs.begin( ), objRefs.end( ) );
	objPal->SetObjs( objs );

	ctrl->SetControllerSequences( seqs );

	return true;
}

bool AnimationExport::exportController( INode* node, Exporter::AccumType accumType )
{
	bool ok = true;

	bool keepTM = false;
	if( seq->GetTargetName( ) == narrow( node->GetName( ) ) )
		keepTM = true;

	Niflib::NiTimeControllerRef control = exportController( node, range, keepTM );
	if( control != NULL )
	{
		Niflib::NiSingleInterpControllerRef interpControl = Niflib::DynamicCast<Niflib::NiSingleInterpController>( control );
		if( interpControl )
		{
			if( Exporter::mNifVersionInt >= Niflib::VER_10_2_0_0 )
			{
				// Get priority from node
				float priority;
				npGetProp( node, NP_ANM_PRI, priority, Exporter::mDefaultPriority );
				seq->AddInterpolator( Niflib::StaticCast<Niflib::NiSingleInterpController>( control ), (Niflib::byte) priority, false /* TODO: Temporary? */ );

				// TODO: ControlledLink is missing Controller reference.
			}
			else
			{
				seq->AddController( narrow( node->GetName( ) ), control );
				if( Niflib::NiKeyframeControllerRef ctrl = Niflib::DynamicCast<Niflib::NiKeyframeController>( control ) )
				{
					Niflib::CycleType ct = seq->GetCycleType( );
					ctrl->SetFlags( ( ctrl->GetFlags( ) & ~0x7 ) | ( 1 << seq->GetCycleType( ) ) );
				}
			}

			// Handle NonAccum
			if( Exporter::mAllowAccum && accumType != Exporter::AT_NONE )
			{
				Niflib::NiNodeRef accnode = ne.getNode( FormatString( _T( "%s NonAccum", node->GetName( ) ) ) );
				objRefs.insert( Niflib::StaticCast<Niflib::NiAVObject>( accnode ) );

				if( Exporter::mNifVersionInt >= Niflib::VER_10_2_0_0 )
				{
					Niflib::NiTransformInterpolatorRef interp = Niflib::DynamicCast<Niflib::NiTransformInterpolator>( interpControl->GetInterpolator( ) );
					Niflib::NiTransformControllerRef accControl = new Niflib::NiTransformController( );
					Niflib::NiTransformInterpolatorRef accInterp = new Niflib::NiTransformInterpolator( );

					accnode->AddController( Niflib::StaticCast<Niflib::NiTimeController>( accControl ) );
					accControl->SetInterpolator( Niflib::StaticCast<Niflib::NiInterpolator>( accInterp ) );

					accInterp->SetTranslation( Niflib::Vector3( 0.0f, 0.0f, 0.0f ) );
					accInterp->SetScale( 1.0f );
					accInterp->SetRotation( Niflib::Quaternion( 1.0f, 0.0f, 0.0f, 0.0f ) );

					if( interp != NULL )
					{
						Niflib::NiTransformDataRef accumData = new Niflib::NiTransformData( );

						// Transfer entire data to accum node
						if( ( accumType & Exporter::AT_XYZ ) == Exporter::AT_XYZ )
						{
							accInterp->SetData( interp->GetData( ) );
							interp->SetData( accumData );
						}
						else
						{
							accInterp->SetData( accumData );
							splitAccum( interp->GetData( ), accumData, accumType );
						}
					}

					seq->AddInterpolator( Niflib::StaticCast<Niflib::NiSingleInterpController>( accControl ), (Niflib::byte) Exporter::mDefaultPriority, false /* TODO: Temporary? */ );

					// TODO: ControlledLink is missing Controller ref.

					accnode->RemoveController( accControl );
				}
			}
		}
		else
			seq->AddController( control );

		Niflib::NiObjectNETRef target = control->GetTarget( );

		// Now remove temporary controller
		target->RemoveController( Niflib::StaticCast<Niflib::NiTimeController>( control ) );
	}

	for( int i = 0, n = node->NumberOfChildren( ); ok && i < n; ++i )
	{
		INode* child = node->GetChildNode( i );
		ok |= exportController( child, Exporter::AT_NONE );
	}

	return ok;
}

Niflib::NiTimeControllerRef AnimationExport::exportController( INode* node, Interval range, bool setTM )
{
	bool skip = false;
	Niflib::NiTimeControllerRef timeControl;

	if( range.Empty( ) )
	{
		range = GetTimeRange( node );
		if( range.Empty( ) )
			return timeControl;
	}

	ne.ProgressUpdate( Exporter::Animation, FormatText( _T( "'%s' Animation" ), node->GetName( ) ) );

	// Primary recursive decent routine

	ObjectState os = node->EvalWorldState( range.Start( ) );

	if( !Exporter::mExportCameras && os.obj && os.obj->SuperClassID( ) == CAMERA_CLASS_ID )
		skip = true;

	else if( !Exporter::mExportLights && os.obj && os.obj->SuperClassID( ) == LIGHT_CLASS_ID )
		skip = true;

	if( !skip && Exporter::mExportTransforms )
	{
		float timeOffset = -FrameToTime( range.Start( ) );
		if( Control* tmCont = GetTMController( node ) )
		{
			Interval validity;
			validity.SetEmpty( );
			Matrix3 tm = node->GetObjTMAfterWSM( range.Start( ) );

			if( INode* parent = node->GetParentNode( ) )
			{
				Matrix3 pm = Inverse( parent->GetObjTMAfterWSM( range.Start( ) ) );
				tm *= pm;
			}

			bool keepData = false;
			float scale = FloatNegINF;
			Niflib::Vector3 trans = TOVECTOR3( tm.GetTrans( ) );
			Niflib::Quaternion rot = TOQUAT( Quat( tm ), true );

			Niflib::NiNodeRef ninode = ne.getNode( node );
			if( setTM )
			{
				trans = TOVECTOR3( tm.GetTrans( ) );
				rot = TOQUAT( Quat( tm ), true );
			}

			std::string name = ninode->GetName( );
			Niflib::NiKeyframeDataRef data;

			if( Exporter::mNifVersionInt < Niflib::VER_10_2_0_0 )
			{
				Niflib::NiKeyframeControllerRef control = new Niflib::NiKeyframeController( );
				Exporter::InitializeTimeController( control, ninode );

				data = new Niflib::NiKeyframeData( );
				control->SetData( data );
				timeControl = Niflib::StaticCast<Niflib::NiTimeController>( control );
			}
			else
			{
				Niflib::NiTransformControllerRef control = new Niflib::NiTransformController( );
				Exporter::InitializeTimeController( control, ninode );

				Niflib::NiTransformInterpolatorRef interp = new Niflib::NiTransformInterpolator( );
				Niflib::NiTransformDataRef tdata = new Niflib::NiTransformData( );

				data = Niflib::StaticCast<Niflib::NiKeyframeData>( tdata );
				control->SetInterpolator( Niflib::StaticCast<Niflib::NiInterpolator>( interp ) );

				interp->SetTranslation( trans );
				interp->SetScale( scale );
				interp->SetRotation( rot );
				interp->SetData( tdata );

				timeControl = Niflib::StaticCast<Niflib::NiTimeController>( control );
			}

			timeControl->SetStartTime( 0.0f );
			timeControl->SetStopTime( FrameToTime( range.Duration( ) - 1 ) );

			Class_ID cID = tmCont->ClassID( );
			if( range.Empty( ) || ( range.Start( ) == range.End( ) ) )
				return timeControl;

#ifdef USE_BIPED

			// TODO: Biped later

#endif
			if( cID == IKCONTROL_CLASS_ID || cID == IKCHAINCONTROL_CLASS_ID )
				keepData |= SampleAnimation( node, range, prsDefault, data );

			else
			{
				if( Control* c = tmCont->GetPositionController( ) )
				{
					int nkeys = 0;

#if VERSION_3DSMAX >= ((5000<<16)+(9<<8)+0) // Version 5+

					if( IListControl* listc = GetIListControlInterface( c ) )
						keepData = SampleAnimation( node, range, prsPos, data );

#endif
					// seperate xyz
					if( keepData )
					{
						// already handled, just skip below
					}
					else if( c->ClassID( ) == IPOS_CONTROL_CLASS_ID )
					{
						Niflib::KeyType kType = Niflib::QUADRATIC_KEY;
						std::vector<FloatKey> xkeys;
						std::vector<FloatKey> ykeys;
						std::vector<FloatKey> zkeys;

						if( Control* x = c->GetXController( ) )
						{
							if( x->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
							{
								kType = Niflib::LINEAR_KEY;
								nkeys += GetKeys<FloatKey, ILinFloatKey>( x, xkeys, range );
							}

							else if( x->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								kType = Niflib::QUADRATIC_KEY;
								nkeys += GetKeys<FloatKey, IBezFloatKey>( x, xkeys, range );
							}

							else if( x->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								kType = Niflib::TBC_KEY;
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( x, xkeys, range );
							}

							else
							{
								kType = Niflib::QUADRATIC_KEY;
								nkeys += GetKeys<FloatKey, IBezFloatKey>( x, xkeys, range );
							}
						}

						if( Control* y = c->GetYController( ) )
						{
							if( y->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, ILinFloatKey>( y, ykeys, range );

							else if( y->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, IBezFloatKey>( y, ykeys, range );

							else if( y->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( y, ykeys, range );

							else
								nkeys += GetKeys<FloatKey, IBezFloatKey>( y, ykeys, range );
						}

						if( Control* z = c->GetZController( ) )
						{
							if( z->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, ILinFloatKey>( z, zkeys, range );

							else if( z->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, IBezFloatKey>( z, zkeys, range );

							else if( z->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( z, zkeys, range );

							else
								nkeys += GetKeys<FloatKey, IBezFloatKey>( z, zkeys, range );
						}

						std::vector<Vector3Key> keys;
						JoinKeys( keys, xkeys, ykeys, zkeys );

						data->SetTranslateType( kType );
						data->SetTranslateKeys( keys );
					}
					else
					{
						std::vector<Vector3Key> keys;
						if( c->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
						{
							data->SetTranslateType( Niflib::LINEAR_KEY );
							nkeys += GetKeys<Vector3Key, ILinPoint3Key>( c, keys, range );
						}

						else if( c->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
						{
							data->SetTranslateType( Niflib::QUADRATIC_KEY );
							nkeys += GetKeys<Vector3Key, IBezPoint3Key>( c, keys, range );
						}

						else if( c->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
						{
							data->SetTranslateType( Niflib::TBC_KEY );
							nkeys += GetKeys<Vector3Key, ITCBPoint3Key>( c, keys, range );
						}
						else
							keepData = SampleAnimation( node, range, prsPos, data );

						if( !keys.empty( ) )
							data->SetTranslateKeys( keys );
					}

					if( nkeys != 0 ) 
						keepData = true;
				}

				// Rotations
				if( Control* c = tmCont->GetRotationController( ) )
				{
					int nkeys = 0;
					if( c->ClassID( ) == Class_ID( LININTERP_ROTATION_CLASS_ID, 0 ) )
					{
						std::vector<QuatKey> keys;
						data->SetRotateType( Niflib::LINEAR_KEY );
						nkeys += GetKeys<QuatKey, ILinRotKey>( c, keys, range );
						data->SetQuatRotateKeys( keys );
					}

					else if( c->ClassID( ) == Class_ID( HYBRIDINTERP_ROTATION_CLASS_ID, 0 ) )
					{
						std::vector<QuatKey> keys;
						data->SetRotateType( Niflib::QUADRATIC_KEY );
						nkeys += GetKeys<QuatKey, IBezQuatKey>( c, keys, range );
						data->SetQuatRotateKeys( keys );
					}

					else if( c->ClassID( ) == Class_ID( EULER_CONTROL_CLASS_ID, 0 ) )
					{
						data->SetRotateType( Niflib::XYZ_ROTATION_KEY );
						if( Control* x = c->GetXController( ) )
						{
							std::vector<FloatKey> keys;
							if( x->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ILinFloatKey>( x, keys, range );
								data->SetXRotateType( Niflib::XYZ_ROTATION_KEY );
							}

							else if( x->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( x, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							else if( x->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( x, keys, range );
								data->SetXRotateType( Niflib::TBC_KEY );
							}

							else
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( x, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							data->SetXRotateKeys( keys );
						}

						if( Control* y = c->GetYController( ) )
						{
							std::vector<FloatKey> keys;
							if( y->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ILinFloatKey>( y, keys, range );
								data->SetXRotateType( Niflib::XYZ_ROTATION_KEY );
							}

							else if( y->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( y, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							else if( y->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( y, keys, range );
								data->SetXRotateType( Niflib::TBC_KEY );
							}

							else
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( y, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							data->SetYRotateKeys( keys );
						}

						if( Control* z = c->GetZController( ) )
						{
							std::vector<FloatKey> keys;
							if( z->ClassID( ) == Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ILinFloatKey>( z, keys, range );
								data->SetXRotateType( Niflib::XYZ_ROTATION_KEY );
							}

							else if( z->ClassID( ) == Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( z, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							else if( z->ClassID( ) == Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) )
							{
								nkeys += GetKeys<FloatKey, ITCBFloatKey>( z, keys, range );
								data->SetXRotateType( Niflib::TBC_KEY );
							}

							else
							{
								nkeys += GetKeys<FloatKey, IBezFloatKey>( z, keys, range );
								data->SetXRotateType( Niflib::QUADRATIC_KEY );
							}

							data->SetZRotateKeys( keys );
						}

						// XYZ can have adverse affects if no keys are present
						if( nkeys == 0 )
							data->SetRotateType( Niflib::QUADRATIC_KEY );

					}
					else if( c->ClassID( ) == Class_ID( TCBINTERP_ROTATION_CLASS_ID, 0 ) )
					{
						std::vector<QuatKey> keys;
						data->SetRotateType( Niflib::TBC_KEY );
						nkeys += GetKeys<QuatKey, ITCBRotKey>( c, keys, range );
						data->SetQuatRotateKeys( keys );
					}
					else
						keepData = SampleAnimation( node, range, prsRot, data );

					if( nkeys != 0 ) // if no changes set the base transform
						keepData = true;
				}

				// Scale
				if( Control* c = tmCont->GetScaleController( ) )
				{
					int nkeys = 0;
					if( c->ClassID( ) == Class_ID( LININTERP_SCALE_CLASS_ID, 0 ) )
					{
						std::vector<FloatKey> keys;
						data->SetScaleType( Niflib::LINEAR_KEY );
						nkeys += GetKeys<FloatKey, ILinFloatKey>( c, keys, range );
						data->SetScaleKeys( keys );
					}

					else if( c->ClassID( ) == Class_ID( LININTERP_SCALE_CLASS_ID, 0 ) )
					{
						std::vector<FloatKey> keys;
						data->SetScaleType( Niflib::QUADRATIC_KEY );
						nkeys += GetKeys<FloatKey, IBezFloatKey>( c, keys, range );

						// I think max has a bug in the Bezier Scale control w.r.t. the keycontroller.
						float timeOffset = -FrameToTime( range.Start( ) );
						for( int i = 0, n = keys.size( ); i < n; ++i )
						{
							FloatKey k = InterpKey<FloatKey>( c, TimeToFrame( keys[ i ].time - timeOffset ), timeOffset );
							keys[ i ].data = k.data;
						}

						data->SetScaleKeys( keys );
					}

					else if( c->ClassID( ) == Class_ID( TCBINTERP_SCALE_CLASS_ID, 0 ) )
					{
						std::vector<FloatKey> keys;
						data->SetScaleType( Niflib::TBC_KEY );
						nkeys += GetKeys<FloatKey, ITCBFloatKey>( c, keys, range );
						data->SetScaleKeys( keys );
					}

					if( nkeys != 0 ) // if no changes set the base transform
						keepData = true;
				}
			}

			// only add transform data object if data actually is present
			if( !keepData )
			{
				ninode->RemoveController( timeControl );
				timeControl = NULL;
			}
			else
				objRefs.insert( Niflib::StaticCast<Niflib::NiAVObject>( ninode ) );
		}
	}

	return timeControl;
}

bool AnimationExport::SampleAnimation( INode* node, Interval& range, PosRotScale prs, Niflib::NiKeyframeDataRef data )
{
	bool keepData = false;

	std::vector<Vector3Key> posKeys;
	std::vector<QuatKey> rotKeys;
	TimeValue interval = ( range.Duration( ) ) / TicksPerFrame;
	Niflib::Quaternion prevq;

	for( TimeValue t = range.Start( ); t <= range.End( ); t += interval )
	{
		Matrix3 tm = ne.getNodeTransform( node, t, true );
		Vector3Key p;
		QuatKey q;

		q.time = p.time = FrameToTime( t );
		p.data = TOVECTOR3( tm.GetTrans( ) );
		q.data = TOQUAT( Quat( tm ), true );

		if( t != range.Start( ) )
		{
			if( QuatDot( q.data, prevq ) < 0.0f )
				q.data.Set( -q.data.w, -q.data.x, -q.data.y, -q.data.z );
		}

		prevq = q.data;

		posKeys.push_back( p );
		rotKeys.push_back( q );
	}

	// Dont really know what else to use since I cant get anything but the raw data.
	if( prs & prsPos && !posKeys.empty( ) )
	{
		data->SetTranslateType( Niflib::LINEAR_KEY );
		data->SetTranslateKeys( posKeys );
		keepData = true;
	}

	if( prs & prsRot && !rotKeys.empty( ) )
	{
		data->SetRotateType( Niflib::LINEAR_KEY );
		data->SetQuatRotateKeys( rotKeys );
		keepData = true;
	}

	if( prs & prsScale )
	{
	}

	return keepData;
}

void AnimationExport::GetTimeRange( Control* c, Interval& range )
{
	ne.GetTimeRange( c, range );
}

Interval AnimationExport::GetTimeRange( INode* node )
{
	Interval range = ne.GetTimeRange( node );
	if( range.Empty( ) )
	{
		// Allow specific types of controllers to use the global range
		if( Control* tmCont = GetTMController( node ) )
		{
			Class_ID cID = tmCont->ClassID( );
			if( cID == BIPSLAVE_CONTROL_CLASS_ID
			 || cID == BIPBODY_CONTROL_CLASS_ID
			 || cID == IKCONTROL_CLASS_ID
			 || cID == IKCHAINCONTROL_CLASS_ID
			 )
			{
				range = ne.globalRange;
			}
		}
	}

	return range;
}

Control* AnimationExport::GetTMController( INode* node )
{
	Control* c = node->GetTMController( );
	if( NULL == c )
		return NULL;

	return c;
}

bool AnimationExport::splitAccum( Niflib::NiTransformDataRef base, Niflib::NiTransformDataRef accum, Exporter::AccumType accumType )
{
	std::vector<Vector3Key> baseTrans = base->GetTranslateKeys( );
	std::vector<Vector3Key> accmTrans;

	accmTrans.resize( baseTrans.size( ) );
	accum->SetTranslateType( base->GetTranslateType( ) );

	for( int i = 0, n = baseTrans.size( ); i < n; ++i )
	{
		accmTrans[ i ] = baseTrans[ i ];
		accmTrans[ i ].data = Niflib::Vector3( 0.0f, 0.0f, 0.0f );

		if( accumType & Exporter::AT_X )
			swap( accmTrans[ i ].data.x, baseTrans[ i ].data.x );

		if( accumType & Exporter::AT_Y )
			swap( accmTrans[ i ].data.y, baseTrans[ i ].data.y );

		if( accumType & Exporter::AT_Z )
			swap( accmTrans[ i ].data.z, baseTrans[ i ].data.z );
	}

	base->SetTranslateKeys( accmTrans );
	accum->SetTranslateKeys( baseTrans );

	// Rotate
	accum->SetRotateType( base->GetRotateType( ) );
	accum->SetQuatRotateKeys( base->GetQuatRotateKeys( ) );
	base->SetQuatRotateKeys( std::vector<QuatKey>( ) );

	accum->SetXRotateType( base->GetXRotateType( ) );
	accum->SetXRotateKeys( base->GetXRotateKeys( ) );
	base->SetXRotateKeys( std::vector<FloatKey>( ) );

	accum->SetYRotateType( base->GetYRotateType( ) );
	accum->SetYRotateKeys( base->GetYRotateKeys( ) );
	base->SetYRotateKeys( std::vector<FloatKey>( ) );

	accum->SetZRotateType( base->GetZRotateType( ) );
	accum->SetZRotateKeys( base->GetZRotateKeys( ) );
	base->SetZRotateKeys( std::vector<FloatKey>( ) );

	// Scale
	accum->SetScaleType( base->GetScaleType( ) );
	accum->SetScaleKeys( base->GetScaleKeys( ) );
	base->SetScaleKeys( std::vector<FloatKey>( ) );

	return true;
}