#pragma once

#include "simplifyingHeader.h"
#include "thread"
#include "atomic"
#include <mutex>

#include "RecordingHandler.h"

#include "BackgroundReference.h"
#include "ContoursExtractor.h"
#include "EdgesIdentifier.h"
#include "ModelBuilder.h"


class PerCamControler
{
private:
	/// Variables

	RecordingHandler * records = nullptr;
	
	CameraSource * camera_source;

	int camera_list_index;

	bool cam_running = false;

	bool show_rays = false;

	boolean initialized = false;

	volatile bool compute_frame = false;
	volatile bool has_computed = false;

	mutex * computation_lock;


	// Texture related
	bool texture_enabled = false;
	int tex_offs_x, tex_offs_y, model_texture_width, model_texture_height;
	unsigned char* model_texture_data;



	int frame_task_mode, next_frame_task_mode;

	VideoCapture * capture = nullptr;
	Mat current_frame, preview_image;


	// OUTPUT
	vector<int> * output_content = new vector<int>;



	thread processing_thread;

	// Semaphores and atomics for coordinating threads
	//atomic<bool> running;
	HANDLE controlSem, workerSem;


	/// Frame processing objects

	BackgroundReference * background_reference;
	ContoursExtractor * contours_extractor;
	EdgesIdentifier * edges_identifier;
	RayGenerator * ray_generator;
	ModelBuilder * model_computer;

	/// Private functions

	static void launchFrameLoop(PerCamControler * thisControler);

	void frameLoop();

	void getFrame();

	void updateModelTextureRegion();

public:
	PerCamControler(CameraHandler * cameraSet, RecordingHandler * records, int cameraListIndex, mutex * computation_lock);
	~PerCamControler();


	bool initialize();

	void referenceOtherCamera(PerCamControler * other_controler);


	HANDLE getSem();
	thread* getLoopThread();


	void grabFrame();

	void takeTextureHandle(unsigned char* model_texture_data, int model_texture_width, int model_texture_height);
	

	Mat getCurrentFrame();
	Mat getPreviewImage();

	RayGenerator * getRayGenerator();

	void processNextFrame();

	void computeFrameProcess();
	void computeSphereContent();


	vector<int> * getSphereContent();

	
	void computeBackgroundReference();

	void handlePreview(int preview_mode);

	bool getShowRays();
	void setShowFullRays(bool show_rays);

	bool hasComputed();




	CameraSource * getCameraSource();

	int getTexOffsetX();
	int getTexOffsetY();


	// Quit the loop
	void quit();


	/*
	void stopProcessing();

	void continueProcessing();
	*/
};

