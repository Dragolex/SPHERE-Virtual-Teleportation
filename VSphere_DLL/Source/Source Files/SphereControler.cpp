/*
This core class represents a "VSphere" and is created when starting the sphere through the external DLL functions
See PluginInterface - > VSpherePlugin.cpp for usage.

The class initializes the cameras with their own threads and handles a loop which coordinates the threads with semaphores.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "SphereControler.h"

#include "CameraSource.h"
#include "PerCamControler.h"
#include "RecordingHandler.h"


/*
Constructor based on the camera- and record-handlers.
It starts the external loops and returns (use the joinSphereThread() function below to join the trhead until the Spehre has been quitted).
*/
SphereControler::SphereControler(CameraHandler * camera_set, RecordingHandler * records)
{
	this->camera_set = camera_set;
	cam_count = camera_set->getCount();

	this->records = records;


	complete_sphere_content = new vector<int>;
	threaded_content_computed_and_ready = false;



	// Semaphore for handling the data output through the plugin
	// It will stop processing if currently reading from the data array
	data_output_sem = (PHANDLE)CreateSemaphore(NULL, 1, 1, NULL);

	// Data mutex for the output and check lock
	data_output_lock = new mutex();
	data_output_check_lock = new mutex();

	// Mutex for final computation
	computation_lock = new mutex();



	addInfoLine("Loading " + to_string(cam_count) + " cameras.");


	if (Settings::getPreviewWindowVariant() > 0) // If preview active
		initPreviewWindows();


	// Create all camera controlers (has to happen first)
	for (int c = 0; c < camera_set->getCount(); c++)
	{
		camera_controlers.push_back(new PerCamControler(camera_set, records, c, computation_lock));
	}


	// Reference every camera to each other (the ModelComputers in the end require the final results from all other cameras processings). 
	for (int c = 0; c < camera_set->getCount(); c++)
	{
		for (int d = 0; d < camera_set->getCount(); d++)
		{
			if (c != d)
				camera_controlers[c]->referenceOtherCamera(camera_controlers[d]); // Reference the other cameras
		}
	}



	addInfoLine("Cameras created.");


	/*
	Initializes the video input of every camera.
	By using openCV it creates a handle to the coresponding hardware camera. This allows to check whether the camera actually exists.
	If the camera does exist, the initialization function here creates the individual threads which
	constantly handle the associated camera (synchronized with the main trhead through semaphores).

	Note: The function call internally handles whether the real camera is accessed, or it just opens the video file of a recorder!
	*/
	for (int c = 0; c < cam_count; c++)
	{
		if (camera_controlers[c]->initialize())
			addInfoLine(camera_controlers[c]->getCameraSource()->getName() + " opened successfully.");
		else
			addError(camera_controlers[c]->getCameraSource()->getName() + " FAILED to open!");
			// Todo: Add a safe error handling; perhaps abortingt he initialisation of whole Sphere.
	}

	addInfoLine("All cameras opened.");


	// Get the required worker semaphores
	for (int c = 0; c < cam_count; c++)
		worker_sems.push_back(camera_controlers[c]->getSem());


	addInfoLine("STARTING SPHERE!");


	// Start the sphere loop
	localSphereLoop = new thread(launchSphereLoop, this, &worker_sems);



	// Start processing of the camera
	for (int c = 0; c < cam_count; c++)
		camera_controlers[c]->processNextFrame();

}

/*
Join the loop thread to the executing thread
*/
void SphereControler::joinSphereThread()
{
	localSphereLoop->join();
}


/*
Destructor.
*/
SphereControler::~SphereControler()
{
	addInfoLine("Quitting Sphere.");

	// Once the thread loop has finished, delete the data
	for (int c = 0; c < cam_count; c++)
	{
		if (camera_controlers.size() > c)
			delete(camera_controlers.at(c));
		if (preview_windows.size() > c)
			delete(preview_windows.at(c));
		if (c < cam_count - 1)
			delete(combined_preview_split_mats[c]);
	}
	delete(combined_preview_window);

	data_output_lock->unlock();
	data_output_check_lock->unlock();

	delete(data_output_lock);
	delete(data_output_check_lock);

	if (texture_enabled)
		delete[](unsigned char*)model_texture_data;
}


/*
Causes the sphere to quit.
*/
void SphereControler::quit()
{
	for (int c = 0; c < cam_count; c++)
	{
		camera_threads.push_back((*camera_controlers[c]->getLoopThread()).native_handle());
		camera_controlers[c]->quit();
	}

	sphere_running = 0; // Thread about to finish	
}

/*
Causes the background reference to be recomputed.
*/
void SphereControler::computeBackgroundReference()
{
	for (int c = 0; c < cam_count; c++)
		camera_controlers[c]->computeBackgroundReference();
}


