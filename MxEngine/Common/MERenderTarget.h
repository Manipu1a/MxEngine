/************************************************************************** 
    *  @Copyright (c) 2022, AX, All rights reserved. 
 
    *  @file     : RenderTarget.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/04/05 10:46 
    *  @brief    : RenderTarget 
**************************************************************************/
#pragma once
#include "d3dUtil.h"


class MERenderTarget
{
public:
	MERenderTarget(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT mipmap = 1);

	MERenderTarget(const MERenderTarget& rhs) = delete;
	MERenderTarget& operator=(const MERenderTarget& rhs) = delete;
	virtual ~MERenderTarget() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv();

	D3D12_VIEWPORT Viewport()const;
	D3D12_VIEWPORT Viewport(UINT mipmap);
	D3D12_RECT ScissorRect()const;
	D3D12_RECT ScissorRect(UINT mipmap);

	void BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	virtual void BuildDescriptors();
	virtual void BuildResource();


	void OnResize(UINT newWidth, UINT newHeight);

	inline UINT GetWidth() { return mWidth; }
	inline UINT GetHeight() { return mHeight; }


protected:
	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mMipMap = 1;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResourceMap = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
};