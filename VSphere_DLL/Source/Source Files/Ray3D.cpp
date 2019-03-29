/*
This important struct represents a ray with a given width along an object-edge vertically on the camera's 2D port in the 3D space.
Therefore it touches exactly the surface of the virtually 'teleported' object.
It will be created by the RayGenerator and initialized externally with all it's values to avoid an additional function call
and to make it possible to re-use Rays instead of recreating them (that is a TODO).

Additionally this struct contains a vector of indices which represents the points along its height where it collided with other rays.
This data is computed and saved through the ModelComputer. In the end, the ModelComputer calls the addAsModelQuads() function of every Ray3D
which computes based on the intersections, the actual areas on the whole ray which represent the real surface of the final 3D-object.
The output are the quads and texture coordinates which will be transfered to the rendering engine.

@Author: Alexander Georgescu
*/

#pragma once

#include "stdafx.h"

#include "Ray3D.h"


/*
This function computes the quads for the surface of the 3D object based on the intersection data.
It's called by the ModelBuilder of every camera by computeModelPart().
*/
void Ray3D::addAsModelQuads(vector<int> * output, vector3df cam_direction, vector<float> * intersection_factor_start_x, vector<float> * intersection_factor_end_x, vector<float> * intersection_factor_start_y, vector<float> * intersection_factor_end_y, vector<bool> * is_visible_plane_starter, vector<int> * texture_coordinates)
{
	if (intersection_indices.size() <= 1) return;

	int quads_added = 0;
	int current_main_intersection_index = 0;

	int lind, ind;
	int inters = intersection_indices.size();


	/*
	This is the currently active variant which forms the quad from the complete width of the ray.
	That means it doesn't matter where along the _width_ of the ray, another ray has intersected, the quad will be fromed from its complete width.
	The information where the intersection has been along the whole length of the ray is given by the arrays
	intersection_factor_start_x and intersection_factor_end_x. Only that data is used currently.

	Technically the arrays intersection_factor_start_y and intersection_factor_end_y do contain the exact information of the itnersection
	along the width of the ray and tehrefore would allow to recompute the exact intersection.
	However to use those values for a complete model without holes, it requires more complex computations.
	An attempt for thatw as made. See the huge out-commented block furher below.
	*/
	int current_starter = -1;
	for (int i = 0; i < inters; ++i) // loop through all intersections
	{
		if (intersection_indices[i] == -1) continue;

		if (current_starter != -1) // If currently inside the object
		{
			if (!(*is_visible_plane_starter)[intersection_indices[i]]) // If the intersection is an end-intersection
			{
				lind = current_starter;
				ind = intersection_indices[i];

				// Add the quad based ont he current and the last index
				addQuad((*intersection_factor_start_x)[lind], 0,
						(*intersection_factor_end_x)[lind], ray_width,
						(*intersection_factor_start_x)[ind], 0,
						(*intersection_factor_end_x)[ind], ray_width,
						cam_direction, dir_along_y, output);


				// Add the coresponding textures
				addTexture(texture_coordinates, lind, ind, output);

				quads_added++;

				/*
				// Full
				addQuad((*intersection_factor_start_x)[lind], (*intersection_factor_start_y)[lind],
				(*intersection_factor_end_x)[lind], (*intersection_factor_end_y)[lind],
				(*intersection_factor_start_x)[ind], (*intersection_factor_start_y)[ind],
				(*intersection_factor_end_x)[ind], (*intersection_factor_end_y)[ind],
				cam_direction, dir_along_y, output);
				*/


				current_starter = -1; // Now again outside the plane
			}
		}
		else // If not inside the object; avaiting a starter intersection
		{
			if ((*is_visible_plane_starter)[intersection_indices[i]]) // Starter intersection occured
				current_starter = intersection_indices[i];
		}

	}

	/*
	The following is an incomplete possibility to use accurate intersection values.
	That is important in the case where multiple, differently angled rays cann collide one, large ray.
	That happens a lot if the "merge_optimizable_edges" functionality is used in the EdgesIdentifier.
	In those cases, the solution above is not usable because the quad would be rendered always rendered at it's whole width.

	To achieve the correct quads in those cases, the whole ray has to be split up for every intersection along its width and computed individually.
	So far this solutiond rastically icnreases computation effort but it might not be the best solution and computation is reduced in the ModelComputer as less rays mean less intersections.
	*/
	/*

		vector<bool> subray_inactive;

		vector<float> subray_factor_start_y;
		vector<float> subray_factor_end_y;

		vector<int> subray_base_intersection_index;


		int current_main_intersection_index = intersection_indices[0];

		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(current_main_intersection_index);

		subray_factor_start_y.push_back(intersection_factor_start_y[current_main_intersection_index]);
		subray_factor_end_y.push_back(intersection_factor_end_y[current_main_intersection_index]);

		if (false)
		for (int i = 0; i < intersection_indices.size(); ++i)
		{
		current_main_intersection_index = intersection_indices[i];

		vector3df last_start = origin_start + intersection_factor_start_x[current_main_intersection_index] * cam_direction
		+ intersection_factor_start_y[current_main_intersection_index] * dir_along_y;
		vector3df last_end = origin_start + intersection_factor_end_x[current_main_intersection_index] * cam_direction
		+ intersection_factor_end_y[current_main_intersection_index] * dir_along_y;


		StaticDebug::add3DCross(last_start, 6, output);
		StaticDebug::add3DCross(last_end, 6, output);
		}

		//return;


		for (int i = 1; i < intersection_indices.size(); ++i)
		{
		current_main_intersection_index = intersection_indices[i];

		float current_main_intersection_start_y = intersection_factor_start_y[current_main_intersection_index];
		float current_main_intersection_end_y = intersection_factor_end_y[current_main_intersection_index];

		// cout << "START Checking FOR from " << current_main_intersection_start_y << " to " << current_main_intersection_end_y << endl;

		bool nothing_added = true;
		int current_subrays = subray_inactive.size();
		int currently_added_subrays = 0;



		for (int j = subray_inactive.size() - 1; j >= 0; --j)
		{
		if (subray_inactive[j]) continue;

		float tocompare_intersection_start_y = subray_factor_start_y[j];
		float tocompare_intersection_end_y = subray_factor_end_y[j];

		if ((tocompare_intersection_end_y-min_width) <= current_main_intersection_start_y)
		{
		// cout << "A ABORTED CHECKING subray from " << tocompare_intersection_start_y << " to " << tocompare_intersection_end_y << " AGAINST main from " << current_main_intersection_start_y << " to " << current_main_intersection_end_y << endl;
		continue;
		}
		if ((tocompare_intersection_start_y+min_width) >= current_main_intersection_end_y)
		{
		// cout << "B ABORTED CHECKING subray from " << tocompare_intersection_start_y << " to " << tocompare_intersection_end_y << " AGAINST main from " << current_main_intersection_start_y << " to " << current_main_intersection_end_y << endl;
		continue;
		}

		subray_inactive[j] = true;

		// cout << "CHECKING subray from " << tocompare_intersection_start_y << " to " << tocompare_intersection_end_y << " AGAINST main from " << current_main_intersection_start_y << " to " << current_main_intersection_end_y << endl;

		float start, end;

		if (current_main_intersection_start_y > tocompare_intersection_start_y) // If the current intersection start is more to the right than the one to compare with
		{ // Add a new sub-ray segment from the edge of the origin ray to the new edge

		if ((current_main_intersection_start_y - tocompare_intersection_start_y) > min_width)
		{
		bool not_overlapping = true;
		for (int k = 0; k < currently_added_subrays; ++k)
		{
		if (((subray_factor_start_y[current_subrays + k] + min_width) >= subray_factor_start_y[j])
		&& ((intersection_factor_start_y[current_main_intersection_index] + min_width) >= subray_factor_end_y[current_subrays + k]))
		not_overlapping = false;
		}

		if (not_overlapping)
		{
		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(subray_base_intersection_index[j]);

		subray_factor_start_y.push_back(subray_factor_start_y[j]);
		subray_factor_end_y.push_back(intersection_factor_start_y[current_main_intersection_index]);

		// cout << "ADDING subray from " << subray_factor_start_y[j] << " to " << intersection_factor_start_y[current_main_intersection_index] << endl;
		nothing_added = false;

		currently_added_subrays++;
		}
		}
		// cout << "Exec 1" << endl;
		start = intersection_factor_start_y[current_main_intersection_index];
		}
		else
		{
		if ((tocompare_intersection_start_y - current_main_intersection_start_y) > min_width)
		{
		bool not_overlapping = true;
		for (int k = 0; k < currently_added_subrays; ++k)
		{
		if (((subray_factor_start_y[current_subrays + k] + min_width) >= intersection_factor_start_y[current_main_intersection_index])
		&& ((subray_factor_start_y[j] + min_width) >= subray_factor_end_y[current_subrays + k]))
		not_overlapping = false;
		}

		if (not_overlapping)
		{
		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(current_main_intersection_index);

		subray_factor_start_y.push_back(intersection_factor_start_y[current_main_intersection_index]);
		subray_factor_end_y.push_back(subray_factor_start_y[j]);

		// cout << "ADDING subray from " << intersection_factor_start_y[current_main_intersection_index] << " to " << subray_factor_start_y[j] << endl;
		nothing_added = false;

		currently_added_subrays++;
		}
		}
		// cout << "Exec 2" << endl;
		start = subray_factor_start_y[j];
		}


		if (current_main_intersection_end_y > tocompare_intersection_end_y) // If the current intersection width is more to the right than the one to compare with
		{ // Add a new sub-ray segment from the width-edge of comparing ray to the edge of the origin

		if ((current_main_intersection_end_y - tocompare_intersection_end_y) > min_width)
		{
		bool not_overlapping = true;
		for (int k = 0; k < currently_added_subrays; ++k)
		{
		if (((subray_factor_start_y[current_subrays + k] + min_width) >= subray_factor_end_y[j])
		&& ((intersection_factor_end_y[current_main_intersection_index] + min_width) >= subray_factor_end_y[current_subrays + k]))
		not_overlapping = false;
		}

		if (not_overlapping)
		{
		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(current_main_intersection_index);

		subray_factor_start_y.push_back(subray_factor_end_y[j]); // End of subray as start of new subray
		subray_factor_end_y.push_back(intersection_factor_end_y[current_main_intersection_index]); // End of current main ray

		// cout << "ADDING subray from " << subray_factor_end_y[j] << " to " << intersection_factor_end_y[current_main_intersection_index] << endl;
		nothing_added = false;

		currently_added_subrays++;
		}
		}
		// cout << "Exec 3" << endl;
		end = subray_factor_end_y[j];
		}
		else
		{
		if ((tocompare_intersection_end_y - current_main_intersection_end_y) > min_width)
		{
		bool not_overlapping = true;
		for (int k = 0; k < currently_added_subrays; ++k)
		{
		if (((subray_factor_start_y[current_subrays + k] + min_width) >= intersection_factor_end_y[current_main_intersection_index])
		&& ((subray_factor_end_y[j] + min_width) >= subray_factor_end_y[current_subrays + k]))
		not_overlapping = false;
		}

		if (not_overlapping)
		{
		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(subray_base_intersection_index[j]);

		subray_factor_start_y.push_back(intersection_factor_end_y[current_main_intersection_index]);
		subray_factor_end_y.push_back(subray_factor_end_y[j]);

		// cout << "ADDING subray from " << intersection_factor_end_y[current_main_intersection_index] << " to " << subray_factor_end_y[j] << endl;
		nothing_added = false;

		currently_added_subrays++;
		}

		}
		// cout << "Exec 4" << endl;
		end = intersection_factor_end_y[current_main_intersection_index];
		}

		if ((end - start) > min_width)
		if ((end - start) > min_width)
		{
		float fk1 = (intersection_factor_end_x[subray_base_intersection_index[j]] - intersection_factor_start_x[subray_base_intersection_index[j]]) / (intersection_factor_end_y[subray_base_intersection_index[j]] - intersection_factor_start_y[subray_base_intersection_index[j]]);
		float fk2 = (intersection_factor_end_x[current_main_intersection_index] - intersection_factor_start_x[current_main_intersection_index]) / (intersection_factor_end_y[current_main_intersection_index] - intersection_factor_start_y[current_main_intersection_index]);

		float x1 = intersection_factor_start_x[subray_base_intersection_index[j]] + (start - intersection_factor_start_y[subray_base_intersection_index[j]]) * fk1;
		float x2 = intersection_factor_start_x[subray_base_intersection_index[j]] + (end - intersection_factor_start_y[subray_base_intersection_index[j]]) * fk1;

		float x3 = intersection_factor_start_x[current_main_intersection_index] + (start - intersection_factor_start_y[current_main_intersection_index]) * fk2;
		float x4 = intersection_factor_start_x[current_main_intersection_index] + (end - intersection_factor_start_y[current_main_intersection_index]) * fk2;

		//if (quads_added == 3)
		{
		// cout << "added" << endl;
		addQuad(x1, start, x2, end, x3, start, x4, end, cam_direction, dir_along_y, output);
		}


		//addQuad(subray_factor_start_x[j], start, subray_factor_end_x[j], end, intersection_factor_start_x[current_main_intersection_index], start, intersection_factor_end_x[current_main_intersection_index], end, cam_direction, dir_along_y, output);

		quads_added++;
		}

		}

		//if (false)
		if (nothing_added)
		{
		// cout << "ADDING ORIGIN subray from " << intersection_factor_start_y[current_main_intersection_index] << " to " << intersection_factor_end_y[current_main_intersection_index] << endl;

		subray_inactive.push_back(false);
		subray_base_intersection_index.push_back(current_main_intersection_index);

		subray_factor_start_y.push_back(intersection_factor_start_y[current_main_intersection_index]);
		subray_factor_end_y.push_back(intersection_factor_end_y[current_main_intersection_index]);
		}

		}

		//cout << "Segment list size: " << subray_inactive.size() << endl;

		//cout << "Quads added for a ray: " << quads_added << endl;



		return;*/

}

