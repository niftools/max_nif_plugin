
#include "stdafx.h"

const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID(0x118f7e02,0xffee238a);

enum 
{
   IPOS_X_REF	=	0,
   IPOS_Y_REF	=	1,
   IPOS_Z_REF	=	2,
   IPOS_W_REF	=	3,
};

struct AnimationImport
{
	AnimationImport( NifImporter& parent )
		: ni( parent )
	{
	}

	NifImporter& ni;

	std::vector<KeyTextValue>	BuildKeyValues( Niflib::NiObjectNETRef nref );
	bool						AddValues( Niflib::NiObjectNETRef nref );
	bool						AddValues( std::vector<Niflib::NiObjectNETRef>& nodes );

	bool						AddValues( Control* c, Niflib::NiKeyframeDataRef data, float time );
	bool						AddBiped( Control* c, Niflib::NiKeyframeDataRef data, float time );

	bool						AddValues( Niflib::NiInterpolatorRef interp, IParamBlock* pblock, float time );

	Control*					MakePosition( Control* tmCont, Class_ID clsid );
	Control*					MakePositionXYZ( Control* tmCont, Class_ID clsid );
	Control*					MakeRotation( Control* tmCont, Class_ID rotClsid, Class_ID rollClsid );
	Control*					MakeScale( Control* tmCont, Class_ID clsid );
	Control*					MakeFloat( IParamBlock* pblock, int idx, Class_ID clsid );

	Control*					GetTMController( const std::wstring& name );
	Control*					GetTMController( Niflib::NiObjectNETRef node );
	Matrix3						GetTM( const std::wstring& name, TimeValue t = 0 );

	bool						GetTransformData( Niflib::ControllerLink& lnk, std::wstring name, Niflib::NiKeyframeDataRef& outData, Point3& p, Quat& q, float& s );

	//bool						ImportGeoMorph( INode* n, Niflib::NiGeomMoprherControllerRef ctrl, float time );
	//INode*					CreateGeoMesh( const std::vector<Niflib::Vector3>& verts, const std::vector<Niflib::Triangle>& tris, Matrix3& tm, INode* parent );

};

// =============================================================
// Global
// =============================================================

static void ClearAnimation( Control* c )
{
	if( c != NULL )
	{
		if( c->IsColorController( ) )
			return;

		if( IKeyControl* ikeys = GetKeyControlInterface( c ) )
			ikeys->SetNumKeys( 0 );

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5

		if( Control* sc = c->GetWController( ) )
			if( sc != c ) ClearAnimation( sc );

#endif

		if( Control* sc = c->GetXController( ) )
			if( sc != c ) ClearAnimation( sc );

		if( Control* sc = c->GetYController( ) )
			if( sc != c ) ClearAnimation( sc );

		if( Control* sc = c->GetZController( ) )
			if( sc != c ) ClearAnimation( sc );

		if( Control* sc = c->GetRotationController( ) )
			if( sc != c ) ClearAnimation( sc );

		if( Control* sc = c->GetPositionController( ) )
			if( sc != c ) ClearAnimation( sc );

		if( Control* sc = c->GetScaleController( ) )
			if( sc != c ) ClearAnimation( sc );
	}
}

// =============================================================
// NifImporter
// =============================================================

void NifImporter::ClearAnimation( INode* node )
{
	if( node != NULL )
	{
		if( node->HasNoteTracks( ) )
		{
			for( int i = node->NumNoteTracks( ) - 1; i >= 0; --i )
			{
				if( NoteTrack* nt = node->GetNoteTrack( i ) )
					node->DeleteNoteTrack( nt, TRUE );
			}
		}

		node->DeleteKeys( TRACK_DOALL );
		::ClearAnimation( node->GetTMController( ) );

		for( int i = 0, n = node->NumberOfChildren( ); i < n; ++i )
			ClearAnimation( node->GetChildNode( i ) );

		// Clear animation priority
		if( node->UserPropExists( NP_ANM_PRI ) )
			node->SetUserPropString( NP_ANM_PRI, _T( "" ) );
	}
}

