#pragma once

#include "MxRendering/Resources/Loaders/DDSTextureLoader.h"
#include "MxRendering/Resources/Loaders/WICTextureLoader.h"

struct Texture;

namespace MxRendering::Resources
{
	HRESULT CreateShaderResourceViewFromFile(const wchar_t* szFileName, Texture* textureData);

	HRESULT LoadModelFromFile(const char* szFileName);
}