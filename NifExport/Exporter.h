#ifndef __EXPORTER_H__
#define __EXPORTER_H__

namespace Niflib
{
   class NiTimeController;
   class NiControllerManager;
   class NiControllerSequence;

   class bhkConvexVerticesShape;
   class bhkNiTriStripsShape;

}
using namespace Niflib;

class BitmapTex;
class AppSettings;
class StdMat2;


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

	// Callback for post-processing instructions
	struct NiCallback
	{
		NiCallback() {};
		virtual ~NiCallback() {};
		virtual Result execute() = 0;
	};

	class CancelExporterException{};


	/* exporter version */
	static int				mVersion;

	/* export options, static for dialog usage. */
	static bool          mUseRegistry;
	static bool				mSelectedOnly;
	static bool				mTriStrips;
	static bool				mExportHidden;
	static bool				mExportFurn;
	static bool				mExportLights;
	static string			mTexPrefix;
	static bool				mVertexColors;
	static float			mWeldThresh;
	static bool				mExportCollision;
	static bool				mRemapIndices;
	static bool				mExportExtraNodes;
	static bool				mExportSkin;
	static bool				mUserPropBuffer;
	static bool          mFlattenHierarchy;
	static bool          mRemoveUnreferencedBones;
	static bool          mSortNodesToEnd;
	static string        mGameName;
	static string        mNifVersion;
	static int           mNifVersionInt;
	static int           mNifUserVersion;
	static bool				mSkeletonOnly;
	static bool				mExportCameras;
	static bool          mGenerateBoneCollision;
	static bool          mExportTransforms;
	static float         mDefaultPriority;
	static ExportType    mExportType;
	static bool          mMultiplePartitions;
	static int           mBonesPerVertex;
	static int           mBonesPerPartition;
	static bool          mUseTimeTags;
	static bool          mAutoDetect;
	static bool          mAllowAccum;
	static string        mCreatorName;
	static bool          mCollapseTransforms;
	static bool          mFixNormals;
	static bool          mTangentAndBinormalExtraData;
	static stringlist    mRotate90Degrees;
	static bool          mSupportPrnStrings;
	static bool          mSuppressPrompts;
	static bool          mUseAlternateStripper;
	static float			bhkScaleFactor;

	Exporter(Interface *i, AppSettings *appSettings);

	Result					doExport(NiNodeRef &root, INode *node);

	/* config is written to the registry and the root node
	of the scene, so that it is restored when opening it
	again. creating a new document will make the exporter
	use the settings stored in the registry. */

	// writes config to root node
	static void				writeConfig(INode *node);
	// writes config to registry
	static void				writeConfig(Interface *i);
	// writes config to registry
	static void				writeKfConfig(Interface *i);

	// reads config from root node
	static void				readConfig(INode *node);
	// reads config from registry
	static void				readConfig(Interface *i);
	// reads config from registry
	static void				readKfConfig(Interface *i);

	static AppSettings * exportAppSettings();
	static AppSettings * importAppSettings(string fname);

