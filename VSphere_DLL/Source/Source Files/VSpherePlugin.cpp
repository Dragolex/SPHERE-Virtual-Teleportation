/*
This file is root of the whole DLL.
It defines all externally accessible fucntions and therefore it's interface.

An important aspect to know:
To use a DLL in Unity the .dll file requires to be part of an Asset of Unity.
Therefore it has to be deployed there every time it is recompiled.
To simplyfy this, a "POST BUILD EVENT" is used (see "properties -> Build Events").
For the current folderstructure, the following script has been inserted there:

if "$(PlatformShortName)" == "x86" (
copy / Y "$(TargetPath)" $(SolutionDir)..\..\..\VSphere\Assets\VSpherePlugin\x86\$(TargetFileName)"
) else (
copy / Y "$(TargetPath)" $(SolutionDir)..\..\..\VSphere\Assets\VSpherePlugin\x86_64\$(TargetFileName)"
)

However note that overwriting is only possible when unity does not use the DLL. Unfortunately Unity doesn't support automatic-unloading.
Therefore the entire unity editor has to be shut down before recompiling.


@Author: Alexander Georgescu
*/


#include "stdafx.h"


// Required for DX texture
#include "IUnityGraphicsD3D11.h"
#include <DirectX11Handler.h>


#include "VSpherePlugin.h"

#include "SphereControler.h"
#include "CameraHandler.h"
#include "RecordingHandler.h"



// --------------------------------------------------------------------------
// Standard Unity DLL functions

DirectX11Handler *render_engine_handler = nullptr;

// Automatically called upon loading the DLL (name may not be changed!)
extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	startConsole();

	addInfoLine("VSphere DLL loaded.\n");

	render_engine_handler = new DirectX11Handler(unityInterfaces);
}

// Note: This function is never called when using the Unity Editor, hwoever it works when exporting an actual EXE with Unity.
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	addInfoLine("Unload plugin.");
	hideConsole();
}



// Pointer to the instance of SphereControler
// I have called the system "VSphere" as in "Virtualisation Sphere". Therefore this name.
SphereControler * VSphere;

// Thread which will handle the Sphere 
thread * outer_sphere_thread;

// Cameras and recorders
CameraHandler * camera_set;
RecordingHandler * recorder_set;

volatile bool sphere_already_running = false;
vector<int> * sphere_content_coordinates;

// For texture (Todo: Create an abstract class with macros to avoid dependance on DirectX11 only! The unity example for low-level natvie plugins provides exampels for that.)
ID3D11Texture2D* modelTexture = nullptr;


// Prepares the functionality of the DLL
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API  PrepareSphere(bool open_console, int recorder_frame_duration_ms)
{
	ClearScreen(); // Put here to clear the screen when restarting the application from the Unity Editor (because Unity calls the UnityPluginLoad function only a single time.)

	if (!open_console)
		hideConsole(); // Hdie console if deactivated (had to be created in UnityPluginLoad() to work at all)


	// Initialize the settings
	Settings::init();

	if (sphere_already_running)
		QuitSphere();


	while (sphere_already_running) {} // Wait until sphere has been quitted


	camera_set = new CameraHandler();
	recorder_set = new RecordingHandler(recorder_frame_duration_ms);
	
	addInfoLine("VSphere prepared.");

	return(true);
}


