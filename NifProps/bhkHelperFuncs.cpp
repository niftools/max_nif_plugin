#pragma warning( disable:4800 )

#include <map>
#include "NifProps.h"
#include "NifStrings.h"
#include "NifPlugins.h"
#include "NifGui.h"
#include "meshadj.h"

using namespace std;

#define MAKE_QUAD(na,nb,nc,nd,sm,b) {MakeQuad(nverts,&(mesh.faces[nf]),na, nb, nc, nd, sm, b);nf+=2;}

// Misc stuff
#define MAX_SEGMENTS	200
#define MIN_SEGMENTS	4

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float(1.0E30)

#define MIN_SMOOTH		0
#define MAX_SMOOTH		1

enum 
{
	POSX = 0,	// right
	POSY = 1,	// back
	POSZ = 2,	// top
	NEGX = 3,	// left
	NEGY = 4,	// front
	NEGZ = 5,	// bottom
};

int direction(Point3 *v) {
	Point3 a = v[0]-v[2];
	Point3 b = v[1]-v[0];
	Point3 n = CrossProd(a,b);
	switch(MaxComponent(n)) {
	  case 0: return (n.x<0)?NEGX:POSX;
	  case 1: return (n.y<0)?NEGY:POSY;
	  case 2: return (n.z<0)?NEGZ:POSZ;
	}
	return 0;
}

// Remap the sub-object material numbers so that the top face is the first one
// The order now is:
// Top / Bottom /  Left/ Right / Front / Back
static int mapDir[6] ={ 3, 5, 0, 2, 4, 1 };

// vertices ( a b c d ) are in counter clockwise order when viewd from 
// outside the surface unless bias!=0 in which case they are clockwise
static void MakeQuad(int nverts, Face *f, int a, int b , int c , int d, int sg, int bias) {
	int sm = 1<<sg;
	assert(a<nverts);
	assert(b<nverts);
	assert(c<nverts);
	assert(d<nverts);
	if (bias) {
		f[0].setVerts( b, a, c);
		f[0].setSmGroup(sm);
		f[0].setEdgeVisFlags(1,0,1);
		f[1].setVerts( d, c, a);
		f[1].setSmGroup(sm);
		f[1].setEdgeVisFlags(1,0,1);
	} else {
		f[0].setVerts( a, b, c);
		f[0].setSmGroup(sm);
		f[0].setEdgeVisFlags(1,1,0);
		f[1].setVerts( c, d, a);
		f[1].setSmGroup(sm);
		f[1].setEdgeVisFlags(1,1,0);
	}
}

void CalcAxisAlignedBox(Mesh& mesh, Box3& box)
{
	int nv = mesh.getNumVerts();
	box.IncludePoints(mesh.getVertPtr(0), nv, NULL);
}

void CalcAxisAlignedBox(Mesh& mesh, Box3& box, Matrix3 *tm)
{
	int nv = mesh.getNumVerts();
	box.IncludePoints(mesh.getVertPtr(0), nv, tm);
}

// Calculate bounding sphere using minimum-volume axis-align bounding box.  Its fast but not a very good fit.
void CalcAxisAlignedSphere(Mesh& mesh, Point3& center, float& radius)
{
	//--Calculate center & radius--//

	//Set lows and highs to first vertex
	size_t nv = mesh.getNumVerts();

	Point3 lows = mesh.getVert(0);
	Point3 highs = mesh.getVert(0);

	//Iterate through the vertices, adjusting the stored values
	//if a vertex with lower or higher values is found
	for ( size_t i = 0; i < nv; ++i ) {
		const Point3 & v = mesh.getVert(i);

		if ( v.x > highs.x ) highs.x = v.x;
		else if ( v.x < lows.x ) lows.x = v.x;

		if ( v.y > highs.y ) highs.y = v.y;
		else if ( v.y < lows.y ) lows.y = v.y;

		if ( v.z > highs.z ) highs.z = v.z;
		else if ( v.z < lows.z ) lows.z = v.z;
	}

	//Now we know the extent of the shape, so the center will be the average
	//of the lows and highs
	center = (highs + lows) / 2.0f;

	//The radius will be the largest distance from the center
	Point3 diff;
	float dist2(0.0f), maxdist2(0.0f);
	for ( size_t i = 0; i < nv; ++i ) {
		const Point3 & v = mesh.getVert(i);

		diff = center - v;
		dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		if ( dist2 > maxdist2 ) maxdist2 = dist2;
	};
	radius = sqrt(maxdist2);
}

