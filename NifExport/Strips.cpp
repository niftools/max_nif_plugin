#include "pch.h"

#include "NvTriStrip/NvTriStrip.h"
using namespace NvTriStrip;
/*
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

	primitive_vector groups;
	stripper.Strip(&groups);

	// triangles left over
	Triangles stris;

	for (i=0; i<groups.size(); i++)
	{
		if (groups[i].Type == TRIANGLE_STRIP)
		{			
			strips.push_back(TriStrip(groups[i].Indices.size()));
			TriStrip &strip = strips.back();

			for (int j=0; j<groups[i].Indices.size(); j++)
				strip[j] = groups[i].Indices[j];
		} else
		{
			int size = stris.size();
			stris.resize(size + groups[i].Indices.size()/3);
			for (int j=(size>0)?(size-1):0; j<stris.size(); j++)
			{
				stris[j][0] = groups[i].Indices[j*3+0];
				stris[j][1] = groups[i].Indices[j*3+1];
				stris[j][2] = groups[i].Indices[j*3+2];
			}
		}
	}

	if (stris.size())
	{
		// stitch em
		TriStrip strip;
		if (strips.size() > 0)
		{
			strip.push_back(strips.back()[strips.back().size()-1]);
			strip.push_back(stris[0][0]);
		}
		for (i=0; i<stris.size(); i++)
		{
			if (i > 0)
			{
				strip.push_back(stris[i][0]);
				strip.push_back(stris[i][0]);
			}

			strip.push_back(stris[i][0]);
			strip.push_back(stris[i][1]);
			strip.push_back(stris[i][2]);
			if (i < stris.size()-1)
				strip.push_back(stris[i][2]);
		}
		strips.push_back(strip);
	}
}
*/
void Exporter::strippify(FaceGroup &grp)
{
   TriStrips &strips = grp.strips;
   strips.clear();
	unsigned short *data = (unsigned short *)malloc(grp.faces.size() * 3 * 2);

	unsigned int i;
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

				for (unsigned int s=0; s<groups[g].numIndices; s++)
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

				for (unsigned int s=0; s<rmGroups[g].numIndices; s++)
				{
					strip[s] = rmGroups[g].indices[s];

					unsigned short a = strip[s], b = groups[g].indices[s];

					grp.verts[a] = tmp.verts[b];
					grp.vnorms[a] = tmp.vnorms[b];
					if (grp.uvs.size() > 0)
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
	NiTriStripsDataRef stripData = new NiTriStripsData();

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
