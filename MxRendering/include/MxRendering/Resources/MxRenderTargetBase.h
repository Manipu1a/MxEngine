#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MERenderTargetBase.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/04/30 11:27 
    *  @brief    : ��ȾĿ����� 
**************************************************************************/

#include "MxRendering/Common/d3dUtil.h"

namespace MxRendering::Resources
{
class MxRenderTargetBase
{
public:
	MxRenderTargetBase(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format, UINT mipmap = 1, UINT RtNum = 1)
	{
	};

	MxRenderTargetBase(const MxRenderTargetBase& rhs) = delete;
	MxRenderTargetBase& operator=(const MxRenderTargetBase& rhs) = delete;
	virtual ~MxRenderTargetBase() = default;

	virtual ID3D12Resource* Resource(UINT resourceIndex = 0) = 0;

	virtual CD3DX12_GPU_DESCRIPTOR_HANDLE Srv(UINT Index = 0) = 0;

	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(UINT Index = 0) = 0;

	D3D12_VIEWPORT Viewport()const 
	{
		return mViewport;
	};
	D3D12_VIEWPORT Viewport(UINT mipmap)
	{
		UINT newWidth = mWidth * std::pow(0.5, mipmap);
		UINT newHeight = mHeight * std::pow(0.5, mipmap);

		return { 0.0f, 0.0f, (float)newWidth, (float)newHeight, 0.0f, 1.0f };
	};

	D3D12_RECT ScissorRect()const
	{
		return mScissorRect;
	};
	D3D12_RECT ScissorRect(UINT mipmap)
	{
		UINT newWidth = mWidth * std::pow(0.5, mipmap);
		UINT newHeight = mHeight * std::pow(0.5, mipmap);
		return { 0, 0, (int)newWidth, (int)newHeight };
	};

	virtual void BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv) = 0;

	virtual void BuildDescriptors() = 0;
	virtual void BuildResource() = 0;

	void OnResize(UINT newWidth, UINT newHeight);

	inline UINT GetWidth() { return mWidth; }
	inline UINT GetHeight() { return mHeight; }

	//״̬�л�
	virtual void RTTransition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) = 0;
	virtual void RTClearView(ID3D12GraphicsCommandList* cmdList, const FLOAT ColorRGBA[4], UINT NumRects, _In_reads_(NumRects) const D3D12_RECT* pRects) = 0;
protected:
	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mRTNum = 0;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mMipMap = 1;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
};
}
