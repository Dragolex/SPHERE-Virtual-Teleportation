/*
This class handles all core classes from the "FrameProcessing" folder and therefore all computing which happens on a single thread and for one camera.
An instance of this class will be created by the SphereControler for every camera.

@Author: Alexander Georgescu
*/

#include "stdafx.h"

#include "PerCamControler.h"
#include "BackgroundReference.h"
#include "ContoursExtractor.h"
#include "EdgesIdentifier.h"
#include "RayGenerator.h"
#include "ModelBuilder.h"

#include <ctime>


/*
Create the controler based on the camera handlers and the idnex of the associated camera.
*/
PerCamControler::PerCamControler(CameraHandler * camera_set, RecordingHandler * records, int camera_list_index, mutex * computation_lock)
{
	// Recorder (see the function getFrame() )
	this->records = records;
	this->camera_list_index = camera_list_index;
	this->camera_source = camera_set->getCameraSource(camera_list_index);

	// This lock is currently not used (is only there for possibledebug purpose)
	//this->computation_lock = computation_lock;

	tex_offs_x = camera_source->getIndex()*camera_source->getSize().X;
	tex_offs_y = 0; // Todo: Change if camera textures are aligned differently (not simply horizontally)


	frame_task_mode = 0; // Start without any task
	next_frame_task_mode = frame_task_mode;


	// Prepare objects required for computing the frame
	background_reference = new BackgroundReference();
	contours_extractor = new ContoursExtractor();
	edges_identifier = new EdgesIdentifier();
	ray_generator = new RayGenerator(camera_source);
	model_computer = new ModelBuilder();


	// Init semaphores
	controlSem = (PHANDLE)CreateSemaphore(NULL, 1, 1, NULL);
	workerSem = (PHANDLE)CreateSemaphore(NULL, 0, 1, NULL);
}

PerCamControler::~PerCamControler()
{
	processing_thread.join(); // Wait for the loop thread to finish

	delete(background_reference);
	delete(contours_extractor);
	delete(edges_identifier);
	delete(ray_generator);
	delete(model_computer);

	delete(output_content);
}


/*
Initialize the camera and return whetehr the camera has been accessed sucessfully.
*/
bool PerCamControler::initialize()
{
	records->startRecordOrPlay(camera_list_index);

	// If not currently reading from file
	if (!records->isPlaying(camera_list_index))
	{
		capture = new VideoCapture(camera_source->getIndex());

		if (!capture->isOpened())
			return(false);

		if (camera_source->getFocusValue() != -1)
		{
			capture->set(CAP_PROP_AUTOFOCUS, 0);
			capture->set(CAP_PROP_FOCUS, camera_source->getFocusValue());
		}
		else
			capture->set(CAP_PROP_AUTOFOCUS, 1);
	}
	else
		addInfoLine("Reading data for " + camera_source->getName() + " from file.");


	frame_task_mode = 1;
	// Set the first iteration to start with computing the background reference
	computeBackgroundReference();


	// Start the actual thread
	processing_thread = thread(launchFrameLoop, this);

	return(true);
}


/*
Add the reference to another camera (requried to compute the actual mdoel frame content in the end)
*/
void PerCamControler::referenceOtherCamera(PerCamControler * other_controler)
{
	model_computer->referenceAnotherRayGenerator(other_controler->getRayGenerator(), false);
}


