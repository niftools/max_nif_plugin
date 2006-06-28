#include "pch.h"

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

void Exporter::nodeTransform(Matrix33 &rot, Vector3 &trans, INode *node, TimeValue t, bool local)
{
	Matrix3 tm = node->GetObjTMAfterWSM(t);
	if (local)
	{
		Matrix3 pm = node->GetParentTM(t);
		pm.Invert();
		tm *= pm;
	}
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

bool Exporter::equal(const Vector3 &a, const Point3 &b, float thresh)
{
	return (fabsf(a.x-b.x) <= thresh) &&
		   (fabsf(a.y-b.y) <= thresh) &&
		   (fabsf(a.z-b.z) <= thresh);
}

NiNodeRef Exporter::makeNode(NiNodeRef &parent, INode *maxNode, bool local)
{
	NiNodeRef node = DynamicCast<NiNode>(CreateBlock("NiNode"));
	
	Matrix33 rot;
	Vector3 trans;
	TimeValue t = 0;
	nodeTransform(rot, trans, maxNode, t, local);
	
	node->SetLocalRotation(rot);
	node->SetLocalTranslation(trans);
	string name = (char*)maxNode->GetName();
	node->SetName(name);

	parent->AddChild(DynamicCast<NiAVObject>(node));
	return node;
}
