#include "MERenderTarget.h"

MERenderTarget::MERenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT mipmap /*= 1*/, UINT RtNum)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;
	mMipMap = mipmap;
	mRTNum = RtNum;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

ID3D12Resource* MERenderTarget::Resource()
{
	return mResourceMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE MERenderTarget::Srv()
{
	return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE MERenderTarget::Rtv()
{
	return mhCpuRtv;
}

D3D12_VIEWPORT MERenderTarget::Viewport() const
{
	return mViewport;
}

D3D12_VIEWPORT MERenderTarget::Viewport(UINT mipmap)
{
	UINT newWidth = mWidth * std::pow(0.5, mipmap);
	UINT newHeight = mHeight * std::pow(0.5, mipmap);

	return { 0.0f, 0.0f, (float)newWidth, (float)newHeight, 0.0f, 1.0f };
}

D3D12_RECT MERenderTarget::ScissorRect() const
{
	return mScissorRect;
}

D3D12_RECT MERenderTarget::ScissorRect(UINT mipmap)
{
	UINT newWidth = mWidth * std::pow(0.5, mipmap);
	UINT newHeight = mHeight * std::pow(0.5, mipmap);
	return { 0, 0, (int)newWidth, (int)newHeight };
}

void MERenderTarget::BuileRenderTarget(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = mRTNum;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	mhCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	BuildDescriptors();
}

void MERenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	// Save references to the descriptors. 
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;
	mhCpuRtv = hCpuRtv;
	//  Create the descriptors
	BuildDescriptors();
}

void MERenderTarget::BuildDescriptors()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE mRTVCpuHandle = mhCpuRtv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mSRVCpuHandle = mhCpuSrv;

	for (UINT i = 0; i < mRTNum; i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2D.PlaneSlice = 0;
		md3dDevice->CreateShaderResourceView(mResourceMaps[i].Get(), &srvDesc, mSRVCpuHandle);
		mSRVCpuHandle.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		md3dDevice->CreateRenderTargetView(mResourceMaps[i].Get(), &rtvDesc, mRTVCpuHandle);
		mRTVCpuHandle.Offset(1, md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	}
}

void MERenderTarget::BuildResource()
{
	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = mMipMap;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	Microsoft::WRL::ComPtr<ID3D12Resource> mNewResource = nullptr;
	for (UINT i = 0; i < mRTNum; i++)
	{
		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mNewResource)));
		mResourceMaps.push_back(mNewResource);
	}
}

void MERenderTarget::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}
