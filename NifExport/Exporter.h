
#pragma once

class Exporter
{
public:

	enum Result
	{
		Error,
		Ok,
		Abort,
		Skip
	};

	enum ExportType
	{
		NIF_WO_ANIM  = 0,
		NIF_WO_KF    = 1,
		SINGLE_KF_WITH_NIF = 2,
		SINGLE_KF_WO_NIF = 3,
		MULTI_KF_WITH_NIF = 4,
		MULTI_KF_WO_NIF = 5,
		NIF_WITH_MGR = 6,
	};

	enum AccumType
	{
		AT_NONE = 0,
		AT_X = 0x01,
		AT_Y = 0x02,
		AT_Z = 0x04,

		AT_XYZ = AT_X | AT_Y | AT_Z,
		AT_FORCE = 0x80000000,
	};

	// Callback for post-processing instructions
	struct NiCallback
	{
		NiCallback( ) { };
		virtual ~NiCallback( ) { };
		virtual Result execute( ) = 0;
	};

	class CancelExporterException{ };

	/* exporter version */
	static int			mVersion;

	/* export options, static for dialog usage. */
	static bool			mUseRegistry;
	static bool			mSelectedOnly;
	static bool			mTriStrips;
	static bool			mExportHidden;
	static bool			mExportFurn;
	static bool			mExportLights;
	static std::wstring mTexPrefix;
	static bool			mVertexColors;
	static float		mWeldThresh;
	static float		mNormThresh;
	static float		mUVWThresh;
	static bool			mExportCollision;
	static bool			mRemapIndices;
	static bool			mExportExtraNodes;
	static bool			mExportSkin;
	static bool			mUserPropBuffer;
	static bool			mFlattenHierarchy;
	static bool			mRemoveUnreferencedBones;
	static bool			mSortNodesToEnd;
	static std::wstring mGameName;
	static std::wstring mNifVersion;
	static int			mNifVersionInt;
	static int			mNifUserVersion;
	static int			mNifUserVersion2;
	static bool			mSkeletonOnly;
	static bool			mExportCameras;
	static bool			mGenerateBoneCollision;
	static bool			mExportTransforms;
	static float		mDefaultPriority;
	static ExportType	mExportType;
	static bool			mMultiplePartitions;
	static int			mBonesPerVertex;
	static int			mBonesPerPartition;
	static bool			mUseTimeTags;
	static bool			mAutoDetect;
	static bool			mAllowAccum;
	static std::wstring mCreatorName;
	static bool			mCollapseTransforms;
	static bool			mZeroTransforms;
	static bool			mFixNormals;
	static bool			mTangentAndBinormalExtraData;
	static stringlist   mRotate90Degrees;
	static bool			mSupportPrnStrings;
	static bool			mSuppressPrompts;
	static bool			mUseAlternateStripper;
	static float		bhkScaleFactor;
	static int			mTangentAndBinormalMethod;
	static bool			mStartNifskopeAfterStart;
	static std::wstring mNifskopeDir;
	static bool			mTriPartStrips;
	static std::wstring mRootType;
	static stringlist   mRootTypes;
	static std::wstring mExportPath;

	Exporter( Interface* i, AppSettings* appSettings );

	Result doExport( Niflib::NiNodeRef& root, INode* node );

	/* config is written to the registry and the root node
    of the scene, so that it is restored when opening it
    again. creating a new document will make the exporter
    use the settings stored in the registry. */

	// writes config to root node
	static void			writeConfig( INode* node );

	// writes config to registry
	static void			writeConfig( Interface* i );

	// reads config from root node
	static void			readConfig( INode* node );

	// reads config from registry
	static void			readConfig( Interface* i );

	static AppSettings* exportAppSettings( );
	static AppSettings* importAppSettings( std::wstring fname );
	
public:
	typedef std::vector<unsigned short> TriStrip;
	typedef std::list<TriStrip> TriStrips;
	typedef vector<Niflib::Triangle> Triangles;
	typedef std::vector<Niflib::TexCoord> TexCoords;

	struct VertexGroup
	{
		int			   idx;
		Point3		   pt;
		Point3		   norm;
		TexCoords	   uvs;
		Niflib::Color4 color;
	};

	struct FaceGroup
	{
		std::vector<VertexGroup>		vgrp;
		std::vector<int>				vmap;
		std::vector<Niflib::Vector3>	verts;
		std::vector<Niflib::Vector3>	vnorms;
		Triangles						faces;
		std::vector<TexCoords>			uvs;
		std::map<int, int>				uvMapping; // first = Max index, second = Nif index
		std::vector<Niflib::Color4>		vcolors;
		std::vector<int>				vidx;
		std::vector<int>				fidx;
		TriStrips						strips;
	};

