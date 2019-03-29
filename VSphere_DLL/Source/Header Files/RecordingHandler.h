#pragma once

#include "simplifyingHeader.h"

#include "CameraRecord.h"

#include <chrono>


class RecordingHandler
{
private:

	volatile int current_position;
	map<int, CameraRecord> * recorders;
	int frame_delay_ms;
	high_resolution_clock::time_point delay_start;

public:

	RecordingHandler(int frame_delay_ms);
	~RecordingHandler(); // Destructor

	void addRecord(int camera_list_index, string file_path);
	void playRecord(int camera_list_index, string file_path);

	void startRecordOrPlay(int camera_list_index);
	void handleBackgroundImage(int camera_list_index, cv::Mat * frame);
	void handleFrame(int camera_list_index, cv::Mat * frame);

	bool delayFrame(int camera_list_index);

	bool isPlaying(int camera_list_index);
	bool justLooped(int camera_list_index);

};