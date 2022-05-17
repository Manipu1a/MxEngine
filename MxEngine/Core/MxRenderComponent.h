#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MxRenderComponent.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 17:00 
    *  @brief    : äÖÈ¾×é¼þ 
**************************************************************************/
#include "MxRenderer.h"
struct MeshGeometry;
struct Material;


class MxRenderComponent
{
public:
    MxRenderComponent();
    ~MxRenderComponent();
    void Release() {}

    std::shared_ptr<MeshGeometry> mGeometrie;
    std::shared_ptr<Material> mMaterial;
};