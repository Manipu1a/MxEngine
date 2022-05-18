#pragma once

#include "../../Common/BaseApplication.h"
#include "../../Common/GameTimer.h"
#include "../../Core/MxRenderer.h"
#include "../../Core/MxWorld.h"


namespace MxEngine
{
    class MxWindowsApplication : public BaseApplication
    {
    public:
        MxWindowsApplication(EngineConfiguration& config);
    	~MxWindowsApplication();
    	
        virtual int Initialize() override;
        virtual void Finalize() override;
        virtual int Tick() override;

        virtual int Run() override;
        virtual void OnResize() override;
        LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        
    protected:
		void CalculateFrameStats();

    public:
        HINSTANCE mhAppInst = nullptr; // application instance handle
        HWND      mhMainWnd = nullptr; // main window handle
		bool      mAppPaused = false;  // is the application paused?
		bool      mMinimized = false;  // is the application minimized?
		bool      mMaximized = false;  // is the application maximized?
		bool      mResizing = false;   // are the resize bars being dragged?
		bool      mFullscreenState = false;// fullscreen enabled

		// Set true to use 4X MSAA (?.1.8).  The default is false.
		bool      m4xMsaaState = false;    // 4X MSAA enabled
		UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

		// Used to keep track of the “delta-time?and game time (?.4).
		GameTimer mTimer;

		float FPS;
		float MSPF;
    	
        std::unique_ptr<MxRenderer> mRenderer;
    	std::unique_ptr<MxWorld> mWorld;
    };
    
}
