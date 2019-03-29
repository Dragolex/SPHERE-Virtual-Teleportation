/*
This namespace provides a couple of functions for debugging.

@Author: Alexander Georgescu
*/

#include "stdafx.h"


namespace StaticDebug
{
	void addDebugLine(string str)
	{
		cout << str << endl;
	}

	void addInfoLine(string str)
	{
		cout << "Info: " << str << endl;
	}

	void addError(string str)
	{
		cout << "ERROR - " << str << endl;
	}

	void addUserError(string str)
	{
		cout << "ERROR - User input - " << str << endl;
	}

	/*
	Adds a cross in 3D made of three quads to mark a point.
	-- Arguments:
	target: Position where to draw
	size: Size of the cross
	data: int-array which will be filled the same way the normal model is computed.
	      So this data can be simply appended to the main array to transfer thsoe debug coordinates to Unity and render.
	*/
	void add3DCross(vector3df target, float size, vector<int> * data)
	{
		vector3df addx(size, 0, 0);
		vector3df addy(0, size, 0);
		vector3df addz(0, 0, size);


		vector3df vec = target - addx - addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addx - addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target - addx + addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addx + addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		for (int i = 0; i < 8; i++)
			data->push_back(0); // Push the texture coordinates


		vec = target - addy - addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addy - addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target - addy + addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addy + addz; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		for (int i = 0; i < 8; i++)
			data->push_back(0); // Push the texture coordinates


		vec = target - addx - addy; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addx - addy; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target - addx + addy; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);

		vec = target + addx + addy; vec *= 100;
		data->push_back(vec.X); 		data->push_back(vec.Y);		data->push_back(vec.Z);
	
		for (int i = 0; i < 8; i++)
			data->push_back(0); // Push the texture coordinates
	}


	void add3DArrow(vector3df origin, vector3df direction, float size, vector<int> * data)
	{
		add3DArrow(origin, direction, size, size, data);
	}

	/*
	Adds an oriented arrow in 3D made of two quads to mark a point.
	-- Arguments:
	target: Position where to draw
	direction: direction to orient towards
	width/length: Size of the cross
	data: int-array which will be filled the same way the normal model is computed.
	      So this data can be simply appended to the main array to transfer thsoe debug coordinates to Unity and render.
	*/
	void add3DArrow(vector3df origin, vector3df direction, float width, float length, vector<int> * data)
	{
		vector3df dir = direction;
		dir.normalize();

		quaternion ref_dir(0,0,0,0);
		ref_dir.lookRotation(direction, vector3df(0, 1, 0));
		

		vector3df arrow_goal = origin + dir * length;
		vector3df arrow_l = origin + ref_dir * vector3df(-width, 0, 0);
		vector3df arrow_r = origin + ref_dir * vector3df(width, 0, 0);
		vector3df arrow_t = origin + ref_dir * vector3df(0, width, 0);
		vector3df arrow_b = origin + ref_dir * vector3df(0, -width, 0);


		arrow_goal *= 100;
		arrow_l *= 100;
		arrow_r *= 100;
		arrow_t *= 100;
		arrow_b *= 100;

		data->push_back(arrow_l.X);
		data->push_back(arrow_l.Y);
		data->push_back(arrow_l.Z);

		data->push_back(arrow_goal.X);
		data->push_back(arrow_goal.Y);
		data->push_back(arrow_goal.Z);

		data->push_back(arrow_r.X);
		data->push_back(arrow_r.Y);
		data->push_back(arrow_r.Z);

		data->push_back(arrow_l.X);
		data->push_back(arrow_l.Y);
		data->push_back(arrow_l.Z);

		for (int i = 0; i < 8; i++)
			data->push_back(0); // Push the texture coordinates


		data->push_back(arrow_t.X);
		data->push_back(arrow_t.Y);
		data->push_back(arrow_t.Z);

		data->push_back(arrow_goal.X);
		data->push_back(arrow_goal.Y);
		data->push_back(arrow_goal.Z);

		data->push_back(arrow_b.X);
		data->push_back(arrow_b.Y);
		data->push_back(arrow_b.Z);

		data->push_back(arrow_t.X);
		data->push_back(arrow_t.Y);
		data->push_back(arrow_t.Z);

		for (int i = 0; i < 8; i++)
			data->push_back(0); // Push the texture coordinates
	}

};