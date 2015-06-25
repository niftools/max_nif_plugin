
#pragma once

class NifImporter : public BaseImporter
{
public:
	
	// Ini settings

	bool			showTextures;
	bool			removeIllegalFaces;
	bool			removeDegenerateFaces;
	bool			enableAutoSmooth;
	float			autoSmoothAngle;
	bool			flipUVTextures;
	bool			enableSkinSupport;
	bool			goToSkeletonBindPosition;
	bool			enableCollision;
	int				vertexColorMode;
	int				useNiftoolsShader;
	bool			mergeNonAccum;
	bool			enableLights;
	bool			enableCameras;
	bool			importUPB;
	bool			doNotReuseExistingBones;

	// Biped/Bones related settings
	bool			importBones;
	std::wstring	skeleton;
	float			bipedHeight;
	std::wstring	skeletonCheck;
	float			bipedAngle;
	float			bipedAnkleAttach;
	bool			bipedTrianglePelvis;
	bool			importSkeleton;
	bool			useBiped;
	bool			hasSkeleton;
	bool			isBiped;
	bool			removeUnusedImportedBones;
	bool			forceRotation;
	bool			browseForSkeleton;
	std::wstring	defaultSkeletonName;
	float			minBoneWidth;
	float			maxBoneWidth;
	float			boneWidthToLengthRatio;
	bool			createNubsForBones;
	stringlist		dummyNodeMatches;
	bool			convertBillboardsToDummyNodes;
	bool			uncontrolledDummies;
	bool			ignoreRootNode;
	bool			autoDetect;
	stringlist		rotate90Degrees;
	bool			supportPrnStrings;

	// Animation related Settings
	bool			replaceTCBRotationWithBezier;
	bool			enableAnimations;
	bool			requireMultipleKeys;
	bool			applyOverallTransformToSkinAndBones;
	bool			clearAnimation;
	bool			addNoteTracks;
	bool			addTimeTags;

	// Particle System settings
	bool			enableParticleSystems;

	// Collision settings
	float			bhkScaleFactor;

	bool			weldVertices;
	float			weldVertexThresh;

	bool			dummyBonesAsLines;

	std::vector<Niflib::NiObjectRef> blocks;
	std::vector<Niflib::NiNodeRef> nodes;
	std::map<std::wstring, int> ctrlCount; // counter for number of controllers referencing a node

	typedef std::map<Niflib::NiObjectNETRef, INode*> NodeToNodeMap;
	typedef std::map<std::wstring, INode*, ltstr> NameToNodeMap;
	
	NodeToNodeMap	nodeMap;
	NameToNodeMap	nodeNameMap;

	std::vector<Niflib::NiNodeRef> specialNodes;

public:

	NifImporter( const TCHAR* Name, ImpInterface* I, Interface* GI, BOOL SupressPrompts );

	virtual void	Initialize( );
	virtual void	ReadBlocks( );
	
	void			BuildNodes( );

	// Ini File related routines
	virtual void	LoadIniSettings( );
	virtual void	SaveIniSettings( );

	void			ApplyAppSettings( );

	bool			HasSkeleton( );
	bool			IsBiped( );

	void			ImportBones( std::vector<Niflib::NiNodeRef>& bones );
	void			ImportBones( Niflib::NiNodeRef blocks, bool recurse = true );
	//void			ImportBipeds( std::vector<Niflib::NiNodeRef>& blocks );
	//void			AlignBiped( IBipMaster* master, Niflib::NiNodeRef block );
	bool			ImportMeshes( Niflib::NiNodeRef block );

	std::wstring	FindImage( const std::wstring& name );

	bool			ImportUPB( INode* node, Niflib::NiNodeRef block );

	void			SetTriangles( Mesh& mesh, const std::vector<Niflib::Triangle>& v );
	void			SetNormals( Mesh& mesh, const std::vector<Niflib::Triangle>& t, const std::vector<Niflib::Vector3>& v );

	bool			ImportMesh( Niflib::NiTriShapeRef triShape );
	bool			ImportMesh( Niflib::NiTriStripsRef triStrips );
	bool			ImportMesh( Niflib::BSLODTriShapeRef triShape );

