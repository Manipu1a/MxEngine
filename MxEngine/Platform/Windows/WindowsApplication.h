#pragma once

#include "../../Common/BaseApplication.h"
#include "../../Common/Camera.h"

namespace MxEngine
{
    class WindowsApplication : public BaseApplication
    {
    public:
        WindowsApplication(): BaseApplication()
        {};

        virtual int Initialize() override;
        virtual void Finalize() override;
        virtual void Tick() override;

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        
    };
    
}
