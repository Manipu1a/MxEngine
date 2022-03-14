#include "Resource.h"
#include "../d3dApp.h"
#include "../CommonVertexFormat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

#define CGLTF_IMPLEMENTATION
#include "../../cgltf/cgltf.h"
#include "../FrameResource.h"

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

HRESULT Resource::LoadModelFromFile(const char* szFileName)
{
	HRESULT hr = S_OK;
	auto d3dDevice = D3DApp::GetApp()->GetDevice().Get();
	auto commderList = D3DApp::GetApp()->GetCommandList().Get();

	cgltf_options options = {};
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, szFileName, &data);
	if (result == cgltf_result_success)
	{
		/* TODO make awesome stuff */
		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "modelGeo";
		result = cgltf_load_buffers(&options, data, szFileName);

		SubmeshGeometry modelSubmesh;

		modelSubmesh.StartIndexLocation = 0;
		modelSubmesh.BaseVertexLocation = 0;

		int VertexCount = 0;
		DirectX::XMFLOAT3* pos = nullptr;
		DirectX::XMFLOAT3* normal = nullptr;
		DirectX::XMFLOAT2* texCoords = nullptr;
		for (int i = 0; i < data->meshes->primitives->attributes_count; ++i)
		{
			auto attributePointer = data->meshes->primitives->attributes + i;

			if (attributePointer->type == cgltf_attribute_type_position)
			{
				VertexCount = attributePointer->data->count;
				auto size = data->meshes->primitives->attributes->data->count * data->meshes->primitives->attributes->data->stride;
				auto p = (char*)attributePointer->data->buffer_view->buffer->data + attributePointer->data->buffer_view->offset;
				void* ver = std::malloc(size);
				std::memcpy(ver, p, size);

				pos = static_cast<DirectX::XMFLOAT3*>(ver);
			}
			else if (attributePointer->type == cgltf_attribute_type_normal)
			{
				auto size = data->meshes->primitives->attributes->data->count * data->meshes->primitives->attributes->data->stride;
				auto p = (char*)attributePointer->data->buffer_view->buffer->data + attributePointer->data->buffer_view->offset;
				void* ver = std::malloc(size);
				std::memcpy(ver, p, size);

				normal = static_cast<DirectX::XMFLOAT3*>(ver);
			}
			else if (attributePointer->type == cgltf_attribute_type_texcoord)
			{
				auto size = data->meshes->primitives->attributes->data->count * data->meshes->primitives->attributes->data->stride;
				auto p = (char*)attributePointer->data->buffer_view->buffer->data + attributePointer->data->buffer_view->offset;
				void* ver = std::malloc(size);
				std::memcpy(ver, p, size);

				texCoords = static_cast<DirectX::XMFLOAT2*>(ver);
			}
		}

		std::vector<Vertex> vertices(VertexCount);

		for (int index = 0; index < VertexCount; ++index)
		{
			vertices[index].Position = *(pos + index);
			vertices[index].Normal = *(normal + index);
			vertices[index].TexCoords = *(texCoords + index);
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice,
			commderList, vertices.data(), vbByteSize, geo->VertexBufferUploader);
		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;

		for (int i = 0; i < data->accessors_count; ++i)
		{
			if (data->accessors->buffer_view->type == cgltf_buffer_view_type_indices)
			{
				ThrowIfFailed(D3DCreateBlob(data->accessors->buffer_view->size, &geo->IndexBufferCPU));
				auto p = (char*)data->buffers->data + data->accessors->buffer_view->offset;
				CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), p, data->accessors->buffer_view->size);
				geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice,
					commderList, p, data->accessors->buffer_view->size, geo->IndexBufferUploader);
				geo->IndexFormat = DXGI_FORMAT_R16_UINT;
				geo->IndexBufferByteSize = data->accessors->buffer_view->size;
				modelSubmesh.IndexCount = data->accessors->buffer_view->size;
			}
			data->accessors++;
		}

		
		geo->DrawArgs["model"] = modelSubmesh;

		D3DApp::GetApp()->SaveMesh(geo);

		//build material
		for (int i = 0; i < data->materials_count; ++i)
		{
			auto materialPointer = data->materials + i;

			if (materialPointer->has_pbr_metallic_roughness)
			{
				if (materialPointer->pbr_metallic_roughness.base_color_texture.texture)
				{
					auto name = materialPointer->pbr_metallic_roughness.base_color_texture.texture->image->uri;
					std::stringstream filename;
					filename << "Assets/GltfModel/" + (string)name + "";

					D3DApp::GetApp()->SaveTexturePath("Default_albedo", AnsiToWString(filename.str()));
				}

				if (materialPointer->pbr_metallic_roughness.metallic_roughness_texture.texture)
				{
					auto name = materialPointer->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri;
					std::stringstream filename;
					filename << "Assets/GltfModel/" + (string)name + "";
					D3DApp::GetApp()->SaveTexturePath("metallicRoughnessTexture", AnsiToWString(filename.str()));
				}
			}

			if (materialPointer->normal_texture.texture)
			{
				auto name = materialPointer->normal_texture.texture->image->uri;
				std::stringstream filename;
				filename << "Assets/GltfModel/" + (string)name + "";
				D3DApp::GetApp()->SaveTexturePath("Default_normal", AnsiToWString(filename.str()));
			}

			if (materialPointer->emissive_texture.texture)
			{
				auto name = materialPointer->emissive_texture.texture->image->uri;
				std::stringstream filename;
				filename << "Assets/GltfModel/" + (string)name + "";
				D3DApp::GetApp()->SaveTexturePath("Default_emissive", AnsiToWString(filename.str()));
			}
		}

	}

	return hr;
}