/*
Adds a new camera to the VSphere
-- Arguments:
index: Index of the camera
channel: Channel of the camera (for example which input from a multi-video grabber shall be used). Use -1 to use only the index.
focus_value: Focus which is available on some webcams
location: 3D location of the camera in space relative to the corigin at 0,0,0. The orientation of the camera is automatically comptued towards this origin.
offset: After orientation is computed, this offset can be added to the location and shift the camera. This makes only sense to recover imperfectly adjusted, recorded material.
-- Returns:
An index to be used for the recording handler.
*/
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ConfigureCamera(int hardware_index, int hardware_channel, int focus_value, int location_x, int location_y, int location_z, int offset_x, int offset_y, int offset_z)
{
	int ind;
	if (hardware_channel < 0)
		ind = camera_set->addCamera(hardware_index,
			vector2di(640, 480), // Todo: Add automatical retrieval of the maximum camera resolution or allow this as external arguments
			vector3df(location_x, location_y, location_z),
			vector3df(offset_x, offset_y, offset_z),
			focus_value);
	else
		ind = camera_set->addCamera(hardware_index,
			hardware_channel,
			vector2di(640, 480),
			vector3df(location_x, location_y, location_z),
			vector3df(offset_x, offset_y, offset_z),
			focus_value);

	addInfoLine("Configured camera with index " + to_string(ind) + ".");

	return(ind);
}

/*
Configure a recording handler. This allows to either record the entire streams as well as the background reference to files, or paly fromt hsoe files.
-- Arguments:
index: Number returned by the ConfigureCamera() function.
file_path: File to write or laod from
type: 0 = nothing; 1 = new recording; 2 = playback;
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ConfigureRecordHandler(int configured_camera_index, char* file_path, int type)
{
	string path = MakeStringCopy(file_path);

	if (type == 1)
	{
		recorder_set->addRecord(configured_camera_index, path);
		addInfoLine("Configured video recording of camera with index " + to_string(configured_camera_index) + ".");
	}
	if (type == 2)
	{
		recorder_set->playRecord(configured_camera_index, path);
		addInfoLine("Configured video playback for camera with index " + to_string(configured_camera_index) + ".");
	}
	addInfoLine("File path is: " + path);

	return(true);
}

/*
Starts the actual sphere (in an alternate thread) and returns. This has to be called after cameras and records have been set.
-- Arguments:
preview_window_variant: 0 = no window; 1 = separate windows for every camera; 2 = combined window for all cameras
preview_type: Type of preview 0-9 (Look in Global -> "Settings.cpp" the array "preview_type_text")
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API StartSphere(int preview_window_variant, int preview_type)
{
	if (sphere_already_running) return(false); // Return if already started

	addInfoLine("Starting VSphere.");

	if (preview_window_variant > 0) addInfoLine("Using preview window.");
	else addInfoLine("Using no preview window.");

	Settings::changePreviewWindowVariant(preview_window_variant);
	Settings::changePreviewType(preview_type);

	// Start the actual VSphere in another thread so the DLL function can return
	outer_sphere_thread = new thread(startThreadedSphere);

	return(true);
}

/*
Actually starting the VSphere.
This thread does not finish until the sphere has not closed and cleaned up.
*/
void startThreadedSphere()
{
	// Create/start the VR sphere
	VSphere = new SphereControler(camera_set, recorder_set);

	// Enable texture
	if (modelTexture != nullptr)
		VSphere->takeTextureHandle(render_engine_handler, modelTexture, GetRequiredTextureWidth(), GetRequiredTextureHeight());

	sphere_content_coordinates = VSphere->getSphereContentCoordinates();
	
	// Signalize that the sphere is ready. // TODO: Replace this boolean by an atomic if unstable
	sphere_already_running = true;

	VSphere->joinSphereThread(); // Join the inner thread of the VSphere. This function returns once the sphere has been closed.

	delete(VSphere); // Call the destructor to free memmory


	addInfoLine("Finishing cameras.");
	delete(camera_set); // Call the destructor to free memmory

	addInfoLine("Finishing recordings.");
	delete(recorder_set); // Call the destructor to free memmory

	addInfoLine("VSphere quitted succesfully.");

	// Allow the system to restart
	sphere_already_running = false;
}

/*
Recompute the background reference at any time from an external command.
*/
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API RecomputeBackgroundReference()
{
	VSphere->computeBackgroundReference();
}

/*
Close the VSphere
*/
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API  QuitSphere()
{
	addInfoLine("Called to quit VSphere.");
	VSphere->quit();
}


