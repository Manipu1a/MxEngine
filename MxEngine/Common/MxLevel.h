#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MELevel.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:18 
    *  @brief    : ≥°æ∞π‹¿Ì∆˜ 
**************************************************************************/
#include "d3dApp.h"
#include "MxObject.h"

class MxLevel
{
public:
    MxLevel();

    UINT AddObject(MxObject& object);
public:

    static UINT GlobalIDManager;
    std::unordered_map<UINT, std::shared_ptr<MxObject>> LevelObjectsMap;
};