// Calculate bounding sphere using average position of the points.  Better fit but slower.
void CalcCenteredSphere(Mesh& mesh, Point3& center, float& radius)
{
	int nv = mesh.getNumVerts();
	Point3 sum(0.0f, 0.0f, 0.0f);
	for (int i=0; i<nv; ++i)
		sum += mesh.getVert(i);
	center = sum / float(nv);
	float radsq = 0.0f;
	for (int i=0; i<nv; ++i){
		Point3 diff = mesh.getVert(i) - center;
		float mag = diff.LengthSquared();
		radsq = max(radsq, mag);
	}
	radius = Sqrt(radsq);
}



#define MAKE_QUAD(na,nb,nc,nd,sm,b) {MakeQuad(nverts,&(mesh.faces[nf]),na, nb, nc, nd, sm, b);nf+=2;}

void BuildBox(Mesh&mesh, float l, float w, float h)
{
	int ix,iy,iz,nf,kv,mv,nlayer,topStart,midStart;
	int nverts,nv,nextk,nextm,wsp1;
	int nfaces;
	Point3 va,vb,p;
	BOOL bias = 0;
	const int lsegs = 1, wsegs = 1, hsegs = 1;

	// Start the validity interval at forever and whittle it down.
	if (h<0.0f) bias = 1;

	// Number of verts
	// bottom : (lsegs+1)*(wsegs+1)
	// top	: (lsegs+1)*(wsegs+1)
	// sides	: (2*lsegs+2*wsegs)*(hsegs-1)

	// Number of rectangular faces.
	// bottom : (lsegs)*(wsegs)
	// top	: (lsegs)*(wsegs)
	// sides	: 2*(hsegs*lsegs)+2*(wsegs*lsegs)

	wsp1 = wsegs + 1;
	nlayer	=  2*(lsegs+wsegs);
	topStart = (lsegs+1)*(wsegs+1);
	midStart = 2*topStart;

	nverts = midStart+nlayer*(hsegs-1);
	nfaces = 4*(lsegs*wsegs + hsegs*lsegs + wsegs*hsegs);

	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.InvalidateTopologyCache();

	nv = 0;

	vb =  Point3(w,l,h)/float(2);	
	va = -vb;

	float dx = w/wsegs;
	float dy = l/lsegs;
	float dz = h/hsegs;

	// do bottom vertices.
	p.z = va.z;
	p.y = va.y;
	for(iy=0; iy<=lsegs; iy++) {
		p.x = va.x;
		for (ix=0; ix<=wsegs; ix++) {
			mesh.setVert(nv++, p);
			p.x += dx;
		}
		p.y += dy;
	}

	nf = 0;

	// do bottom faces.
	for(iy=0; iy<lsegs; iy++) {
		kv = iy*(wsegs+1);
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv+wsegs+1, kv+wsegs+2, kv+1, 1, bias);
			kv++;
		}
	}
	assert(nf==lsegs*wsegs*2);

	// do top vertices.
	p.z = vb.z;
	p.y = va.y;
	for(iy=0; iy<=lsegs; iy++) {
		p.x = va.x;
		for (ix=0; ix<=wsegs; ix++) {
			mesh.setVert(nv++, p);
			p.x += dx;
		}
		p.y += dy;
	}

	// do top faces (lsegs*wsegs);
	for(iy=0; iy<lsegs; iy++) {
		kv = iy*(wsegs+1)+topStart;
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv+1, kv+wsegs+2,kv+wsegs+1, 2, bias);
			kv++;
		}
	}
	assert(nf==lsegs*wsegs*4);

	// do middle vertices 
	for(iz=1; iz<hsegs; iz++) {

		p.z = va.z + dz * iz;

		// front edge
		p.x = va.x;	 p.y = va.y;
		for (ix=0; ix<wsegs; ix++) { mesh.setVert(nv++, p);	 p.x += dx;	}

		// right edge
		p.x = vb.x;	  p.y = va.y;
		for (iy=0; iy<lsegs; iy++) { mesh.setVert(nv++, p);	 p.y += dy;	}

		// back edge
		p.x =  vb.x;  p.y =	 vb.y;
		for (ix=0; ix<wsegs; ix++) { mesh.setVert(nv++, p);	 p.x -= dx;	}

		// left edge
		p.x = va.x;	 p.y =	vb.y;
		for (iy=0; iy<lsegs; iy++) { mesh.setVert(nv++, p);	 p.y -= dy;	}
	}

	if (hsegs==1) {
		// do FRONT faces -----------------------
		kv = 0;
		mv = topStart;
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv+1, mv+1, mv, 3, bias);
			kv++;
			mv++;
		}

		// do RIGHT faces.-----------------------
		kv = wsegs;	 
		mv = topStart + kv;
		for (iy=0; iy<lsegs; iy++) {
			MAKE_QUAD(kv, kv+wsp1, mv+wsp1, mv, 4, bias);
			kv += wsp1;
			mv += wsp1;
		}	

		// do BACK faces.-----------------------
		kv = topStart - 1;
		mv = midStart - 1;
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv-1, mv-1, mv, 5, bias);
			kv --;
			mv --;
		}

		// do LEFT faces.----------------------
		kv = lsegs*(wsegs+1);  // index into bottom
		mv = topStart + kv;
		for (iy=0; iy<lsegs; iy++) {
			MAKE_QUAD(kv, kv-wsp1, mv-wsp1, mv, 6, bias);
			kv -= wsp1;
			mv -= wsp1;
		}
	}

	else {
		// do front faces.
		kv = 0;
		mv = midStart;
		for(iz=0; iz<hsegs; iz++) {
			if (iz==hsegs-1) mv = topStart;
			for (ix=0; ix<wsegs; ix++) 
				MAKE_QUAD(kv+ix, kv+ix+1, mv+ix+1, mv+ix, 3, bias);
			kv = mv;
			mv += nlayer;
		}

		assert(nf==lsegs*wsegs*4 + wsegs*hsegs*2);

		// do RIGHT faces.-------------------------
		// RIGHT bottom row:
		kv = wsegs; // into bottom layer. 
		mv = midStart + wsegs; // first layer of mid verts


		for (iy=0; iy<lsegs; iy++) {
			MAKE_QUAD(kv, kv+wsp1, mv+1, mv, 4, bias);
			kv += wsp1;
			mv ++;
		}

		// RIGHT middle part:
		kv = midStart + wsegs; 
		for(iz=0; iz<hsegs-2; iz++) {
			mv = kv + nlayer;
			for (iy=0; iy<lsegs; iy++) {
				MAKE_QUAD(kv+iy, kv+iy+1, mv+iy+1, mv+iy, 4, bias);
			}
			kv += nlayer;
		}

		// RIGHT top row:
		kv = midStart + wsegs + (hsegs-2)*nlayer; 
		mv = topStart + wsegs;
		for (iy=0; iy<lsegs; iy++) {
			MAKE_QUAD(kv, kv+1, mv+wsp1, mv, 4, bias);
			mv += wsp1;
			kv++;
		}

		assert(nf==lsegs*wsegs*4 + wsegs*hsegs*2 + lsegs*hsegs*2);

		// do BACK faces. ---------------------
		// BACK bottom row:
		kv = topStart - 1;
		mv = midStart + wsegs + lsegs;
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv-1, mv+1, mv, 5, bias);
			kv --;
			mv ++;
		}

		// BACK middle part:
		kv = midStart + wsegs + lsegs; 
		for(iz=0; iz<hsegs-2; iz++) {
			mv = kv + nlayer;
			for (ix=0; ix<wsegs; ix++) {
				MAKE_QUAD(kv+ix, kv+ix+1, mv+ix+1, mv+ix, 5, bias);
			}
			kv += nlayer;
		}

		// BACK top row:
		kv = midStart + wsegs + lsegs + (hsegs-2)*nlayer; 
		mv = topStart + lsegs*(wsegs+1)+wsegs;
		for (ix=0; ix<wsegs; ix++) {
			MAKE_QUAD(kv, kv+1, mv-1, mv, 5, bias);
			mv --;
			kv ++;
		}

		assert(nf==lsegs*wsegs*4 + wsegs*hsegs*4 + lsegs*hsegs*2);

		// do LEFT faces. -----------------
		// LEFT bottom row:
		kv = lsegs*(wsegs+1);  // index into bottom
		mv = midStart + 2*wsegs +lsegs;
		for (iy=0; iy<lsegs; iy++) {
			nextm = mv+1;
			if (iy==lsegs-1) 
				nextm -= nlayer;
			MAKE_QUAD(kv, kv-wsp1, nextm, mv, 6, bias);
			kv -=wsp1;
			mv ++;
		}

		// LEFT middle part:
		kv = midStart + 2*wsegs + lsegs; 
		for(iz=0; iz<hsegs-2; iz++) {
			mv = kv + nlayer;
			for (iy=0; iy<lsegs; iy++) {
				nextm = mv+1;
				nextk = kv+iy+1;
				if (iy==lsegs-1) { 
					nextm -= nlayer;
					nextk -= nlayer;
				}
				MAKE_QUAD(kv+iy, nextk, nextm, mv, 6, bias);
				mv++;
			}
			kv += nlayer;
		}

		// LEFT top row:
		kv = midStart + 2*wsegs + lsegs+ (hsegs-2)*nlayer; 
		mv = topStart + lsegs*(wsegs+1);
		for (iy=0; iy<lsegs; iy++) {
			nextk = kv+1;
			if (iy==lsegs-1) 
				nextk -= nlayer;
			MAKE_QUAD(kv, nextk, mv-wsp1, mv, 6, bias);
			mv -= wsp1;
			kv++;
		}
	}

	mesh.setNumTVerts(0);
	mesh.setNumTVFaces(0);
	for (nf = 0; nf<nfaces; nf++) {
		Face& f = mesh.faces[nf];
		DWORD* nv = f.getAllVerts();
		Point3 v[3];
		for (int ix =0; ix<3; ix++)
			v[ix] = mesh.getVert(nv[ix]);
		int dir = direction(v);
		mesh.setFaceMtlIndex(nf,0);
		//mesh.setFaceMtlIndex(nf,mapDir[dir]);
	}

	mesh.InvalidateTopologyCache();
}