/*
Returns whether a new model frame is available.
"Model frame" refers here to the 3D model currently representing the real object.
*/
bool SphereControler::checkNewModelFrame()
{
	bool res = false;

	data_output_check_lock->lock();
	if (has_new_model_frame)
	{
		has_new_model_frame = false;
		res = true;
	}
	data_output_check_lock->unlock();

	return(res);
}


/*
Get the size of elements in the output array.
*/
int SphereControler::getSphereContentSize()
{
	return(complete_sphere_content->size());
}
/*
Get the output array.
*/
vector<int> * SphereControler::getSphereContentCoordinates()
{
	return(complete_sphere_content);
}



/*
Start the loop.
*/
void SphereControler::launchSphereLoop(SphereControler * thisControler, vector<HANDLE> * workerSems)
{
	thisControler->sphereLoop(workerSems);
}
/*
Loop function
*/
void SphereControler::sphereLoop(vector<HANDLE> * workerSems)
{
	sphere_running = 2; // standard running

	camera_threads.clear();

	fpsBench fps_counter;
	int prev_mode = 0;
	bool press = false;

	// Loop
	while (sphere_running>0)
	{
		fps_counter.newFrame(true);


		// Grab the next frame for all channels
		for (int c = 0; c < cam_count; c++)
			camera_controlers[c]->grabFrame();

		// Send signal to the cameraFrameLoops that next frame can be processed.
		for (int c = 0; c < cam_count; c++)
			camera_controlers[c]->processNextFrame();


		// Wait for signal from the cameraFrameLoops that processing has finished.
		bool contin = true;
		while (contin) // Prevent spurious wakeup
		{
			WaitForMultipleObjects(cam_count, &(*workerSems)[0], true, Settings::getThreadTimeoutMS()*1.25f); // not INFINITE 
			contin = false();
			for (int c = 0; c < cam_count; c++)
				if (!camera_controlers[c]->hasComputed()) contin = true;
		}


		
		////// Process everything with the new data
		
		
		
		
		if (!threaded_content_computed_and_ready) // the current loop iteration has processed the frames and computed the segments from the 2D images
		{
			has_new_model_frame = false;

			// -> Give command to compute the sphere content (quads) from the segments
			for (int c = 0; c < cam_count; c++)
				camera_controlers[c]->computeSphereContent();

			threaded_content_computed_and_ready = true;
		}
		else // The current iteration has processed the frames and prepared the content
		{
			data_output_lock->lock();

			// ->Content is ready
			complete_sphere_content->clear();

			// Unite all contents to one array
			for (int c = 0; c < cam_count; c++)
			{
				vector<int> * content_data = camera_controlers[c]->getSphereContent();

				// Combine to the final dataset
				complete_sphere_content->insert(end(*complete_sphere_content), begin(*content_data), end(*content_data));
			}


			data_output_check_lock->lock();
			has_new_model_frame = true;
			data_output_check_lock->unlock();

			data_output_lock->unlock();

			// Allow to continue
			ReleaseSemaphore(data_output_sem, 1, NULL);

			// -> Give command to compute the segments of the next frame
			for (int c = 0; c < cam_count; c++)
				camera_controlers[c]->computeFrameProcess();
		

			//updateTexture(); // moved to the EndRetrievingModel function

			threaded_content_computed_and_ready = false; 
		}
		



		if (Settings::getPreviewType() != 0)
			handlePreviewWindows();


		// This code allows to break the computation at the end of a record (only if the cemera inputs are records)
		// It continues at any key, however the Preview Window or the commandline needs to have focus.
		/*
		if (records!=nullptr)
			if (records->justLooped(0))
				waitKey(0);
		*/

	}
	// The loop has ended -> means the Spehre has been caused to quit


	// Send the signal a last time to allow the upper calls of camera_controlers[c]->quit(); to take effect and the camera threads quit
	for (int c = 0; c < cam_count; c++)
		camera_controlers[c]->processNextFrame();

	time_t ref_time = time(0);

	// Wait until threads have finished
	WaitForMultipleObjects(camera_threads.size(), &(camera_threads[0]), true, Settings::getThreadTimeoutMS());

	if ((difftime(time(0), ref_time) * 1000) >(Settings::getThreadTimeoutMS()*0.8)) // Timed out 
	{
		for (int c = 0; c < cam_count; c++)
			TerminateThread(camera_threads[c], 0);

		addError("Quitting the camera threads required killing!");
	}

	addInfoLine("Quitting main sphere thread.");

	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
};


void SphereControler::lockOutput()
{
	data_output_lock->lock();
}
void SphereControler::releaseOutput()
{
	data_output_lock->unlock();
}

void SphereControler::waitForNextFrame()
{
	WaitForSingleObject(data_output_sem, INFINITE);
}


