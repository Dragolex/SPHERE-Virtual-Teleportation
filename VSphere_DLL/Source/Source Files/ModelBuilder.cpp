/*
This core class uses the prepared rays from the RayGenerators associated to all cameras.
An instance of this exists for every camera (but is executed on their own thread).
Based on its own rays (from the RayGenerator associated to the same camera like this ModelBuilder)
it computes the intersections with all other rays of the current frame.
The result are several linear arrays with the required data about the intersection.
Additionally the index (the position in the mentioned arrays) of every intersection
is saved in the "intersection_indices" vector of every own ray.

When using only two cameras every collision will be computed twice thus computing more than required.
However this happens on different threads and memmory areas, preventing an overly significant lost of time and therefore no syncing between cores is required during the computation of the intersections.
When more than two cameras are used, the fraction which is computed multiple times is reduced.


Input (from RayGenerator):
	rays pointer	// Vector with rays produced by the input edges

Output:
	output_content	// Vector of ints representing the output model section (made out of quads)
					// for the camera associated to this ModelBuilder, inclduing the texture coordinates.
					// The sum of the output of all ModelBuilds forms the complete model but are independant of each other.

@Author: Alexander Georgescu
*/


#include "stdafx.h"

#include "ModelBuilder.h"


ModelBuilder::ModelBuilder()
{
	max_ray_length = Settings::getMaxRayLength();
}

ModelBuilder::~ModelBuilder()
{
	delete(cam_direction_vec);
	delete(cam_direction_vec_norm);

	for (int i = 0; i < other_cam_direction_vecs.size(); ++i)
		delete(other_cam_direction_vecs[i]);
}


/*
Adds a Ray Generator to the the internal list of associated ones.
Due to the fact that the orientation of all cameras are said to be constant, several values are precomputed here.

The argument "finalize_with_own_ray_generator" has only to be true when the provided ray_generator is exactly
that RayGenerator which is associated to the same camera like this ModelBuilder AND it is
the very last generator which will be added to this ModelBuilder.
The reason is that some of the pre-computed values depend on the own RayGenerator.
*/
void ModelBuilder::referenceAnotherRayGenerator(RayGenerator * ray_generator, bool finalize_with_own_ray_generator)
{
	if (finalize_with_own_ray_generator) // is last ray generator and the one of the same camera like this ModelBuilder
	{
		// Precompute several vector and a dot prodct (Todo: Perhaps simplify)
		cam_direction_vec = new vector3df(ray_generator->getCameraSource()->getDirection() * vector3df(0, 0, max_ray_length));
		cam_direction_vec_norm = new vector3df(cam_direction_vec->X, cam_direction_vec->Y, cam_direction_vec->Z);
		cam_direction_vec_norm->normalize();
		cam_direction_dot = cam_direction_vec->dotProduct(*cam_direction_vec);

		// Get the rays of that generator
		rays = ray_generator->getRays();

		vector3df * nn = new vector3df(ray_generator->getCameraSource()->getDirection() * vector3df(0, 0, 1));

		// Precompute some more values for every of the previously associated RayGenerators.
		for (int i = 0; i < other_ray_generators.size(); ++i)
		{
			quaternion other_dir = other_ray_generators[i]->getCameraSource()->getDirection();
			vector3df * other_dir_vec = new vector3df(other_ray_generators[i]->getCameraSource()->getDirection() * vector3df(0, 0, max_ray_length));


			other_rays.push_back(other_ray_generators[i]->getRays());
			other_cam_direction_vecs.push_back(other_dir_vec);
			other_cam_direction_dots.push_back(other_dir_vec->dotProduct(*other_dir_vec));

			ab_direction_dots.push_back(cam_direction_vec->dotProduct(*other_dir_vec));


			quotients.push_back(cam_direction_dot * other_cam_direction_dots.back() - ab_direction_dots.back()*ab_direction_dots.back());
		}
	}
	else
	{
		if (cam_direction_vec != nullptr)
			StaticDebug::addError("Referencing another ray generator after referencing the own one!");

		// Add the other RayGenerator to the list.
		other_ray_generators.push_back(ray_generator);
	}
}


