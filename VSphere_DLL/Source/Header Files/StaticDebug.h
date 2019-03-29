#pragma once

#include "simplifyingHeader.h"


namespace StaticDebug
{
	void addDebugLine(string str);

	void addInfoLine(string str);

	void addError(string str);

	void addUserError(string str);


	void add3DCross(vector3df target, float size, vector<int> * data);
	void add3DArrow(vector3df origin, vector3df direction, float size, vector<int> * output_content);
	void add3DArrow(vector3df origin, vector3df direction, float width, float length, vector<int> * output_content);
};