	// maps face groups to material ID
	typedef std::map<int, FaceGroup> FaceGroups;
	typedef std::set<INode*> INodeMap;
	typedef std::map<std::wstring, Niflib::NiNodeRef> NodeMap;
	typedef std::map<INode*, Niflib::NiNodeRef> NodeToNodeMap;
	typedef std::list<NiCallback*> CallbackList;
	typedef std::list<Niflib::Ref<Niflib::NiNode> > NodeList;

	Interface*			mI;
	Niflib::NiNodeRef	mNiRoot;
	AppSettings*		mAppSettings;
	NodeMap				mNameMap;
	NodeToNodeMap		mNodeMap;
	INodeMap			mCollisionNodes;
	INodeMap			mHandledNodes;
	INode*				mSceneCollisionNode;
	CallbackList		mPostExportCallbacks;
	Box3				mBoundingBox;
	//...
	Interval			globalRange;

	bool				mRequiredPostProcessing;



	Result				exportNodes( Niflib::NiNodeRef& root, INode* node );
	Result				exportCollision( Niflib::NiNodeRef& root, INode* node );
	Result				scanForCollision( INode* node );
	bool				isCollision( INode* node );
	bool				isHandled( INode* node );
	bool				markAsHandled( INode* node );
	Result				scanForAnimation( INode* node );
	Result				scanForIgnore( INode* node );

	/* utility functions */
	Mtl*				getMaterial( INode* node, int subMtl );
	bool				TMNegParity( const Matrix3& m );
	void				convertMatrix( Niflib::Matrix33& dst, const Matrix3& src );
	void				nodeTransform( Niflib::Matrix33& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local = true );
	void				nodeTransform( Niflib::QuaternionXYZW& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local = true );
	void				objectTransform( Niflib::Matrix33& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local = true );
	void				objectTransform( Niflib::QuaternionXYZW& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local = true );
	Matrix3				getNodeTransform( INode* node, TimeValue t, bool local = true );
	Matrix3				getObjectTransform( INode* node, TimeValue t, bool local = true );
	Point3				getVertexNormal( Mesh* mesh, int faceNo, RVertex* rv );
	bool				equal( const Niflib::Vector3& a, const Point3& b, float thresh );
	BitmapTex*			getTexture( Mtl* mtl );
	BitmapTex*			getTexture( Mtl* mtl, int i );
	void				getTextureMatrix( Matrix3& mat, Mtl* mtl );
	Niflib::NiNodeRef	makeNode( Niflib::NiNodeRef& parent, INode* maxNode, bool local = true );
	Niflib::NiNodeRef	makeSwitchNode( Niflib::NiNodeRef& parent, INode* maxNode, bool local = true );
	Niflib::NiNodeRef	getNode( const wstring& name );
	Niflib::NiNodeRef	getNode( INode* maxNode );
	Niflib::NiNodeRef	findNode( INode* maxNode );
	// returns true if the node contains collision objects
	bool				isCollisionGroup( INode* maxNode, bool root = true );
	// returns true if the node contains meshes
	bool				isMeshGroup( INode* maxNode, bool root = true );
	void				CalcBoundingBox( INode* node, Box3& box, int all = 1 );
	void				CalcBoundingSphere( INode* node, Point3 center, float& radius, int all = 1 );

	/* tristrips */
	void				strippify( TriStrips& strips, std::vector<Niflib::Vector3>& verts, std::vector<Niflib::Vector3>& norms, const Triangles& tris );
	void				strippify( FaceGroup& grp );
	Niflib::NiTriStripsDataRef makeTriStripsData( const TriStrips& strips );

	/* mesh export */
	// adds a vertex to a face group if it doesn't exist yet. returns new or previous index into the
	// vertex array.
	int					addVertex( FaceGroup& grp, int face, int vi, Mesh* mesh, const Matrix3& texm, std::vector<Niflib::Color4>& vertColors );
	// creates face groups from faces with same sub material id
	bool				splitMesh( INode* node, Mesh&, FaceGroups& grps, TimeValue t, std::vector<Niflib::Color4>& vertColors, bool noSplit );
	// creates a NiTriStrips or NiTriShape hierarchy from a face group
	Niflib::NiTriBasedGeomRef makeMesh( Niflib::NiNodeRef& parent, Mtl* mtl, FaceGroup& grp, bool exportStrips );
	// splits mesh and converts it into nif blocks
	Result				exportMesh( Niflib::NiNodeRef& parent, INode* node, TimeValue t );

	/* textures and materials */
	// creates NiTexturingProperty + NiSourceTexture
	void				makeTexture( Niflib::NiAVObjectRef &parent, Mtl* mtl );
	//...
	// creates a NiMaterialProperty
	void				makeMaterial( Niflib::NiAVObjectRef& parent, Mtl* mtl );
	bool				exportNiftoolsShader( Niflib::NiAVObjectRef parent, Mtl* mtl );

