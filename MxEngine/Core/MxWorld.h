#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEWorld.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:52 
    *  @brief    : ���� 
**************************************************************************/
#include "MxRenderer.h"

namespace MxEngine
{
	class MxLevel;

	class MxWorld
	{
	public:
		MxWorld(HINSTANCE& hInstance);
		~MxWorld();

		static MxWorld* GetWorld();
		MxLevel* GetMainLevel();

		void Initialize();

		int TickWorld();

		inline float GetFPS() { return FPS; }

	private:
		static MxWorld* mWorld;

		float FPS;
		float MSPF;

		GameTimer mTimer;
		bool      mAppPaused = false;  // is the application paused?

		std::unique_ptr<MxLevel> Level;
	};

}