/*
Launch the loop for this camera.
*/
void PerCamControler::launchFrameLoop(PerCamControler * thisControler)
{
	thisControler->frameLoop();
}
void PerCamControler::frameLoop()
{
	cam_running = true;

	timeBench bench(0);
	valueBench averageSegments;
	valueBench averageComputingTime;

	int preview_mode = Settings::getPreviewType();

	while (cam_running) // Loops
	{
		/*
		if (!running.load()) // Use busy-waiting when whole VSphere system is paused because processing power is notsignificant then
		{
			ReleaseSemaphore(workerSem, 1, NULL); // Send signal to launchSphereLoop
			continue;
		};
		*/

		while(!compute_frame)
			WaitForSingleObject(controlSem, Settings::getThreadTimeoutMS()); // Wait for signal from the "sphereLoop" in the SphereControler
		compute_frame = false;

		has_computed = false;




		preview_mode = Settings::getPreviewType();


		switch (frame_task_mode)
		{
		case 0: break; // Nothing
		case 1: // Processing the frame by computing the segments
			{
				if (!initialized) break;

				getFrame(); // Retrieve the frame from the camera or record


				bench.startTime();


				// Compute the binary mask
				background_reference->computeRGBbinaryMask();
				// Compute the contours
				contours_extractor->computeContour();
				// Compute the edges
				edges_identifier->computeEdges(/*preview_mode!=7*/ false, &averageSegments);

				//computation_lock->lock();
				// Generate the rays
				ray_generator->generateRays();
				//computation_lock->unlock();


				// Update the global texture
				if (texture_enabled)
					updateModelTextureRegion();

				/*
				// Some settings receivable from the camera

				//CAP_PROP_AUTO_EXPOSURE CAP_PROP_EXPOSURE CAP_PROP_BRIGHTNESS
				cout << "Backlight: " << capture->get(CAP_PROP_BACKLIGHT) << endl;
				cout << "Aperture: " << capture->get(CAP_PROP_APERTURE) << endl;
				cout << "Gain: " << capture->get(CAP_PROP_GAIN) << endl;
				cout << "Settings: " << capture->get(CAP_PROP_SETTINGS) << endl;
				cout << "White bal U: " << capture->get(CAP_PROP_WHITE_BALANCE_BLUE_U) << endl;
				cout << "White bal V: " << capture->get(CAP_PROP_WHITE_BALANCE_RED_V) << endl;
				*/

				bench.pauseTime();

				// Handle the preview image
				handlePreview(preview_mode);
			}
			break;
		case 2: // Re-initialize by computing a new background reference
			{
				getFrame(); // Retrieve the frame from the camera or record

				if (!records->isPlaying(camera_list_index)) // If not reading from a record
				{
					int frameNum = Settings::getBackgroundReferenceComputingFrames();

					background_reference->startNewBackground(&current_frame, frameNum);

					for (int i = 0; i < frameNum; i++)
					{
						capture->grab(); // Grab for every channel here (coordinating threads like in the "frameLoop"
										 // would overcomplicate things and timing is not relevant in this case
						capture->retrieve(current_frame, camera_source->getChannel());

						// Add the frame to the background reference
						background_reference->addFrame();
					}

					// Finalize the new background reference
					background_reference->finalizeBackground();

					addInfoLine("Computed background reference for camera: " + camera_source->getName());
				}
				else // If reading from a record
				{
					// A background reference is required neevrtheless but it wont be made from new frames
					background_reference->startNewBackground(&current_frame, -1);
				}

				// If reading (playing) from a record, the background reference will be set here from the file. If currently creating a new record, the file will be saved.
				records->handleBackgroundImage(camera_list_index, background_reference->getBackground());
				background_reference->finalizeBackground();



				// Reinitialize the contorus extractor
				contours_extractor->initData(&current_frame, background_reference->getBackground(), background_reference->getBinaryMask());
				// Reinitialize the edges identifier
				edges_identifier->initData(current_frame.cols, current_frame.rows, contours_extractor->getContourGrid(), contours_extractor->getInoutGrid());
				// Reinitialize the ray generator
				ray_generator->initData(edges_identifier->getEdgesStarts(), edges_identifier->getEdgesEnds(), edges_identifier->getEdgesOrientations(), getTexOffsetX(), getTexOffsetY());
				// Reinitialize the model computer
				model_computer->referenceAnotherRayGenerator(ray_generator, true);
				

				preview_image = background_reference->getBackground()->clone();


				bench.resetTime();

				frame_task_mode = next_frame_task_mode;

				initialized = true;
			}
			break;

		case 3: // Process the content of the frame based on the current segments
			{
				bench.startTime();

				//computation_lock->lock();

				// Compute the intersections of rays
				model_computer->intersectRays();

				if (show_rays)  // ((time(0) % 2) == 1)
					ray_generator->visualizeRays(output_content, 640);
				else
					model_computer->computeModelPart(output_content);

				//computation_lock->unlock();


				// Finalize bench
				bench.endTime();


				// Delay frame if required (only used when reading a record from file)
				if (records != nullptr)
					records->delayFrame(camera_list_index);


				//// Display some debug bench values
					if (bench.getAverage() != 0)
						averageComputingTime.addValue(bench.getAverage());
					bench.printAverage(1, ("Calculation for camera " + camera_source->getName() + " took %f milliseconds.\n").c_str());

					if (records->justLooped(camera_list_index))
					{
						averageSegments.printAverageFull(-1, "Average segments detected in camera " + camera_source->getName() + ": %f");
						averageSegments.resetValue();

						averageComputingTime.printAverageFull(-1, "Average computation time for camera " + camera_source->getName() + ": %f");
						averageComputingTime.resetValue();
					}
				////

			}
			break;
		}
		frame_task_mode = next_frame_task_mode;


		has_computed = true;
		ReleaseSemaphore(workerSem, 1, NULL); // Send signal to the sphereLoop (SphereControler) to continue
	}

	addInfoLine("Quitting thread for: " + camera_source->getName());
}


