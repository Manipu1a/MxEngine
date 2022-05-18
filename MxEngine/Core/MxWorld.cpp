#include "MxWorld.h"
#include "MxLevel.h"

namespace MxEngine
{
	MxWorld* MxWorld::mWorld = nullptr;

	MxWorld* MxWorld::GetWorld()
	{
		return mWorld;
	}
}


MxEngine::MxWorld::MxWorld()
{
	Level = std::make_unique<MxLevel>();
	mWorld = this;
	FPS = 0;
	MSPF = 0;
}

MxEngine::MxWorld::~MxWorld()
{
	Level.release();
}

MxEngine::MxLevel* MxEngine::MxWorld::GetMainLevel()
{
	return Level.get();
}

void MxEngine::MxWorld::Initialize()
{
	Level->CreateLevelContent();
}

int MxEngine::MxWorld::TickWorld()
{
	return 0;
}
