#include "pch.h"
#include <obj/NiLight.h>
#include <obj/NiAmbientLight.h>
#include <obj/NiPointLight.h>
#include <obj/NiDirectionalLight.h>
#include <obj/NiSpotLight.h>
#include <obj/NiTimeController.h>

#ifndef FOOTPRINT_CLASS_ID
#  define FOOTPRINT_CLASS_ID Class_ID(0x3011,0)        
#endif

bool Exporter::TMNegParity(const Matrix3 &m)
{
	return (DotProd(CrossProd(m.GetRow(0),m.GetRow(1)),m.GetRow(2))<0.0)?true:false;
}

Point3 Exporter::getVertexNormal(Mesh* mesh, int faceNo, RVertex* rv)
{
	Face* f = &mesh->faces[faceNo];
	DWORD smGroup = f->smGroup;
	int numNormals;
	Point3 vertexNormal;
	
	// Is normal specified
	// SPCIFIED is not currently used, but may be used in future versions.
	if (rv->rFlags & SPECIFIED_NORMAL)
	{
		vertexNormal = rv->rn.getNormal();
	} else 
	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	if ((numNormals = rv->rFlags & NORCT_MASK) && smGroup) 
	{
		// If there is only one vertex is found in the rn member.
		if (numNormals == 1) 
		{
			vertexNormal = rv->rn.getNormal();
		} else 
		{
			// If two or more vertices are there you need to step through them
			// and find the vertex with the same smoothing group as the current face.
			// You will find multiple normals in the ern member.
			for (int i = 0; i < numNormals; i++) 
			{
				if (rv->ern[i].getSmGroup() & smGroup) 
					vertexNormal = rv->ern[i].getNormal();
			}
		}

	} else 
		// Get the normal from the Face if no smoothing groups are there
		vertexNormal = mesh->getFaceNormal(faceNo);
	
	return vertexNormal;
}

void Exporter::convertMatrix(Matrix33 &dst, const Matrix3 &src)
{
	Point3 r0 = src.GetRow(0);
	Point3 r1 = src.GetRow(1);
	Point3 r2 = src.GetRow(2);

	dst.Set(r0.x, r0.y, r0.z,
		    r1.x, r1.y, r1.z,
			r2.x, r2.y, r2.z);
}

Matrix3 Exporter::getNodeTransform(INode *node, TimeValue t, bool local)
{
	Matrix3 tm = node->GetNodeTM(t);
	if (local)
	{
		INode *parent = node->GetParentNode();
		if (parent != NULL) {
			Matrix3 pm = parent->GetNodeTM(t);
			pm.Invert();
			tm *= pm;
		}
	}
	return tm;
}

Matrix3 Exporter::getObjectTransform(INode *node, TimeValue t, bool local)
{
   Matrix3 tm = node->GetObjTMAfterWSM(t);
   if (local)
   {
      INode *parent = node->GetParentNode();
      if (parent != NULL) {
         Matrix3 pm = parent->GetNodeTM(t);
         pm.Invert();
         tm *= pm;
      }
   }
   return tm;
}

void Exporter::nodeTransform(Matrix33 &rot, Vector3 &trans, INode *node, TimeValue t, bool local)
{
	Matrix3 tm = getNodeTransform(node, t, local);
	convertMatrix(rot, tm);
	trans.Set(tm.GetTrans().x, tm.GetTrans().y, tm.GetTrans().z);
}

void Exporter::nodeTransform(QuaternionXYZW &rot, Vector3 &trans, INode *node, TimeValue t, bool local)
{
	Matrix33 rm;
	nodeTransform(rm, trans, node, t, local);

	Quaternion q = rm.AsQuaternion();
	rot.x = q.x;
	rot.y = q.y;
	rot.z = q.z;
	rot.w = q.w;
}

void Exporter::objectTransform(Matrix33 &rot, Vector3 &trans, INode *node, TimeValue t, bool local)
{
   Matrix3 tm = getObjectTransform(node, t, local);
   convertMatrix(rot, tm);
   trans.Set(tm.GetTrans().x, tm.GetTrans().y, tm.GetTrans().z);
}

