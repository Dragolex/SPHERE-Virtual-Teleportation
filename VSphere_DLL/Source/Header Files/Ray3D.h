#pragma once

#include "simplifyingHeader.h"

#include "opencv2/opencv.hpp"

#include "StaticDebug.h"

#include <math.h>

#include <queue>



struct Ray3D
{
	vector3df origin;
	vector3df origin_start;
	vector3df origin_end;

	vector3df normal;

	vector3df dir_along_y;
	float ray_width, ray_width_sq;

	int tex_start_x, tex_start_y, tex_end_x, tex_end_y;

	bool inside_is_on_the_right;


#ifdef DEBUG
	int camera_source_index;
#endif

	vector<int> intersection_indices;

private:
	void addQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, vector3df cam_direction, vector3df dir_along_y, vector<int> * output);
	void addTexture(vector<int> * texture_coordinates, int last_index, int index, vector<int> * output);
	
public:
	void addAsModelQuads(vector<int> * output, vector3df cam_direction, vector<float> * intersection_factor_start_x, vector<float> * intersection_factor_end_x, vector<float> * intersection_factor_start_y, vector<float> * intersection_factor_end_y, vector<bool> * is_visible_plane_starter, vector<int> * texture_coordinates);
	void addAsRayQuad(vector<int> * output, vector3df cam_direction, float maxLength, bool show_orientation);
};