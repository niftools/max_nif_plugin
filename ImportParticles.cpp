
#include "stdafx.h"

Class_ID PArray_CLASS_ID( 0xe3c25b5, 0x109d1659 );
Class_ID PCLOUD_CLASS_ID( 0x1c0f3d2f, 0x30310af9 );

bool NifImporter::ImportParticleSystems( Niflib::NiNodeRef node )
{
	bool ok = true;
	try
	{
		// Check nodes children for Particle Systems and import
		std::vector<Niflib::NiParticleSystemRef> psnodes = Niflib::DynamicCast<Niflib::NiParticleSystem>( node->GetChildren( ) );
		for( std::vector<Niflib::NiParticleSystemRef>::iterator itr = psnodes.begin( ), end = psnodes.end( ); itr != end; ++itr )
			ok |= ImportParticleSystem( *itr );

		// Check nodes children for other nodes to check
		std::vector<Niflib::NiNodeRef> nodes = Niflib::DynamicCast<Niflib::NiNode>( node->GetChildren( ) );
		for( std::vector<Niflib::NiNodeRef>::iterator itr = nodes.begin( ), end = nodes.end( ); itr != end; ++itr )
			ok |= ImportParticleSystems( *itr );
	}
	catch( std::exception& e )
	{
		e = e;
		ok = false;
	}
	catch( ... )
	{
		ok = false;
	}

	return ok;
}

bool NifImporter::ImportParticleSystem( Niflib::NiParticleSystemRef particleSystem )
{
	SimpleObject* psysObj;
	std::wstring name = wide( particleSystem->GetName( ) );
	
	if( wildmatch( _T( "PCloud*" ), name ) )
		psysObj = ImportPCloud( particleSystem );

	if( psysObj )
	{
		//if( INode* node = CreateImportNode( wide( particleSystem->GetName( ) ).c_str( ), psysObj, NULL ) )
		if( ImpNode* inode = i->CreateNode( ) )
		{
			INode* node = inode->GetINode( );
			Niflib::Matrix44 baseTM = ( importBones ) ? particleSystem->GetLocalTransform( ) : particleSystem->GetWorldTransform( );
			
			inode->SetTransform( 0, TOMATRIX3( baseTM ) );
			inode->Reference( psysObj );

			node->SetName( wide( particleSystem->GetName( ) ).c_str( ) );
			
			if( Niflib::NiNodeRef parentNode = particleSystem->GetParent( ) )
			{
				std::wstring parentName = wide( parentNode->GetName( ) );
				if( INode* pnode = GetNode( parentName ) )
					pnode->AttachChild( node );
			}

			// Get list of modifiers
			std::vector<Niflib::NiPSysModifierRef> modifiers = particleSystem->GetModifiers( );

			// Are there any gravity modifiers?
			if( Niflib::NiPSysGravityModifierRef gravModifier = SelectFirstObjectOfType<Niflib::NiPSysGravityModifier>( modifiers ) )
			{
				Niflib::NiNodeRef gravObj = gravModifier->GetGravityObject( );
				INode* gravBone = GetNode( wide( gravObj->GetName( ) ) );
				
				if( !gravBone->GetInterface( GRAVITYOBJECT_CLASS_ID ) )
				{
					INode* gravParentNode = gravBone->GetParentNode( );

					// Remove and delete gravity bone
					gravBone->Detach( 0 );
					gravBone->Delete( 0, 1 );

					// Create new gravity space warp object
					gravBone = CreateGravityWarp( gravModifier, gravParentNode );
				}

				int j = -1;
				if( SimpleWSMObject2* wsm = (SimpleWSMObject2*) gravBone->GetObjectRef( ) )
				{
					if( IDerivedObject* dobj = CreateDerivedObject( psysObj ) )
					{
						Modifier* m = wsm->CreateWSMMod( gravBone );

						dobj->SetAFlag( A_LOCK_TARGET );
						dobj->AddModifier( m );
						dobj->ClearAFlag( A_LOCK_TARGET );

						node->SetObjectRef( dobj );
					}
				}
			}

			if( Mtl* pMat = ImportMaterialAndTextures( inode, Niflib::DynamicCast<Niflib::NiAVObject>( particleSystem ) ) )
			{
				gi->GetMaterialLibrary( ).Add( pMat );
				node->SetMtl( pMat );
			}

			RegisterNode( particleSystem, node );

			return true;
		}
	}

	return false;
}

