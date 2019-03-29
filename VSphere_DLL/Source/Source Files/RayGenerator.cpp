/*
This core class handles the resulting edges from the EdgesIdentifier and prepares Ray3D instances
as required for the ModelBuilder.

Input (from EdgesIdentifier)
	segment_starts pointer			// int representing the linear coordinate (if the image were just one-dimensional) of the start point of the segments
	segment_ends pointer			// int representing the linear coordinate (if the image were just one-dimensional) of the end point of the segments
	segment_orientations pointer	// Boolean array how the edge is oriented
									// True if the inside of the object is on the right
									// False if it is on the left.
									// Left and right refers to when looking from the start to the end point.

Output:
	rays pointer					// Vector with rays produced by the input edges


@Author: Alexander Georgescu
*/


#include "stdafx.h"

#include "RayGenerator.h"



RayGenerator::RayGenerator(CameraSource * camera_source)
{
	this->camera_source = camera_source;
}

RayGenerator::~RayGenerator()
{
	int ray_count = rays.size();
	for (int i = 0; i < ray_count; ++i)
		delete(rays[i]);
	rays.clear();
}

/*
Initialize the pointers to the data from the EdgesIdentifier.
*/
void RayGenerator::initData(vector<int> * segment_starts, vector<int> * segment_ends, vector<bool> * segment_orientations, int tex_offs_x, int tex_offs_y)
{
	cam_w = camera_source->getSize().X;
	cam_h = camera_source->getSize().Y;
	cam_direction = camera_source->getDirection();
	cam_left_top = camera_source->getOrigin() + camera_source->getToCorner();

	this->tex_offs_x = tex_offs_x;
	this->tex_offs_y = tex_offs_y;

	this->segment_starts = segment_starts;
	this->segment_ends = segment_ends;
	this->segment_orientations = segment_orientations;
}


/*
Add a new ray. Todo: To save memmroy allocations, perhaps use some sort of pool of Ray3D and reuse them.
*/
void RayGenerator::addRay(int x1, int y1, int x2, int y2, bool orientation, vector3df cam_left_top, quaternion cam_direction)
{
	Ray3D * ray = new Ray3D();

	// Texture coordinates (Coordinates of the camera
	ray->tex_start_x = x1 + tex_offs_x;
	ray->tex_start_y = y1 + tex_offs_y;
	ray->tex_end_x = x2 + tex_offs_x;
	ray->tex_end_y = y2 + tex_offs_y;

	
	// Coordinates of the origin in space
	ray->origin_start = cam_left_top + cam_direction*vector3df(x1, -y1, 0);
	ray->origin_end = cam_left_top + cam_direction*vector3df(x2, -y2, 0);
	ray->origin = cam_left_top + cam_direction*vector3df(x1 + (x2 - x1) / 2, -(y1 + (y2 - y1) / 2), 0);

	ray->dir_along_y = (ray->origin_end - ray->origin_start);

	ray->ray_width = ray->dir_along_y.getLength(); // Todo: try to avoid getLength
	ray->ray_width_sq = ray->dir_along_y.getLengthSQ() / 2;

	ray->dir_along_y.normalize();

	vector3df pt = ray->origin_start + cam_direction*vector3df(0, 0, 10);
	ray->normal = (ray->origin_start - ray->origin_end).crossProduct(ray->origin_end - pt);
	ray->normal.normalize();

	ray->inside_is_on_the_right = orientation;


#ifdef DEBUG
	// Only for debug 
	ray->camera_source_index = camera_source->getIndex();
#endif

	rays.push_back(ray);
}

/*
Generate the current set of rays based on the current edges.
*/
void RayGenerator::generateRays()
{
	int ray_count = rays.size();
	for (int i = 0; i < ray_count; ++i) // TODO: Remove the delete system and re-use rays
		delete(rays[i]);
	rays.clear();


	int segs = segment_starts->size();
	for (int i = 0; i < segs; ++i)
	{
		if ((i > 0) && (i < segs - 1))
		{
			// Remove edges staying alone (more efficient to omit here than to remove from the vectors of edges)
			if ((*segment_starts)[i] != (*segment_ends)[i - 1])
				if ((*segment_ends)[i] != (*segment_starts)[i + 1])
					continue;
		}

		// Add the ray with the coresponding data
		addRay((*segment_starts)[i] % cam_w, (*segment_starts)[i] / cam_w, (*segment_ends)[i] % cam_w, (*segment_ends)[i] / cam_w, (*segment_orientations)[i], cam_left_top, cam_direction);
	}
}


/*
A debug function used to display the complete rays and not the actual models only.
*/
void RayGenerator::visualizeRays(vector<int> * output_content, int length)
{
	output_content->clear();
	debug_quads.clear();

	// Prepare values
	vector3df origin = camera_source->getOrigin();
	quaternion cam_direction = camera_source->getDirection();
	vector3df direction_vector(cam_direction * vector3df(0, 0, 1));
	direction_vector.normalize();


	vector3df cam_left_top = origin + camera_source->getToCorner();
	vector3df left_bottom = cam_left_top + cam_direction * vector3df(0, -cam_h, 0);
	vector3df right_top = cam_left_top + cam_direction * vector3df(cam_w, 0, 0);
	vector3df right_bottom = cam_left_top + cam_direction * vector3df(cam_w, -cam_h, 0);

	// Add 4 crosses and an arrow showing the virtual camera plane
	add3DArrow(origin, direction_vector, 25, &debug_quads);
	add3DCross(cam_left_top, 10, &debug_quads);
	add3DCross(left_bottom, 10, &debug_quads);
	add3DCross(right_top, 10, &debug_quads);
	add3DCross(right_bottom, 10, &debug_quads);


	// Add all rays completely (not as the model segments)
	for (int i = 0; i < rays.size(); ++i)
	{
		rays[i]->addAsRayQuad(output_content, direction_vector, length, true);
	}

	// Add debug quads
	for (int i = 0; i < debug_quads.size(); ++i)
		output_content->push_back(debug_quads[i]);

}

vector<Ray3D*> * RayGenerator::getRays()
{
	return(&rays);
}

CameraSource * RayGenerator::getCameraSource()
{
	return(camera_source);
}