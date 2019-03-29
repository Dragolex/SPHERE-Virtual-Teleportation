/*
This file is based on the official Unity Low-Level Native Plugin example:
https://docs.unity3d.com/Manual/NativePluginInterface.html
*/

#include <DirectX11Handler.h>

DirectX11Handler::DirectX11Handler(IUnityInterfaces* unityInterfaces)
{
	IUnityGraphicsD3D11* d3d = unityInterfaces->Get<IUnityGraphicsD3D11>();
	device = d3d->GetDevice();
}

ID3D11Device* DirectX11Handler::getDevice()
{
	return(device);
}

void DirectX11Handler::updateSimpleSubresource(ID3D11Resource *model_texture, const void *model_texture_data, UINT textureRowPitch)
{
	ID3D11DeviceContext* ctx = NULL;
	device->GetImmediateContext(&ctx);
	// Update texture data, and free the memory buffer
	ctx->UpdateSubresource(model_texture, 0, NULL, model_texture_data, textureRowPitch, 0);
	ctx->Release();
}
