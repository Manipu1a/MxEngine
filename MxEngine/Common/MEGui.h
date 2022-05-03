/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEGui.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/03 23:25 
    *  @brief    : ui 
**************************************************************************/
#pragma once
#include "d3dApp.h"

class MEGui
{
public:
    MEGui() {}
    void Initialize(ID3D12Device* device, HWND& winHandle);
    void tick_pre();
    void onTick();
    void tick_post();

    void draw_frame();
    void clear();


    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mGuiSrvDescriptorHeap = nullptr;
};