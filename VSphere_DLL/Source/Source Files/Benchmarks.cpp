/*
Those classes provide a few simple benchmarks.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include <ctime>
#include <chrono>


// Count and display frames
fpsBench::fpsBench()
{
	time_t refTime = time(0);
	cur_frames = 0;
	frames = -1;
}

void fpsBench::newFrame(bool printResult)
{
	if (frames == -1)
		ref_time = time(0);

	frames++;

	if (difftime(time(0), ref_time) >= 1)
	{
		cur_frames = frames;
		frames = -1;

		if (printResult)
			printFps();
	}
}

void fpsBench::printFps()
{
	printf("FPS: %d\n", cur_frames);
}





timeBench::timeBench(int accuray_type)
{
	this->accuray_type = accuray_type; // 0: milliseconds; 1: microseconds; 2: nanoseconds
	resetTime();
}

void timeBench::startTime()
{
	ref_time = high_resolution_clock::now();
}

void timeBench::pauseTime()
{
	switch (accuray_type)
	{
	case 0: time_sum += duration_cast<milliseconds>(high_resolution_clock::now() - ref_time).count(); break;
	case 1: time_sum += duration_cast<microseconds>(high_resolution_clock::now() - ref_time).count(); break;
	case 2: time_sum += duration_cast<nanoseconds>(high_resolution_clock::now() - ref_time).count(); break;
	}

	ref_time = high_resolution_clock::now();
}

void timeBench::endTime()
{
	switch (accuray_type)
	{
	case 0: time_sum += duration_cast<milliseconds>(high_resolution_clock::now() - ref_time).count(); break;
	case 1: time_sum += duration_cast<microseconds>(high_resolution_clock::now() - ref_time).count(); break;
	case 2: time_sum += duration_cast<nanoseconds>(high_resolution_clock::now() - ref_time).count(); break;
	}
	
	count++;
}

void timeBench::resetTime()
{
	time_sum = 0;
	count = 0;
	ref_time = high_resolution_clock::now();
	ref_time_print = high_resolution_clock::now();
}

double timeBench::getAverage()
{
	return(time_sum / ((double)count));
}

void timeBench::printAverage(float everySeconds, string str)
{
	printAverage(everySeconds, str.c_str());
}

void timeBench::printAverage(float everySeconds, const char* str)
{
	if (everySeconds <= 0)
	{
		printf(str, getAverage());
	}
	else
		if (duration_cast<std::chrono::seconds>(high_resolution_clock::now() - ref_time_print).count() >= everySeconds)
		{
			ref_time_print = high_resolution_clock::now();
			printAverage(-1, str);
		}
}




time_t ref_time;
time_t ref_time_print;
double value_sum;
double max_val, min_val;
int count;

valueBench::valueBench()
{
	resetValue();
}

void valueBench::addValue(double new_value)
{
	value_sum += new_value;
	if (new_value > max_val) max_val = new_value;
	if (new_value < min_val) min_val = new_value;

	count++;
}

void valueBench::resetValue()
{
	min_val = 10000000;
	max_val = -10000000;
	count = 0;
	value_sum = 0;
	ref_time = time(0);
}


double valueBench::getAverage()
{
	return(value_sum / ((double)count));
}


void valueBench::printAverage(float everySeconds, string str)
{
	printAverage(everySeconds, str.c_str());
}

void valueBench::printAverage(float everySeconds, const char* str)
{
	if (everySeconds <= 0)
	{
		printf(str, getAverage());
	}
	else
		if (difftime(time(0), ref_time_print) >= everySeconds)
		{
			ref_time_print = time(0);
			printAverage(-1, str);
		}

}


void valueBench::printAverageFull(float everySeconds, const char* str)
{
	printAverageFull(everySeconds, string(str));
}

void valueBench::printAverageFull(float everySeconds, string str)
{
	if (everySeconds <= 0)
	{
		printf((str + " Min: " + to_string(min_val) + " Max: " + to_string(max_val) + "\n").c_str(), getAverage());
	}
	else
		if (difftime(time(0), ref_time_print) >= everySeconds)
		{
			ref_time_print = time(0);
			printAverageFull(-1, str);
		}

}