void NifImporter::ClearAnimation( )
{
	if( clearAnimation )
	{
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7

		if( IFrameTagManager* tagMgr = (IFrameTagManager*) GetCOREInterface( FRAMETAGMANAGER_INTERFACE ) )
		{
			int n = tagMgr->GetTagCount( );
			for( int i = n - 1; i >= 0; --i )
				tagMgr->DeleteTag( tagMgr->GetTagID( i ) );
		}

#endif

		ClearAnimation( gi->GetRootNode( ) );
	}
}

bool NifImporter::ImportAnimation( )
{
	if( !enableAnimations )
		return false;

	if( nodes.empty( ) )
		return false;

	AnimationImport ai( *this );
	
	std::vector<Niflib::NiObjectNETRef> roots;
	roots.push_back( root );

	return ai.AddValues( roots );
}

bool NifImporter::AddNoteTracks( float time, std::wstring name, std::wstring target, Niflib::NiTextKeyExtraDataRef textKeyData, bool loop )
{
	if( textKeyData == NULL )
		return false;

	std::vector<StringKey> textKeys = textKeyData->GetKeys( );
	if( !textKeys.empty( ) )
	{
		Interval range = gi->GetAnimRange( );
		for( std::vector<StringKey>::iterator itr = textKeys.begin( ); itr != textKeys.end( ); ++itr )
		{
			TimeValue t = TimeToFrame( time + (*itr).time );
			if( t < range.Start( ) )
				range.SetStart( t );

			if( t > range.End( ) )
				range.SetEnd( t );
		}

		gi->SetAnimRange( range );

		if( addNoteTracks )
		{
			if( INode* n = GetNode( target ) )
			{
				DefNoteTrack* nt = (DefNoteTrack*) NewDefaultNoteTrack( );
				n->AddNoteTrack( nt );

				for( std::vector<StringKey>::iterator itr = textKeys.begin( ); itr != textKeys.end( ); ++itr )
				{
					TimeValue t = TimeToFrame( time + (*itr).time );

					if( wildmatch( _T( "start*" ), wide( (*itr).data ) ) )
					{
						stringlist args = TokenizeCommandLine( wide( (*itr).data ).c_str( ), true );
						if( args.empty( ) )
							continue;

						bool hasName = false;
						bool hasLoop = false;
						bool hasAt = false;

						for( stringlist::iterator itr = args.begin( ); itr != args.end( ); ++itr )
						{
							if( strmatch( _T( "-name" ), *itr ) )
							{
								if( ++itr == args.end( ) )
									break;

								hasName = true;
							}
							else if( strmatch( _T( "-loop" ), *itr ) )
								hasLoop = true;

							else if( strmatch( _T( "-at" ), *itr ) )
							{
								if( ++itr == args.end( ) )
									break;

								hasAt = true;
							}

						}

						if( !hasName )
						{
							if( name.empty( ) )
								name = FormatString( _T( "EMPTY_SEQUENCE_AT_%df" ), int( t * FramesPerSecond / TicksPerFrame ) );

							args.push_back( _T( "-name" ) );
							args.push_back( name );
						}

						if( !hasLoop && loop )
							args.push_back( _T( "-loop" ) );

						if( !hasAt )
						{
							args.push_back( _T( "-at" ) );
							args.push_back( _T( "y" ) );
						}

						std::wstring line = JoinCommandLine( args );

						NoteKey* key = new NoteKey( t, line.c_str( ), 0 );
						nt->keys.Append( 1, &key );
					}
					else
					{
						NoteKey* key = new NoteKey( t, wide( (*itr).data ).c_str( ), 0 );
						nt->keys.Append( 1, &key );
					}
				}
			}
		}

		if( addTimeTags )
		{
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7

			if( IFrameTagManager* tagMgr = (IFrameTagManager*) GetCOREInterface( FRAMETAGMANAGER_INTERFACE ) )
			{
				for( std::vector<StringKey>::iterator itr = textKeys.begin( ); itr != textKeys.end( ); ++itr )
					tagMgr->CreateNewTag( const_cast<TCHAR*>( wide( (*itr).data ).c_str( ) ), TimeToFrame( time + (*itr).time ), 0, FALSE );
			}

#endif
		}

		return true;
	}

	return false;
}

// =============================================================
// AnimationImport
// =============================================================