public:
	typedef vector<unsigned short> TriStrip;
	typedef	list<TriStrip> TriStrips;
	typedef vector<Triangle> Triangles;

	struct FaceGroup
	{
		vector<Vector3>		verts;
		vector<Vector3>		vnorms;
		Triangles			faces;
		vector<TexCoord>	uvs;
		vector<Color4>		vcolors;
		vector<int>       vidx;
		TriStrips         strips;
	};

	// maps face groups to material ID
	typedef std::map<int, FaceGroup>	FaceGroups;	
	typedef std::set<INode*> INodeMap;	
	typedef std::map<string, NiNodeRef>	NodeMap;	
	typedef std::map<INode*, NiNodeRef>	NodeToNodeMap;	
	typedef std::list<NiCallback*> CallbackList; 
	typedef std::list<Ref<NiNode> > NodeList;

	Interface				*mI;
	NiNodeRef				mNiRoot;
	AppSettings          *mAppSettings;
	NodeMap              mNameMap;
	NodeToNodeMap		 mNodeMap;
	INodeMap             mCollisionNodes;
	INodeMap             mHandledNodes;
	INode*               mSceneCollisionNode;
	CallbackList         mPostExportCallbacks;
	bool                 mIsBethesda;
	Box3                 mBoundingBox;
	NodeList             mAnimationRoots;

	Result					exportNodes(NiNodeRef &root, INode *node);
	Result					exportCollision(NiNodeRef &root, INode *node);
	Result               scanForCollision(INode *node);
	bool					isCollision(INode *node);
	bool					isHandled(INode *node);
	bool					markAsHandled(INode* node);

	/* utility functions */
	Mtl						*getMaterial(INode *node, int subMtl);
	bool					TMNegParity(const Matrix3 &m);
	void					convertMatrix(Matrix33 &dst, const Matrix3 &src);
	void					nodeTransform(Matrix33 &rot, Vector3 &trans, INode *node, TimeValue t, bool local=true);
	void					nodeTransform(QuaternionXYZW &rot, Vector3 &trans, INode *node, TimeValue t, bool local=true);
	Matrix3              getTransform(INode *node, TimeValue t, bool local=true);
	Point3					getVertexNormal(Mesh* mesh, int faceNo, RVertex* rv);
	bool					equal(const Vector3 &a, const Point3 &b, float thresh);
	BitmapTex				*getTexture(Mtl *mtl);
	BitmapTex				*getTexture(Mtl *mtl, int i);
	void					getTextureMatrix(Matrix3 &mat, Mtl *mtl);
	NiNodeRef				makeNode(NiNodeRef &parent, INode *maxNode, bool local=true);
	NiNodeRef				getNode(const string& name);
	NiNodeRef				getNode(INode* maxNode);
	// returns true if the node contains collision objects
	bool					isCollisionGroup(INode *maxNode, bool root=true);
	// returns true if the node contains meshes
	bool					isMeshGroup(INode *maxNode, bool root=true);
	void              CalcBoundingBox(INode *node, Box3& box, int all=1);
	void              CalcBoundingSphere(INode *node, Point3 center, float& radius, int all=1);

	/* tristrips */
	void					strippify(TriStrips &strips, vector<Vector3> &verts, vector<Vector3> &norms, const Triangles &tris);
	void					strippify(FaceGroup &grp);
	NiTriStripsDataRef		makeTriStripsData(const TriStrips &strips);

	/* mesh export */
	// adds a vertex to a face group if it doesn't exist yet. returns new or previous index into the
	// vertex array.
	int 					addVertex(FaceGroup &grp, int face, int vi, Mesh *mesh, const Matrix3 &texm, vector<Color4>& vertColors);
	// adds a face to a face group
	void					addFace(FaceGroup &grp, int face, const int vi[3], Mesh *mesh, const Matrix3 &texm, vector<Color4>& vertColors);
	// creates face groups from faces with same sub material id
	bool					splitMesh(INode *node, Mesh *, FaceGroups &grps, TimeValue t, vector<Color4>& vertColors);
	// creates a NiTriStrips or NiTriShape hierarchy from a face group
	NiTriBasedGeomRef makeMesh(NiNodeRef &parent, Mtl *mtl, FaceGroup &grp, bool exportStrips);
	// splits mesh and converts it into nif blocks
	Result					exportMesh(NiNodeRef &parent, INode *node, TimeValue t);

	/* texture & material */
	// creates NiTexturingProperty + NiSourceTexture
	void					makeTexture(NiAVObjectRef &parent, Mtl *mtl);
	bool              makeTextureDesc(BitmapTex *bmTex, Niflib::TexDesc &td);
	// creates a NiMaterialProperty
	void					makeMaterial(NiAVObjectRef &parent, Mtl *mtl);
	bool              exportCiv4Shader(NiAVObjectRef parent, Mtl* mtl);

	/* havok & collision */
	int						addVertex(vector<Vector3> &verts, vector<Vector3> &vnorms, const Point3 &pt, const Point3 &norm);
	void					addFace(Triangles &tris, vector<Vector3> &verts, vector<Vector3> &vnorms, 
		int face, const int vi[3], Mesh *mesh, Matrix3& tm);
	bool					makeCollisionHierarchy(NiNodeRef &parent, INode *node, TimeValue t);

	/* creates a bhkRigidBody */
	bhkRigidBodyRef			makeCollisionBody(INode *node);
	/* creates a collision shape from a node */
	bhkShapeRef				makeCollisionShape(INode *node, Matrix3& tm, bhkRigidBodyRef body);

	bhkShapeRef				makeTriStripsShape(INode *node, Matrix3& tm);
	bhkShapeRef				makeBoxShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makeSphereShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makeCapsuleShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makeListShape(INode *node, Matrix3& tm, bhkRigidBodyRef body);
	bhkShapeRef				makebhkBoxShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makebhkSphereShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makebhkCapsuleShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makeProxyShape(INode *node, Object *obj, Matrix3& tm);
	bhkShapeRef				makeConvexShape(INode *node, Object* obj, Matrix3& tm);
	bhkShapeRef				makeModifierShape(INode *node, Object* obj, Modifier* mod, Matrix3& tm);

	Ref<bhkConvexVerticesShape> makeConvexShape(Mesh& mesh, Matrix3& tm);
	Ref<bhkNiTriStripsShape>	makeTriStripsShape(Mesh& mesh, Matrix3& sm);

	/* skin export */
	bool makeSkin(NiTriBasedGeomRef shape, INode *node, FaceGroup &grp, TimeValue t);
	bool exportSkin();

	/* animation export */
	Result doAnimExport(Ref<NiControllerSequence> root);
	Result doAnimExport(Ref<NiControllerManager> ctrl, INode *node);
	bool isNodeTracked(INode *node);
	bool isNodeKeyed(INode *node);
	Ref<NiTimeController> CreateController(INode *node, Interval range);
	static void InitializeTimeController(Ref<NiTimeController> ctrl, NiNodeRef parent);

	/* misc export */
	bool exportUPB(NiNodeRef &root, INode *node);
	bool removeUnreferencedBones(NiNodeRef node);
	void sortNodes(NiNodeRef node);
	NiNodeRef exportBone(NiNodeRef parent, INode *node);
	Result exportLight(NiNodeRef root, INode *node, GenLight* light);
	void getChildNodes(INode *node, vector<NiNodeRef>&list);
	bool exportPrn(NiNodeRef &root, INode *node);
	NiNodeRef createAccumNode(NiNodeRef parent, INode *node);
	int countNodes(INode *node);
	bool isSkeletonRoot(INode *node);
	void ApplyAllSkinOffsets( NiAVObjectRef & root );

	/* Progress Bar stuff */
	enum ProgressSection
	{
		Geometry,
		Animation,
		Collision,
		Skin,
		ProgressSectionCount
	};
	int progressCounters[ProgressSectionCount];
	int progressMax[ProgressSectionCount];
	void ProgressUpdate( ProgressSection section, const TCHAR *s );
};

#endif 