void Exporter::objectTransform(QuaternionXYZW &rot, Vector3 &trans, INode *node, TimeValue t, bool local)
{
   Matrix33 rm;
   objectTransform(rm, trans, node, t, local);

   Quaternion q = rm.AsQuaternion();
   rot.x = q.x;
   rot.y = q.y;
   rot.z = q.z;
   rot.w = q.w;
}


bool Exporter::equal(const Vector3 &a, const Point3 &b, float thresh)
{
	return (fabsf(a.x-b.x) <= thresh) &&
		   (fabsf(a.y-b.y) <= thresh) &&
		   (fabsf(a.z-b.z) <= thresh);
}

NiNodeRef Exporter::findNode(INode* maxNode)
{
	NodeToNodeMap::iterator itr = mNodeMap.find(maxNode);
	if (itr != mNodeMap.end())
		return (*itr).second;
	return NiNodeRef();
}

NiNodeRef Exporter::getNode(INode* maxNode)
{
	string name = maxNode->GetName();

	NodeToNodeMap::iterator itr = mNodeMap.find(maxNode);
	if (itr != mNodeMap.end())
		return (*itr).second;
	NiNodeRef node = getNode(name);
	mNodeMap[maxNode] = node;
	return node;
}
NiNodeRef Exporter::getNode(const string& name)
{
   NodeMap::iterator itr = mNameMap.find(name);
   if (itr != mNameMap.end())
      return (*itr).second;
   NiNodeRef node = CreateNiObject<NiNode>();
   if ( IsFallout3() || IsSkyrim() ) {
      node->SetFlags( 14 );
   }
   node->SetName(name);
   mNameMap[name] = node;
   return node;
}

NiNodeRef Exporter::makeNode(NiNodeRef &parent, INode *maxNode, bool local)
{
	string name = (char*)maxNode->GetName();
	NiNodeRef node = getNode(maxNode);

	Matrix33 rot;
	Vector3 trans;
	TimeValue t = 0;
	nodeTransform(rot, trans, maxNode, t, local);

	node->SetLocalRotation(rot);
	node->SetLocalTranslation(trans);

	exportUPB(node, maxNode);

	// Normal Embedded Animation 
	if (mExportType == NIF_WO_KF)
		CreateController(maxNode, Interval());

	parent->AddChild(DynamicCast<NiAVObject>(node));

	return node;
}

bool Exporter::isCollisionGroup(INode *maxNode, bool root)
{
	if (root)
	{
		if (!maxNode->IsGroupHead())
			return false;
	} 
	else
	{
      if (isCollision(maxNode))
			return true;
	}

	for (int i=0; i<maxNode->NumberOfChildren(); i++) 
	{
		if (isCollisionGroup(maxNode->GetChildNode(i), false))
			return true;
	}

	return false;
}

