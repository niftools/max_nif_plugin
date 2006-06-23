#include "pch.h"

using namespace triangle_stripper;

void Exporter::strippify(TriStrips &strips, vector<Vector3> &verts, vector<Vector3> &norms, const Triangles &tris)
{
	vector<unsigned int> idcs(tris.size()*3);
	int i;
	for (i=0; i<tris.size(); i++)
	{
		idcs[i * 3 + 0] = tris[i][0];
		idcs[i * 3 + 1] = tris[i][1];
		idcs[i * 3 + 2] = tris[i][2];
	}
	
	tri_stripper stripper(idcs);

	tri_stripper::primitives_vector groups;
	stripper.Strip(&groups);

	for (i=0; i<groups.size(); i++)
	{
		if (groups[i].m_Type != tri_stripper::PT_Triangle_Strip)
			continue;
		
		strips.push_back(TriStrip(groups[i].m_Indices.size()));
		TriStrip &strip = strips.back();

		for (int j=0; j<groups[i].m_Indices.size(); j++)
			strip[j] = groups[i].m_Indices[j];
	}
}

void Exporter::strippify(TriStrips &strips, FaceGroup &grp)
{
	unsigned short *data = (unsigned short *)malloc(grp.faces.size() * 3 * 2);

	int i;
	for (i=0; i<grp.faces.size(); i++)
	{
		data[i * 3 + 0] = grp.faces[i][0];
		data[i * 3 + 1] = grp.faces[i][1];
		data[i * 3 + 2] = grp.faces[i][2];
	}
	
	PrimitiveGroup * groups = 0;
	unsigned short numGroups = 0;
	
	// GF 3+
	SetCacheSize(CACHESIZE_GEFORCE3);
	// don't generate hundreds of strips
	SetStitchStrips(true);
	GenerateStrips(data, grp.faces.size()*3, &groups, &numGroups);

	free( data );
	
	if (!groups)
		return;

	if (!mRemapIndices)
	{
		for (int g=0; g<numGroups; g++)
		{
			if (groups[g].type == PT_STRIP)
			{
				strips.push_back(TriStrip(groups[g].numIndices));
				TriStrip &strip = strips.back();

				for (int s=0; s<groups[g].numIndices; s++)
					strip[s] = groups[g].indices[s];
			}
		}
	} else
	{
		// remap indices
		PrimitiveGroup *rmGroups;
		RemapIndices(groups, numGroups, grp.verts.size(), &rmGroups);

		FaceGroup tmp = grp;

		for (int g=0; g<numGroups; g++)
		{
			if (rmGroups[g].type == PT_STRIP)
			{
				strips.push_back(TriStrip(rmGroups[g].numIndices));
				TriStrip &strip = strips.back();

				for (int s=0; s<rmGroups[g].numIndices; s++)
				{
					strip[s] = rmGroups[g].indices[s];

					unsigned short a = strip[s], b = groups[g].indices[s];

					grp.verts[a] = tmp.verts[b];
					grp.vnorms[a] = tmp.vnorms[b];
					grp.uvs[a] = tmp.uvs[b];
					if (grp.vcolors.size() > 0)
						grp.vcolors[a] = tmp.vcolors[b];
				}
			}
		}
		delete [] rmGroups;
	}
	delete [] groups;
}

NiTriStripsDataRef Exporter::makeTriStripsData(const TriStrips &strips)
{
	NiTriStripsDataRef stripData = DynamicCast<NiTriStripsData>(CreateBlock("NiTriStripsData"));

	if (strips.size() > 0)
	{
		stripData->SetStripCount(strips.size());

		int i = 0;
		TriStrips::const_iterator it;
		for (it=strips.begin(); it!=strips.end(); ++it)
			stripData->SetStrip(i++, *it);
	}

	return stripData;
}
