/*
This core class computes the edges between the keypoints forming the contour.

Input (from ContourExtractor):
	contour_pixels pointer			// Array with pixels telling whetehr they are contour or not
	contour_keypooints_grid pointer // Array representing a grid on the image (it's size is determiend by Settings::getContourMaskSize())
									// The values tell which point inside every grid cell is the gravity center of all contour_pixels inside this cell.
									// Therefore those are the keypoints used later for computing the actual edges
	in_out_grid pointer				// Grid of ints telling how many pixels inside the cell were inside the object as perceived by the camera
									// This allows to differ later on which side of the contour the object lies and which side is the outline.
Output:
	segment_starts pointer			// int representing the linear coordinate (if the image were just one-dimensional) of the start point of the segments
	segment_ends pointer			// int representing the linear coordinate (if the image were just one-dimensional) of the end point of the segments
	segment_orientations pointer	// Boolean array how the edge is oriented
									// True if the inside of the object is on the right
									// False if it is on the left.
									// Left and right refers to when looking from the start to the end point.


@Author: Alexander Georgescu
*/

#include "stdafx.h"


#include "EdgesIdentifier.h"
#include "ContourSegment.h"

#include <ctime>


EdgesIdentifier::EdgesIdentifier()
{
	this->contour_mask_size = Settings::getContourMaskSize();
}

/*
Initialize environment data
*/
void EdgesIdentifier::initData(int frame_w, int frame_h, int * contours_grid, int * inout_grid)
{
	this->contours_grid = contours_grid;
	this->inout_grid = inout_grid;

	this->frame_w = frame_w;
	this->frame_h = frame_h;
	
	grid_w = frame_w / contour_mask_size;
	grid_h = frame_h / contour_mask_size;


	// Calculate number of elements in mask contours_grid
	grid_cell_pixelcount = contour_mask_size*contour_mask_size;
	mask_pixelcount = (frame_w * frame_h) / grid_cell_pixelcount;


	// Todo: Put value in settings
	segments_start.reserve(500);
	segments_end.reserve(500);
	segments_orientation.reserve(500);
}


