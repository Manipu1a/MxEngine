#include "MxLevel.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/FrameResource.h"
#include "../Common/d3dApp.h"
#include "MxRenderComponent.h"

UINT MxLevel::GlobalIDManager = 0;

MxLevel::MxLevel()
{

}

void MxLevel::CreateLevelContent()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.0f, 100, 100);

	UINT sphereVertexOffset = 0;
	UINT sphereIndexOffset = 0;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	auto totalVertexCount =
		sphere.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].Tangent = sphere.Vertices[i].TangentU;
		vertices[k].TexCoords = sphere.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetApp()->GetDevice().Get(),
		D3DApp::GetApp()->GetCommandList().Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(D3DApp::GetApp()->GetDevice().Get(),
		D3DApp::GetApp()->GetCommandList().Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	geo->DrawArgs["sphere"] = sphereSubmesh;


	auto default = std::make_shared<Material>();
	default->Name = "defaultM";
	default->MatCBIndex = 0;
	default->AlbedoSrvHeapIndex = -1;
	default->MetallicSrvHeapIndex = -1;
	default->NormalSrvHeapIndex = -1;
	default->RoughnessSrvHeapIndex = -1;
	default->BaseColor = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	default->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	default->Roughness = 0.3f;
	default->Metallic = 0.0f;
	default->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	default->AmbientColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	default->SpecularStrength = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	default->Ao = 1.0f;

	MxObject NewObject("object1");
	NewObject.RenderComponent->mGeometrie = std::move(geo);
	NewObject.RenderComponent->mMaterial = std::move(default);
	AddObject(NewObject);
}

UINT MxLevel::AddObject(MxObject& object)
{
	UINT Id = GlobalIDManager++;
	object.ObjectID = Id;
	LevelObjectsMap[Id] = std::make_shared<MxObject>(object);
	
	return Id;
}