extern void BuildSphere(Mesh&mesh, float radius, int segs, int smooth, float startAng)
{
	Point3 p;	
	int ix,na,nb,nc,nd,jx,kx;
	int nf=0,nv=0;
	float delta, delta2;
	float a,alt,secrad,secang,b,c;
	float hemi = 0.0f;

	LimitValue(segs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(smooth, MIN_SMOOTH, MAX_SMOOTH);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);

	float totalPie(0.0f);
	if (hemi>=1.0f) hemi = 0.9999f;
	hemi = (1.0f-hemi) * PI;
	float basedelta=2.0f*PI/(float)segs;
	delta2 = basedelta;
	delta  = basedelta;

	int rows = int(hemi/delta) + 1;
	int realsegs=segs;
	int nverts = rows * realsegs + 2;
	int nfaces = rows * realsegs * 2;
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	int lastvert=nverts-1;

	// Top vertex 
	mesh.setVert(nv, 0.0f, 0.0f, radius);
	nv++;

	// Middle vertices 
	alt=delta;
	for(ix=1; ix<=rows; ix++) {		
		a = (float)cos(alt)*radius;		
		secrad = (float)sin(alt)*radius;
		secang = startAng; //0.0f
		for(jx=0; jx<segs; ++jx) {
			b = (float)cos(secang)*secrad;
			c = (float)sin(secang)*secrad;
			mesh.setVert(nv++,b,c,a);
			secang+=delta2;
		}
		alt+=delta;		
	}

	/* Bottom vertex */
	mesh.setVert(nv++, 0.0f, 0.0f,-radius);

	// Now make faces 

	BitArray startSliceFaces;
	BitArray endSliceFaces;

	// Make top conic cap
	for(ix=1; ix<=segs; ++ix) {
		nc=(ix==segs)?1:ix+1;
		mesh.faces[nf].setEdgeVisFlags(1,1,1);
		mesh.faces[nf].setSmGroup(smooth?1:0);
		mesh.faces[nf].setMatID(1);
		mesh.faces[nf].setVerts(0, ix, nc);
		nf++;
	}

	/* Make midsection */
	int lastrow=rows-1,lastseg=segs-1,almostlast=lastseg-1;
	for(ix=1; ix<rows; ++ix) {
		jx=(ix-1)*segs+1;
		for(kx=0; kx<segs; ++kx) {
			na = jx+kx;
			nb = na+segs;
			nc = (kx==lastseg)? jx+segs: nb+1;
			nd = (kx==lastseg)? jx : na+1;

			mesh.faces[nf].setEdgeVisFlags(1,1,0);
			mesh.faces[nf].setSmGroup(smooth?1:0);
			mesh.faces[nf].setMatID(1); 
			mesh.faces[nf].setVerts(na,nb,nc);
			nf++;

			mesh.faces[nf].setEdgeVisFlags(0,1,1);
			mesh.faces[nf].setSmGroup(smooth?1:0);
			mesh.faces[nf].setMatID(1);
			mesh.faces[nf].setVerts(na,nc,nd);
			nf++;
		}
	}

	// Make bottom conic cap
	na = mesh.getNumVerts()-1;
	int botsegs=segs;
	jx = (rows-1)*segs+1;lastseg=botsegs-1;
	int fstart = nf;
	for(ix=0; ix<botsegs; ++ix) {
		nc = ix + jx;
		nb = (ix==lastseg)?jx:nc+1;
		mesh.faces[nf].setEdgeVisFlags(1,1,1);
		mesh.faces[nf].setSmGroup(smooth?1:0);
		mesh.faces[nf].setMatID(1);
		mesh.faces[nf].setVerts(na, nb, nc);
		nf++;
	}

	mesh.setNumTVerts(0);
	mesh.setNumTVFaces(0);
	mesh.InvalidateTopologyCache();
}



