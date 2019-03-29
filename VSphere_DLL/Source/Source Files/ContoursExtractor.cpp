/*
This core class extracts contour pixels from the prepared background-less image.
Therefore it takes the prepared data from the BackgroundReference. Just like it it is also bound to exacty one camera.

Input (from BackgroundReference):
	BinaryMask pointer				// Bool array covering the frame image containing which pixels are background and which object

Output:
	contour_pixels pointer			// Array with pixels telling whetehr they are contour or not
	contour_keypooints_grid pointer // Array representing a grid on the image (it's size is determiend by Settings::getContourMaskSize())
									// The values tell which point inside every grid cell is the gravity center of all contour_pixels inside this cell.
									// Therefore those are the keypoints used later for computing the actual edges
	in_out_grid pointer				// Grid of ints telling how many pixels inside the cell were inside the object as perceived by the camera
									// This allows to differ later on which side of the contour the object lies and which side is the outline.


@Author: Alexander Georgescu
*/

#include "stdafx.h"


#include "ContoursExtractor.h"
#include "ContourSegment.h"

#include <ctime>


ContoursExtractor::ContoursExtractor()
{
	this->noisepixel_tolerance = Settings::getNoisepixelTolerance();
	this->contour_mask_size = Settings::getContourMaskSize();
}

ContoursExtractor::~ContoursExtractor()
{
	// Delete data / free memory

	if (contour_pixels != nullptr)
		delete[] contour_pixels;

	// Create the new contours grid
	if (contour_keypooints_grid != nullptr)
		delete[] contour_keypooints_grid;

	// Create the new in/out grid
	if (inout_grid != nullptr)
		delete[] inout_grid;
}


/*
Initialize environment data
*/
void ContoursExtractor::initData(Mat * frame, Mat * background, bool * binaryMask)
{
	this->frame = frame;

	this->binaryMask = binaryMask;

	// Get the pointer of the first pixel of the background Mat (this is the fastest way to access its contents)
	bc = background->ptr<uchar>(0);

	// Get the pointer of the first pixel of the frame Mat (this is the fastest way to access its contents)
	f = frame->ptr<uchar>(0);


	frame_w = frame->cols;
	frame_h = frame->rows;

	contour_mask_pixelcount = contour_mask_size*contour_mask_size;
	grid_w = frame_w / contour_mask_size;
	grid_h = frame_h / contour_mask_size;

	// Calculate number of total pixels for later
	pixelcount = background->cols * background->rows;

	cout << "Pixels per frame: " << pixelcount << endl;


	// Calculate number of elements in mask contours_grid
	mask_pixelcount = pixelcount / contour_mask_pixelcount;


	// Create the new binary contour pixel mask
	if (contour_pixels == nullptr)
		contour_pixels = new bool[pixelcount];

	// Create the new contours grid
	if (contour_keypooints_grid == nullptr)
		contour_keypooints_grid = new int[mask_pixelcount];

	// Create the new in/out grid
	if (inout_grid == nullptr)
		inout_grid = new int[mask_pixelcount];
}


/*
Compute the actual contours with the input and outputs as described.
*/
void ContoursExtractor::computeContour()
{
	int ps = 0;
	int contour_count = 0;
	int cenx = 0;
	int ceny = 0;

	for (int y = 0; y < frame_h; y += contour_mask_size)
	{
		for (int x = 0; x < frame_w; x += contour_mask_size)
		{
			contour_count = 0;
			cenx = 0;
			ceny = 0;

			//int ps = x / contour_mask_size + (y / contour_mask_size) * grid_w;

			inout_grid[ps] = 0;

			for (int xx = 0; xx < contour_mask_size; xx++)
			{
				for (int yy = 0; yy < contour_mask_size; yy++)
				{
					int i = (x + xx + (y + yy)*frame_w);

					if (binaryMask[i])
						inout_grid[ps]++;

					if ((binaryMask[i] != binaryMask[i - 1]) || (binaryMask[i] != binaryMask[i + 1])
						|| ((i > frame_w) && ((binaryMask[i] != binaryMask[i - frame_w]))))
					{
						contour_pixels[i] = true;

						cenx += xx;
						ceny += yy;
						contour_count++;
					}
					else
						contour_pixels[i] = false;
				}
			}

			if (contour_count >= noisepixel_tolerance)
			{
				cenx /= contour_count;
				ceny /= contour_count;
				contour_keypooints_grid[ps] = cenx + ceny*frame_w;
			}
			else
			{
				contour_keypooints_grid[ps] = -1;
			}

			ps++;
		}
	}

}




int * ContoursExtractor::getContourGrid()
{
	if (contour_keypooints_grid == nullptr)
	{
		StaticDebug::addError("Trying to use getContourGrid before any ContoursExtractor has been computed!");
		contour_keypooints_grid = new int[mask_pixelcount]; // prevent direct error
	}
	return(contour_keypooints_grid);
}

int * ContoursExtractor::getInoutGrid()
{
	if (inout_grid == nullptr)
	{
		StaticDebug::addError("Trying to use getInoutGrid before the ContoursExtractor has been computed!");
		inout_grid = new int[mask_pixelcount]; // prevent direct error
	}
	return(inout_grid);
}


/*
Draw the preview image based on the computed in out mask/grid
*/
void ContoursExtractor::previewInoutMask(Mat * dest)
{
	if ((dest->rows) * (dest->cols) != pixelcount)
		return;
	if (contour_pixels == nullptr)
		return;


	uchar* d = dest->ptr<uchar>(0);

	int val = 0;
	int j = 0;
	for (int x = 0; x < frame_w; x += contour_mask_size)
	{
		for (int y = 0; y < frame_h; y += contour_mask_size)
		{
			int ps = x / contour_mask_size + (y / contour_mask_size) * grid_w;

			for (int xx = 0; xx < contour_mask_size; xx++)
			{
				for (int yy = 0; yy < contour_mask_size; yy++)
				{
					val = 255 - ((inout_grid[ps] / (float)contour_mask_pixelcount)) * 255;
					j = (x+xx + (y+yy)*frame_w) * 3;
					d[j] = val;
					d[j + 1] = val;
					d[j + 2] = val;
				}
			}
		}
	}
}



/*
Draw a preivew image with the contour pixels
*/
void ContoursExtractor::previewContourMask(Mat * dest)
{
	if ((dest->rows) * (dest->cols) != pixelcount)
		return;
	if (contour_pixels == nullptr)
		return;


	uchar* d = dest->ptr<uchar>(0);

	int j = 0;
	for (int i = 1; i < pixelcount; i++)
	{
		if (contour_pixels[i])
		{
			d[j] = 255;
			d[j + 1] = 255;
			d[j + 2] = 255;
		}
		else
		{
			d[j] = 0;
			d[j + 1] = 0;
			d[j + 2] = 0;
		}

		j += 3;
	}
}


/*
Draw a preview with the keypoints.
*/
void ContoursExtractor::previewContourKeypointMask(Mat * dest)
{
	if ((dest->rows) * (dest->cols) != pixelcount)
		return;
	if (contour_pixels == nullptr)
		return;
	
	computeContour();

	uchar* d = dest->ptr<uchar>(0);

	int ps = 0;
	
	for (int y = 0; y < grid_h; ++y)
	{
		for (int x = 0; x < grid_w; ++x)
		{
			int vv = contour_keypooints_grid[ps++];

			if (vv != -1)
			{
				int resPix = x*contour_mask_size + (y*contour_mask_size)*frame_w + vv;

				d[resPix * 3] = 255;
				d[resPix * 3 + 1] = 255;
				d[resPix * 3 + 2] = 255;
			}
		}
	}

}

