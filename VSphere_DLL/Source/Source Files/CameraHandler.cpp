/*
Class handling cameras.
It contains an array with classes and functionality to separate between channels which require direct "frame grabbing".

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "CameraHandler.h"
#include "CameraSource.h"


CameraHandler::~CameraHandler()
{
	while (!cameras.empty())
	{
		delete(cameras.back());
		cameras.pop_back();
	}
}

int CameraHandler::addCamera(int cameraIndex, vector2di size, vector3df origin, vector3df offset, double focus_value)
{
	cameras.push_back(new CameraSource(cameraIndex, true, size, origin, offset, focus_value));
	return(cameras.size() - 1);
}
int CameraHandler::addCamera(int cameraIndex, int channel, vector2di size, vector3df origin, vector3df offset, double focus_value)
{
	CameraHandler::cameras.push_back(new CameraSource(cameraIndex, channel, isGrabberChannel(cameraIndex), size, origin, offset, focus_value));
	return(cameras.size() - 1);
}


/*
"Frame rgabbing" is in the nd a hardware call (through openCV) which causes the camera to load its current frame data to local memmory.
After grabbing, the frame can be retrieved and decoded.However decoding takes sometime therefore if the camera frames
should be synced between eachotherr like required for the VSphere, it is recomended to perform grabbing of all cameras once before retrieving and decoding every frame.
there is an exception when using cameras with multiple channels because grabbing should only be called ocne in that case (as it applies to the device, not to the channel).
This fucntion returns true when the certain device has no sub-channels OR is the representing the first subchannel of the device.
It returns false for all other channels.
*/
bool CameraHandler::isGrabberChannel(int cameraIndex)
{
	if (cameras_with_subchannels.find(cameraIndex) == cameras_with_subchannels.end())
	{
		cameras_with_subchannels.insert(cameraIndex);
		return(true);
	}
	return(false);
}


int CameraHandler::getCount()
{
	return(cameras.size());
}

CameraSource * CameraHandler::getCameraSource(int list_index)
{
	return(cameras.at(list_index));
}