void AddFace(Face *f,int a,int b,int c,int evis,int smooth_group)
{ 
	const int ALLF = 4;
	f[0].setSmGroup(smooth_group);
	f[0].setMatID((MtlID)0); 	 /*default */
	if (evis==0) f[0].setEdgeVisFlags(1,1,0);
	else if (evis==1) f[0].setEdgeVisFlags(0,1,1);
	else if (evis==2) f[0].setEdgeVisFlags(0,0,1);
	else if (evis==ALLF) f[0].setEdgeVisFlags(1,1,1);
	else f[0].setEdgeVisFlags(1,0,1);	
	f[0].setVerts(a,b,c);
}


void BuildScubaMesh(Mesh &mesh, int segs, int smooth, int llsegs, 
					float radius1, float radius2, float cylh)
{
	Point3 p;
	int ix,jx,ic = 1;
	int nf=0,nv=0, capsegs=(int)(segs/2.0f),csegs=0;
	float ang;	
	float startAng = 0.0f;	
	float totalPie = TWOPI;
	int lsegs = llsegs-1 + 2*capsegs;
	int levels=csegs*2+(llsegs-1);
	int capv=segs,sideedge=capsegs+csegs;
	int totlevels=levels+capsegs*2+2;
	int tvinslice=totlevels+totlevels-2;
	float delta = (float)2.0*PI/(float)segs;
	int VertexPerLevel=segs;
	int nfaces=2*segs*(levels+1);
	int ntverts=2*(segs+1)+llsegs-1;
	int *edgelstl=new int[totlevels];
	int *edgelstr=new int[totlevels];
	int lastlevel=totlevels-1,dcapv=capv-1,dvertper=VertexPerLevel-1;
	edgelstr[0] = edgelstl[0] = 0;
	edgelstr[1] = 1;
	edgelstl[1] = capv;
	for (int i=2;i<=sideedge;i++){ 
		edgelstr[i]=edgelstr[i-1]+capv;
		edgelstl[i]=edgelstr[i]+dcapv;
	}
	while ((i<lastlevel)&&(i<=totlevels-sideedge)){ 
		edgelstr[i]=edgelstr[i-1]+VertexPerLevel;
		edgelstl[i]=edgelstr[i]+dcapv;
		i++;
	}
	while (i<lastlevel) { 
		edgelstr[i]=edgelstr[i-1]+capv;
		edgelstl[i]=edgelstr[i]+dcapv;
		i++;
	}
	edgelstl[lastlevel]= (edgelstr[lastlevel]=edgelstl[i-1]+1);
	int nverts=edgelstl[lastlevel]+1;
	nfaces+=2*segs*(2*capsegs-1);

	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	mesh.setNumTVerts(0);
	mesh.setNumTVFaces(0);
	mesh.setSmoothFlags(smooth != 0);

	// bottom vertex 
	float height = cylh + radius1 + radius2;
	mesh.setVert(nv, Point3(0.0f,0.0f,height));
	mesh.setVert(nverts-1, Point3(0.0f,0.0f,0.0f));		

	// Top (1) and bottom (2) cap vertices
	float ru,cang,sang;
	int msegs=segs,deltaend=nverts-capv-1;
	ang = startAng;	 
	msegs--;
	float rincr=PI/(2.0f*capsegs),aincr;
	for (jx = 0; jx<=msegs; jx++) 
	{
		cang=(float)cos(ang);
		sang=(float)sin(ang);
		for(ix=1; ix<=sideedge; ix++) {
			aincr = (rincr*(float)ix);
			ru=(float)sin(aincr);

			p.x = cang*radius1*ru;
			p.y = sang*radius1*ru;	
			p.z = (jx==0) ? height-radius1*(1.0f-(float)cos(aincr)) : mesh.verts[edgelstr[ix]].z;
			mesh.setVert(edgelstr[ix]+jx, p);

			p.x = cang*radius2*ru;
			p.y = sang*radius2*ru;	
			p.z = (jx==0) ? radius2*(1.0f-(float)cos(aincr)) : mesh.verts[edgelstr[lastlevel-ix]].z ;
			mesh.setVert(edgelstr[lastlevel-ix]+jx,p);
		}
		ang += delta;
	}

	//// Middle vertices 
	//int sidevs,startv=edgelstr[sideedge],deltav;				
	//for(ix=1; ix<llsegs; ix++) {
	//   // Put center vertices all the way up
	//   float   u = float(ix)/float(llsegs);
	//   float rad = (radius1*(1.0f-u) + radius2*u);
	//   p.z = cylh *((float)ix/float(llsegs)) + radius2;
	//   ang = startAng;
	//   for (sidevs=0;sidevs<VertexPerLevel;sidevs++)
	//      p.x = (float)cos(ang)*rad;
	//      p.y = (float)sin(ang)*rad;
	//      mesh.setVert(nv, p);
	//      nv++;
	//      ang += delta;
	//   }	
	//}

	//top layer done, now reflect sides down 
	int sidevs,deltav;
	int startv=edgelstr[sideedge];
	int endv=edgelstr[totlevels-capsegs-1]; 
	if (llsegs>1)
	{
		float sincr = cylh/llsegs;
		for (sidevs=0;sidevs<VertexPerLevel;sidevs++)
		{
			Point3 topp = mesh.verts[startv];
			Point3 botp = mesh.verts[endv];
			p.x = (topp.x + botp.x) /  2.0f;
			p.y = (topp.y + botp.y) /  2.0f;
			deltav=VertexPerLevel;
			for (ic=1;ic<llsegs;ic++)
			{
				p.z = topp.z-sincr*ic;
				mesh.setVert(startv+deltav, p);
				deltav+=VertexPerLevel;
			}
			startv++;
		}
	}
	int lasttvl=0,lasttvr=0;
	int lvert=segs;
	int t0,t1,b0,b1,tvt0=0,tvt1=0,tvb0=1,tvb1=2,fc=0,smoothgr=(smooth?4:0),vseg=segs+1;
	int tvcount=0,lowerside=lastlevel-sideedge,onside=0;

	BOOL ok,wrap;
	// Now make faces ---
	for (int clevel=0;clevel<lastlevel-1;clevel++)
	{
		t1=(t0=edgelstr[clevel])+1;
		b1=(b0=edgelstr[clevel+1])+1;
		ok=1; wrap=FALSE;
		if ((clevel>0)&&(onside==1)) {
			tvt0++;tvt1++;tvb0++,tvb1++;
		}
		if (clevel==1) {
			tvt0=1;tvt1=2;
		}
		if (clevel==sideedge) {
			tvt1+=lvert;tvt0+=lvert;tvb0+=vseg;tvb1+=vseg;onside++;
		} else if (clevel==lowerside) {
			tvt1+=vseg;tvt0+=vseg;tvb0+=lvert;tvb1+=lvert;onside++;
		}
		while ((b0<edgelstl[clevel+1])||ok)
		{
			if (b1==edgelstr[clevel+2]) {
				b1=edgelstr[clevel+1]; 
				t1=edgelstr[clevel];
				ok=FALSE;
				wrap=(onside!=1);
			}
			if (smooth) smoothgr=4;
			AddFace(&mesh.faces[fc++],t0,b0,b1,0,smoothgr);
			if (clevel>0) {
				AddFace(&mesh.faces[fc++],t0,b1,t1,1,smoothgr);
				t0++;t1++;
			}
			b0++;b1++;tvb0++,tvb1++;
		}
	}
	smoothgr=(smooth?4:0);
	t1=(t0=edgelstr[lastlevel-1])+1;b0=edgelstr[lastlevel];
	int lastpt=lastlevel;
	if (onside==1) {
		tvt0++;
		tvt1++;
		tvb0++;
		tvb1++;
	}
	if (sideedge==1) {
		tvt1+=vseg;
		tvt0+=vseg;
		tvb0+=lvert;
		tvb1+=lvert;
		onside++;
	}
	while (t0<edgelstl[lastpt]) {
		if (t1==edgelstr[lastlevel]) {
			t1=edgelstr[lastlevel-1];
			tvt1-=segs;
		}
		AddFace(&mesh.faces[fc++],t0,b0,t1,1,smoothgr);
		t0++;t1++;
	}
	for (i=0;i<nverts;i++) 
		mesh.verts[i].z -= (radius2 + cylh/2.0f);

	if (edgelstr) delete []edgelstr;
	if (edgelstl) delete []edgelstl;
	assert(fc==mesh.numFaces);
	//	assert(nv==mesh.numVerts);
	mesh.InvalidateTopologyCache();
}



