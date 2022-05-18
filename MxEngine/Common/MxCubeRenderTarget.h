//***************************************************************************************
// CubeRenderTarget.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once
#include "MxRenderTargetBase.h"

namespace MxEngine
{
	enum class CubeMapFace : int
	{
		PositiveX = 0,
		NegativeX = 1,
		PositiveY = 2,
		NegativeY = 3,
		PositiveZ = 4,
		NegativeZ = 5
	};

	class MxCubeRenderTarget : public MxRenderTargetBase
	{
	public:
		MxCubeRenderTarget(ID3D12Device* device,
			UINT width, UINT height,
			DXGI_FORMAT format,UINT mipmap = 1, UINT RtNum = 6);
		
		MxCubeRenderTarget(const MxCubeRenderTarget& rhs)=delete;
		MxCubeRenderTarget& operator=(const MxCubeRenderTarget& rhs)=delete;
		virtual ~MxCubeRenderTarget()=default;

		virtual ID3D12Resource* Resource(UINT Index = 0) override;

		virtual CD3DX12_GPU_DESCRIPTOR_HANDLE Srv(UINT Index = 0) override;

		virtual CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(UINT Index = 0) override;

		virtual void BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
			CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv) override;


		void BuildDescriptors(
			CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
			CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
			CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

		virtual void BuildDescriptors() override;
		virtual void BuildResource() override;

		virtual void RTTransition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) override;
		virtual void RTClearView(ID3D12GraphicsCommandList* cmdList, const FLOAT ColorRGBA[4], UINT NumRects, _In_reads_(NumRects) const D3D12_RECT* pRects) override;
	private:

		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];

		Microsoft::WRL::ComPtr<ID3D12Resource> mCubeMap = nullptr;
	};
}
 