#pragma once

#include "Raytracer.h"

#include <windows.h>
#include <memory>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>

// Link the Direct3D 11 import libraries required by Renderer.cpp:
//   d3d11.lib        -> D3D11CreateDeviceAndSwapChain, device/context APIs
//   d3dcompiler.lib  -> D3DCompileFromFile (runtime HLSL compilation)
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <vector>
#include <chrono>
#include <algorithm>

class Renderer
{
public:
	int width, height;
	Raytracer raytracer;
	std::vector<math::vec4> pixels;
	bool needsRender = true; // re-render the scene into the texture when set

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	IDXGISwapChain* swapChain;
	D3D11_VIEWPORT viewport;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;

	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
	ID3D11Texture2D* canvasTexture = nullptr;
	ID3D11ShaderResourceView* canvasTextureView = nullptr;
	ID3D11RenderTargetView* canvasRenderTargetView = nullptr;
	ID3D11SamplerState* colorSampler;
	UINT indexCount;

public:
	Renderer(HWND window, int width, int height)
		: raytracer(width, height)
	{
		Initialize(window, width, height);
	}
	void Update();
	void InitShaders();
	void Initialize(HWND window, int width, int height);
	void Render();
	void Clean();
};