extern HINSTANCE hInstance;
class MagicCode
{
private:
	struct Triangle { int a, b, c; };
	typedef int (__stdcall * fnCalcCapsule)
	    (int nverts, const Point3 *verts, 
		Point3& pt1, Point3& pt2, float& r1, float& r2);

	typedef int (__stdcall * fnCalcOrientedBox)
	    (int nverts, const Point3 *verts,
		float& udim, float& vdim, float& ndim,
		Point3& center, Point3& Uaxis, Point3& Vaxis, Point3& Naxis);

	typedef void (__stdcall * fnCalcMassProps)
	(	int nverts, const Point3* verts, 
		int ntris, const Triangle* tris,
		int iBodyCoords, float &rfMass,
		Point3& rkCenter, Matrix3& rkInertia);

	HMODULE hMagicLib;
	fnCalcCapsule CalcCapsule;
	fnCalcOrientedBox CalcOrientedBox;
	fnCalcMassProps CalcMassProps;

public:
	MagicCode() : hMagicLib(0), CalcCapsule(0), CalcOrientedBox(0), CalcMassProps(0) {
	}

	~MagicCode() {
		if (hMagicLib) FreeLibrary(hMagicLib);
	}

	bool Initialize()
	{
		if (hMagicLib == NULL)
		{
			char curfile[_MAX_PATH];
			GetModuleFileName(hInstance, curfile, MAX_PATH);
			PathRemoveFileSpec(curfile);
			PathAppend(curfile, "NifMagic.dll");
			hMagicLib = LoadLibraryA( curfile );
			if (hMagicLib == NULL)
				hMagicLib = LoadLibraryA( "Nifmagic.dll" );
			CalcCapsule = (fnCalcCapsule)GetProcAddress( hMagicLib, "CalcCapsule" );
			CalcOrientedBox = (fnCalcOrientedBox)GetProcAddress( hMagicLib, "CalcOrientedBox" );
			CalcMassProps = (fnCalcMassProps)GetProcAddress( hMagicLib, "CalcMassProps" );
		}
		// Now returns TRUE if ANY of the desired methods are present.
		// Checks for individual methods will need to check both Initialize() and the appropriate Has...() method.
		return ( NULL != CalcCapsule || NULL != CalcOrientedBox || NULL != CalcMassProps );
	}

