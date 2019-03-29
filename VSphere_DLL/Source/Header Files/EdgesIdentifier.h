#pragma once

#include "simplifyingHeader.h"


#include "ContoursExtractor.h"

#include "ContourSegment.h"


class EdgesIdentifier
{
private:
	int contour_mask_size;

	ContoursExtractor * contours_extractor;

	int mask_pixelcount;
	int grid_cell_pixelcount;

	//int camera_list_index;

	int frame_w, frame_h;
	int grid_w, grid_h;

	vector<int> segments_start;
	vector<int> segments_end;
	vector<bool> segments_orientation;


	// references from other components

	int * contours_grid;
	int * inout_grid;

public:
	EdgesIdentifier();

	void initData(int frame_w, int frame_h, int * contours_grid, int * inout_grid);

	void computeEdges(bool optimise, valueBench * bench);


	vector<int> * getEdgesStarts();
	vector<int> * getEdgesEnds();
	vector<bool> * getEdgesOrientations();

	void previewEdges2D(cv::Mat * dest);
};