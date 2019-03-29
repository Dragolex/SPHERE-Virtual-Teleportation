#pragma once

#include "simplifyingHeader.h"

#include "EdgesIdentifier.h"
#include "Ray3D.h"



class RayGenerator
{
private:
	CameraSource * camera_source;
	int cam_w, cam_h;
	quaternion cam_direction;
	vector3df cam_left_top;

	int tex_offs_x, tex_offs_y;


	vector<Ray3D*> rays;

	vector<int> debug_quads;


	void addRay(int x1, int y1, int x2, int y2, bool ortientation, vector3df left_top, quaternion cam_direction);



	// From other classes

	vector<int> * segment_starts;
	vector<int> * segment_ends;
	vector<bool> * segment_orientations;


public:

	RayGenerator(CameraSource * camera_source);
	~RayGenerator();
	
	void initData(vector<int> * segment_starts, vector<int> * segment_ends, vector<bool> * segment_orientations, int tex_offs_x, int tex_offs_y);

	

	void generateRays();

	void visualizeRays(vector<int> * output_content, int length);

	vector<Ray3D*> * getRays();

	CameraSource * getCameraSource();
};


