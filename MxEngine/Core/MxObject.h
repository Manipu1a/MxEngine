#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEObject.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:01 
    *  @brief    : Object 
**************************************************************************/
#include "MxRenderer.h"
class MxRenderComponent;

class MxObject
{
public:
    MxObject();
	MxObject(const std::string& name);

    DirectX::XMFLOAT3 Location;
    DirectX::XMFLOAT3 Rotation;
    DirectX::XMFLOAT3 Scale;

    std::shared_ptr<MxRenderComponent> RenderComponent;
    std::string ObjectName;
    UINT ObjectID;
};