/*
Modify a series of settings.
This simple but somewhat insecure method based on a string as a command is used to avoid dozens of functions.
TODO: Change
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetInternalData(char* data_element)
{
	string element = MakeStringCopy(data_element);

	addInfoLine("Received command: " + element);
	
	if (element == "Full rays: true")
	{
		VSphere->setShowFullRays(true);
		return(true);
	}

	if (element == "Full rays: false")
	{
		VSphere->setShowFullRays(false);
		return(true);
	}

	if (element == "Last preview type")
	{
		Settings::changePreviewType(-1);
		addInfoLine("Switched to preview type: " + Settings::getPreviewString());
		return(true);
	}

	if (element == "Next preview type")
	{
		Settings::changePreviewType(-2);
		addInfoLine("Switched to preview type: " + Settings::getPreviewString());
		return(true);
	}


	for (int i = 0; i < Settings::getMaxPreviewTypes(); ++i)
	{
		if (element == "Preview type: " + to_string(i))
		{
			Settings::changePreviewType(i);
			addInfoLine("Switched to preview type: " + Settings::getPreviewString());			
			return(true);
		}
	}
	
	if (element == "Preview window variant: 0")
	{
		Settings::changePreviewWindowVariant(0);
		VSphere->initPreviewWindows();
		addInfoLine("Disabled preview window.");
		return(true);
	}

	if (element == "Preview window variant: 1")
	{
		Settings::changePreviewWindowVariant(1);
		VSphere->initPreviewWindows();
		addInfoLine("Enabled preview windows.");
		return(true);
	}

	if (element == "Preview window variant: 2")
	{
		Settings::changePreviewWindowVariant(2);
		VSphere->initPreviewWindows();
		addInfoLine("Enabled preview windows.");
		return(true);
	}


	if (element == "Last preview window order offset")
	{
		Settings::changePreviewWindowOrderOffset(-1, camera_set->getCount());
		addInfoLine("Changed preview window order offset.");
		return(true);
	}

	if (element == "Next preview window order offset")
	{
		Settings::changePreviewWindowOrderOffset(-2, camera_set->getCount());
		addInfoLine("Changed preview window order offset.");
		return(true);
	}


	for (int i = 0; i < camera_set->getCount(); ++i)
	{
		if (element == "Preview window order offset: " + to_string(i))
		{
			Settings::changePreviewWindowOrderOffset(i, camera_set->getCount());
			addInfoLine("Changed preview window order offset.");
			return(true);
		}
	}

	addError("Command NOT RECOGNIZED! String: " + element);

	return(false);
}


/*
Modify a series of settings and data.
This simple but somewhat insecure method based on a string as a command is used to avoid dozens of functions.
TODO: Change
*/
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetInternalData(char* data_element)
{
	string element = MakeStringCopy(data_element);

	if (element == "Is running")
		return(sphere_already_running ? 1 : 0);

	if (element == "Number of cameras")
		return(camera_set->getCount());

	if (element == "Number of preview modes")
		return(Settings::getMaxPreviewTypes());

	if (element == "Preview type")
		return(Settings::getPreviewType());

	if (element == "Preview window order offset")
		return(Settings::getPreviewWindowOrderOffset());


	for (int i = 0; i < camera_set->getCount(); i++)
	{
		if (element == "Origin of camera " + to_string(i) + "value X")
			return(camera_set->getCameraSource(i)->getOrigin().X);
		if (element == "Origin of camera " + to_string(i) + "value Y")
			return(camera_set->getCameraSource(i)->getOrigin().Y);
		if (element == "Origin of camera " + to_string(i) + "value Z")
			return(camera_set->getCameraSource(i)->getOrigin().Z);

		if (element == "Size of camera " + to_string(i) + "value X")
			return(camera_set->getCameraSource(i)->getSize().X);
		if (element == "Size of camera " + to_string(i) + "value Y")
			return(camera_set->getCameraSource(i)->getSize().Y);
	}

}



