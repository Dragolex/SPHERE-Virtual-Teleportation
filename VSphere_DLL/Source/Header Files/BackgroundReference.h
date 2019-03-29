#pragma once

#include "simplifyingHeader.h"

#include <vector>


class BackgroundReference
{
private:
	int background_color_tolerance;

	Mat * background = nullptr;
	bool * binaryMask = nullptr;

	uchar* bc;
	Mat * frame;
	uchar* f;

	int pixelcount;


	int expectedFrames;
	double frameFactor;


public:
	BackgroundReference();
	~BackgroundReference();

	void startNewBackground(Mat * frame, int expectedFrames);
	void addFrame();
	void finalizeBackground();

	void computeRGBbinaryMask();

	Mat * getBackground();
	bool * getBinaryMask();

	void previewNonbackgroundImageRGB(cv::Mat * dest, bool onlyBinary);
	//void previewNonbackgroundImageHSV(cv::Mat * dest, bool onlyBinary);
};