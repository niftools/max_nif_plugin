#ifndef __EXPORTER_H__
#define __EXPORTER_H__

using namespace Niflib;

class BitmapTex;
class AppSettings;

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

	// reads config from root node
	static void				readConfig(INode *node);
	// reads config from registry
	static void				readConfig(Interface *i);

private:

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
	};

	// maps face groups to material ID
	typedef std::map<int, FaceGroup>	FaceGroups;	

	Interface				*mI;
	NiNodeRef				mNiRoot;
   AppSettings          *mAppSettings;

	Result					exportCollision(NiNodeRef &root, INode *node);
	Result					exportMeshes(NiNodeRef &root, INode *node);

	/* utility functions */
	Mtl						*getMaterial(INode *node, int subMtl);
	bool					TMNegParity(const Matrix3 &m);
	void					convertMatrix(Matrix33 &dst, const Matrix3 &src);
	void					nodeTransform(Matrix33 &rot, Vector3 &trans, INode *node, TimeValue t, bool local=true);
	void					nodeTransform(QuaternionXYZW &rot, Vector3 &trans, INode *node, TimeValue t, bool local=true);
	Point3					getVertexNormal(Mesh* mesh, int faceNo, RVertex* rv);
	bool					equal(const Vector3 &a, const Point3 &b, float thresh);
	BitmapTex				*getTexture(Mtl *mtl);
	void					getTextureMatrix(Matrix3 &mat, Mtl *mtl);
	NiNodeRef				makeNode(NiNodeRef &parent, INode *maxNode, bool local=true);
	// returns true if the node contains collision objects
	bool					isCollisionGroup(INode *maxNode, bool root=true);
	// returns true if the node contains meshes
	bool					isMeshGroup(INode *maxNode, bool root=true);

	/* tristrips */
	void					strippify(TriStrips &strips, vector<Vector3> &verts, vector<Vector3> &norms, const Triangles &tris);
	void					strippify(TriStrips &strips, FaceGroup &grp);
	NiTriStripsDataRef		makeTriStripsData(const TriStrips &strips);

	/* mesh export */
	// adds a vertex to a face group if it doesn't exist yet. returns new or previous index into the
	// vertex array.
	int 					addVertex(FaceGroup &grp, int face, int vi, Mesh *mesh, const Matrix3 &texm);
	// adds a face to a face group
	void					addFace(FaceGroup &grp, int face, const int vi[3], Mesh *mesh, const Matrix3 &texm);
	// creates face groups from faces with same sub material id
	bool					splitMesh(INode *node, Mesh *, FaceGroups &grps, TimeValue t);
	// creates a NiTriStrips or NiTriShape hierarchy from a face group
	bool					makeMesh(NiNodeRef &parent, Mtl *mtl, FaceGroup &grp);
	// splits mesh and converts it into nif blocks
	Result					exportMesh(NiNodeRef &parent, INode *node, TimeValue t);

	/* texture & material */
	// creates NiTexturingProperty + NiSourceTexture
	void					makeTexture(NiAVObjectRef &parent, Mtl *mtl);
	// creates a NiMaterialProperty
	void					makeMaterial(NiAVObjectRef &parent, Mtl *mtl);

	/* havok & collision */
	int						addVertex(vector<Vector3> &verts, vector<Vector3> &vnorms, const Point3 &pt, const Point3 &norm);
	void					addFace(Triangles &tris, vector<Vector3> &verts, vector<Vector3> &vnorms, 
								int face, const int vi[3], Mesh *mesh);
	bool					makeCollisionHierarchy(NiNodeRef &parent, INode *node, TimeValue t);
	
	/* creates a bhkRigidBody */
	bhkRigidBodyRef			makeCollisionBody(INode *node);
	/* creates a collision shape from a node */
	bhkSphereRepShapeRef	makeCollisionShape(INode *node);

	bhkSphereRepShapeRef	makeTriStripsShape(INode *node);
	bhkSphereRepShapeRef	makeBoxShape(Object *obj);
	bhkSphereRepShapeRef	makeSphereShape(Object *obj);
	bhkSphereRepShapeRef	makeCapsuleShape(Object *obj);
};

#endif 
