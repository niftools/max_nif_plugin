#include "pch.h"

void Exporter::strippify(TriStrips &strips, vector<Vector3> &verts, vector<Vector3> &norms, const Triangles &tris)
{
	unsigned short *data = (unsigned short *)malloc(tris.size() * 3 * 2);

	int i;
	for (i=0; i<tris.size(); i++)
	{
		data[i * 3 + 0] = tris[i][0];
		data[i * 3 + 1] = tris[i][1];
		data[i * 3 + 2] = tris[i][2];
	}
	
	PrimitiveGroup * groups = 0;
	unsigned short numGroups = 0;
	
	// GF 3+
	SetCacheSize(CACHESIZE_GEFORCE3);
	// don't generate hundreds of strips
	SetStitchStrips(true);
	GenerateStrips(data, tris.size()*3, &groups, &numGroups);

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
		RemapIndices(groups, numGroups, verts.size(), &rmGroups);

		vector<Vector3> tverts = verts;
		vector<Vector3> tnorms = norms;

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
					verts[a] = tverts[b];
					norms[a] = tnorms[b];
				}
			}
		}

		delete [] rmGroups;
	}

	delete [] groups;
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
