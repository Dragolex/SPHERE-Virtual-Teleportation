/*
This file is based on the official Unity Low-Level Native Plugin example:
https://docs.unity3d.com/Manual/NativePluginInterface.html
*/

#pragma once
#include "UnityInterface.h"

// Should only be used on the rendering thread unless noted otherwise.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D11)
{
	ID3D11Device* (UNITY_INTERFACE_API * GetDevice)();
};
UNITY_REGISTER_INTERFACE_GUID(0xAAB37EF87A87D748ULL,0xBF76967F07EFB177ULL,IUnityGraphicsD3D11)
