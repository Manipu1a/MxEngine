#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEObject.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:01 
    *  @brief    : Object 
**************************************************************************/
#include "d3dApp.h"

class MxObject
{
public:
    MxObject(const std::string& name, UINT id);

public:

    std::string ObjectName;
    UINT ObjectID;
};