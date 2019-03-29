/*
This class handles the records.
Through the handleFrame() function it automatically separates between currently recording a video and playback.
Same counts for handleBackgroundImage().

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "RecordingHandler.h"

#include <thread>


RecordingHandler::RecordingHandler(int frame_delay_ms)
{
	recorders = new map<int, CameraRecord>();
	current_position = 0;
	this->frame_delay_ms = frame_delay_ms;
}
RecordingHandler::~RecordingHandler(void)
{
	// Close all readers and writers
	for (map<int, CameraRecord>::iterator iterator = recorders->begin(); iterator != recorders->end(); iterator++)
	{
		if (iterator->second.getWriter() != nullptr)
			iterator->second.getWriter()->release();
		if (iterator->second.getReader() != nullptr)
			iterator->second.getReader()->release();
	}

	delete(recorders);
}


void RecordingHandler::addRecord(int camera_list_index, string file_path)
{
	recorders->insert(make_pair(camera_list_index, CameraRecord(true, file_path)));
}

void RecordingHandler::playRecord(int camera_list_index, string file_path)
{
	recorders->insert(make_pair(camera_list_index, CameraRecord(false, file_path)));
}

// To be executed inside the cam control
void RecordingHandler::startRecordOrPlay(int camera_list_index)
{
	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if (it != recorders->end())
	{
		if (it->second.getRecording()) // Save to file
		{
			it->second.setWriter(new VideoWriter(it->second.getFilePath() + ".mpg", CV_FOURCC('P', 'I', 'M', '1'), 30, Size(640, 480), true));			
		}
		else // Read from file
		{
			it->second.setReader(new VideoCapture(it->second.getFilePath() + ".mpg"));
		}
	}
}


/*
If reading (playing) from a record, the background reference will be set here from the file.If currently creating a new record, the file will be saved.
*/
void RecordingHandler::handleBackgroundImage(int camera_list_index, cv::Mat * background_image)
{
	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if (it != recorders->end())
	{
		if (it->second.getRecording()) // Save to file
			imwrite(it->second.getFilePath() + "_background.png", *background_image);
		else // read from file
			imread(it->second.getFilePath() + "_background.png").copyTo(*background_image);
	}
}

void RecordingHandler::handleFrame(int camera_list_index, cv::Mat * frame)
{
	delay_start = high_resolution_clock::now();

	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if (it != recorders->end())
	{
		if (it->second.getRecording()) // Save to file
			it->second.getWriter()->write(*frame);
		else // read from file
		{
			it->second.setJustLooped(false);

			// Synchronize frames
			it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, current_position);
			if (camera_list_index == 0)
				current_position++;

			//cout << "FRAME: " << it->second.getReader()->get(CV_CAP_PROP_POS_FRAMES) << endl;

			//it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, 107); // Examples how to freeze a frame for debugging purpose.
			//it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, 36);
			//it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, 25);
			//it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, 73);

			Mat fr;
			if ((!it->second.getReader()->read(fr)) || (fr.empty()))
			{
				current_position = 0;
				it->second.getReader()->set(CV_CAP_PROP_POS_FRAMES, 0);
				it->second.getReader()->read(fr);
				it->second.setJustLooped(true);
			}

			fr.copyTo(*frame);
		}
	}
}

/*
Perform a delay when reading from a record
*/
bool RecordingHandler::delayFrame(int camera_list_index)
{
	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if ((it != recorders->end()) && (!it->second.getRecording()))
	{
		int passed_time = chrono::duration_cast<milliseconds>(high_resolution_clock::now() - delay_start).count();
		if (passed_time < frame_delay_ms)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(frame_delay_ms - passed_time));
			return(true);
		}
	}
	return(false);
}


bool RecordingHandler::isPlaying(int camera_list_index)
{
	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if (it != recorders->end())
		return(!it->second.getRecording());

	return(false);
}

/*
Returns true when the video has just been restarted
*/
bool RecordingHandler::justLooped(int camera_list_index)
{
	map<int, CameraRecord>::iterator it = recorders->find(camera_list_index);

	if (it != recorders->end())
		return(it->second.getJustLooped());

	return(false);
}