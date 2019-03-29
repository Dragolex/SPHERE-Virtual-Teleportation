#pragma once

#include "simplifyingHeader.h"

#include "CameraSource.h"

class CameraHandler
{
private:
	vector<CameraSource*> cameras;

	set<int> cameras_with_subchannels;
	bool isGrabberChannel(int cameraIndex);


public:
	int addCamera(int cameraIndex, vector2di size, vector3df origin, vector3df offset, double focus_value);
	int addCamera(int cameraIndex, int channel, vector2di size, vector3df origin, vector3df offset, double focus_value);

	~CameraHandler();

	int getCount();

	CameraSource * getCameraSource(int list_index);
};