/*
Perform the actual intersection of the rays based on the current values (automtically provide through the rays pointers).
*/
void ModelBuilder::intersectRays()
{
	debug_quads.clear();

	int intersect_ind = 0, local_intersections = 0;

	// This mechanism ensures that the data is not cleared while it is being accessed by the other thread using the data
	// This is required because the whole processing of the camera frames happens in two steps (see SpehreControler)
	// TODO: Use manually handled arrays instead of vectors to avoid unpredictable memmory-allocations
	if (currently_A)
	{
		intersection_factor_start_x = &A_intersection_factor_start_x;
		intersection_factor_start_y = &A_intersection_factor_start_y;
		intersection_factor_end_x = &A_intersection_factor_end_x;
		intersection_factor_end_y = &A_intersection_factor_end_y;
		intersection_represents_entering_real_surface = &A_is_visible_plane_starter;
		texture_coordinates = &A_texture_coordinates;
	}
	else
	{
		intersection_factor_start_x = &B_intersection_factor_start_x;
		intersection_factor_start_y = &B_intersection_factor_start_y;
		intersection_factor_end_x = &B_intersection_factor_end_x;
		intersection_factor_end_y = &B_intersection_factor_end_y;
		intersection_represents_entering_real_surface = &B_is_visible_plane_starter;
		texture_coordinates = &B_texture_coordinates;
	}

	// Data clearing
	intersection_factor_start_x->clear();
	intersection_factor_start_y->clear();
	intersection_factor_end_x->clear();
	intersection_factor_end_y->clear();
	intersection_represents_entering_real_surface->clear();
	texture_coordinates->clear();

	currently_A = !currently_A;
	////


	int colls = 0;
	int own_rays = rays->size();

	for (int k = 0; k < own_rays; ++k) // Loop through all own rays
	{
		intersection_values.clear();
		local_intersections = 0;

		int len1 = other_rays.size();
		for (int i = 0; i < len1; ++i) // Loop through all sets of other rays
		{
			int len2 = other_rays[i]->size();
			for (int j = 0; j < len2; ++j) // Loop through the rays of this set of other rays
			{

				// Compute the (squared) distance of the lines formed by the center of the two rays
				float dist = CustomMath::compute_line_distance((*other_rays[i])[j]->origin - (*rays)[k]->origin, *cam_direction_vec, *other_cam_direction_vecs[i], cam_direction_dot, other_cam_direction_dots[i], ab_direction_dots[i], quotients[i]);

				// If this distance is smaller than the sum of the height of the two rays
				if (dist < ((*rays)[k]->ray_width_sq + (*other_rays[i])[j]->ray_width_sq)) // An intersection is possible
				{
					vector3df line_orig, line_target;


					// Compute how the the planes of the two rays inetrsect each other.
					// The result is a line defined by line_orig and line_target.
					// In the following this line will be called "main intersection line"
					if (2 == CustomMath::compute_plane_collission((*rays)[k]->normal, (*other_rays[i])[j]->normal, (*rays)[k]->origin, (*other_rays[i])[j]->origin, &line_orig, &line_target))
					{ // Planes are not paralel -> intersection line computed						
							
						float pos_other_start = 0, pos_other_end = 0, pos_this_start = 0, pos_this_end = 0;

						// The direction of this main intersection line is computed.
						vector3df intersection_line_direction = (line_target - line_orig);

						//cout << "A dir x: " << line_target.X << " dir y: " << line_target.Y << " dir z: " << line_target.Z << endl;
						//cout << "B dir x: " << line_orig.X << " dir y: " << line_orig.Y << " dir z: " << line_orig.Z << endl;


						intersection_line_direction.normalize(); /// Todo: Maybe remove
						float dot_a = intersection_line_direction.dotProduct(intersection_line_direction);

#ifdef DEBUG_INTERSECTIONS
						// Size of the intersection crosses when debug is enabled
						int siz = 2;
#endif


						// Precompute values
						float b = intersection_line_direction.dotProduct((*other_cam_direction_vecs[i]));
						float D = (dot_a * other_cam_direction_dots[i] - b*b);

						if (D < 3000)
							continue; // If D is small, the lines are nearly parallel and collission is not significant -> continue with next intersection check


						// The following two lines compute where the main intersection-line (as computed before)
						// intersects with the the START EDGE of the OTHER Ray.
						vector3df origin_a_to_b = line_orig - (*other_rays[i])[j]->origin_start;
						pos_other_start = CustomMath::compute_line_collission_eff(origin_a_to_b, intersection_line_direction, *other_cam_direction_vecs[i], dot_a, other_cam_direction_dots[i], b, D);
#ifdef DEBUG_INTERSECTIONS 
						add3DCross(line_target + pos_other_start*direction_a, siz, &debug_quads);
#endif

						// The following two lines compute where the main intersection-line (as computed before)
						// intersects with the the END EDGE of the OTHER Ray.
						origin_a_to_b = line_orig - (*other_rays[i])[j]->origin_end;
						pos_other_end = CustomMath::compute_line_collission_eff(origin_a_to_b, intersection_line_direction, *other_cam_direction_vecs[i], dot_a, other_cam_direction_dots[i], b, D );
#ifdef DEBUG_INTERSECTIONS
						add3DCross(line_target + pos_other_end*direction_a, siz, &debug_quads);
#endif


						// Precompute values
						b = intersection_line_direction.dotProduct((*cam_direction_vec));
						D = (dot_a * cam_direction_dot - b*b);

						if (D < 3000)
							continue; // If D is small, the lines are nearly parallel and collission is not significant -> continue with next intersection check


						float this_ray_pos_start = 0;
						float this_ray_pos_end = 0;

						// The following two lines compute where the main intersection-line (as computed before)
						// intersects with the the START EDGE of the OWN Ray.
						origin_a_to_b = line_orig - (*rays)[k]->origin_start;
						pos_this_start = CustomMath::compute_line_collission_eff_full(origin_a_to_b, intersection_line_direction, *cam_direction_vec, dot_a, cam_direction_dot, b, D, &this_ray_pos_start);
#ifdef DEBUG_INTERSECTIONS
						add3DCross(line_target + pos_this_start*direction_a, siz, &debug_quads);
#endif


						// The following two lines compute where the main intersection-line (as computed before)
						// intersects with the the END EDGE of the OWN Ray.
						origin_a_to_b = line_orig - (*rays)[k]->origin_end;
						pos_this_end = CustomMath::compute_line_collission_eff_full(origin_a_to_b, intersection_line_direction, *cam_direction_vec, dot_a, cam_direction_dot, b, D, &this_ray_pos_end);
#ifdef DEBUG_INTERSECTIONS
						add3DCross(line_target + pos_this_end*direction_a, siz, &debug_quads);
#endif


						// The base values now are the following:
						// pos_this_start
						// pos_this_end
						// pos_other_start
						// pos_other_end
						// They are all factors along the main intersection line
						// and determine exactly how the the Rays are related locationwise to each other.


						//boolean inversed = false;
						//boolean other_inversed = false;

						// Swap the end with the start to always ensure that the start contains the smaller value
						if (pos_this_end < pos_this_start)
						{
							swap(pos_this_start, pos_this_end);
							//inversed = true;
						}
						if (pos_other_end < pos_other_start)
						{
							swap(pos_other_start, pos_other_end);
							//other_inversed = true;
						}
							

						boolean collided = true;


						/*
						The following code tries all the variants how the Rays can intersect each other
						and choses the right variant.

						The resulting values are the "intersection factors".
						There is an X factor which tells where along the length of the Ray the intersection occured.
						And there is an Y factor which tells where along the height of Ray it occured.
						Together this is the full information of the intersection for every ray.
						*/
	
						// THIS partially overlaps OTHER and PRECEDES: OWN_START <= OTHER_START <= OWN_END <= OTHER_END
						if ((pos_this_start <= pos_other_start) &&
							(pos_other_start <= pos_this_end) &&
							(pos_this_end <= pos_other_end))
						{
#ifdef DEBUG_MSG
							//if (own_rays == 1)
								cout << "Variant 1" << endl;
#endif
							intersection_factor_start_y->push_back((pos_other_start - pos_this_start));
							intersection_factor_end_y->push_back((pos_this_end - pos_this_start));
						}
						else

						// THIS partially overlaps OTHER and EXITS: OTHER_START <= OWN_START <= OTHER_END <= OWN_END
						if ((pos_other_start <= pos_this_start) &&
							(pos_this_start <= pos_other_end) &&
							(pos_other_end <= pos_this_end))
						{
#ifdef DEBUG_MSG
							//if (own_rays == 1)
								cout << "Variant 3" << endl;
#endif

							intersection_factor_start_y->push_back(0);
							intersection_factor_end_y->push_back((pos_other_end - pos_this_start));
						}
						else

						// THIS completely inside OTHER: OTHER_START <= OWN_START <= OWN_END <= OTHER_END
						if ((pos_other_start <= pos_this_start) &&
							(pos_this_start <= pos_this_end) &&
							(pos_this_end <= pos_other_end))
						{
#ifdef DEBUG_MSG
							//if (own_rays == 1)
								cout << "Variant 5" << endl;
#endif
							intersection_factor_start_y->push_back(0);
							intersection_factor_end_y->push_back((pos_this_end - pos_this_start));
						}
						else

						// OTHER completely inside THIS: OWN_START <= OTHER_START <= OTHER_END <= OWN_END
						if ((pos_this_start <= pos_other_start) &&
							(pos_other_start <= pos_other_end) &&
							(pos_other_end <= pos_this_end))
						{
#ifdef DEBUG_MSG						
							//if (own_rays == 1)
								cout << "Variant 7" << endl;
#endif

							intersection_factor_start_y->push_back((pos_other_start - pos_this_start));
							intersection_factor_end_y->push_back((pos_other_end - pos_this_start));
						}
						else
							collided = false; // No type of collission detected


						if (collided) // If there has been a real collision
						{
							colls++;

							// Compute the angle between the main intersection line and the direction of the camera
							double intersection_angle = atan2(
								intersection_line_direction.X*cam_direction_vec_norm->Y*(*rays)[k]->normal.Z + cam_direction_vec_norm->X*(*rays)[k]->normal.Y*intersection_line_direction.Z + (*rays)[k]->normal.X*intersection_line_direction.Y*cam_direction_vec_norm->Z - intersection_line_direction.Z*cam_direction_vec_norm->Y*(*rays)[k]->normal.X - cam_direction_vec_norm->Z*(*rays)[k]->normal.Y*intersection_line_direction.X - (*rays)[k]->normal.Z*intersection_line_direction.Y*cam_direction_vec_norm->X
								, intersection_line_direction.dotProduct(*cam_direction_vec_norm)
							);
								
							double intersection_sine = abs(sinf(intersection_angle));
							double intersection_cos = cosf(intersection_angle);


							// Adjust the X factors by the sine
							(*intersection_factor_start_y)[intersect_ind] *= intersection_sine;
							(*intersection_factor_end_y)[intersect_ind] *= intersection_sine;

							// Compute the X factors
							intersection_factor_start_x->push_back(this_ray_pos_start * max_ray_length + (*intersection_factor_start_y)[intersect_ind] * (abs(intersection_cos)));
							intersection_factor_end_x->push_back(this_ray_pos_end * max_ray_length + (*intersection_factor_start_y)[intersect_ind] * intersection_cos);


							// Correction if the difference between x values is larger than the height of the ray
							if (((*intersection_factor_start_x)[intersect_ind] - (*intersection_factor_end_x)[intersect_ind]) > (*rays)[k]->ray_width)
								(*intersection_factor_start_x)[intersect_ind] = (*intersection_factor_end_x)[intersect_ind] + (*rays)[k]->ray_width;
							else
								if (((*intersection_factor_end_x)[intersect_ind] - (*intersection_factor_start_x)[intersect_ind]) > (*rays)[k]->ray_width)
								(*intersection_factor_end_x)[intersect_ind] = (*intersection_factor_start_x)[intersect_ind] + (*rays)[k]->ray_width;
								
							// Correction of the Y values
							(*intersection_factor_start_y)[intersect_ind] = abs((*intersection_factor_start_y)[intersect_ind]);
							(*intersection_factor_end_y)[intersect_ind] = abs((*intersection_factor_end_y)[intersect_ind]);



							// Add the texture coordinates from the camera associated to the "other ray" which intersected with the own one.
							texture_coordinates->push_back((*other_rays[i])[j]->tex_start_x);
							texture_coordinates->push_back((*other_rays[i])[j]->tex_start_y);
							texture_coordinates->push_back((*other_rays[i])[j]->tex_end_x);
							texture_coordinates->push_back((*other_rays[i])[j]->tex_end_y);



							/*
							Compute how the own ray is oriented towards the camera of the other ray.
							This determines how the data is used which tells on which side
							of the ray the actual surface begins (the inside/outside classificiation computed by the EdgesIdentifier).
							The result is whether this intersection represents a point along the own Ray where it enters the surface of the real 3D object (true) or it leaves it (false).
							*/
							vector3df rel_pos = (*rays)[k]->origin_start - (*other_rays[i])[j]->origin_start;
							if ((rel_pos.dotProduct((*other_rays[i])[j]->normal)) > 0)
								intersection_represents_entering_real_surface->push_back((*other_rays[i])[j]->inside_is_on_the_right);
							else
								intersection_represents_entering_real_surface->push_back(!(*other_rays[i])[j]->inside_is_on_the_right);


							// Add the average of the start x and the end x to an additional array which will be used to order the intersections
							intersection_values.push_back(((*intersection_factor_start_x)[intersect_ind] + (*intersection_factor_start_x)[intersect_ind])/2);


							// Add the index of the current intersection to the list of the own array
							// Todo: Replace this vector by a managed array
							(*rays)[k]->intersection_indices.push_back(intersect_ind);

							intersect_ind++;
							local_intersections++;
						}
					}
				}
			}
		}

		/*
		The order in which the rays of the other cameras have intersected with this (own) ray is not predictable.
		To actually compute the quads forming the real surface of the 3D object it is important to be ordered along the length of the Ray.
		Therefore the following code sorts the indices by using the values saved in intersection_values[].
		*/
		bool resort = true;
		while (resort)
		{
			resort = false;
			
			int main_ind = intersect_ind - local_intersections + 1;

			for (int local_ind = 1; local_ind < local_intersections; local_ind++)
			{
				/*
				float min_dif = 10;
				if ((*rays)[k]->intersection_indices[local_ind] == -1) continue; // skip
				if (abs((intersection_values)[local_ind] - (intersection_values)[local_ind - 1]) < min_dif)
				{
					(*rays)[k]->intersection_indices[local_ind] = -1; // Remove this intersection
				}
				else
				*/

				if (intersection_values[local_ind] < intersection_values[local_ind - 1])
				{
					swap(intersection_values[local_ind], intersection_values[local_ind - 1]);
					swap((*rays)[k]->intersection_indices[local_ind], (*rays)[k]->intersection_indices[local_ind - 1]);

					resort = true;
				}

				main_ind++;
			}
		}


		/*
		// Remove senseless intersections where two starts are following one after another.
		// This is currently disabled because those cases are handled automatically when rendering the rays.

		bool inside_object = false;
		for (int l = 1; l < local_intersections; ++l)
		{
			if ((*rays)[k]->intersection_indices[l] == -1) continue; // skip

			if (inside_object)
			{
				if ((*is_visible_plane_starter)[(*rays)[k]->intersection_indices[l]]) // If the intersection is a starter
					(*rays)[k]->intersection_indices[l] = -1; // remove the intersection because we are already inside
				else
					inside_object = false;
			}
			else
			{
				if (!(*is_visible_plane_starter)[(*rays)[k]->intersection_indices[l]]) // If it is not a starter but an end-intersection
					(*rays)[k]->intersection_indices[l] = -1; // remove this intersection because we are still outside of the object
				else
					inside_object = true;
			}

		}
		*/

#ifdef DEBUG_MSG 
		cout << "Frame computed with " << colls << "collisions" << endl;
#endif

	}
	

}


/*
Causes the own rays to compute the actual quads and add them to the vector of ints which is the final output
*/
void ModelBuilder::computeModelPart(vector<int> * output_content)
{
	output_content->clear();

	for (int i = 0; i < (*rays).size(); ++i)
	{
		(*rays)[i]->addAsModelQuads(output_content, *cam_direction_vec_norm, intersection_factor_start_x, intersection_factor_end_x, intersection_factor_start_y, intersection_factor_end_y, intersection_represents_entering_real_surface, texture_coordinates);
	}

	for (int i = 0; i < debug_quads.size(); ++i)
		output_content->push_back(debug_quads[i]);
}