bool AnimationImport::AddValues( std::vector<Niflib::NiObjectNETRef>& nodes )
{
	bool ok = false;
	for( std::vector<Niflib::NiObjectNETRef>::iterator itr = nodes.begin( ), end = nodes.end( ); itr != end; ++itr  )
	{
		ok |= AddValues( *itr );
		Niflib::NiNodeRef ninode = (*itr );

		if( ninode )
			ok |= AddValues( Niflib::DynamicCast<Niflib::NiObjectNET>( ninode->GetChildren( ) ) );
	}

	return ok;
}

bool AnimationImport::AddValues( Niflib::NiObjectNETRef nref )
{
	if( Niflib::NiTextKeyExtraDataRef keydata = SelectFirstObjectOfType<Niflib::NiTextKeyExtraData>( nref->GetExtraData( ) ) )
		ni.AddNoteTracks( 0.0f, std::wstring( ), wide( nref->GetName( ) ), keydata, false );

	bool ok = false;
	float time = 0.0f;

	Niflib::list<Niflib::NiTimeControllerRef> clist = nref->GetControllers( );
	if( Niflib::NiTransformControllerRef tc = SelectFirstObjectOfType<Niflib::NiTransformController>( clist ) )
	{
		if( Niflib::NiTransformInterpolatorRef interp = tc->GetInterpolator( ) )
		{
			if( Niflib::NiTransformDataRef data = interp->GetData( ) )
			{
				if( Control* c = GetTMController( nref ) )
					ok |= AddValues( c, data, time );
			}
		}
	}
	else if( Niflib::NiKeyframeControllerRef kf = SelectFirstObjectOfType<Niflib::NiKeyframeController>( clist ) )
	{
		if( Niflib::NiKeyframeDataRef kfData = kf->GetData( ) )
		{
			if( Control* c = GetTMController( nref ) )
				ok |= AddValues( c, kfData, time );
		}
	}
	/*
	else if( Niflib::NiGeomMorpherControllerRef gmc = SelectFirstObjectOfType<Niflib::NiGeomMorpherController>( clist ) )
	{
		if( INode* n = ni.GetNode( nref ) )
			ok |= ImportGeoMorph( n, gmc, time );
	}
	*/

	return ok;
}

