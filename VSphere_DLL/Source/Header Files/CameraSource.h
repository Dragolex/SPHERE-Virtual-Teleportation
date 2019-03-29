#pragma once

#include "simplifyingHeader.h"

#include "customIrrlicht.h"


struct CameraSource
{
private:
	int cam_index;
	int channel = 0;
	bool is_grabber_channel;
	vector2di * size;
	vector3df * origin;
	quaternion * direction;
	vector3df * to_corner;
	double focus_value;
	string cam_name;


public:
	CameraSource();

	CameraSource(int ind, bool grabber_channel, vector2di size, vector3df origin, vector3df offset, double focus_value);
	CameraSource(int ind, int ch, bool grabber_channel, vector2di size, vector3df origin, vector3df offset, double focus_value);

	~CameraSource();


	int getIndex();
	int getChannel();
	int getIsGrabberChannel();
	vector2di getSize();
	int getPixelCount();


	vector3df getOrigin();

	double getFocusValue();
	string getName();

	quaternion getDirection();
	vector3df getToCorner();
};