bool Exporter::isMeshGroup(INode *maxNode, bool root)
{
	if (root)
	{
		if (!maxNode->IsGroupHead())
			return false;
	} 
	else
	{
		if (!isCollision(maxNode))
		{
			TimeValue t = 0;
			ObjectState os = maxNode->EvalWorldState(t); 
			if (os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
				return true;
		}
	}

	for (int i=0; i<maxNode->NumberOfChildren(); i++) 
	{
		if (isMeshGroup(maxNode->GetChildNode(i), false))
			return true;
	}

	return false;
}

bool Exporter::exportUPB(NiNodeRef &root, INode *node)
{
   bool ok = false;
   if (!mUserPropBuffer)
      return ok;

   // Write the actual UPB sans any np_ prefixed strings
   TSTR upb;
   node->GetUserPropBuffer(upb);
   if (!upb.isNull())
   {
      string line;
      istringstream istr(string(upb), ios_base::out);
      ostringstream ostr;
      while (!istr.eof()) {
         std::getline(istr, line);
         if (!line.empty() && 0 != line.compare(0, 3, "np_"))
            ostr << line << endl;
      }
      if (!ostr.str().empty())
      {
         NiStringExtraDataRef strings = CreateNiObject<NiStringExtraData>();	
         strings->SetName("UPB");
         strings->SetData(ostr.str());
         root->AddExtraData(DynamicCast<NiExtraData>(strings));
         ok = true;
      }
   }
   return ok;
}


bool Exporter::removeUnreferencedBones(NiNodeRef node)
{
   NiNodeRef parent = node->GetParent();
   bool remove = (NULL != parent) && !node->IsSkinInfluence();
   Matrix44 ntm = node->GetLocalTransform();
   vector<NiAVObjectRef> children = node->GetChildren();
   for (vector<NiAVObjectRef>::iterator itr = children.begin(); itr != children.end(); ++itr)
   {
      NiAVObjectRef& child = (*itr);
      bool childRemove = false;
      if (child->IsDerivedType(NiNode::TYPE))
      {
         childRemove = removeUnreferencedBones(StaticCast<NiNode>(child));
      }
      if (childRemove)
      {
         node->RemoveChild(child);
      }
      else if (remove) // Reparent abandoned nodes to root
      {
         Matrix44 tm = child->GetLocalTransform();
         child->SetLocalTransform( ntm * tm );
         node->RemoveChild(child);
         mNiRoot->AddChild(child);
      }
   }
   return remove;
}

struct SortNodeEquivalence
{
   inline bool operator()(const NiAVObjectRef& lhs, const NiAVObjectRef& rhs) const
   {
      if (!lhs) return !rhs;
      if (!rhs) return true;
      string ltype = lhs->GetType().GetTypeName();
      string rtype = rhs->GetType().GetTypeName();
      if (ltype == rtype)
         return false;
      if (ltype == "bhkCollisionObject")
         return true;
      if (rtype == "bhkCollisionObject")
         return false;
      if (ltype == "NiNode")
         return false;
      else if (rtype == "NiNode")
         return true;
      return (ltype < rtype); 
   }
};

void Exporter::sortNodes(NiNodeRef node)
{
   node->SortChildren(SortNodeEquivalence());

   vector<NiNodeRef> children = DynamicCast<NiNode>(node->GetChildren());
   for (vector<NiNodeRef>::iterator itr = children.begin(); itr != children.end(); ++itr)
      sortNodes(*itr);
}


Exporter::Result Exporter::exportLight(NiNodeRef parent, INode *node, GenLight* light)
{
   TimeValue t = 0;
   NiLightRef niLight;
   switch (light->Type())
   {
   case OMNI_LIGHT:
      {
         if (light->GetAmbientOnly())
         {
            niLight = new NiAmbientLight();
         }
         else
         {
            NiPointLightRef pointLight = new NiPointLight();
            float atten = light->GetAtten(t, ATTEN_START);
            switch (light->GetDecayType())
            {
            case 0: pointLight->SetConstantAttenuation(1.0f); break;
            case 1: pointLight->SetLinearAttenuation( atten / 4.0f ); break;
            case 2: pointLight->SetQuadraticAttenuation( sqrt(atten / 4.0f) ); break;
            }
            niLight = StaticCast<NiLight>(pointLight);
         }
     }
      break;
   case TSPOT_LIGHT:
   case FSPOT_LIGHT:
      niLight = new NiSpotLight();
      break;
   case DIR_LIGHT:
   case TDIR_LIGHT:
      niLight = new NiDirectionalLight();
      break;
   }
   if (niLight == NULL)
      return Skip;

   niLight->SetName(node->GetName());

   Matrix3 tm = getObjectTransform(node, t, !mFlattenHierarchy);
   niLight->SetLocalTransform( TOMATRIX4(tm, false) );

   niLight->SetDimmer( light->GetIntensity(0) );
   Color3 rgbcolor = TOCOLOR3( light->GetRGBColor(0) );
   if (light->GetAmbientOnly())
   {
      niLight->SetDiffuseColor(Color3(0,0,0));
      niLight->SetSpecularColor(Color3(0,0,0));
      niLight->SetAmbientColor(rgbcolor);
   }
   else
   {
      niLight->SetDiffuseColor(rgbcolor);
      niLight->SetSpecularColor(rgbcolor);
      niLight->SetAmbientColor(Color3(0,0,0));
   }
   parent->AddChild( DynamicCast<NiAVObject>(niLight) );
   return Ok;
}

void Exporter::CalcBoundingBox(INode *node, Box3& box, int all)
{
   if (NULL == node) 
      return;

   Matrix3 tm = node->GetObjTMAfterWSM(0);
   if (node->IsBoneShowing()) {
      box.IncludePoints(const_cast<Point3*>(&tm.GetTrans()), 1, NULL);
   } else {
      if (Object *o = node->GetObjectRef()) {
         if (o->SuperClassID()==GEOMOBJECT_CLASS_ID) {
            if (  o->ClassID() == BONE_OBJ_CLASSID 
               || o->ClassID() == Class_ID(BONE_CLASS_ID,0)
               || o->ClassID() == Class_ID(0x00009125,0) /* Biped Twist Helpers */
               )
            {
               box.IncludePoints(const_cast<Point3*>(&tm.GetTrans()), 1, NULL);
            }
            else
            {
               Box3 local;
               o->GetLocalBoundBox(0, node, mI->GetActiveViewport(), local);
               box.IncludePoints(&local.Min(), 1, NULL);
               box.IncludePoints(&local.Max(), 1, NULL);
            }
         }
         else if (mExportCameras && o->SuperClassID()==CAMERA_CLASS_ID)
         {
            box.IncludePoints(const_cast<Point3*>(&tm.GetTrans()), 1, NULL);
         }
      }
   }
   if (all < 0)
      return;

   all = (all>0 ? all : -1);
   for (int i=0; i<node->NumberOfChildren(); i++) {
      CalcBoundingBox(node->GetChildNode(i), box, all );
   }
}

void Exporter::CalcBoundingSphere(INode *node, Point3 center, float& radius, int all)
{
   if (NULL == node) 
      return;

   Matrix3 tm = node->GetObjTMAfterWSM(0);
   Point3 pt = (tm.GetTrans() - center);
   float len = pt.Length();

   if (node->IsBoneShowing()) {
      radius = max(len, radius);
   } else {
      if (Object *o = node->GetObjectRef()) {
         if (o->SuperClassID()==GEOMOBJECT_CLASS_ID) {
            if (  o->ClassID() == BONE_OBJ_CLASSID 
               || o->ClassID() == Class_ID(BONE_CLASS_ID,0)
               || o->ClassID() == Class_ID(0x00009125,0) /* Biped Twist Helpers */
               )
            {
               radius = max(len, radius);
            }
            else
            {
               radius = max(len, radius);
            }
         }
         else if (mExportCameras && o->SuperClassID()==CAMERA_CLASS_ID)
         {
            radius = max(len, radius);
         }
      }
   }
   if (all < 0)
      return;

   all = (all>0 ? all : -1);
   for (int i=0; i<node->NumberOfChildren(); i++) {
      CalcBoundingSphere(node->GetChildNode(i), center, radius, all );
   }
}



void Exporter::getChildNodes(INode *node, vector<NiNodeRef>& list)
{
   for (int i=0; i<node->NumberOfChildren(); i++) 
   {
      INode * child = node->GetChildNode(i);
      ObjectState os = node->EvalWorldState(0); 
      bool addBone = false;
      bool local = !mFlattenHierarchy;
	  bool meshGroup = isMeshGroup(node);

	  TSTR nodeName = node->GetName();

	  if (wildmatch("Bip?? Footsteps", nodeName))
	  {
		  addBone = false;
	  }
      else if (node->IsBoneShowing()) 
      {
         addBone = true;
      }
      else if (os.obj && os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID)
      {
         Class_ID clsid = os.obj->ClassID();
         if (  os.obj 
            && (  clsid == BONE_OBJ_CLASSID 
            || clsid == Class_ID(BONE_CLASS_ID,0)
            || clsid == Class_ID(0x00009125,0) /* Biped Twist Helpers */
            )
            ) 
         {
			 // skip mesh groups in skeleton only situations
			 if (!meshGroup || !local || !mSkeletonOnly)
				addBone = true;
         } 
         else if (!mSkeletonOnly)
         {
            if (mExportType != NIF_WO_ANIM && isNodeTracked(node)) {
               addBone = true;
            } else if ( mExportExtraNodes || (mExportType != NIF_WO_ANIM && isNodeKeyed(node) ) ) {
               addBone = true;
            }
         }
         else if (mExportCameras && os.obj && os.obj->SuperClassID()==CAMERA_CLASS_ID)
         {
            addBone = true;
         }
         else if (meshGroup && local && !mSkeletonOnly) // only create node if local
         {
            addBone = true;
         } 
      }
      if (addBone)
      {
         list.push_back( getNode(child) );
      }
      getChildNodes(child, list);
   }
}

// Special case of a single branch being exported

bool Exporter::exportPrn(NiNodeRef &obj, INode *node) {
   // Export Prn Text strings for any parent bones if parent is root
	if (mSupportPrnStrings && Exporter::mNifVersionInt >= VER_10_0_1_0) {
      if (INode *parentNode = node->GetParentNode()){
         string parentName = parentNode->GetName();
         NiStringExtraDataRef strings = new NiStringExtraData();	
         strings->SetName("Prn");
         strings->SetData(parentName);
         obj->AddExtraData(DynamicCast<NiExtraData>(strings));
         return true;
      }
   }
   return false;
}


int Exporter::countNodes(INode *node)
{
   int counter = 1;
   for (int i=0; i<node->NumberOfChildren(); i++) {
      counter += countNodes(node->GetChildNode(i));
   }
   return counter;
}

bool Exporter::isSkeletonRoot(INode *node)
{
   if (wildmatch("Bip??", node->GetName())) {
      return true;
   } else if ( node->GetParentNode() == mI->GetRootNode() ) {
      return true;
   }

   return false;
}

void Exporter::ApplyAllSkinOffsets( NiAVObjectRef & root ) {
   NiGeometryRef niGeom = DynamicCast<NiGeometry>(root);
   if ( niGeom != NULL && niGeom->IsSkin() == true ) {
      niGeom->ApplySkinOffset();
   }

   NiNodeRef niNode = DynamicCast<NiNode>(root);
   if ( niNode != NULL ) {
      //Call this function on all children
      vector<NiAVObjectRef> children = niNode->GetChildren();

      for ( unsigned i = 0; i < children.size(); ++i ) {
         ApplyAllSkinOffsets( children[i] );
      }
   }
} 

struct SortVectorEquivalence
{
	inline bool operator()(const Vector3& lhs, const Vector3& rhs) const
	{
		float diff = lhs.x - rhs.x;
		if (diff < 0)
			return true;
		if (diff == 0)
		{
			diff = lhs.y - rhs.y;
			if (diff < 0)
				return true;
			if (diff == 0)
			{
				diff = lhs.z - rhs.z;
				if (diff < 0)
					return true;
			}
		}
		return false;
	}
	inline bool operator()(const Float4& lhs, const Float4& rhs) const
	{
		float diff = lhs[0] - rhs[0];
		if (diff < 0)
			return true;
		if (diff == 0)
		{
			diff = lhs[1] - rhs[1];
			if (diff < 0)
				return true;
			if (diff == 0)
			{
				diff = lhs[2] - rhs[2];
				if (diff < 0)
					return true;
				if (diff == 0)
				{
					diff = lhs[3] - rhs[3];
					if (diff < 0)
						return true;
				}
			}
		}
		return false;
	}
};

void Exporter::sortVector3(vector<Vector3>& vector)
{
	std::stable_sort(vector.begin(), vector.end(), SortVectorEquivalence());
}

void Exporter::sortVector4(vector<Vector4>& vector)
{
	std::stable_sort(vector.begin(), vector.end(), SortVectorEquivalence());
}

Exporter::Result Exporter::scanForIgnore(INode *node)
{   
	if (NULL == node) 
		return Exporter::Skip;

	BOOL ignore = FALSE;
	if (node->GetUserPropBool("np_ignore", ignore))
	{
		markAsHandled(node);
	}
	for (int i=0; i<node->NumberOfChildren(); i++) {
		scanForIgnore(node->GetChildNode(i));
	}
	return Exporter::Ok;
}


