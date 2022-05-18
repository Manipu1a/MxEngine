/************************************************************************** 
    *  @Copyright (c) 2022, AX, All rights reserved. 
 
    *  @file     : RenderTarget.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/04/05 10:46 
    *  @brief    : RenderTarget 
**************************************************************************/
#pragma once
#include "MxRenderTargetBase.h"

namespace  MxEngine
{
	class MxRenderTarget : public MxRenderTargetBase
	{
	public:
		MxRenderTarget(ID3D12Device* device,
			UINT width, UINT height,
			DXGI_FORMAT format, UINT mipmap = 1, UINT RtNum = 1);

		MxRenderTarget(const MxRenderTarget& rhs) = delete;
		MxRenderTarget& operator=(const MxRenderTarget& rhs) = delete;
		virtual ~MxRenderTarget() = default;

		virtual ID3D12Resource* Resource(UINT Index = 0) override;

		virtual CD3DX12_GPU_DESCRIPTOR_HANDLE Srv(UINT Index = 0) override;

		virtual CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(UINT Index = 0) override;

		virtual void BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
			CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv) override;

		/*
		void BuildDescriptors(
			CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
			CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
			CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
			*/
		virtual void BuildDescriptors() override;
		virtual void BuildResource() override;

		virtual void RTTransition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) override;
		virtual void RTClearView(ID3D12GraphicsCommandList* cmdList, const FLOAT ColorRGBA[4], UINT NumRects, _In_reads_(NumRects) const D3D12_RECT* pRects) override;
	protected:

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mResourceMaps;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	};
}