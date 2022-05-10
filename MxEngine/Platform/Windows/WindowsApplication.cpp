#include "WindowsApplication.h"

int MxEngine::WindowsApplication::Initialize()
{
    return BaseApplication::Initialize();
}

void MxEngine::WindowsApplication::Finalize()
{
    BaseApplication::Finalize();
}

void MxEngine::WindowsApplication::Tick()
{
    BaseApplication::Tick();
}

LRESULT MxEngine::WindowsApplication::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    
}
