#pragma once

#include "simplifyingHeader.h"

#include <Ray3D.h>



class CustomMath
{
public:
	static void RGB2HSV(float r, float g, float b,
		float &h, float &s, float &v);

	static float compute_line_distance(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D);

	static float compute_line_collission(vector3df w, vector3df u, vector3df v, float a, float c, float b, float D, float * seg1, float * seg2);
	static float compute_line_collission_eff(vector3df origin_a_to_b, vector3df direction_a, vector3df direction_b, float dot_a, float dot_b, float a_dot_b, float D);
	static float compute_line_collission_eff_full(vector3df origin_a_to_b, vector3df direction_a, vector3df direction_b, float dot_a, float dot_b, float a_dot_b, float D, float * line_pos);

	static int compute_plane_collission(vector3df normal_a, vector3df normal_b, vector3df base_a, vector3df base_b, vector3df * line_orig, vector3df * line_target);

	static double lengthdir_x(double len, double dir);
	static double lengthdir_y(double len, double dir);
};
