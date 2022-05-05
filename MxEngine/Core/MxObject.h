#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEObject.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:01 
    *  @brief    : Object 
**************************************************************************/

#include "../Common/d3dApp.h"

class MxRenderComponent;

class MxObject
{
public:
    MxObject();
	MxObject(const std::string& name);


    XMFLOAT3 Location;
    XMFLOAT3 Rotation;
    XMFLOAT3 Scale;

    std::shared_ptr<MxRenderComponent> RenderComponent;
    std::string ObjectName;
    UINT ObjectID;
};