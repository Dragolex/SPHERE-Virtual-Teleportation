/*
This file provides a couple of computation functionalities.
Most of this is reproduced from: http://geomalgorithms.com/a05-_intersect-1.html
The code has been highly optimized and enhanced with more arguments to avoid recalculating constant results.
*/

#include "stdafx.h"

#include "CustomMath.h"


float CustomMath::compute_line_distance(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D)
{
	float    d = u.dotProduct(w);
	float    e = v.dotProduct(w);

	float seg1 = (b*e - c*d) / D;
	float seg2 = (a*e - b*d) / D;

	// get the difference of the two closest points
	vector3df dP = w + (seg1 * u) - (seg2 * v);

	return(dP.getLengthSQ());   // return the closest distance
}


float CustomMath::compute_line_collission(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D, float * seg1, float * seg2)
{
	float    d = u.dotProduct(w);
	float    e = v.dotProduct(w);
	
	*seg1 = (b*e - c*d) / D;
	*seg2 = (a*e - b*d) / D;

	// get the difference of the two closest points
	vector3df dP = w + ((*seg1) * u) - ((*seg2) * v);

	return(dP.getLengthSQ());   // return the closest distance
}

// Returns where on line 1 it intersects line 2
float CustomMath::compute_line_collission_eff(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D)
{
	float    d = u.dotProduct(w);
	float    e = v.dotProduct(w);

	return((b*e - c*d) / D);
}

float CustomMath::compute_line_collission_eff_full(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D, float * line_pos)
{
	float    d = u.dotProduct(w);
	float    e = v.dotProduct(w);

	*line_pos = (a*e - b*d) / D;
	return((b*e - c*d) / D);
}





// intersect3D_2Planes(): find the 3D intersection of two planes
//    Input:  two planes Pn1 and Pn2
//    Output: *L = the intersection line (when it exists)
//    Return: 0 = disjoint (no intersection)
//            1 = the two  planes coincide
//            2 =  intersection in the unique line *L
int CustomMath::compute_plane_collission(vector3df normal_a, vector3df normal_b, vector3df base_a, vector3df base_b, vector3df * line_orig, vector3df * line_target)
{
	vector3df u = normal_a.crossProduct(normal_b);          // cross product

	float    ax = (u.X >= 0 ? u.X : -u.X);
	float    ay = (u.Y >= 0 ? u.Y : -u.Y);
	float    az = (u.Z >= 0 ? u.Z : -u.Z);

	// test if the two planes are parallel

	if ((ax + ay + az) < 1.0f) {        // Pn1 and Pn2 are near parallel
											 // test if disjoint or coincide
		vector3df v(base_b - base_a);
		if (normal_a.dotProduct(v) == 0)          // Pn2.V0 lies in Pn1
			return 1;                    // Pn1 and Pn2 coincide
		else
			return 0;                    // Pn1 and Pn2 are disjoint
	}

	// Pn1 and Pn2 intersect in a line
	// first determine max abs coordinate of cross product
	int maxc;                       // max coordinate
	if (ax > ay)
		if (ax > az)
			maxc = 1;
		else maxc = 3;
	else
		if (ay > az)
			maxc = 2;
		else maxc = 3;

	// next, to get a point on the intersect line
	// zero the max coord, and solve for the other two
	float    d1, d2;            // the constants in the 2 plane equations
	d1 = -normal_a.dotProduct(base_a);  // note: could be pre-stored  with plane
	d2 = -normal_b.dotProduct(base_b);  // ditto

	switch (maxc) {             // select max coordinate
	case 1:                     // intersect with x=0
		line_orig->X = 0;
		line_orig->Y = (d2*normal_a.Z - d1*normal_b.Z) / u.X;
		line_orig->Z = (d1*normal_b.Y - d2*normal_a.Y) / u.X;
		break;
	case 2:                     // intersect with y=0
		line_orig->X = (d1*normal_b.Z - d2*normal_a.Z) / u.Y;
		line_orig->Y = 0;
		line_orig->Z = (d2*normal_a.X - d1*normal_b.X) / u.Y;
		break;
	case 3:                     // intersect with z=0
		line_orig->X = (d2*normal_a.Y - d1*normal_b.Y) / u.Z;
		line_orig->Y = (d1*normal_b.X - d2*normal_a.X) / u.Z;
		line_orig->Z = 0;
	}

	line_target->set((*line_orig) + u);
	return 2;
}



double CustomMath::lengthdir_x(double len, double dir) {
	return(cos(dir*M_PI / 180) * len);
}

double CustomMath::lengthdir_y(double len, double dir) {
	return(-sin(dir*M_PI / 180) * len);
}



void CustomMath::RGB2HSV(float r, float g, float b,
	float &h, float &s, float &v)
{
	float K = 0.f;

	if (g < b)
	{
		std::swap(g, b);
		K = -1.f;
	}

	if (r < g)
	{
		std::swap(r, g);
		K = -2.f / 6.f - K;
	}

	float chroma = r - std::min(g, b);
	h = fabs(K + (g - b) / (6.f * chroma + 1e-20f));
	s = chroma / (r + 1e-20f);
	v = r;
}