/*
Adds an actual quad with the given coordinates to the output vector
*/
void Ray3D::addQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, vector3df cam_direction, vector3df dir_along_y, vector<int> * output)
{
	vector3df last_start = origin_start + x1 * cam_direction
		+ y1 * dir_along_y;
	vector3df last_end = origin_start + x2 * cam_direction
		+ y2 * dir_along_y;

	vector3df start = origin_start + x3 * cam_direction
		+ y3 * dir_along_y;
	vector3df end = origin_start + x4 * cam_direction
		+ y4 * dir_along_y;


	// Quad
	output->push_back((int)((last_start.X)*(float)100));
	output->push_back((int)((last_start.Y)*(float)100));
	output->push_back((int)((last_start.Z)*(float)100));

	output->push_back((int)((start.X)*(float)100));
	output->push_back((int)((start.Y)*(float)100));
	output->push_back((int)((start.Z)*(float)100));

	output->push_back((int)((last_end.X)*(float)100));
	output->push_back((int)((last_end.Y)*(float)100));
	output->push_back((int)((last_end.Z)*(float)100));

	output->push_back((int)((end.X)*(float)100));
	output->push_back((int)((end.Y)*(float)100));
	output->push_back((int)((end.Z)*(float)100));
}

/*
Add the texture coordinates to the output vector
*/
void Ray3D::addTexture(vector<int> * texture_coordinates, int last_index, int index, vector<int> * output)
{
	output->push_back((*texture_coordinates)[last_index * 4]);
	output->push_back((*texture_coordinates)[last_index * 4 + 1]);

	output->push_back((*texture_coordinates)[index * 4]);
	output->push_back((*texture_coordinates)[index * 4 + 1]);

	output->push_back((*texture_coordinates)[last_index * 4 + 2]);
	output->push_back((*texture_coordinates)[last_index * 4 + 3]);

	output->push_back((*texture_coordinates)[index * 4 + 2]);
	output->push_back((*texture_coordinates)[index * 4 + 3]);
}

