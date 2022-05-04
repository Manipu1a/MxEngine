#include "MxWorld.h"
#include "MxRenderer.h"
#include "../Common/MxLevel.h"

MxWorld* MxWorld::mWorld = nullptr;

MxWorld::MxWorld(HINSTANCE& hInstance)
{
	Renderer = std::make_unique<MxRenderer>(hInstance);
	Level = std::make_unique<MxLevel>();
	mWorld = this;
}

MxWorld::~MxWorld()
{
	Level.release();
	Renderer.release();
}

MxWorld* MxWorld::GetWorld()
{
	return mWorld;
}

void MxWorld::Initialize()
{
	Renderer->Initialize();
}

int MxWorld::Run()
{
	return Renderer->Run();
}
