#include "pch.h"

void Exporter::strippify(TriStrips &strips, const Triangles &tris)
{
	unsigned short *data = (unsigned short *)malloc(tris.size() * 3 * 2);

	for (int i=0; i<tris.size(); i++)
	{
		data[i * 3 + 0] = tris[i][0];
		data[i * 3 + 1] = tris[i][1];
		data[i * 3 + 2] = tris[i][2];
	}
	
	PrimitiveGroup * groups = 0;
	unsigned short numGroups = 0;
	
	// GF 3, what about others?
	SetCacheSize(CACHESIZE_GEFORCE3);
	// don't generate hundreds of strips
	SetStitchStrips(true);
	GenerateStrips(data, tris.size()*3, &groups, &numGroups);

	free( data );
	
	if (!groups)
		return;
	
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

NiTriStripsDataRef Exporter::makeTriStripsData(const Triangles &tris)
{
	TriStrips strips;
	strippify(strips, tris);

	return makeTriStripsData(strips);
}
