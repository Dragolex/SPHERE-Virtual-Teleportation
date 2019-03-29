#pragma once

#include "simplifyingHeader.h"

#include <random>


class LargeRandom
{
private:
	static default_random_engine generator;

public:
	static int getRandom(int max);
	static int getRandom(int min, int max);

};