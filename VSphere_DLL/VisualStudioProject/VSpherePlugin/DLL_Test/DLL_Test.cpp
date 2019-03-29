// DLL_Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Dll_Test.h"

#include <iostream>

#include <thread>

/*
This is a simple test project which uses the DLL.
It does not contain any 3D rendering (for that purpose the engine of Unity is required. See the Unity project in "VSphere").
However all computations on the DLL site are performed and the preview windows can be seen.
*/
int main()
{
	printf("Testing VSphere DLL.\n");

	/*
	The LoadLibraryA(path) function does seem to require the absolute path to the DLL. Therefore a path with ".\\" does not seem to work.
	The absolute working-directory is received and modified to fit the path of the DLL, however this is only suited for usage through visual studio.
	*/
	
	string total_root_directory = computeRootPath(); // The root directory where the VSphere as well as the VSphere_DLL and Records directories are


	// Add the path of the actual DLL
	string dllPath = total_root_directory + "VSphere\\Assets\\VSpherePlugin\\x86_64\\VSpherePlugin.dll";
	// Laod the Dll
	HINSTANCE hGetProcIDDLL = LoadLibraryA(dllPath.c_str());


	if (!hGetProcIDDLL) {
		printf("could not load the dynamic library.\n");
	}
	else
	{
		/* Load all the functions of the DLL */

		func_arg_bool_int PrepareSphere = (func_arg_bool_int)GetProcAddress(hGetProcIDDLL, "PrepareSphere");
		if (!PrepareSphere) {
			printf("could not locate the function");
		}

		func_arg_int_int StartSphere = (func_arg_int_int)GetProcAddress(hGetProcIDDLL, "StartSphere");
		if (!StartSphere) {
			printf("could not locate the function");
		}

		func UnityPluginLoad = (func)GetProcAddress(hGetProcIDDLL, "UnityPluginLoad");
		if (!UnityPluginLoad) {
			printf("could not locate the function");
		}

		func_bool CheckNewModel = (func_bool)GetProcAddress(hGetProcIDDLL, "CheckNewModel");
		if (!CheckNewModel) {
			printf("could not locate the function");
		}

		func_int_arg_9int ConfigureCamera = (func_int_arg_9int)GetProcAddress(hGetProcIDDLL, "ConfigureCamera");
		if (!ConfigureCamera) {
			printf("could not locate the function");
		}

		func_arg_int_str_int ConfigureRecordHandler = (func_arg_int_str_int)GetProcAddress(hGetProcIDDLL, "ConfigureRecordHandler");
		if (!ConfigureRecordHandler) {
			printf("could not locate the function");
		}

		func_arg_intptrptr_intptr StartRetrievingModel = (func_arg_intptrptr_intptr)GetProcAddress(hGetProcIDDLL, "StartRetrievingModel");
		if (!StartRetrievingModel) {
			printf("could not locate the function");
		}

		func_bool EndRetrievingModel = (func_bool)GetProcAddress(hGetProcIDDLL, "EndRetrievingModel");
		if (!EndRetrievingModel) {
			printf("could not locate the function");
		}
			
		/* ---------- */


		//UnityPluginLoad(); // Not required in this sample without Unity (normally it gets automatically called by the Unity Engine)


		PrepareSphere(true, 33); // Prepare the sphere with enabled console (true) and 33ms standard delay between frames (this delay is only used when frames are read from files and not directly streamed)


		// Prepare sample cameras (see the DLL for arguments)
		int camA = ConfigureCamera(0, -1, 5,
			1, 1, 320, // Root coordinate of the camera in space. It's orientation is always towards 0,0,0
			45, 0, 0); // Additional offset of the camera (after the orientation towards 0,0,0 has been computed)
		int camB = ConfigureCamera(1, -1, 5,
			1, 480,	1,
			135, 0,	0);


		string records_root_path = total_root_directory + "Records\\"; // Relative path to the records


		// Prepare the recording of the cameras or their playback
		int record_type = 2; // 0: nothing; 1: Recording; 2: Playing;
		ConfigureRecordHandler(camA, (records_root_path + "TestRecordB" + to_string(camA)).c_str(), record_type);
		ConfigureRecordHandler(camB, (records_root_path + "TestRecordB" + to_string(camB)).c_str(), record_type);



		StartSphere(1, 9); /* Start the sphere with the a combined preview window (2 = combined; 1 = separated)
						      and preview type number 0-9 for varying types or use the Unity example which has a GUI to switch at runtime.
						   Values:
							   0 "Preview disabled",
							   1 "Original live image",
							   2 "Dynamic background reference",
							   3 "Live image without background (by RGB)",
							   4 "Binary mask",
							   5 "In or Out mask grid",
							   6 "Contours mask",
							   7 "Contour keypoints",
							   8 "Contour segments",
							   9 "Contour segments as overlay"
						   */


		while (true)
		{
			if (CheckNewModel()) // If a new model is ready to be transfered
			{
				printf("--- DLL TEST --- NEW MODEL FRAME --- ");

				int quads;
				int * data;
				int coord;

				StartRetrievingModel(&data, &quads); // Retrieve the new model.
				// After this call the data can be accessed through this array and the memmory is locked until call of EndRetrievingModel().

				// Loop through every coordinate. Factor explanation:
				// quads: number of quads to build the model
				// 4: Number of coordinates for the quad
				// 3: values per coordinate (x, y, z)
				// 8: texture coordinates (UV); 2 for every of the four coorinates of the quad
				for (int i = 0; i < quads * (4 * 3 + 8); ++i)
				{
					coord = data[i];

					// The actual rendering of the model would happen here
					// For this test it is empty or you can use the following sample to simulate the slight delay rendering would produce.

					//if ((int)(i/6000) != (int)((i-1) / 6000))
						//std::this_thread::sleep_for(std::chrono::milliseconds(1));

					//printf("Coordinate is: %d \n", coord);
				}

				printf("RETRIEVED COORDINATES OF %d QUADS!\n", quads);

				EndRetrievingModel(); // End retrieving model (unlocks the output data)
			}

			// Additional delay if desired
			//std::this_thread::sleep_for(std::chrono::milliseconds(400));
		}

	}

    return 0;
}


// Comptues the root directory where the VSphere as well as the VSphere_DLL and Records directories are
string computeRootPath()
{
	int folders_to_move = 6; // Number of folders to go back from the current working directory
	char pathBuffer[MAX_PATH];
	int rpos = MAX_PATH;
	GetModuleFileNameA(NULL, pathBuffer, MAX_PATH); // Get the full path of this test EXE
	for (int i = 0; i < folders_to_move; ++i)
		rpos = string(pathBuffer).substr(0, rpos).find_last_of("\\/") - 1; // Move folders_to_move folders back
	rpos += 1;

	return(string(pathBuffer).substr(0, rpos) + "\\");
}