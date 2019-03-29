/*
This struct represents an available camera and provides an interface to all its data.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "CameraSource.h"


CameraSource::CameraSource() // Constructor required due to the usage in a vector
{}

CameraSource::CameraSource(int ind, bool grabber_channel, vector2di size, vector3df origin, vector3df offset, double focus_value) // Simple inputcam
{
	cam_index = ind;
	channel = -1;
	this->is_grabber_channel = grabber_channel;
	this->size = new vector2di(size.X, size.Y);
	this->origin = new vector3df(origin.X, origin.Y, origin.Z);
	this->focus_value = focus_value;

	direction = new quaternion();

	if (ind == 0)
		direction->lookRotation(-origin, vector3df(0, 1, 0)).normalize();
	else
		direction->lookRotation(-origin, vector3df(0, 1, 1)).normalize();

	to_corner = new vector3df((*direction) * vector3df(-size.X / 2, size.Y / 2, 0));

	this->origin = new vector3df(origin.X + offset.X, origin.Y + offset.Y, origin.Z + offset.Z);


	if (channel < 0)
		cam_name = string("Cam ") + to_string(cam_index);
	else
		cam_name = string("Cam ") + to_string(cam_index) + string(" Ch ") + to_string(channel);
}

CameraSource::CameraSource(int ind, int ch, bool grabber_channel, vector2di size, vector3df origin, vector3df offset, double focus_value) // Channel of multichannel inputcam
{
	cam_index = ind;
	channel = ch;
	this->is_grabber_channel = grabber_channel;
	this->size = new vector2di(size.X, size.Y);
	this->origin = new vector3df(origin.X, origin.Y, origin.Z);
	this->focus_value = focus_value;

	direction = new quaternion();
	direction->lookRotation(-origin, vector3df(0, 1, 0)).normalize();

	to_corner = new vector3df((*direction) * vector3df(-size.X / 2, size.Y / 2, 0));

	this->origin = new vector3df(origin.X + offset.X, origin.Y + offset.Y, origin.Z + offset.Z);

	if (channel < 0)
		cam_name = string("Cam ") + to_string(cam_index);
	else
		cam_name = string("Cam ") + to_string(cam_index) + string(" Ch ") + to_string(channel);
}


CameraSource::~CameraSource()
{
	delete(size);
	delete(origin);
	delete(direction);
	delete(to_corner);
}


int CameraSource::getIndex()
{
	return(cam_index);
}
int CameraSource::getChannel()
{
	return(channel);
}
int CameraSource::getIsGrabberChannel()
{
	return(is_grabber_channel);
}

vector2di CameraSource::getSize()
{
	return(*size);
}

int CameraSource::getPixelCount()
{
	return(size->X * size->Y);
}

vector3df CameraSource::getOrigin()
{
	return(*origin);
}

double CameraSource::getFocusValue()
{
	return(focus_value);
}

string CameraSource::getName()
{
	return(cam_name);
}


quaternion CameraSource::getDirection()
{
	return(*direction);
}

vector3df CameraSource::getToCorner()
{
	return(*to_corner);
}