	bool HasCalcCapsule() {return NULL != CalcCapsule;}
	bool HasCalcOrientedBox() {return NULL != CalcOrientedBox;}
	bool HasCalcMassProps() {return NULL != CalcMassProps;}
	void DoCalcCapsule(Mesh &mesh, Point3& pt1, Point3& pt2, float& r1, float& r2)
	{
		if (Initialize() && HasCalcCapsule())
		{
			CalcCapsule( mesh.getNumVerts(), &mesh.verts[0], pt1, pt2, r1, r2);
		}
	}

	void DoCalcOrientedBox(Mesh &mesh, float& udim, float& vdim, float& ndim, Point3& center, Matrix3& rtm)
	{
		if (Initialize() && HasCalcOrientedBox())
		{
			Point3 Uaxis, Vaxis, Naxis;
			CalcOrientedBox(mesh.getNumVerts(), &mesh.verts[0], udim, vdim, ndim, center, Uaxis, Vaxis, Naxis);
			rtm.Set(Uaxis, Vaxis, Naxis, center);
		}
	}

	void DoCalcMassProps( Mesh &mesh,
		bool bBodyCoords, float &rfMass,
		Point3& rkCenter, Matrix3& rkInertia)
	{
		if (Initialize() && HasCalcMassProps())
		{
			vector<Triangle> tris;
			tris.resize(mesh.getNumFaces());
			for (int i=0; i<mesh.getNumFaces(); ++i)
			{
				Triangle& tri = tris[i];
				Face& face = mesh.faces[i];
				tri.a = face.getVert(0);
				tri.b = face.getVert(1);
				tri.c = face.getVert(2);
			}
			CalcMassProps( mesh.getNumVerts(), &mesh.verts[0]
				, tris.size(), &tris[0]
				, bBodyCoords ? 1 : 0, rfMass, rkCenter, rkInertia );
		}
	}

} TheMagicCode;

