/*
Provides a nearly limitless randomizer.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "LargeRandom.h"
#include <random>


default_random_engine LargeRandom::generator;

int LargeRandom::getRandom(int max)
{
	return(getRandom(0, max));
}
int LargeRandom::getRandom(int min, int max)
{
	uniform_int_distribution<int> distribution(min, max);
	return(distribution(LargeRandom::generator));
}

