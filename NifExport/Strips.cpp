
#include "stdafx.h"
#include "NvTriStrip\NvTriStrip.h"

void Exporter::strippify( FaceGroup& grp )
{
	TriStrips& strips = grp.strips;
	strips.clear( );
	unsigned short* data = (unsigned short*) malloc( grp.faces.size( ) * 3 * 2 );

	unsigned int i;
	for( i = 0; i < grp.faces.size( ); i++ )
	{
		data[ i * 3 + 0 ] = grp.faces[ i ][ 0 ];
		data[ i * 3 + 1 ] = grp.faces[ i ][ 1 ];
		data[ i * 3 + 2 ] = grp.faces[ i ][ 2 ];
	}

	NvTriStrip::PrimitiveGroup* groups = 0;
	unsigned short numGroups = 0;

	// GF 3+
	NvTriStrip::SetCacheSize( CACHESIZE_GEFORCE3 );

	// don't generate hunders of strips
	NvTriStrip::SetStitchStrips( true );
	NvTriStrip::GenerateStrips( data, grp.faces.size( ) * 3, &groups, &numGroups );

	free( data );

	if( !groups )
		return;

	if( !mRemapIndices )
	{
		for( int g = 0; g < numGroups; g++ )
		{
			strips.push_back( TriStrip( groups[ g ].numIndices ) );
			TriStrip& strip = strips.back( );

			for( unsigned int s = 0; s < groups[ g ].numIndices; s++ )
				strip[ s ] = groups[ g ].indices[ s ];
		}
	}
	else
	{
		// remap indices
		NvTriStrip::PrimitiveGroup* rmGroups;
		NvTriStrip::RemapIndices( groups, numGroups, grp.verts.size( ), &rmGroups );

		FaceGroup tmp = grp;

		for( int g = 0; g < numGroups; g++ )
		{
			if( rmGroups[ g ].type == NvTriStrip::PT_STRIP )
			{
				strips.push_back( TriStrip( rmGroups[ g ].numIndices ) );
				TriStrip& strip = strips.back( );

				for( unsigned int s = 0; s < rmGroups[ g ].numIndices; s++ )
				{
					strip[ s ] = rmGroups[ g ].indices[ s ];
					unsigned short a = strip[ s ];
					unsigned short b = groups[ g ].indices[ s ];

					grp.verts[ a ] = tmp.verts[ b ];
					grp.vnorms[ a ] = tmp.vnorms[ b ];

					if( grp.uvs.size( ) > 0 )
						grp.uvs[ a ] = tmp.uvs[ b ];

					if( grp.vcolors.size( ) > 0 )
						grp.vcolors[ a ] = tmp.vcolors[ b ];
				}
			}
		}
		
		delete[] rmGroups;
	}

	delete[] groups;
}

Niflib::NiTriStripsDataRef Exporter::makeTriStripsData( const TriStrips& strips )
{
	Niflib::NiTriStripsDataRef stripData = new Niflib::NiTriStripsData( );

	if( strips.size( ) > 0 )
	{
		stripData->SetStripCount( strips.size( ) );

		int i = 0;
		TriStrips::const_iterator it;

		for( it = strips.begin( ); it != strips.end( ); ++it )
			stripData->SetStrip( i++, *it );
	}

	return stripData;
}