/*
Add the texture handle which is required to transfer the texture data to the rendering engine.
*/
void SphereControler::takeTextureHandle(DirectX11Handler *render_engine_handler, ID3D11Texture2D* model_texture, int model_texture_width, int model_texture_height)
{
	this->texture_enabled = true;

	this->render_engine_handler = render_engine_handler;
	this->model_texture = model_texture;
	this->model_texture_width = model_texture_width;
	this->model_texture_height = model_texture_height;

	// Alocate the main system memmory to temporarily hold the texture data
	model_texture_data = new unsigned char[model_texture_width * 4 * model_texture_height];

	for (int c = 0; c < cam_count; c++)
		camera_controlers[c]->takeTextureHandle(model_texture_data, model_texture_width, model_texture_height);
}

/*
Update the actual texture from the cameras.
*/
void SphereControler::updateTexture()
{
	if (texture_enabled)
		render_engine_handler->updateSimpleSubresource(model_texture, model_texture_data, model_texture_width * 4);
}

/*
Initialize the preview windows (also called when the preview window variant is changed but not when changing preview type).
*/
void SphereControler::initPreviewWindows()
{
	/*
	// Delete existing windows

	// Todo: Fix this to save memmory (issue is apparently an OpenCV issue as it blocks when trying to close a window)

	if (combined_preview_window != nullptr)
	{
		delete(combined_preview_window);
		combined_preview_window = nullptr;
	}
	for (int i = 0; i < preview_windows.size(); ++i)
		delete(preview_windows[i]);
	preview_windows.clear();
	*/


	addInfoLine("Initializing preview windows.");

	// Location of preview window
	int	windX = 300, windY = 150;
	int refw = camera_set->getCameraSource(0)->getSize().X;
	int refh = camera_set->getCameraSource(0)->getSize().Y;


	// Preview windows
	switch (Settings::getPreviewWindowVariant())
	{
		case 1: // Create separate windows for the preview of every camera
			for (int c = 0; c < camera_set->getCount(); c++)
			{
				if ((c % 2 == 0) && (c != 0)) { windY += refh; windX -= c*refw; } // Variables to form a square with the preview windows
				preview_windows.push_back(new SimpleNamedWindow(camera_set->getCameraSource(c)->getName(), windX, windY)); // Create the new preview window
				addInfoLine("Preview window created with name: " + camera_set->getCameraSource(c)->getName());
				windX += refw;
			}
			startWindowThread();
		break;
		case 2: // Create a combined preview window
			combined_preview_window = new SimpleNamedWindow("Preview Window", windX, windY);

			for (int c = 0; c < camera_set->getCount() - 1; c++)
				combined_preview_split_mats.push_back(combined_preview_window->getMat(Settings::getPreviewScaleFactor()*refw, Settings::getPreviewScaleFactor()*refh));

			combined_preview_window->getMat((1 + Settings::getPreviewScaleFactor())*refw, refh);

			startWindowThread();

			addInfoLine("Created combined preview window.");
		break;
	}


}

/*
Display the preview image.
*/
void SphereControler::handlePreviewWindows()
{
	int refw = camera_set->getCameraSource(0)->getSize().X;
	int refh = camera_set->getCameraSource(0)->getSize().Y;


	switch (Settings::getPreviewWindowVariant())
	{
	case 0: break;
	case 1:
		for (int c = 0; c < cam_count; c++)
			preview_windows[c]->setMat(&(camera_controlers[c]->getPreviewImage()));
		if (combined_preview_window != nullptr)
			combined_preview_window->hide();
		break;
	default:
		int cam = Settings::getPreviewWindowOrderOffset() % cam_count;

		Mat * img = combined_preview_window->getMat();

		// Copy the currently main preview image
		camera_controlers[cam]->getPreviewImage().copyTo((*img)(Rect(0, 0, refw, refh)));

		// Rescale and copy all other windows
		for (int c = 0; c < cam_count - 1; c++)
		{
			if ((++cam) == cam_count) cam = 0;

			resize(camera_controlers[cam]->getPreviewImage(),
				*combined_preview_split_mats[c],
				combined_preview_split_mats[c]->size(),
				0,
				0,
				INTER_CUBIC);

			combined_preview_split_mats[c]->copyTo((*img)(Rect(refw, c*(Settings::getPreviewScaleFactor()*refh), (Settings::getPreviewScaleFactor() * refw), (Settings::getPreviewScaleFactor() * refh))));

			rectangle(*img, Point(refw -1, c*refh*Settings::getPreviewScaleFactor()), Point((1+ Settings::getPreviewScaleFactor())*refw, (c+1)*refh*Settings::getPreviewScaleFactor()), Scalar(255, 255, 255), 1, 8, 0);
		}

		rectangle(*img, Point(0, 0), Point(refw, refh), Scalar(255, 255, 255), 2, 8, 0);

		combined_preview_window->setMat(img);

		for (int c = 0; c < preview_windows.size(); c++)
			preview_windows[c]->hide();
		break;
	}
}


/*
Set whether all rays should be shown.
*/
void SphereControler::setShowFullRays(bool showRays)
{
	for (int c = 0; c < cam_count; c++)
		camera_controlers[c]->setShowFullRays(showRays);
}