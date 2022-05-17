#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MELevel.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:18 
    *  @brief    : ≥°æ∞π‹¿Ì∆˜ 
**************************************************************************/
#include "../Core/MxRenderer.h"
#include "MxObject.h"

namespace MxEngine
{
	class MxLevel
	{
	public:
		MxLevel();

		void CreateLevelContent();
		UINT AddObject(MxObject& object);
	public:

		static UINT GlobalIDManager;
		std::unordered_map<UINT, std::shared_ptr<MxObject>> LevelObjectsMap;
	};
}