// Functions regarding texture

/*
Get the size of the texture required to cover all cameras.
All cameras have to be configured before calling this but the Sphere does not nee dto have been started.
*/
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRequiredTextureWidth()
{
	if (camera_set->getCount() == 0)
	{
		addError("Trying to get texture size with no configured cameras!");
		return(0);
	}

	int width = 0;
	for (int i = 0; i < camera_set->getCount(); ++i)
		width += camera_set->getCameraSource(i)->getSize().X; // Todo: Currently all cameras are aligned horizintally. They should be changed to a square pattern

	return(roundToNextPotency(width, 2));
}

/*
Get the size of the texture required to cover all cameras.
All cameras have to be configured before calling this but the Sphere does not nee dto have been started.
*/
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRequiredTextureHeight()
{
	if (camera_set->getCount() == 0)
	{
		addError("Trying to get texture size with no configured cameras!");
		return(0);
	}

	int height = camera_set->getCameraSource(0)->getSize().Y;  // Todo: Currently all cameras are aligned horizontally. They should be changed to a square pattern
	return(roundToNextPotency(height, 2));
}


/*
Fucntion to transfer the poitner of the texture created on Unity side.
This pointer is used to actualize the texture afetr every frame.
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ProvideTextureHandle(void* textureHandle)
{
	modelTexture = (ID3D11Texture2D*)textureHandle;
	assert(modelTexture);

	return(true);
}



// Functions for transfering the model

/*
Returns when a new model has been show
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CheckNewModel()
{
	if (!sphere_already_running) return(false);

	return(VSphere->checkNewModelFrame());
}

// Wait until the next frame is ready
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API WaitForNextModel()
{
	if (!sphere_already_running) return;

	while (!VSphere->checkNewModelFrame()) {};
	VSphere->waitForNextFrame();
}

/*
Retrieve the new model and provides the result in data pointers.
-- Arguments:
quadsData: pointer to an array of INTs which will contain the quads and UV data
quadsCount: pointer to a single int telling the number of quads

The array contains the data of the new model which is formed by quads (four points on one plane).
The following values are used for eevry quad:
Value 1-3: X, Y and Z coordinate of the first point multiplicated by 1000 (to achieve accuray but not require to transfer floats)
Value 4-6: X, Y and Z coordinate of the second point multiplicated by 1000
Value 7-9: X, Y and Z coordinate of the third point multiplicated by 1000
Value 10-12: X, Y and Z coordinate of the fourth point multiplicated by 1000
Value 13-20: U and V coordinates for every point in pairs of two.
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API StartRetrievingModel(int** quadsData, int* quadsCount)
{
	// Lock the output
	VSphere->lockOutput();

	*quadsData =  sphere_content_coordinates->data();
	*quadsCount = sphere_content_coordinates->size() / 20; // Divided by 20 because 20 values per quad)
	//*quadsCount = VSphere->getSphereContentSize() / 20;

	return true;
}

/*
End retrieving the new model (unlocks the data to be overwritten by the next model, however the next frame is already being processed in the meantime)
*/
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API EndRetrievingModel()
{
	VSphere->updateTexture();

	// Unlock the output
	VSphere->releaseOutput();

	return true;
}





// Shows the console
void startConsole()
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	printf("Console started.\n");
}

void hideConsole()
{
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);
}

void ClearScreen()
{
	system("cls"); // Only on windows; Todo: Alternatives
}

// Transforms external char data to a string
string MakeStringCopy(const char* str) {
	if (str == NULL) return NULL;
	char* res = (char*)malloc(strlen(str) + 1);
	strcpy(res, str);
	return(String(res));
}

// Rounds to the next given potency (required to have textures of 2^X size)
int roundToNextPotency(int value, int potency)
{
	int val = potency;
	for (; val < value; val *= potency) {}
	return(val);
}