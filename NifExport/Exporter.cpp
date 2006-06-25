#include "pch.h"

int Exporter::mVersion=013;
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
	BSXFlagsRef bsx = DynamicCast<BSXFlags>(CreateBlock("BSXFlags"));
	bsx->SetName("BSX");
	bsx->SetFlags(0x00000002);

	NiStringExtraDataRef strings = DynamicCast<NiStringExtraData>(CreateBlock("NiStringExtraData"));	
	strings->SetName("UPB");
	strings->SetData("Ellasticity = 0.300000\r\nFriction = 0.300000\r\nUnyielding = 0\r\nProxy_Geometry = <None>\r\nUse_Display_Proxy = 0\r\nDisplay_Children = 1\r\nDisable_Collisions = 0\r\nInactive = 0\r\nDisplay_Proxy = <None>\r\nMass = 0.000000\r\nSimulation_Geometry = 2\r\nCollision_Groups = 589825\r\n");

	root->AddExtraData(DynamicCast<NiExtraData>(bsx));
	root->AddExtraData(DynamicCast<NiExtraData>(strings));

	mNiRoot = root;
	return exportTree(root, node);
}

Exporter::Result Exporter::exportTree(NiNodeRef &parent, INode *node)
{
	bool coll = npIsCollision(node);
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
