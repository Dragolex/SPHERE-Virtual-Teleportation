/*
Some global settings.

TODO:
	Enable to adjust more of them from outside the DLL.
	Perhaps extract the settings related to camera processing to another class individually attached to one certain camera (to enable different processings ettings for every camera)

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "Settings.h"


// Modifiable settings

int Settings::preview_type;
int Settings::max_preview_types;
vector<string> Settings::preview_type_text;

int Settings::preview_window_variant;
int Settings::preview_window_order_offset;


void Settings::init()
{
	// preview type at launch
	preview_type = 1;
	preview_window_variant = 1;
	preview_window_order_offset = 0;


	preview_type_text = {
		"Preview disabled",
		"Original live image",
		"Dynamic background reference",
		"Live image without background (by RGB)",
		"Binary mask",
		"In or Out mask grid",
		"Contours mask",
		"Contour keypoints",
		"Contour segments",
		"Contour segments as overlay"
	};

	max_preview_types = preview_type_text.size();
}

////// Camera processing settings: ///////

/*
Number of frames to generate the backgroundReference from.
*/
int Settings::getBackgroundReferenceComputingFrames()
{
	return(20);
}

/*
The tolerance in absolute color value between 1 and 255 which classifies a pixel between actual object and background when comparing a frame with the backgroundReference.
See Frameprocessing -> BackgroundReference.cpp for its usage.
*/
int Settings::getBackgroundColorTolerance()
{
	return(35); // 35 //22 // 50
}

/*
The width and height of a cell where contour keypoints are found.
See Frameprocessing -> ContourExtractor and EdgesIdentifier for its usage.
Attention, not all values possible.
*/
int Settings::getContourMaskSize()
{
	return(8); // 4
}

/*
How many non-background pixels shall be ignored inside every cell
*/
int Settings::getNoisepixelTolerance()
{
	return(5); // 3 // 5
}

/*
If edges-merging-optimisation is enabled, how the maximum angle difference is tolerated (in radian)
See Frameprocessing -> EdgesIdentifier and look for "merge_optimizable_edges" to see its usage.
*/
float Settings::getSegmentOptimisationTolerance()
{
	return(0.25f); //0.35f
}

/*
How long the rays can be
*/
int Settings::getMaxRayLength()
{
	return(640);
}
///////////


float Settings::getMaxPreviewTypes()
{
	return(max_preview_types);
}

int Settings::getThreadTimeoutMS()
{
	return(400);
}

float Settings::getPreviewScaleFactor()
{
	return(0.3333);
}




int Settings::getPreviewType()
{
	return(preview_type);
}
string Settings::getPreviewString()
{
	return(preview_type_text[preview_type]);
}

int Settings::getPreviewWindowVariant()
{
	return(preview_window_variant);
}

int Settings::getPreviewWindowOrderOffset()
{
	return(preview_window_order_offset);
}




// Set a new preview type (or -1 to go to next; -2 to return to last )
void Settings::changePreviewType(int new_type)
{
	if (new_type == -1)
		new_type = preview_type + 1;
	if (new_type == -2)
		new_type = preview_type - 1;

	if (new_type >= max_preview_types)
		new_type = 0;
	if (new_type < 0)
		new_type = max_preview_types - 1;

	preview_type = new_type;
}

// Set a new preview window offset (or -1 to go to next; -2 to return to last )
void Settings::changePreviewWindowOrderOffset(int offset, int maximum)
{
	if (offset == -1)
		offset = preview_window_order_offset + 1;
	if (offset == -2)
		offset = preview_window_order_offset - 1;

	if (offset < 0)
		offset = maximum - 1;
	if (offset >= maximum)
		offset = 0;
		
	preview_window_order_offset = offset;
}

void Settings::changePreviewWindowVariant(int variant)
{
	preview_window_variant = variant;
}