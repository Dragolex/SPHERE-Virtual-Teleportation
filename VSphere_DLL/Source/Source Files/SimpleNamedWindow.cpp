/*
This class has the purpose to provide a simple, direct way of displaying an image (MAT structure from OpenCV).
It uses the HighGUI features of OpenCV.

@Author: Alexander Georgescu
*/

#include "stdafx.h"


SimpleNamedWindow::SimpleNamedWindow(string name, int posX, int posY)
{
	showing = false;
	prepared = false;
	windowName = name;
	x = posX;
	y = posY;

	image = nullptr;

	prepared = true;
}

SimpleNamedWindow::~SimpleNamedWindow()
{
	hide();
}


void SimpleNamedWindow::prepare(string name, int posX, int posY)
{
	windowName = name;
	x = posX;
	y = posY;

	prepared = true;
}	

void SimpleNamedWindow::show()
{
	if (!prepared)
	{
		addError("Trying to show a MAT before preparing the window.");
		return;
	}
	if (!showing)
	{
		namedWindow(windowName, 1);
		moveWindow(windowName, x, y);
	}
	showing = true;
}
void SimpleNamedWindow::hide()
{
	destroyWindow(windowName); // Attention! This appears to freeze when called from the external application... TODO: Debug, but no clue how..
}

void SimpleNamedWindow::setMat(Mat * image)
{
	if (!prepared)
	{
		addError("Trying to show a MAT before preparing the window.");
		return;
	}
	if (!showing) show();

	this->image = image;
	imshow(windowName, *image);
	waitKey(1);
}

Mat * SimpleNamedWindow::getMat()
{
	return(image);
}
Mat * SimpleNamedWindow::getMat(int width, int height)
{
	image = new Mat(height, width, CV_8UC3, Scalar(0, 0, 0));
	return(image);
}