	//bool			ImportMultipleGeometry( Niflib::NiNodeRef parent, std::vector<Niflib::NiTriBasedGeomRef>& glist );
	StdMat2*		ImportMaterialAndTextures( ImpNode* node, Niflib::NiAVObjectRef avObject );
	bool			ImportMaterialAndTextures( ImpNode* node, std::vector<Niflib::NiTriBasedGeomRef>& glist );
	//bool			ImportNiftoolsShader( ImpNode* node, Niflib::NiAVObjectRef avObject, StdMat2* m );
	//bool			ImportTransform( ImpNode* node, Niflib::NiAVObjectRef avObject );
	bool			ImportMesh( ImpNode* node, TriObject* o, Niflib::NiTriBasedGeomRef triGeom, Niflib::NiTriBasedGeomDataRef triGeomData, std::vector<Niflib::Triangle>& tris );
	bool			ImportVertexColor( INode* tnode, TriObject* o, std::vector<Niflib::Triangle>& tris, std::vector<Niflib::Color4> cv, int cv_offset = 0 );
	bool			ImportSkin( ImpNode* node, Niflib::NiTriBasedGeomRef triGeom, int v_start = 0 );
	bool			ImportSpecialNodes( );

	bool			ImportParticleSystems( Niflib::NiNodeRef root );
	bool			ImportParticleSystem( Niflib::NiParticleSystemRef particleSystem );

	SimpleObject*	ImportPCloud( Niflib::NiParticleSystemRef particleSystem );

	INode*			CreateGravityWarp( Niflib::NiPSysGravityModifierRef gravModifier, INode* parentNode );

	Texmap*			CreateTexture( Niflib::TexDesc& desc );
	Texmap*			CreateTexture( Niflib::NiTexturePropertyRef desc );
	Texmap*			CreateTexture( Niflib::BSEffectShaderPropertyRef effectShader );
	Texmap*			CreateTexture( const std::wstring& name );
	Texmap*			CreateTexture( const std::wstring& filename, Niflib::BSLightingShaderPropertyRef mode );
	Texmap*			CreateNormalBump( LPCTSTR name, Texmap* nmap );
	Texmap*			CreateMask( LPCTSTR name, Texmap* map, Texmap* mask );

	INode*			CreateBone( const std::wstring& name, Point3 startPos, Point3 endPos, Point3 zAxis );
	INode*			CreateHelper( const std::wstring& name, Point3 startPos );
	INode*			CreateCamera( const std::wstring& name );

	INode*			CreateImportNode( const wchar_t* name, Object* obj, INode* parent );

	//bool			ImportLights( Niflib::NiNodeRef node );
	//bool			ImportLights( std::vector<Niflib::NiLightRef> lights );

	// Primary collision entry point. Tests for bhk objects
	bool			ImportCollision( Niflib::NiNodeRef node );

	void			RegisterNode( Niflib::NiObjectNETRef node, INode* inode );
	INode*			FindNode( Niflib::NiObjectNETRef node );

	INode*			GetNode( Niflib::NiNodeRef node );
	INode*			GetNode( Niflib::NiObjectNETRef obj );

	void			RegisterNode( const std::wstring& name, INode* inode );
	INode*			GetNode( const std::wstring& name );
	INode*			GetNode( const TSTR& name );

	std::wstring	GetSkeleton( AppSettings* appSettings );

	bool			ShowDialog( );
	virtual bool	DoImport( );

	// Animation Helpers
	bool			ImportAnimation( );
	void			ClearAnimation( );
	void			ClearAnimation( INode* node );
	bool			AddNoteTracks( float time, std::wstring name, std::wstring target, Niflib::NiTextKeyExtraDataRef textKeyData, bool loop );
	
	bool			ImportMtlAndTexAnimation( std::list<Niflib::NiTimeControllerRef> controllers, Mtl* mat );
	bool			ImportTextureAnimation( int subAnimID, Niflib::KeyType keyType, std::vector<FloatKey> keys, Texmap* tex );
	bool			ImportMaterialAnimation( int paramBlockID, int subAnimID, Niflib::KeyType keyType, std::vector<FloatKey> keys, Mtl* mtl );

	void			WeldVertices( Mesh& mesh );

	bool			IsSkyrim( ) const		{ return nifVersion == 0x14020007 && userVersion == 12; }
	bool			IsFallout3( ) const		{ return nifVersion == 0x14020007 && userVersion == 11; }
	bool			IsOblivion( ) const		{ return ( nifVersion == 0x14000004 || nifVersion == 0x14000005 ) && ( userVersion == 11 || userVersion == 10 ); }
	bool			IsMorrowind( ) const	{ return nifVersion == 0x04000002 && ( userVersion == 11 || userVersion == 10 ); }

protected:

	NifImporter( );
};