#include "MxWorld.h"
#include "MxRenderer.h"
#include "MxLevel.h"

MxWorld* MxWorld::mWorld = nullptr;

MxWorld::MxWorld(HINSTANCE& hInstance)
{
	Renderer = std::make_unique<MxRenderer>(hInstance);
	Level = std::make_unique<MxLevel>();
	mWorld = this;
	FPS = 0;
	MSPF = 0;
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

MxLevel* MxWorld::GetMainLevel()
{
	return Level.get();
}

void MxWorld::Initialize()
{
	Level->CreateLevelContent();
	Renderer->Initialize();
}

int MxWorld::Run()
{
	return TickWorld();
}

int MxWorld::TickWorld()
{
	MSG msg = {0};
 
	mTimer.Reset();

	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
		{	
			mTimer.Tick();

			if( !mAppPaused )
			{
				CalculateFrameStats();
				Renderer->TickRenderer(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

void MxWorld::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		FPS = (float)frameCnt; // fps = frameCnt / 1
		MSPF = 1000.0f / FPS;

		//wstring fpsStr = to_wstring(fps);
		//wstring mspfStr = to_wstring(mspf);
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}