	/* havok & collision */
	int					addVertex( std::vector<Niflib::Vector3>& verts, std::vector<Niflib::Vector3>& vnorms, const Point3& pt, const Point3& norm );
	void				addFace( Triangles& tris, std::vector<Niflib::Vector3>& verts, std::vector<Niflib::Vector3>& vnorms, int face, const int vi[ 3 ], Mesh* mesh, Matrix3& tm );
	//...
	/* creates a bhkRigidBody */
	Niflib::bhkRigidBodyRef makeCollisionBody( INode* node );
	/* creates a collision shape from a node */
	Niflib::bhkShapeRef	makeCollisionShape( INode* node, Matrix3& tm, Niflib::bhkRigidBodyRef body, Niflib::HavokMaterial mtlDefault );

	Niflib::bhkShapeRef makeTriStripsShape( INode* node, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeBoxShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeSphereShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeCapsuleShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeListShape( INode* node, Matrix3& tm, Niflib::bhkRigidBodyRef body, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makebhkBoxShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makebhkSphereShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makebhkCapsuleShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );

	Niflib::bhkConvexVerticesShapeRef	 makeConvexShape( Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkNiTriStripsShapeRef		 makeTriStripsShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkPackedNiTriStripsShapeRef makePackedTriStripsShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter );
	//Niflib::bhkMoppBvTreeShapeRef		 makeCompressedMeshShape( FaceGroups& faceGrp, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter );
	Niflib::bhkMoppBvTreeShapeRef		 makeCompressedMeshShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter );

	Niflib::bhkShapeRef makeProxyShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyBoxShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxySphereShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyConvexShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyTriStripShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyPackedTriStripShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyCapsuleShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeProxyOBBShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	
	Niflib::bhkShapeRef makeModifierShape( INode* node, Object* obj, Modifier* mod, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModBoxShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModSphereShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModCapsuleShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModConvexShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModOBBShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModTriStripShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModPackedTriStripShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModPackedTriStripShape( INode* tnode, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	Niflib::bhkShapeRef makeModPackedTriStripShape( INodeTab& map, Matrix3& tm, Niflib::HavokMaterial mtlDefault );
	// ...

	/* skin export */
	bool				makeSkin( Niflib::NiTriBasedGeomRef shape, INode* node, FaceGroup& grp, TimeValue t );
	bool				exportSkin( );

	/* animation export */
	Result				doAnimExport( Niflib::NiControllerSequenceRef root );
	Result				doAnimExport( Niflib::NiControllerManagerRef ctrl, INode* node );
	bool				isNodeTracked( INode* node );
	bool				isNodeKeyed( INode* node );
	Niflib::Ref<Niflib::NiTimeController> CreateController( INode* node, Interval range );
	static void			InitializeTimeController( Niflib::Ref<Niflib::NiTimeController> ctrl, Niflib::NiNodeRef parent );
	Result				exportGeomMorpherControl( Modifier* mod, std::vector<Niflib::Vector3>& verts, std::vector<int>& baseVertIdx, Niflib::NiObjectNETRef owner );
	void				GetTimeRange( Control* c, Interval& range );
	Interval			GetTimeRange( INode* node );

	/* misc export */
	bool				exportUPB( Niflib::NiNodeRef& root, INode* node );
	bool				removeUnreferencedBones( Niflib::NiNodeRef node );
	void				sortNodes( Niflib::NiNodeRef node );
	Niflib::NiNodeRef   exportBone( Niflib::NiNodeRef parent, INode* node );
	Result				exportLight( Niflib::NiNodeRef root, INode* node, GenLight* light );
	void				getChildNodes( INode* node, std::vector<Niflib::NiNodeRef>& list );
	//...
	bool				exportPrn( Niflib::NiNodeRef& root, INode* node );
	Niflib::NiNodeRef	createAccumNode( Niflib::NiNodeRef parent, INode* node );
	int					countNodes( INode* node );
	bool				isSkeletonRoot( INode* node );
	void				ApplyAllSkinOffsets( Niflib::NiAVObjectRef& root );
	void				sortVector3( std::vector<Niflib::Vector3>& vector );
	void				sortVector4( std::vector<Niflib::Vector4>& vector );

	/* specific versions */
	bool IsFallout3( ) const;
	bool IsOblivion( ) const;
	bool IsMorrowind( ) const;
	bool IsSkyrim( ) const;

	/* Progress Bar stuff */
	enum ProgressSection
	{
		Geometry,
		Animation,
		Collision,
		Skin,
		ProgressSectionCount
	};
	int progressCounters[ ProgressSectionCount ];
	int progressMax[ ProgressSectionCount ];
	void ProgressUpdate( ProgressSection section, const TCHAR* s );

};