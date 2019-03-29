#pragma once

#include "simplifyingHeader.h"


class CameraRecord
{
private:

	bool recording;
	bool just_looped;
	string file_path;
	cv::VideoWriter * video_writer = nullptr;
	cv::VideoCapture * video_reader = nullptr;


public:

	CameraRecord(bool recording, string file_path);

	bool getRecording();
	string getFilePath();

	void setWriter(VideoWriter * video_writer);
	void setReader(VideoCapture * video_reader);

	void setJustLooped(bool just_looped);

	VideoWriter * getWriter();
	VideoCapture * getReader();

	bool getJustLooped();

};