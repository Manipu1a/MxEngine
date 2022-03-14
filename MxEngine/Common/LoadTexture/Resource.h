#pragma once

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

struct Texture;

namespace Resource
{
	HRESULT CreateShaderResourceViewFromFile(const wchar_t* szFileName, Texture* textureData);

	HRESULT LoadModelFromFile(const char* szFileName);
}