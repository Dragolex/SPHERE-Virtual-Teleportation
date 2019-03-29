#pragma once

#include "simplifyingHeader.h"

#include <chrono>

using namespace std::chrono;


class fpsBench
{
private:
	time_t ref_time;
	int cur_frames;
	int frames;


public:
	fpsBench();

	void newFrame(bool printResult);

	void printFps();
};


class timeBench
{
private:
	high_resolution_clock::time_point ref_time;
	high_resolution_clock::time_point ref_time_print;
	int accuray_type = 0;
	double time_sum;
	int count;

public:
	timeBench(int accuray_type);

	void startTime();

	void pauseTime();

	void endTime();

	void resetTime();

	double getAverage();
	void printAverage(float everySeconds, std::string str);
	void printAverage(float everySeconds, const char* str);
};



class valueBench
{
private:
	time_t ref_time;
	time_t ref_time_print;
	double value_sum;
	double max_val, min_val;
	int count;

public:
	valueBench();

	void addValue(double new_value);

	void resetValue();

	double getAverage();
	void printAverage(float everySeconds, std::string str);
	void printAverage(float everySeconds, const char* str);

	void printAverageFull(float everySeconds, std::string str);
	void printAverageFull(float everySeconds, const char* str);
};