#pragma once

#include "simplifyingHeader.h"

#include "BackgroundReference.h"
#include "ContourSegment.h"



class ContoursExtractor
{
private:
	int noisepixel_tolerance;
	int contour_mask_size;

	uchar* bc;
	
	Mat * frame;
	uchar* f;

	Mat * tempormask;
	
	int pixelcount;
	int mask_pixelcount;
	int contour_mask_pixelcount;

	int frame_w, frame_h;
	int grid_w, grid_h;

	bool * contour_pixels = nullptr;

	int * contour_keypooints_grid = nullptr;
	int * inout_grid = nullptr;


	// references from other components

	bool * binaryMask;

public:
	ContoursExtractor();
	~ContoursExtractor();

	void initData(Mat * frame, Mat * background, bool * binaryMask);

	//void computeContoursPixels();
	void computeContour();

	int * getContourGrid();
	int * getInoutGrid();


	void previewInoutMask(cv::Mat * dest);
	void previewContourMask(cv::Mat * dest);
	void previewContourKeypointMask(Mat * dest);
};

