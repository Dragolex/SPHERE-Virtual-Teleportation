#pragma once

#include "simplifyingHeader.h"

#include <vector>


class Settings
{
private:
	static int preview_type;
	static int max_preview_types;

	static int preview_window_variant;
	static int preview_window_order_offset;


	// Array with text containing the various types of preview 
	static vector<string> preview_type_text;

public:

	// Settings to modify

	static void init();

	static int getBackgroundReferenceComputingFrames();
	static int getBackgroundColorTolerance();
	static int getNoisepixelTolerance();
	static int getContourMaskSize();

	static int getThreadTimeoutMS();

	static float getSegmentOptimisationTolerance();
	static float getPreviewScaleFactor();

	static float getMaxPreviewTypes();

	static int getPreviewType();
	static string getPreviewString();
	
	static int getPreviewWindowVariant();
	static int getPreviewWindowOrderOffset();

	static int getMaxRayLength();


	// Set a new preview type (or -1 to go to next; -2 to return to last )
	static void changePreviewType(int new_type);

	// Set a new preview window type (or -1 to go to next; -2 to return to last )
	static void changePreviewWindowOrderOffset(int offset, int maximum);

	static void changePreviewWindowVariant(int variant);
};