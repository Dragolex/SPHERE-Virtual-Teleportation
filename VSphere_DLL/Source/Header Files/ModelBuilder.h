#pragma once

#include "simplifyingHeader.h"

#include "RayGenerator.h"
#include "Ray3D.h"



class ModelBuilder
{
private:

	vector<vector<Ray3D*>*> other_rays;

	vector<RayGenerator*> other_ray_generators;

	vector3df * cam_direction_vec = nullptr;
	vector3df * cam_direction_vec_norm;
	float cam_direction_dot;
	vector<vector3df*> other_cam_direction_vecs;
	vector<float> other_cam_direction_dots;
	vector<float> ab_direction_dots;
	vector<float> quotients;


	vector<int> debug_quads;


	int max_ray_length;

	int dbg_test = 0;


	boolean currently_A = true;

	vector<float> * intersection_factor_start_x;
	vector<float> * intersection_factor_end_x;

	vector<float> * intersection_factor_start_y;
	vector<float> * intersection_factor_end_y;

	vector<bool> * intersection_represents_entering_real_surface;
	vector<int> * texture_coordinates;

	vector<float> intersection_values;


	vector<float> A_intersection_factor_start_x;
	vector<float> A_intersection_factor_end_x;

	vector<float> A_intersection_factor_start_y;
	vector<float> A_intersection_factor_end_y;

	vector<bool> A_is_visible_plane_starter;
	vector<int> A_texture_coordinates;



	vector<float> B_intersection_factor_start_x;
	vector<float> B_intersection_factor_end_x;

	vector<float> B_intersection_factor_start_y;
	vector<float> B_intersection_factor_end_y;

	vector<bool> B_is_visible_plane_starter;
	vector<int> B_texture_coordinates;



	// From other class

	vector<Ray3D*> * rays;


public:

	ModelBuilder();
	~ModelBuilder();

	void referenceAnotherRayGenerator(RayGenerator * ray_generator, bool finalize_with_own_ray_generator);

	void intersectRays();
	void computeModelPart(vector<int> * output_content);
};


