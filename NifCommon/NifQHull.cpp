#include "Max.h"
#include "MeshDelta.h"

//#undef Float
//#undef PI
//#define WYKOBI_SINGLE_PRECISION
//
//#include "wykobi/wykobi.hpp"
//#include "wykobi/wykobi_instantiate.hpp"
//#include "wykobi/wykobi_algorithm.hpp"

#include <qhull/qhull.h>
#include <qhull/mem.h>
#include <qhull/qset.h>
#include <qhull/geom.h>
#include <qhull/merge.h>
#include <qhull/poly.h>
#include <qhull/io.h>
#include <qhull/stat.h>

using namespace Niflib;

vector<Triangle> compute_convex_hull(const vector<Vector3>& verts)
{  
	vector<Triangle> tris;

	int dim=3;	              /* dimension of points */
	int numpoints=0;          /* number of points */
	coordT *points=0;         /* array of coordinates for each point */ 
	boolT ismalloc=0;         /* True if qhull should free points in qh_freeqhull() or reallocation */ 
	char flags[]= "qhull i Qt"; /* option flags for qhull, see qh_opt.htm */
	FILE *outfile= stdout;    /* output from qh_produce_output()			
							  use NULL to skip qh_produce_output() */ 
	FILE *errfile= stderr;    /* error messages from qhull code */ 
	int exitcode;             /* 0 if no error from qhull */
	facetT *facet;	          /* set by FORALLfacets */
	int curlong, totlong;	  /* memory remaining after qh_memfreeshort */

	vertexT *vertex, **vertexp;
	setT *vertices;

	numpoints = verts.size();
	points = new coordT[3 * verts.size()];
	for (int i=0, n=verts.size(); i<n; ++i) {
		points[i*3 + 0] = verts[i].x;
		points[i*3 + 1] = verts[i].y;
		points[i*3 + 2] = verts[i].z;
	}

	/* initialize dim, numpoints, points[], ismalloc here */
	exitcode= qh_new_qhull (dim, numpoints, points, ismalloc,
		flags, outfile, errfile);
	if (!exitcode) { /* if no error */ 
		/* 'qh facet_list' contains the convex hull */
		FORALLfacets {
			vertices = qh_facet3vertex (facet);
			if (qh_setsize (vertices) == 3) {
				Triangle tri;
				int i = 0;
				FOREACHvertex_(vertices) {
					tri[i++] = qh_pointid(vertex->point);
				}
				tris.push_back(tri);
			}
			qh_settempfree(&vertices);
		}
	}
	qh_freeqhull(!qh_ALL);  
	qh_memfreeshort (&curlong, &totlong);
	if (curlong || totlong)
		fprintf (errfile, "qhull internal warning (main): did not free %d bytes of long memory (%d pieces)\n", 
		totlong, curlong);

	delete[] points;
	return tris;
};

void compute_convex_hull(Mesh& mesh, Mesh& outmesh)
{  
	MNMesh mn;
	map<int, int> ptmap;

	vector<Triangle> tris;

	int dim=3;	              /* dimension of points */
	int numpoints=0;          /* number of points */
	coordT *points=0;         /* array of coordinates for each point */ 
	boolT ismalloc=0;         /* True if qhull should free points in qh_freeqhull() or reallocation */ 
	char flags[]= "qhull i Qt"; /* option flags for qhull, see qh_opt.htm */
	FILE *outfile= stdout;    /* output from qh_produce_output()			
							  use NULL to skip qh_produce_output() */ 
	FILE *errfile= stderr;    /* error messages from qhull code */ 
	int exitcode;             /* 0 if no error from qhull */
	facetT *facet;	          /* set by FORALLfacets */
	int curlong, totlong;	  /* memory remaining after qh_memfreeshort */
	setT *vertices;

	setT *pts;
	pointT *point;

	numpoints = mesh.getNumVerts();
	points = new coordT[3 * numpoints];
	for (int i=0; i<numpoints; ++i) {
		Point3& pt = mesh.getVert(i);
		points[i*3 + 0] = pt.x;
		points[i*3 + 1] = pt.y;
		points[i*3 + 2] = pt.z;
	}

	/* initialize dim, numpoints, points[], ismalloc here */
	exitcode= qh_new_qhull (dim, numpoints, points, ismalloc,
		flags, outfile, errfile);
	if (!exitcode) { /* if no error */ 
		vertexT *vertex, **vertexp;
		int id;
		int numpoints=0, point_i, point_n;
		int allpoints= qh num_points + qh_setsize (qh other_points);
		pts = qh_settemp (allpoints);
		qh_setzero (pts, 0, allpoints);
		vertices= qh_facetvertices (qh facet_list, NULL, !qh_ALL);
		FOREACHvertex_(vertices) {
			id= qh_pointid (vertex->point);
			if (id >= 0) {
				SETelem_(pts, id)= vertex->point;
				numpoints++;
			}
		}
		qh_settempfree (&vertices);

		int i=0;
		FOREACHpoint_i_(pts) {
			if (point) {
				int id = qh_pointid(point);
				ptmap[id] = mn.NewVert(Point3(point[0], point[1], point[2]));
				i++;
			}
		}
		qh_settempfree (&pts);

		FORALLfacets {
			vertices = qh_facet3vertex (facet);
			if (qh_setsize (vertices) == 3) {
				Triangle tri;
				int i = 0;
				FOREACHvertex_(vertices) {
					int id = qh_pointid(vertex->point);
					tri[i++] = ptmap[id];
				}
				mn.NewTri(tri.v1, tri.v2, tri.v3);
			}
			qh_settempfree(&vertices);
		}
	}
	qh_freeqhull(!qh_ALL);  
	qh_memfreeshort (&curlong, &totlong);
	delete[] points;

	mn.MakeConvex();
	mn.FillInMesh();
	mn.EliminateBadVerts(0);
	mn.Triangulate();
	mn.OutToTri(outmesh);
}