/*
For cameras with subChannel. Execute before calling readFrame() in different threads
*/
void PerCamControler::grabFrame()
{
	if (camera_source->getIsGrabberChannel())
		if (capture != nullptr)
			capture->grab();
}

/*
Handle the current frame image
*/
void PerCamControler::getFrame()
{
	if (capture != nullptr)
		capture->retrieve(current_frame, camera_source->getChannel()); // get from camera

	if (records != nullptr)
		records->handleFrame(camera_list_index, &current_frame); // Either get from video record instead or save the frame fromt he camera into a new video
}



/*
Continue (or initially start) the frame loop by setting the semaphore.
*/
void PerCamControler::processNextFrame()
{
	ReleaseSemaphore(controlSem, 1, NULL);
	compute_frame = true;
}


/*
In the next iteration of the loop: Get frames and compute the background reference
*/
void PerCamControler::computeBackgroundReference()
{
	addInfoLine("Computing background reference for camera: " + camera_source->getName());

	int cur = frame_task_mode;
	frame_task_mode = 2;
	next_frame_task_mode = cur;
}

/*
In the next iteration of the loop: Process the most actual frame and compute the edges and generate the rays
*/
void PerCamControler::computeFrameProcess()
{
	frame_task_mode = 1;
	next_frame_task_mode = 1;
}

/*
In the next iteration of the loop: Use the currently processed frame and compute the collissions of rays as well as the content of the sphere (the quads)
*/
void PerCamControler::computeSphereContent()
{
	frame_task_mode = 3;
	next_frame_task_mode = 3;
}

/*
Quit the processing loop of this camera.
*/
void PerCamControler::quit()
{
	cam_running = false;
	frame_task_mode = -1;
	next_frame_task_mode = -1;
}


/*
Take and save the texture pointer from the render engine
*/
void PerCamControler::takeTextureHandle(unsigned char* model_texture_data, int model_texture_width, int model_texture_height)
{
	texture_enabled = true;

	this->model_texture_data = model_texture_data;
	this->model_texture_width = model_texture_width;
	this->model_texture_height = model_texture_height;
}


/*
Update the texture through the pointer from the render engine
*/
void PerCamControler::updateModelTextureRegion()
{
	int w = current_frame.cols * 3;
	int h = current_frame.rows;

	int pixelcount = w*h;
	int right_offset = (tex_offs_x + (model_texture_width - (tex_offs_x + w / 3))) * 4;

	int j = (tex_offs_y*model_texture_width + tex_offs_x) * 4;

	uchar* d = current_frame.ptr<uchar>(0);
	
	for (int i = 0; i < pixelcount; i += 3)
	{
		if ((i % w) == 0)
			if (i != 0)
				j += right_offset;

		model_texture_data[j] = d[i + 2];
		model_texture_data[j + 1] = d[i + 1];
		model_texture_data[j + 2] = d[i];
		model_texture_data[j + 3] = 255;
		j += 4;
	}
}