/*
Add the entire ray with a given maximum length to the output vector (as a quad with 0.0 texture).
*/
void Ray3D::addAsRayQuad(vector<int> * output, vector3df cam_direction, float maxLength, bool show_orientation)
{

	float ldist = 0, dist = maxLength;

	output->push_back((int)((origin_start.X + cam_direction.X*ldist)*(float)100));
	output->push_back((int)((origin_start.Y + cam_direction.Y*ldist)*(float)100));
	output->push_back((int)((origin_start.Z + cam_direction.Z*ldist)*(float)100));

	output->push_back((int)((origin_start.X + cam_direction.X*dist)*(float)100));
	output->push_back((int)((origin_start.Y + cam_direction.Y*dist)*(float)100));
	output->push_back((int)((origin_start.Z + cam_direction.Z*dist)*(float)100));


	output->push_back((int)((origin_end.X + cam_direction.X*ldist)*(float)100));
	output->push_back((int)((origin_end.Y + cam_direction.Y*ldist)*(float)100));
	output->push_back((int)((origin_end.Z + cam_direction.Z*ldist)*(float)100));

	output->push_back((int)((origin_end.X + cam_direction.X*dist)*(float)100));
	output->push_back((int)((origin_end.Y + cam_direction.Y*dist)*(float)100));
	output->push_back((int)((origin_end.Z + cam_direction.Z*dist)*(float)100));


	for (int i = 0; i < 8; i++)
		output->push_back(0); // Add empty value for the texture

	if (show_orientation)
	{
		if (inside_is_on_the_right)
			StaticDebug::add3DArrow(origin, -normal, 15, output);
		else
			StaticDebug::add3DArrow(origin, normal, 15, output);
	}

}