/*
The camera record represents a certain record which is either curretnly played or recorded.
For information about the recording system see "HandlerModules"->RecordingHandler.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "CameraRecord.h"


CameraRecord::CameraRecord(bool recording, string file_path)
{
	this->recording = recording;
	this->file_path = file_path;
	just_looped = false;
}

bool CameraRecord::getRecording()
{
	return(recording);
}

string CameraRecord::getFilePath()
{
	return(file_path);
}


void CameraRecord::setWriter(VideoWriter * video_writer)
{
	this->video_writer = video_writer;
}

void CameraRecord::setReader(VideoCapture * video_reader)
{
	this->video_reader = video_reader;
}

void CameraRecord::setJustLooped(bool just_looped)
{
	this->just_looped = just_looped;
}


VideoWriter * CameraRecord::getWriter()
{
	return(video_writer);
}

VideoCapture * CameraRecord::getReader()
{
	return(video_reader);
}

bool CameraRecord::getJustLooped()
{
	return(just_looped);
}
