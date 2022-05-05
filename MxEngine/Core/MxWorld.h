#pragma once
/************************************************************************** 
    *  @Copyright (c) 2022, HARO, All rights reserved. 
 
    *  @file     : MEWorld.h 
    *  @version  : ver 1.0 
 
    *  @author   : Haro 
    *  @date     : 2022/05/04 10:52 
    *  @brief    : ���� 
**************************************************************************/
#include "../Common/d3dApp.h"

class MxRenderer;
class MxLevel;

class MxWorld
{
public:
    MxWorld(HINSTANCE& hInstance);
    ~MxWorld();

    static MxWorld* GetWorld();

    void Initialize();
    int Run();
    int TickWorld();

    inline float GetFPS() {return FPS;}
    
private:
    static MxWorld* mWorld;

    void CalculateFrameStats();

    float FPS;
    float MSPF;
    
    GameTimer mTimer;
    bool      mAppPaused = false;  // is the application paused?
    
    std::unique_ptr<MxLevel> Level;
    std::unique_ptr<MxRenderer> Renderer;
};
