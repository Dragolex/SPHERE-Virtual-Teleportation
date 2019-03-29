#pragma once

#include "simplifyingHeader.h"


class SimpleNamedWindow
{
private:
	string windowName;
	int x, y;
	bool prepared, showing;
	cv::Mat * image;


public:
	SimpleNamedWindow(string name, int posX, int posY);
	~SimpleNamedWindow();

	void prepare(string name, int posX, int posY);

	void show();
	void hide();

	void setMat(cv::Mat * image);

	cv::Mat * getMat();
	cv::Mat * getMat(int width, int height);

};