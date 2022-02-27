#include "Resource.h"
#include "../d3dApp.h"
#include "../CommonVertexFormat.h"


HRESULT Resource::CreateShaderResourceViewFromFile(const wchar_t* szFileName, Texture* textureData)
{
	if (!szFileName)
		return E_INVALIDARG;

	auto d3dDevice = D3DApp::GetApp()->GetDevice().Get();
	auto commderList = D3DApp::GetApp()->GetCommandList().Get();
	HRESULT hr;
	WCHAR ext[_MAX_EXT];
	_wsplitpath_s(szFileName, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

	if (_wcsicmp(ext, L".dds") == 0)
	{
		hr = DirectX::CreateDDSTextureFromFile12(d3dDevice, commderList, szFileName, textureData->Resource, textureData->UploadHeap);
	}
	else
	{
		hr = DirectX::CreateWICTextureFromFile12(d3dDevice, commderList,szFileName, textureData->Resource, textureData->UploadHeap);
	}

	return hr;
}