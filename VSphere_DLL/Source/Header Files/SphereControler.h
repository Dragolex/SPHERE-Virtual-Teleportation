#pragma once


#include "simplifyingHeader.h"

#include "PerCamControler.h"
#include "SimpleNamedWindow.h"

#include <vector>

#include <windows.h>
#include <mutex>

#include <DirectX11Handler.h>


class SphereControler
{
private:
	int cam_count;

	CameraHandler * camera_set = nullptr;
	RecordingHandler * records = nullptr;

	vector<PerCamControler*> camera_controlers;
	vector<SimpleNamedWindow*> preview_windows;
	SimpleNamedWindow * combined_preview_window = nullptr;
	vector<cv::Mat*> combined_preview_split_mats;

	int sphere_running;
	vector<HANDLE> camera_threads;

	thread * localSphereLoop;
	bool threaded_content_computed_and_ready;

	vector<int> * complete_sphere_content;

	// For thread coordination
	vector<HANDLE> worker_sems;
	HANDLE data_output_sem;

	mutex * data_output_lock;
	mutex * data_output_check_lock;

	mutex * computation_lock;


	// For texture
	bool texture_enabled = false;
	DirectX11Handler *render_engine_handler;
	ID3D11Texture2D * model_texture;
	unsigned char* model_texture_data;
	int model_texture_width, model_texture_height;
	


	// volatiles
	volatile bool has_new_model_frame = false;
	




	static void launchSphereLoop(SphereControler * thisControler, vector<HANDLE> * workerSems);
	void sphereLoop(vector<HANDLE> * workerSems);


	void handlePreviewWindows();


public:
	SphereControler(CameraHandler * camera_set, RecordingHandler * records);
	void joinSphereThread();
	~SphereControler();

	void quit();

	void computeBackgroundReference();

	bool checkNewModelFrame();
	void waitForNextFrame();

	void lockOutput();
	void releaseOutput();

	void updateTexture();

	void takeTextureHandle(DirectX11Handler *renderEngineHandler, ID3D11Texture2D* modelTexture, int width, int height);
	
	int getSphereContentSize();
	vector<int> * getSphereContentCoordinates();

	void initPreviewWindows();

	void setShowFullRays(bool showRays);
};

