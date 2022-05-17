#include "MxWorld.h"
#include "MxRenderer.h"
#include "MxLevel.h"

MxWorld* MxWorld::mWorld = nullptr;

MxWorld::MxWorld(HINSTANCE& hInstance)
{
	Level = std::make_unique<MxLevel>();
	mWorld = this;
	FPS = 0;
	MSPF = 0;
}

MxWorld::~MxWorld()
{
	Level.release();
}

MxWorld* MxWorld::GetWorld()
{
	return mWorld;
}

MxLevel* MxWorld::GetMainLevel()
{
	return Level.get();
}

void MxWorld::Initialize()
{
	Level->CreateLevelContent();
}

int MxWorld::TickWorld()
{
	return 0;
}
