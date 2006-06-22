#include "pch.h"

int Exporter::mVersion=001;
bool Exporter::mSelectedOnly=false;
bool Exporter::mTriStrips=true;
bool Exporter::mExportHidden=false;
bool Exporter::mExportFurn=true;
bool Exporter::mExportLights=false;
bool Exporter::mVertexColors=true;
float Exporter::mWeldThresh=0.1f;
string Exporter::mTexPrefix="textures";
bool Exporter::mExportCollision=true;
bool Exporter::mRemapIndices=true;

Exporter::Exporter(Interface *i)
	: mI(i)
{

}

Exporter::Result Exporter::export(NiNodeRef &root, INode *node)
{
	mNiRoot = root;
	return exportTree(root, node);
}

Exporter::Result Exporter::exportTree(NiNodeRef &parent, INode *node)
{
	bool coll = isCollision(node);
	if ((coll && !mExportCollision) ||
		(node->IsHidden() && !mExportHidden && !coll) ||
		(mSelectedOnly && !node->Selected()))
		return Skip;

	TimeValue t = 0;
	NiNodeRef prevParent = parent;
	if (!coll && node->IsGroupHead())
	{
		NiNodeRef n = DynamicCast<NiNode>(CreateBlock("NiNode"));
		Matrix33 rot;
		Vector3 trans;
		nodeTransform(rot, trans, node, t);
		n->SetLocalRotation(rot);
		n->SetLocalTranslation(trans);
		string name = (char*)node->GetName();
		n->SetName(name);

		parent->AddChild(DynamicCast<NiAVObject>(n));
		parent = n;
//		fprintf(pStream,"%s%s \"%s\" {\n", indent.data(), ID_GROUP, FixupName(node->GetName())); 
//		indentLevel++;
	}


	ObjectState os = node->EvalWorldState(t); 

	Result result;
	if (os.obj) 
	{
		// We look at the super class ID to determine the type of the object.
		switch(os.obj->SuperClassID()) 
		{
			case GEOMOBJECT_CLASS_ID: 
				if (!coll)
				{
					result = exportMesh(parent, node, t);
					if (result != Ok)
						return result;
				} else
				{
					if (!makeCollisionHierarchy(mNiRoot, node, t))
						return Error;
				}
				break;
/*
			case CAMERA_CLASS_ID:
				if (GetIncludeObjCamera()) ExportCameraObject(node, indentLevel); 
				break;
			case LIGHT_CLASS_ID:
				if (GetIncludeObjLight()) ExportLightObject(node, indentLevel); 
				break;
			case SHAPE_CLASS_ID:
				if (GetIncludeObjShape()) ExportShapeObject(node, indentLevel); 
				break;
			case HELPER_CLASS_ID:
				if (GetIncludeObjHelper()) ExportHelperObject(node, indentLevel); 
				break;
			}
*/
		}
	}

	for (int i=0; i<node->NumberOfChildren(); i++) 
	{
		Result result = exportTree(parent, node->GetChildNode(i));
		if (result!=Ok && result!=Skip)
			return result;
	}

	if (node->IsGroupHead())
		parent = prevParent;

	return Ok;
}
