/*
This core class handles how the background of a camera frame is removed. It is bound to exactly one camera (indirectly through the input frame pointer).

The method bases on a background-sample which is computed by an avergae of frames (which may not contain anything that shall be rednered later in 3D).
When the background from a frame shall be removed it compares the colors of image with the background reference.

Input (from PerCamControler):
	frame 2D image (MAT) pointer // Representing a frame from the camera

Output:
	BinaryMask pointer			 // Bool array covering the frame image containing which pixels are background and which object


Todo: Much more advanced techniques than the simple one used here are available (however they tend to be more timeconsuming)
	  Experiments with alternative color-representations (for example HSV) might be an option.

@Author: Alexander Georgescu
*/


#include "stdafx.h"

#include "BackgroundReference.h"


BackgroundReference::BackgroundReference()
{
	background_color_tolerance = Settings::getBackgroundColorTolerance();
}

BackgroundReference::~BackgroundReference()
{
	// Free the background
	if (background != nullptr)
		delete(background);

	// free the binary mask
	if (binaryMask != nullptr)
		delete[] binaryMask;
}

/*
Start creating a new reference background from a given number of frames.
-- Arguments:
frame: pointer to first the image/frame (datastructure Mat from OpenCV).
	   This pointer should contain the new frame every time addFrame() is called
	   and it should be the same pointer which will contain the further camera frames during the execution of the VSphere.
expectedFrames: Number of frames to expect for computing the reference background
*/
void BackgroundReference::startNewBackground(Mat * frame, int expectedFrames)
{
	this->frame = frame;
	f = frame->ptr<uchar>(0);

	this->expectedFrames = expectedFrames;

	pixelcount = frame->cols * frame->rows;
	
	//Create the new binary mask
	if (binaryMask == nullptr)
		binaryMask = new bool[pixelcount];


	// We are using a 32bit image matrix for storing the background so a normal 8bit frame fits 4 times.
	// Therefore the factor needs to be 4 / totalFrames
	frameFactor = ((float)4) / expectedFrames;
	

	if (expectedFrames > 0)
		background = nullptr; 
	
	if (expectedFrames == -1)
		background = new Mat();
}

/*
Add another frame (data still in the same pointer provided in startNewBackground() )
*/
void BackgroundReference::addFrame()
{
	if (background == nullptr)
	{
		background = new Mat();

		// Create a background of 32bit type
		frame->convertTo(*background, CV_32SC3, frameFactor, 0);

		bc = background->ptr<uchar>(0);
	}
	else
	{
		// Add every frame to this background. Thanks to the factor 'frameFactor', the 32 bit are filled up accurately
		// no matter how many frames were used
		addWeighted(*background, 1, *frame, frameFactor, 0, *background, CV_32SC3);
	}

}

/*
Call when all frames have been added.
*/
void BackgroundReference::finalizeBackground()
{
	if (expectedFrames > 0)
	{
		// Scale back down to a 8bit image for easier comparison later -> Results in an average from all frames
		background->convertTo(*background, CV_8UC3, 0.5, 0);
	}

	bc = background->ptr<uchar>(0);
}



Mat * BackgroundReference::getBackground()
{
	if (background == nullptr)
	{
		StaticDebug::addError("Trying to use getBackground before a background has been computed!");
		background = new Mat(); // prevent direct error
	}
	return(background);
}

bool * BackgroundReference::getBinaryMask()
{
	if (binaryMask == nullptr)
	{
		StaticDebug::addError("Trying to use getBinaryMask before a background has been computed!");
		binaryMask = new bool[pixelcount]; // prevent direct error
	}
	return(binaryMask);
}


/*
Compute the mask for the frame which is currently in the pointer which has been given through startNewBackground()
*/
void BackgroundReference::computeRGBbinaryMask()
{
	int j = 0;
	for (int i = 0; i < pixelcount - 1; ++i)
	{
		binaryMask[i] = (f[j] - bc[j] < background_color_tolerance && f[j + 1] - bc[j + 1] < background_color_tolerance && f[j + 2] - bc[j + 2] < background_color_tolerance);
		j += 3;
	}
}


/*
Draw a preview onto a given image.
*/
void BackgroundReference::previewNonbackgroundImageRGB(Mat * dest, bool onlyBinary)
{
	uchar* d = dest->ptr<uchar>(0);

	int j = 0;
	for (int i = 0; i < pixelcount; i++)
	{
		if (binaryMask[i])
		{
			d[j] = 0;
			d[j + 1] = 0;
			d[j + 2] = 0;
		}
		else
		{
			if (onlyBinary)
			{
				d[j] = 255;
				d[j + 1] = 255;
				d[j + 2] = 255;
			}
			else
			{
				d[j] = f[j];
				d[j + 1] = f[j + 1];
				d[j + 2] = f[j + 2];
			}
		}

		j += 3;
	}

}