extern bool CanCalcCapsule()
{
	return TheMagicCode.Initialize() && TheMagicCode.HasCalcCapsule();
}
extern bool CanCalcOrientedBox()
{
	return TheMagicCode.Initialize() && TheMagicCode.HasCalcOrientedBox();
}
extern bool CanCalcMassProps()
{
	return TheMagicCode.Initialize() && TheMagicCode.HasCalcMassProps();
}

// Calculate capsule from mesh.  While radii on the endcaps is possible we do 
//   currently calculate then differently.
extern void CalcCapsule(Mesh &mesh, Point3& pt1, Point3& pt2, float& r1, float& r2)
{
	TheMagicCode.DoCalcCapsule(mesh, pt1, pt2, r1, r2);
}

// Calculate OBB (oriented bounding box) from mesh.  Returns each of the 3 dimensions of the box, 
// its center, and the rotation matrix necessary to get the orientation.
extern void CalcOrientedBox(Mesh &mesh, float& udim, float& vdim, float& ndim, Point3& center, Matrix3& rtm)
{
	TheMagicCode.DoCalcOrientedBox(mesh, udim, vdim, ndim, center, rtm);
}

extern void CalcMassProps( Mesh &mesh,
							bool bBodyCoords, float &rfMass,
							Point3& rkCenter, Matrix3& rkInertia)
{
	TheMagicCode.DoCalcMassProps(mesh, bBodyCoords, rfMass, rkCenter, rkInertia);
}

extern void BuildCapsule(Mesh &mesh, Point3 pt1, Point3 pt2, float r1, float r2)
{
	int segs = 12;
	int hsegs = 1;
	int smooth = 1;

	float h = (pt1 - pt2).Length();

	Point3 center = ((pt2 + pt1) / 2.0f);
	Point3 norm = Normalize(pt2 - pt1);
	Matrix3 mat;
	MatrixFromNormal(norm,mat);
	Matrix3 newTM = mat * TransMatrix(center);

	// Build capsule to suggested size
	BuildScubaMesh(mesh, segs, smooth, hsegs, r1, r2, h);

	// Reorient the capsule.
	MNMesh mn(mesh);
	Matrix3 tm(true);
	tm.Translate(center);
	mn.Transform(newTM);
	mn.OutToTri(mesh);
}
