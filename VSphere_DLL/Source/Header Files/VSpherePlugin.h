#pragma once

#include "simplifyingHeader.h"
#include "UnityInterface.h"




extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload();



extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API PrepareSphere(bool open_console, int recorder_frame_duration_ms);

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetInternalData(char* data_element);
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetInternalData(char* data_element);

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ConfigureCamera(int hardware_index, int hardware_channel, int focus_value, int location_x, int location_y, int location_z, int offset_x, int offset_y, int offset_z);

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ConfigureRecordHandler(int configured_camera_index, char* file_path, int type);

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API StartSphere(int preview_window_variant, int preview_type);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API RecomputeBackgroundReference();

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API QuitSphere();






extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRequiredTextureWidth();
extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRequiredTextureHeight();

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ProvideTextureHandle(void* textureHandle);



extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CheckNewModel();
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API WaitForNextModel();

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API StartRetrievingModel(int** quadsData, int* quadsCount);
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API EndRetrievingModel();






void startThreadedSphere();

void startConsole();
void hideConsole();
void ClearScreen();
string MakeStringCopy(const char* str);
int roundToNextPotency(int value, int potency);
