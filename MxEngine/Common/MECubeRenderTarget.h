//***************************************************************************************
// CubeRenderTarget.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "d3dUtil.h"
#include "MERenderTarget.h"

enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

class MECubeRenderTarget : public MERenderTarget
{
public:
	MECubeRenderTarget(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format,UINT mipmap = 1);
		
	MECubeRenderTarget(const MECubeRenderTarget& rhs)=delete;
	MECubeRenderTarget& operator=(const MECubeRenderTarget& rhs)=delete;
	virtual ~MECubeRenderTarget()=default;

	//ID3D12Resource* Resource();
	//CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int faceIndex);

	//D3D12_VIEWPORT Viewport()const;
	//D3D12_VIEWPORT Viewport(UINT mipmap);
	//D3D12_RECT ScissorRect()const;
	//D3D12_RECT ScissorRect(UINT mipmap);

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

	virtual void BuildDescriptors() override;
	virtual void BuildResource() override;
	//void OnResize(UINT newWidth, UINT newHeight);

	//inline UINT GetWidth() { return mWidth; }
	//inline UINT GetHeight() { return mHeight; }

private:

	//ID3D12Device* md3dDevice = nullptr;

	//D3D12_VIEWPORT mViewport;
	//D3D12_RECT mScissorRect;

	//UINT mWidth = 0;
	//UINT mHeight = 0;
	//UINT mMipMap = 1;

	//DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	//CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	//CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];

	//Microsoft::WRL::ComPtr<ID3D12Resource> mCubeMap = nullptr;
};

 