SimpleObject* NifImporter::ImportPCloud( Niflib::NiParticleSystemRef particleSystem )
{
	if( SimpleObject* obj = (SimpleObject*) gi->CreateInstance( GEOMOBJECT_CLASS_ID, PCLOUD_CLASS_ID ) )
	{
		
		if( IParamArray* parray = obj->GetParamBlock( ) )
		{
			std::vector<Niflib::NiPSysModifierRef> modifiers = particleSystem->GetModifiers( );
			if( modifiers.size( ) <= 0 )
				return NULL;

			Niflib::NiPSysEmitterRef emitterRef = SelectFirstObjectOfType<Niflib::NiPSysEmitter>( modifiers );
			if( emitterRef->IsDerivedType( Niflib::NiPSysMeshEmitter::TYPE ) )
			{
				Niflib::NiPSysMeshEmitterRef meshEmitterRef = Niflib::DynamicCast<Niflib::NiPSysMeshEmitter>( emitterRef );
				std::vector<Niflib::NiTriBasedGeomRef> emitters = meshEmitterRef->GetEmitterMeshes( );

				// Set emitter type to Object
				parray->SetValue( 0 /* PB_CREATEIN */, 0, 3 /* IDC_AP_FILLOBJECT */ );
				
				// Find emitter geo in scene at set
				INode* distnode = GetNode( wide( emitters[ 0 ]->GetName( ) ) );
				obj->ReplaceReference( 2 /* DISTNODE */, distnode );

				// Set reference mesh to emitter geo
				parray->SetValue( 3 /* PB_SPEEDDIR */, 0, 2 /* IDC_AP_PCLOUDDIROBJ */ );
				obj->ReplaceReference( 3 /* MBASE */, distnode );

				// Request update of emitter geo name
				PartID pid;
				obj->NotifyRefChanged( FOREVER, distnode, pid, REFMSG_NODE_NAMECHANGE );
			}

			// Set Width/Height/Depth of emitter
			parray->SetValue( 22 /* PB_EMITRWID */, 0, 100.0f );
			parray->SetValue( 23 /* PB_EMITRHEIGHT */, 0, 100.0f );
			parray->SetValue( 24 /* PB_EMITRDEPTH */, 0, 100.0f );

			// Set viewport display to mesh
			parray->SetValue( 35 /* PB_VIEWPORTSHOWS */, 0, 2 /* IDC_SP_VIEWDISPMESH */ );

			// Set total particles to display
			Niflib::NiPSysDataRef psysData = Niflib::DynamicCast<Niflib::NiPSysData>( particleSystem->GetData( ) );
			parray->SetValue( 8 /* PB_BIRTHMETHOD */, 0, 1 /* IDC_SP_GENUSETTL */ );
			parray->SetValue( 10 /* PB_PTOTALNUMBER */, 0, psysData->GetBSMaxParticles( ) );

			// Set speed + variation
			parray->SetValue( 1 /* PB_SPEED */, 0, emitterRef->GetSpeed( ) );
			parray->SetValue( 2 /* PB_SPEEDVAR */, 0, emitterRef->GetSpeedVariation( ) / 100.0f ); // Percentage

			// Start time/Stop time
			std::list<Niflib::NiTimeControllerRef> controllers = particleSystem->GetControllers( );
			Niflib::NiPSysEmitterCtlrRef emitterCtrlRef = SelectFirstObjectOfType<Niflib::NiPSysEmitterCtlr>( controllers );

			parray->SetValue( 11 /* PB_EMITSTART */, 0, TimeToFrame( emitterCtrlRef->GetStartTime( ) ) );
			parray->SetValue( 12 /* PB_EMITSTOP */, 0, TimeToFrame( emitterCtrlRef->GetStopTime( ) ) );
			
			// Set lifespan and variation
			parray->SetValue( 14 /* PB_LIFE */, 0, TimeToFrame( emitterRef->GetLifeSpan( ) ) );
			parray->SetValue( 15 /* PB_LIFEVAR */, 0, TimeToFrame( emitterRef->GetLifeSpanVariation( ) ) );

			// Set initial radius and variation
			parray->SetValue( 17 /* PB_SIZE */, 0, emitterRef->GetInitialRadius( ) );
			parray->SetValue( 18 /* PB_SIZEVAR */, 0, emitterRef->GetRadiusVariation( ) / 100.0f ); // Percentage

			// Set particle type
			parray->SetValue( 26 /* PB_PARTICLECLASS */, 0, 0 /* IDC_SP_TYPESTD */ );
			parray->SetValue( 27 /* PB_PARTICLETYPE */, 0, 3 /* IDC_SP_TYPEFAC */ );
		}

		return obj;
	}

	return NULL;
}

INode* NifImporter::CreateGravityWarp( Niflib::NiPSysGravityModifierRef gravModifier, INode* parentNode )
{
	if( ImpNode* gravNode = i->CreateNode( ) )
	{
		INode* inode = gravNode->GetINode( );
		Niflib::NiNodeRef gravObj = gravModifier->GetGravityObject( );
		Niflib::Matrix44 baseTM = ( importBones ) ? gravObj->GetLocalTransform( ) : gravObj->GetWorldTransform( );
		
		if( SimpleObject* simpleObj = (SimpleObject*) gi->CreateInstance( WSM_OBJECT_CLASS_ID, Class_ID( GRAVITYOBJECT_CLASS_ID, 0 ) ) )
		{
			gravNode->SetName( wide( gravObj->GetName( ) ).c_str( ) );
			gravNode->SetTransform( 0, TOMATRIX3( baseTM ) );
			gravNode->Reference( simpleObj );

			if( IParamBlock2* pblock2 = simpleObj->GetParamBlockByID( 0 ) )
			{
				pblock2->SetValue( 0 /* PB_STRENGTH */, 0, gravModifier->GetStrength( ) );
				pblock2->SetValue( 1 /* PB_DECAY */, 0, gravModifier->GetDecay( ) );
				pblock2->SetValue( 2 /* PB_TYPE */, 0, gravModifier->GetForceType( ) == Niflib::FORCE_PLANAR ? 0 : 1 );
				pblock2->SetValue( 3 /* PB_ICONSIZE */, 0, 50.0f );

				parentNode->AttachChild( inode );

				return inode;
			}
		}
	}

	return NULL;
}