bool AnimationImport::AddValues( Control* c, Niflib::NiKeyframeDataRef data, float time )
{
#ifdef USE_BIPED

	// Bipeds are special. And will crash if you dont treat them with care
	if( ( c->ClassID( ) == BIPSLAVE_CONTROL_CLASS_ID )
	||  ( c->ClassID( ) == BIPBODY_CONTROL_CLASS_ID )
	||  ( c->ClassID( ) == FOOTPRINT_CLASS_ID )
	{
		return AddBiped( c, data, time );
	}

#endif

	std::vector<Vector3Key> posKeys = data->GetTranslateKeys( );
	std::vector<QuatKey> quatKeys = data->GetQuatRotateKeys( );
	std::vector<FloatKey> sclKeys = data->GetScaleKeys( );
	std::vector<FloatKey> xKeys = data->GetXRotateKeys( );
	std::vector<FloatKey> yKeys = data->GetYRotateKeys( );
	std::vector<FloatKey> zKeys = data->GetZRotateKeys( );

	// Require more than one key to import (to avoid zero frame positioning used in mw and daoc
	if( ni.requireMultipleKeys &&
	  !( posKeys.size( ) > 1
	  || quatKeys.size( ) > 1
	  || sclKeys.size( ) > 1
	  || xKeys.size( ) > 1
	  || yKeys.size( ) > 1
	  || zKeys.size( ) > 1
	  ))
	{
		return false;
	}

	// Handle Translation
	switch( data->GetTranslateType( ) )
	{
	case Niflib::LINEAR_KEY:
		if( Control* subCtrl = MakePositionXYZ( c, Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			std::vector<FloatKey> xkeys, ykeys, zkeys;
			SplitKeys( posKeys, xkeys, ykeys, zkeys );

			MergeKeys<ILinFloatKey, FloatKey>( subCtrl->GetXController( ), xkeys, time );
			MergeKeys<ILinFloatKey, FloatKey>( subCtrl->GetYController( ), ykeys, time );
			MergeKeys<ILinFloatKey, FloatKey>( subCtrl->GetZController( ), zkeys, time );
		}
		break;

	case Niflib::QUADRATIC_KEY:
	case Niflib::XYZ_ROTATION_KEY:
		if( Control* subCtrl = MakePositionXYZ( c, Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			std::vector<FloatKey> xkeys, ykeys, zkeys;
			SplitKeys( posKeys, xkeys, ykeys, zkeys );

			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetXController( ), xkeys, time );
			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetYController( ), ykeys, time );
			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetZController( ), zkeys, time );
		}
		break;

	case Niflib::TBC_KEY:
		if( Control* subCtrl = MakePositionXYZ( c, Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			std::vector<FloatKey> xkeys, ykeys, zkeys;
			SplitKeys( posKeys, xkeys, ykeys, zkeys );

			MergeKeys<ITCBFloatKey, FloatKey>( subCtrl->GetXController( ), xkeys, time );
			MergeKeys<ITCBFloatKey, FloatKey>( subCtrl->GetYController( ), ykeys, time );
			MergeKeys<ITCBFloatKey, FloatKey>( subCtrl->GetZController( ), zkeys, time );
		}
		break;
	}

	// Handle rotation
	switch( data->GetRotateType( ) )
	{
	case Niflib::LINEAR_KEY:
		if( Control* subCtrl = MakeRotation( c, Class_ID( LININTERP_ROTATION_CLASS_ID, 0 ), Class_ID( LININTERP_FLOAT_CLASS_ID, 0 ) ) )
			MergeKeys<ILinRotKey, QuatKey>( subCtrl, quatKeys, time );

		break;

	case Niflib::QUADRATIC_KEY:
		if( Control* subCtrl = MakeRotation( c, Class_ID( HYBRIDINTERP_ROTATION_CLASS_ID, 0 ), Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
			MergeKeys<IBezQuatKey, QuatKey>( subCtrl, quatKeys, time );

		break;

	case Niflib::XYZ_ROTATION_KEY:
		if( Control* subCtrl = MakeRotation( c, Class_ID( EULER_CONTROL_CLASS_ID, 0 ), Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetXController( ), xKeys, time );
			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetYController( ), yKeys, time );
			MergeKeys<IBezFloatKey, FloatKey>( subCtrl->GetZController( ), zKeys, time );
		}
		break;

	case Niflib::TBC_KEY:
		if( ni.replaceTCBRotationWithBezier )
		{
			// TCB simply is not working for me. Better off with Bezier as a workaround
			if( Control* subCtrl = MakeRotation( c, Class_ID( HYBRIDINTERP_ROTATION_CLASS_ID, 0 ), Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
				MergeKeys<IBezQuatKey, QuatKey>( subCtrl, quatKeys, time );
		}
		else
		{
			// TCB simply is not working for me. Better off with Bezier as a workaround
			if( Control* subCtrl = MakeRotation( c, Class_ID( TCBINTERP_ROTATION_CLASS_ID, 0 ), Class_ID( TCBINTERP_FLOAT_CLASS_ID, 0 ) ) )
				MergeKeys<ITCBRotKey, QuatKey>( subCtrl, quatKeys, time );
		}
		break;
	}

	// Handle Scale
	switch( data->GetScaleType( ) )
	{
	case Niflib::LINEAR_KEY:
		if( Control* subCtrl = MakeScale( c, Class_ID( LININTERP_SCALE_CLASS_ID, 0 ) ) )
			MergeKeys<ILinScaleKey, FloatKey>( subCtrl, sclKeys, time );

		break;

	case Niflib::QUADRATIC_KEY:
	case Niflib::XYZ_ROTATION_KEY:
		if( Control* subCtrl = MakeScale( c, Class_ID( HYBRIDINTERP_SCALE_CLASS_ID, 0 ) ) )
			MergeKeys<IBezScaleKey, FloatKey>( subCtrl, sclKeys, time );

		break;

	case Niflib::TBC_KEY:
		if( Control* subCtrl = MakeScale( c, Class_ID( TCBINTERP_SCALE_CLASS_ID, 0 ) ) )
			MergeKeys<ITCBScaleKey, FloatKey>( subCtrl, sclKeys, time );

		break;
	}

	return true;
}

Control* AnimationImport::GetTMController( const std::wstring& name )
{
	INode* n = ni.GetNode( name );
	if( NULL == n )
		return NULL;

	Control* c = n->GetTMController( );
	if( NULL == c )
		return NULL;

	return c;
}

Control* AnimationImport::GetTMController( Niflib::NiObjectNETRef obj )
{
	if( obj->IsDerivedType( Niflib::NiNode::TYPE ) )
	{
		Niflib::NiNodeRef node = Niflib::StaticCast<Niflib::NiNode>( obj );
		if( INode* n = ni.GetNode( node ) )
			return n->GetTMController( );
	}

	return GetTMController( wide( obj->GetName( ) ).c_str( ) );
}

Matrix3 AnimationImport::GetTM( const std::wstring& name, TimeValue t )
{
	INode* n = ni.GetNode( name );
	if( NULL == n )
		return Matrix3( true );

	return n->GetObjTMAfterWSM( t );
}

Control* AnimationImport::MakePositionXYZ( Control* tmCont, Class_ID clsid )
{
	Interface* ip = ni.gi;

	// First make the controller and XYZ Independent position controller, then fix individuals
	if( Control* c = MakePosition( tmCont, IPOS_CONTROL_CLASS_ID ) )
	{
		if( Control* x = c->GetXController( ) )
		{
			if( x->ClassID( ) != clsid )
			{
				if( Control* tmp = (Control*) ip->CreateInstance( CTRL_FLOAT_CLASS_ID, clsid ) )
					c->ReplaceReference( IPOS_X_REF, tmp );
			}
		}

		if( Control* y = c->GetYController( ) )
		{
			if( y->ClassID( ) != clsid )
			{
				if( Control* tmp = (Control*) ip->CreateInstance( CTRL_FLOAT_CLASS_ID, clsid ) )
					c->ReplaceReference( IPOS_Y_REF, tmp );
			}
		}

		if( Control* z = c->GetZController( ) )
		{
			if( z->ClassID( ) != clsid )
			{
				if( Control* tmp = (Control*) ip->CreateInstance( CTRL_FLOAT_CLASS_ID, clsid ) )
					c->ReplaceReference( IPOS_Z_REF, tmp );
			}
		}

		return c;
	}

	return NULL;
}

Control* AnimationImport::MakePosition( Control* tmCont, Class_ID clsid )
{
	Interface* ip = ni.gi;
	
	if( Control* c = tmCont->GetPositionController( ) )
	{
		if( c->ClassID( ) != clsid )
		{
			if( Control* tmpCtrl = (Control*) ip->CreateInstance( CTRL_POSITION_CLASS_ID, clsid ) )
				tmpCtrl->DeleteThis( );

			else
				c = tmpCtrl;
		}

		return c;
	}

	return NULL;
}

Control* AnimationImport::MakeRotation( Control* tmCont, Class_ID rotClsid, Class_ID rollClsid )
{
	Interface* ip = ni.gi;

	if( Control* c = tmCont->GetRotationController( ) )
	{
		if( c->ClassID( ) != rotClsid )
		{
			if( Control* tmpCtrl = (Control*) ip->CreateInstance( CTRL_ROTATION_CLASS_ID, rotClsid ) )
			{
				if( !tmCont->SetRotationController( tmpCtrl ) )
					tmpCtrl->DeleteThis( );

				else
					c = tmpCtrl;
			}
		}

		if( Control* r = tmCont->GetRollController( ) )
		{
			if( r->ClassID( ) != rollClsid )
			{
				if( Control* r = (Control*) ip->CreateInstance( CTRL_FLOAT_CLASS_ID, rollClsid ) )
					if( !tmCont->SetRollController( r ) )
						r->DeleteThis( );
			}
		}

		return c;
	}

	return NULL;
}

Control* AnimationImport::MakeScale( Control* tmCont, Class_ID clsid )
{
	Interface* ip = ni.gi;

	if( Control* c = tmCont->GetScaleController( ) )
	{
		if( c->ClassID( ) != clsid )
		{
			if( Control* tmpCtrl = (Control*) ip->CreateInstance( CTRL_SCALE_CLASS_ID, clsid ) )
			{
				if( !tmCont->SetScaleController( tmpCtrl ) )
					tmpCtrl->DeleteThis( );

				else
					c = tmpCtrl;
			}
		}

		return c;
	}

	return NULL;
}

bool NifImporter::ImportMtlAndTexAnimation( std::list<Niflib::NiTimeControllerRef> controllers, Mtl* mat )
{
	for( std::list<Niflib::NiTimeControllerRef>::iterator itr = controllers.begin( ); itr != controllers.end( ); ++itr )
	{
		if( Niflib::NiSingleInterpControllerRef interpCtrl = Niflib::DynamicCast<Niflib::NiSingleInterpController>( (*itr) ) )
		{
			if( Niflib::NiFloatInterpolatorRef flInterp = Niflib::DynamicCast<Niflib::NiFloatInterpolator>( interpCtrl->GetInterpolator( ) ) )
			{
				int subAnimID = 0;
				std::vector<FloatKey> keys = flInterp->GetData( )->GetKeys( );
				Texmap* diffTex = mat->GetSubTexmap( ID_DI );
				Niflib::KeyType keyType = flInterp->GetData( )->GetKeyType( );

				if( Niflib::BSLightingShaderPropertyFloatControllerRef ctrl = Niflib::DynamicCast<Niflib::BSLightingShaderPropertyFloatController>( interpCtrl ) )
				{
					switch( ctrl->GetTargetVariable( ) )
					{
					case Niflib::BSLTV_OFFSET_U:
						ImportTextureAnimation( 0, keyType, keys, diffTex );
						break;

					case Niflib::BSLTV_OFFSET_V:
						ImportTextureAnimation( 1, keyType, keys, diffTex );
						break;

					case Niflib::BSLTV_SCALE_U:
						ImportTextureAnimation( 2, keyType, keys, diffTex );
						break;

					case Niflib::BSLTV_SCALE_V:
						ImportTextureAnimation( 3, keyType, keys, diffTex );
						break;

					default:
						continue;
					}
				}
				else if( Niflib::BSEffectShaderPropertyFloatControllerRef ctrl = Niflib::DynamicCast<Niflib::BSEffectShaderPropertyFloatController>( interpCtrl ) )
				{
					switch( ctrl->GetTargetVariable( ) )
					{
					case Niflib::BSETV_OFFSET_U:
						ImportTextureAnimation( 0, keyType, keys, diffTex );
						break;

					case Niflib::BSETV_OFFSET_V:
						ImportTextureAnimation( 1, keyType, keys, diffTex );
						break;

					case Niflib::BSETV_SCALE_U:
						ImportTextureAnimation( 2, keyType, keys, diffTex );
						break;

					case Niflib::BSETV_SCALE_V:
						ImportTextureAnimation( 3, keyType, keys, diffTex );
						break;

					case Niflib::BSETV_ALPHA:
						ImportMaterialAnimation( 2, 0, keyType, keys, mat );
						break;

					default:
						continue;
					}
				}
			}
		}
	}

	return true;
}

bool NifImporter::ImportTextureAnimation( int subAnimID, Niflib::KeyType keyType, std::vector<FloatKey> keys, Texmap* tex )
{
	StdUVGen* stdUVGen = (StdUVGen*) tex->GetTheUVGen( );

	if( keyType == Niflib::QUADRATIC_KEY )
	{
		if( Control* tmpCtrl = (Control*) gi->CreateInstance( CTRL_FLOAT_CLASS_ID, Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			MergeKeys<IBezFloatKey, FloatKey>( tmpCtrl, keys, 0.0f );
			stdUVGen->SubAnim( 0 )->AssignController( tmpCtrl, subAnimID );

			return true;
		}
	}

	return false;
}

bool NifImporter::ImportMaterialAnimation( int paramBlockID, int subAnimID, Niflib::KeyType keyType, std::vector<FloatKey> keys, Mtl* mtl )
{
	if( keyType == Niflib::QUADRATIC_KEY )
	{
		if( Control* tmpCtrl = (Control*) gi->CreateInstance( CTRL_FLOAT_CLASS_ID, Class_ID( HYBRIDINTERP_FLOAT_CLASS_ID, 0 ) ) )
		{
			MergeKeys<IBezFloatKey, FloatKey>( tmpCtrl, keys, 0.0f );
			MSTR str = mtl->SubAnim( paramBlockID )->SubAnimName( subAnimID );
			mtl->SubAnim( paramBlockID )->AssignController( tmpCtrl, subAnimID );
			
			return true;
		}
	}

	return false;
}