/*
Compute the new set of results with the current input in the mentioned input pointers.
Arguments:
	merge_optimizable_edges -> if true it tries to merge connected edges with similar direction.
							   This greatly reduces the number of edges and therefore later calculations at intersection.
							   However this requires a different approach when computing the 3D model from the rays.
							   That method _might_ drastically increase total computation time again.
							   To elaborate this is the most significant TODO in this project.
							   Look into DataStructs -> Ray3D for more information and an incomplete attempt.
							   Current this optimization is disabled.
	bench -> a bench instance helpful for showing information about how many segments have been computed on average over time
*/
void EdgesIdentifier::computeEdges(bool merge_optimizable_edges, valueBench * bench)
{
	segments_start.clear();
	segments_end.clear();
	segments_orientation.clear();


	int disp = 0;
	int gridcord;
	int repeats = 0;
	int segment_end;

	vector<int> gridcord_q;
	vector<int> segment_start_q;
	vector<int> directions_q;


	/*
	To understand the following algorithm better, read the Thesis associated to this project. Chapter Edges detection.
	The algorithm described there has a recursive description. However to reduce computation and memmory access it has been implemented lineary based on vectors using push_back() and pop().
	Todo: Replace vectors by manually handled arrays to reduce possible memmory allocations.
	*/

	// Loop through all grid cells of the contours grid (mask)
	for (int i = 0; i < mask_pixelcount; i += 1)
	{
		if (contours_grid[i] != -1) // If it has a value and has not been handled already
		{
			gridcord_q.push_back(i);

			int segment_start = ((i % grid_w) + (i / grid_w)*frame_w)*contour_mask_size + contours_grid[i];

			segment_start_q.push_back(segment_start);

			int last_direction = -1;
			int direction = -1;

			directions_q.push_back(direction);

			// While there are still coordinates in the grid to check
			while (!gridcord_q.empty())
			{
				// Get the data of the current iteration
				gridcord = gridcord_q.back();
				gridcord_q.pop_back();

				segment_start = segment_start_q.back();
				segment_start_q.pop_back();

				direction = directions_q.back();
				directions_q.pop_back();
				//

				// If the current position has already been handled or is not a keypoint
				if (contours_grid[gridcord] == -1)
					continue;
				else
				{
					repeats = 0;

					if (direction == -1) // The direction has not been set yet -> This is the start, meaning all directions need to be checked
										 // "Checking" means that the system looks whether the next cell in that direction around the current cell (8 possible directions) is another keypoint.
										 // If at least a keypoint has been found an edge is assumed between the current keypoint and that one.
					{
						if ((gridcord % grid_w) != (grid_w - 1)) // x < w
							if ((gridcord / grid_w) != (grid_h - 1))
								if ((gridcord % grid_w) != 0) // x > 0
									if ((gridcord / grid_w) != 0)
									{
										if (contours_grid[gridcord + 1] != -1)
										{
											gridcord_q.push_back(gridcord + 1);
											directions_q.push_back(0);
											repeats++;
										}

										if (contours_grid[gridcord + 1 + grid_w] != -1)
										{
											gridcord_q.push_back(gridcord + 1 + grid_w);
											directions_q.push_back(1);
											repeats++;
										}

										if (contours_grid[gridcord + grid_w] != -1)
										{
											gridcord_q.push_back(gridcord + grid_w);
											directions_q.push_back(2);
											repeats++;
										}

										if (contours_grid[gridcord - 1 + grid_w] != -1)
										{
											gridcord_q.push_back(gridcord - 1 + grid_w);
											directions_q.push_back(3);
											repeats++;
										}
									}
					}
					else // Otherwise this is not the first segment tested and therefore the last segment alread has a known direction
						 // Based on this direction, only similar possible directions are checked for keypoints.
						 // Reason is that it is simply more likely that a contour continues in the same direction.
						 // No contour is lost however because only visited grid cells are set to -1 and
						 // the outer loop does not quit until all cells have been visited.
						 // Segment-chains which are too short and therefore senseless will be omitted later.
					{
						switch (direction)
						{
						case 0:
							if ((gridcord % grid_w) != (grid_w - 1)) // x < w
							{

								if (contours_grid[gridcord + 1] != -1)
								{
									gridcord_q.push_back(gridcord + 1);
									directions_q.push_back(0);
									repeats++;
								}

								if ((gridcord / grid_w) != (grid_h - 1)) // y < h
								{
									if (contours_grid[gridcord + 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 + grid_w);
										directions_q.push_back(1);
										repeats++;
									}
								}

								if ((gridcord / grid_w) != 0) // y > 0
								{
									if (contours_grid[gridcord + 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 - grid_w);
										directions_q.push_back(7);
										repeats++;
									}
								}
							}
							break;

						case 1:
							if ((gridcord % grid_w) != (grid_w - 1)) // x < w
							{
								if ((gridcord / grid_w) != (grid_h - 1)) // y < h
								{
									if (contours_grid[gridcord + 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 + grid_w);
										directions_q.push_back(1);
										repeats++;
									}

									if (contours_grid[gridcord + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + grid_w);
										directions_q.push_back(2);
										repeats++;
									}
								}

								if (contours_grid[gridcord + 1] != -1)
								{
									gridcord_q.push_back(gridcord + 1);
									directions_q.push_back(0);
									repeats++;
								}
							}
							break;

						case 2:
							if ((gridcord / grid_w) != (grid_h - 1))
							{
								if ((gridcord % grid_w) != (grid_w - 1)) // x < w
								{
									if (contours_grid[gridcord + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + grid_w);
										directions_q.push_back(2);
										repeats++;
									}

									if (contours_grid[gridcord + 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 + grid_w);
										directions_q.push_back(1);
										repeats++;
									}
								}

								if ((gridcord % grid_w) != 0)
								{
									if (contours_grid[gridcord - 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 + grid_w);
										directions_q.push_back(3);
										repeats++;
									}
								}
							}
							break;

						case 3:
							if ((gridcord % grid_w) != 0) // x > 0
							{
								if ((gridcord / grid_w) != (grid_h - 1)) // y < h
								{
									if (contours_grid[gridcord - 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 + grid_w);
										directions_q.push_back(3);
										repeats++;
									}

									if (contours_grid[gridcord + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + grid_w);
										directions_q.push_back(2);
										repeats++;
									}
								}

								if (contours_grid[gridcord - 1] != -1)
								{
									gridcord_q.push_back(gridcord - 1);
									directions_q.push_back(4);
									repeats++;
								}
							}
							break;

						case 4:
							if ((gridcord % grid_w) != 0)
							{
								if (contours_grid[gridcord - 1] != -1)
								{
									gridcord_q.push_back(gridcord - 1);
									directions_q.push_back(4);
									repeats++;
								}

								if ((gridcord / grid_w) != (grid_h - 1))
								{
									if (contours_grid[gridcord - 1 + grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 + grid_w);
										directions_q.push_back(3);
										repeats++;
									}
								}

								if ((gridcord / grid_w) != 0) // y > 0
								{
									if (contours_grid[gridcord - 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 - grid_w);
										directions_q.push_back(5);
										repeats++;
									}
								}
							}
							break;

						case 5:
							if ((gridcord / grid_w) != 0)
							{
								if ((gridcord % grid_w) != 0)
								{
									if (contours_grid[gridcord - 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 - grid_w);
										directions_q.push_back(5);
										repeats++;
									}

									if (contours_grid[gridcord - 1] != -1)
									{
										gridcord_q.push_back(gridcord - 1);
										directions_q.push_back(4);
										repeats++;
									}
								}

								if (contours_grid[gridcord - grid_w] != -1)
								{
									gridcord_q.push_back(gridcord - grid_w);
									directions_q.push_back(6);
									repeats++;
								}
							}
							break;

						case 6:
							if ((gridcord / grid_w) != 0)
							{
								if (contours_grid[gridcord - grid_w] != -1)
								{
									gridcord_q.push_back(gridcord - grid_w);
									directions_q.push_back(6);
									repeats++;
								}

								if ((gridcord % grid_w) != (grid_w - 1))
								{
									if (contours_grid[gridcord + 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 - grid_w);
										directions_q.push_back(7);
										repeats++;
									}
								}

								if ((gridcord % grid_w) != 0)
								{
									if (contours_grid[gridcord - 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - 1 - grid_w);
										directions_q.push_back(5);
										repeats++;
									}
								}
							}
							break;

						case 7:
							if ((gridcord % grid_w) != (grid_w - 1))
							{
								if ((gridcord / grid_w) != 0)
								{
									if (contours_grid[gridcord + 1 - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord + 1 - grid_w);
										directions_q.push_back(7);
										repeats++;
									}

									if (contours_grid[gridcord - grid_w] != -1)
									{
										gridcord_q.push_back(gridcord - grid_w);
										directions_q.push_back(6);
										repeats++;
									}
								}

								if (contours_grid[gridcord + 1] != -1)
								{
									gridcord_q.push_back(gridcord + 1);
									directions_q.push_back(0);
									repeats++;
								}
							}
							break;
						}
					}


					//if ((last_direction != direction) || (depth > 20)) // This disabled line is part of a simplified optimization system (for optimization, see the top of this function)
					{
						if ((direction != -1))// && (depth > 3))  |  Direction not being -1 means that this cannot be the very first tested keypoint -> continue to save this edge as valid
						{
							// The following code determines the orientation of the segment
							// The orientation tells whether the actual inside of the object is on the right (true) or the left (false).
							// For this reason it uses the data from the inout_grid.

							// This value helps preventing senseless edge-segments
							// When the values of the cells in the inout_grid around the segments are too similar
							// (difference between them smaller than max_dif), then this segment will be omitted.
							// This happens when the keypoint appears to be surrounded by opaque object or completely outside of the object.
							int max_dif = grid_cell_pixelcount / 3; // Todo: Perhaps add this as another setting

							bool valid_segment = true;

							if ((gridcord > (grid_h*grid_w) - 2 * grid_w - 3) || (gridcord < 2 * grid_w + 3)) // If this segment is already at the very edge of the screen, skip (reason is to avoid the values from exceeding the array; alternative would be having dozens of additional comparisions against the width and height)
								valid_segment = false;
							else
								switch (direction)
								{
								case 0: // Horizontal to the right
									if (abs(inout_grid[gridcord + grid_w] - inout_grid[gridcord - grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord + grid_w] > inout_grid[gridcord - grid_w]);
									else
										if (abs(inout_grid[gridcord + 2 * grid_w] - inout_grid[gridcord - 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord + 2 * grid_w] > inout_grid[gridcord - 2 * grid_w]);
										else valid_segment = false;
										break;
								case 4: // Horizontal to the left
									if (abs(inout_grid[gridcord + grid_w] - inout_grid[gridcord - grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord + grid_w] < inout_grid[gridcord - grid_w]);
									else
										if (abs(inout_grid[gridcord + 2 * grid_w] - inout_grid[gridcord - 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord + 2 * grid_w] < inout_grid[gridcord - 2 * grid_w]);
										else valid_segment = false;
										break;

								case 1: // Right-Down
									if (abs(inout_grid[gridcord - 1 + grid_w] - inout_grid[gridcord + 1 - grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1 + grid_w] > inout_grid[gridcord + 1 - grid_w]);
									else
										if (abs(inout_grid[gridcord - 2 + 2 * grid_w] - inout_grid[gridcord + 2 - 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2 + 2 * grid_w] > inout_grid[gridcord + 2 - 2 * grid_w]);
										else valid_segment = false;
										break;
								case 5: // Left-Up
									if (abs(inout_grid[gridcord - 1 + grid_w] - inout_grid[gridcord + 1 - grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1 + grid_w] < inout_grid[gridcord + 1 - grid_w]);
									else
										if (abs(inout_grid[gridcord - 2 + 2 * grid_w] - inout_grid[gridcord + 2 - 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2 + 2 * grid_w] < inout_grid[gridcord + 2 - 2 * grid_w]);
										else valid_segment = false;
										break;

								case 2: // Downwards
									if (abs(inout_grid[gridcord - 1] - inout_grid[gridcord + 1]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1] > inout_grid[gridcord + 1]);
									else
										if (abs(inout_grid[gridcord - 2] - inout_grid[gridcord + 2]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2] > inout_grid[gridcord + 2]);
										else valid_segment = false;
										break;
								case 6: // Upwards
									if (abs(inout_grid[gridcord - 1] - inout_grid[gridcord + 1]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1] < inout_grid[gridcord + 1]);
									else
										if (abs(inout_grid[gridcord - 2] - inout_grid[gridcord + 2]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2] < inout_grid[gridcord + 2]);
										else valid_segment = false;
										break;

								case 3: // Left-Down
									if (abs(inout_grid[gridcord - 1 - grid_w] - inout_grid[gridcord + 1 + grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1 - grid_w] > inout_grid[gridcord + 1 + grid_w]);
									else
										if (abs(inout_grid[gridcord - 2 - 2 * grid_w] - inout_grid[gridcord + 2 + 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2 - 2 * grid_w] > inout_grid[gridcord + 2 + 2 * grid_w]);
										else valid_segment = false;
										break;
								case 7: // Right-Up
									if (abs(inout_grid[gridcord - 1 - grid_w] - inout_grid[gridcord + 1 + grid_w]) > max_dif)
										segments_orientation.push_back(inout_grid[gridcord - 1 - grid_w] < inout_grid[gridcord + 1 + grid_w]);
									else
										if (abs(inout_grid[gridcord - 2 - 2 * grid_w] - inout_grid[gridcord + 2 + 2 * grid_w]) > max_dif)
											segments_orientation.push_back(inout_grid[gridcord - 2 - 2 * grid_w] < inout_grid[gridcord + 2 + 2 * grid_w]);
										else valid_segment = false;
										break;
								}

							// Calculate the linear value for the end. It is formed from corner of every grid cell when mapped on the image plus the offset of the keypoint in that cell
							segment_end = ((gridcord % grid_w) + (gridcord / grid_w)*frame_w)*contour_mask_size + contours_grid[gridcord];


							// If still a valid segment
							if (valid_segment)
							{
								segments_start.push_back(segment_start);
								segments_end.push_back(segment_end);
							}

							// Add the segment_end as the next segment start to continue from there
							REPEAT(repeats)
								segment_start_q.push_back(segment_end);
						}
						else
							// Add the segment_start as the next segment start to continue from there
							REPEAT(repeats)
								segment_start_q.push_back(segment_start);
					}
					/*
					// This disabled line is part of a simplified optimization system (for optimization, see the top of this function)
					else
					REPEAT(repeats)
					segment_start_q.push_back(segment_start);
					*/

					contours_grid[gridcord] = -1;
				}


			}

			//cout << "gridcord: " << gridcord_q.size() << endl;
			//cout << "segment_start_q: " << segment_start_q.size() << endl;
			//cout << "directions_q: " << directions_q.size() << endl;
		}
	}

	// optimization through merging
	if (merge_optimizable_edges) // Currently disabled in this example but it is possible to activate it by commenting this line out and see the preview image of the resulting edges
	{
		int segs = segments_start.size();

		float tol = Settings::getSegmentOptimisationTolerance();

		int begin_x = 0, begin_y = 0, begin, seg_pos = 0;
		bool keep = true;

		int end_x, end_y;

		// Loop through all segments
		for (int i = 0; i < segs; ++i)
		{
			end_x = (segments_end[i] % frame_w);
			end_y = (segments_end[i] / frame_w);

			// If not the first segment and the accurate direction (not the classification into 8 neighbours) is larger than "tol"
			if ((i == 0) || (segments_end[i - 1] != segments_start[i]) || (abs(atan2(end_y - (segments_start[i] / frame_w), end_x - (segments_start[i] % frame_w)) - atan2(end_y - begin_y, end_x - begin_x)) > tol))
			{
				if ((i > 0) && keep) // If the segments should be kept
				{
					segments_start[seg_pos] = begin; // Add them as a single one
					segments_end[seg_pos] = segments_end[i - 1];
					++seg_pos;
				}

				begin = segments_start[i];
				begin_x = (begin % frame_w);
				begin_y = (begin / frame_w);

				if ((i == 0) || (segments_end[i - 1] != segments_start[i]))
					keep = false; // Do not keep following the segment anymore
			}
			else keep = true; // Keep this segment
		}

		segments_start.resize(seg_pos);
		segments_end.resize(seg_pos);
	}

	// Add to the bench
	if (segments_start.size() != 0)
		bench->addValue(segments_start.size());
}


vector<int> * EdgesIdentifier::getEdgesStarts()
{
	return(&segments_start);
}

vector<int> * EdgesIdentifier::getEdgesEnds()
{
	return(&segments_end);
}

vector<bool> * EdgesIdentifier::getEdgesOrientations()
{
	return(&segments_orientation);
}


/*
Preview visualisation of the edges including their orientation
*/
void EdgesIdentifier::previewEdges2D(Mat * dest)
{
	int segs = (int)segments_start.size();

	for (int i = 0; i < segs; i++)
	{
		// Transfer the lienar int values into real coordinates on the image
		vector2df start(segments_start[i] % frame_w, (int)(segments_start[i] / frame_w));
		vector2df end(segments_end[i] % frame_w, (int)(segments_end[i] / frame_w));

		/*
		line(*dest, Point(start.X - 2, start.Y), Point(start.X + 2, start.Y), Scalar(255, 255, 255), 1, CV_AA); // CV_AA
		line(*dest, Point(start.X, start.Y - 2), Point(start.X, start.Y + 2), Scalar(255, 255, 255), 1, CV_AA); // CV_AA
		line(*dest, Point(end.X - 2, end.Y), Point(end.X + 2, end.Y), Scalar(255, 255, 255), 1, CV_AA); // CV_AA
		line(*dest, Point(end.X, end.Y - 2), Point(end.X, end.Y + 2), Scalar(255, 255, 255), 1, CV_AA); // CV_AA
		*/

		float angle = atan2(start.Y - end.Y, start.X - end.X) * 180 / PI;

		vector2df dir = end - start;
		start += dir / 2;

		int len = contour_mask_size;

		vector2df end1, end2;

		if (segments_orientation[i])
			end1 = start + vector2df(CustomMath::lengthdir_x(len, angle + 70), -CustomMath::lengthdir_y(len, angle + 70));
		else
			end1 = start + vector2df(CustomMath::lengthdir_x(len, angle - 70), -CustomMath::lengthdir_y(len, angle - 70));

		if (segments_orientation[i])
			end2 = start + vector2df(CustomMath::lengthdir_x(len, angle + 110), -CustomMath::lengthdir_y(len, angle + 110));
		else
			end2 = start + vector2df(CustomMath::lengthdir_x(len, angle - 110), -CustomMath::lengthdir_y(len, angle - 110));


		// Draw the orientation lines
		line(*dest, Point(start.X, start.Y), Point(end1.X, end1.Y), Scalar(150, 150, 200), 1, CV_AA); // CV_AA
		line(*dest, Point(start.X, start.Y), Point(end2.X, end2.Y), Scalar(150, 150, 200), 1, CV_AA); // CV_AA


		start = vector2df(segments_start[i] % frame_w, (int)(segments_start[i] / frame_w));
		end = vector2df(segments_end[i] % frame_w, (int)(segments_end[i] / frame_w));
		
		// Draw the contour line
		line(*dest, Point(start.X, start.Y), Point(end.X, end.Y), Scalar(100, 170, 0), 1, CV_AA); // CV_AA
		line(*dest, Point(start.X + 1, start.Y + 1), Point(end.X + 1, end.Y + 1), Scalar(100, 170, 0), 1, CV_AA); // CV_AA
	}
}