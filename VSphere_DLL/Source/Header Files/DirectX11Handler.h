/*
This file is based on the official Unity Low-Level Native Plugin example:
https://docs.unity3d.com/Manual/NativePluginInterface.html
*/

#pragma once

#include "simplifyingHeader.h"

#include <IUnityGraphicsD3D11.h>
#include <d3d11.h>


class DirectX11Handler
{
private:
	ID3D11Device* device;

public:

	DirectX11Handler(IUnityInterfaces* unityInterfaces);

	ID3D11Device* getDevice();

	void updateSimpleSubresource(ID3D11Resource *pDstResource, const void *pSrcData, UINT SrcRowPitch);

};

 