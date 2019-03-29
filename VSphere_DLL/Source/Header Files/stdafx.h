// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <windows.h>

#include <stdio.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <set>


//#include "SimpleVecs.h"
//#include "quaternion.h"

#include "opencv2/opencv.hpp"

#include "Settings.h"
#include "StaticDebug.h"
#include "Benchmarks.h"
#include "SimpleNamedWindow.h"
#include "CustomMath.h"
#include "LargeRandom.h"

#include "CameraHandler.h"


#include "customIrrlicht.h"



#define _USE_MATH_DEFINES
#include <math.h>


#define REPEAT(rep) for(int __u = 0; __u < rep; __u++) 

#define FUNC(NAME, RETURN_TYPE, FUNCTION) \
    struct { RETURN_TYPE operator () FUNCTION } NAME;


/*
DECLARE_LAMBDA(plus, int, (int i, int j)
{
	return i + j;
});
*/

//#define DEBUG
//#define DEBUG_MSG
//#define DEBUG_INTERSECTIONS




using namespace std;
using namespace cv;
using namespace StaticDebug;