/*
Generate the correct preview image and apply it to the preview_image
*/
void PerCamControler::handlePreview(int preview_mode)
{
	if (!initialized) return;

	// Clear preview image
	if (preview_mode != 0)
	{
		uchar* d = preview_image.ptr<uchar>(0);
		int pixelcount = camera_source->getPixelCount();
		for (int i = 0; i < pixelcount * 3; i += 1)
			d[i] = 0;
	}

	switch (preview_mode)
	{
	case 1: current_frame.copyTo(preview_image); break;
	case 2: background_reference->getBackground()->copyTo(preview_image); break;
	case 3: background_reference->previewNonbackgroundImageRGB(&preview_image, false); break;
	case 4: background_reference->previewNonbackgroundImageRGB(&preview_image, true); break;
	case 5: contours_extractor->previewInoutMask(&preview_image); break; // previewBinaryMask
	case 6: contours_extractor->previewContourMask(&preview_image); break;
	case 7:	contours_extractor->previewContourKeypointMask(&preview_image); break;
	case 8: edges_identifier->previewEdges2D(&preview_image); break;
	case 9: current_frame.copyTo(preview_image); edges_identifier->previewEdges2D(&preview_image); break;
	}


	/*
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


	// Draw preview name text
	if (preview_mode != 0)
		putText(preview_image, "Preview of " + camera_source->getName() + " - " + Settings::getPreviewString(), Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5f, Scalar(255, 255, 255), 1, LINE_AA);
	if ((preview_mode == 7) || (preview_mode == 8) || (preview_mode == 9))
		putText(preview_image, "Segments: " + to_string(edges_identifier->getEdgesEnds()->size()), Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.5f, Scalar(255, 255, 255), 1, LINE_AA);


	// Draw the guidance lines ontop

	line(preview_image, Point(0, camera_source->getSize().Y / 2), Point(camera_source->getSize().X, camera_source->getSize().Y / 2), Scalar(0, 0, 200), 1, CV_AA);
	line(preview_image, Point(camera_source->getSize().X / 2, 0), Point(camera_source->getSize().X / 2, camera_source->getSize().Y), Scalar(0, 0, 200), 1, CV_AA);

	line(preview_image, Point(3 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 - 5), Point(3 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 + 5), Scalar(0, 0, 200), 1, CV_AA);
	line(preview_image, Point(5 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 - 5), Point(5 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 + 5), Scalar(0, 0, 200), 1, CV_AA);

	line(preview_image, Point(camera_source->getSize().X / 2 - 5, 3 * (camera_source->getSize().Y / 8)), Point(camera_source->getSize().X / 2 + 5, 3 * (camera_source->getSize().Y / 8)), Scalar(0, 0, 200), 1, CV_AA);
	line(preview_image, Point(camera_source->getSize().X / 2 - 5, 5 * (camera_source->getSize().Y / 8)), Point(camera_source->getSize().X / 2 + 5, 5 * (camera_source->getSize().Y / 8)), Scalar(0, 0, 200), 1, CV_AA);

	line(preview_image, Point(2 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 - 8), Point(2 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 + 8), Scalar(0, 0, 200), 1, CV_AA);
	line(preview_image, Point(6 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 - 8), Point(6 * (camera_source->getSize().X / 8), camera_source->getSize().Y / 2 + 8), Scalar(0, 0, 200), 1, CV_AA);

	line(preview_image, Point(camera_source->getSize().X / 2 - 8, 2 * (camera_source->getSize().Y / 8)), Point(camera_source->getSize().X / 2 + 8, 2 * (camera_source->getSize().Y / 8)), Scalar(0, 0, 200), 1, CV_AA);
	line(preview_image, Point(camera_source->getSize().X / 2 - 8, 6 * (camera_source->getSize().Y / 8)), Point(camera_source->getSize().X / 2 + 8, 6 * (camera_source->getSize().Y / 8)), Scalar(0, 0, 200), 1, CV_AA);
}


// Some getters

vector<int> * PerCamControler::getSphereContent()
{
	return(output_content);
}

HANDLE PerCamControler::getSem()
{
	return(workerSem);
}

thread* PerCamControler::getLoopThread()
{
	return(&processing_thread);
}

Mat PerCamControler::getCurrentFrame()
{
	return(current_frame);
}

Mat PerCamControler::getPreviewImage()
{
	return(preview_image);
}

RayGenerator * PerCamControler::getRayGenerator()
{
	return(ray_generator);
}

bool PerCamControler::getShowRays()
{
	return(show_rays);
}

void PerCamControler::setShowFullRays(bool show_rays)
{
	this->show_rays = show_rays;
}

bool PerCamControler::hasComputed()
{
	return(has_computed);
}

CameraSource * PerCamControler::getCameraSource()
{
	return(camera_source);
}

int PerCamControler::getTexOffsetX()
{
	return(tex_offs_x);
}

int PerCamControler::getTexOffsetY()
{
	return(tex_offs_y);
}
