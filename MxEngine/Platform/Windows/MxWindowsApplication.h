#pragma once

#include "../../Common/BaseApplication.h"
#include "../../Common/GameTimer.h"
#include "../../Core/MxWorld.h"
#include "MxEditor/Core/Editor.h"

#pragma comment(lib, "MxUI.lib")
#pragma comment(lib, "MxEditor.lib")



namespace MxEngine
{
	class MxWindowsApplication : public BaseApplication
	{
	public:
		MxWindowsApplication(EngineConfiguration& config,const std::string& p_projectPath, const std::string& p_projectName );
		~MxWindowsApplication() override;
    	
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
    	

		std::unique_ptr<MxWorld> mWorld;
		MxEditor::Core::Context m_context;
		MxEditor::Core::Editor m_editor;
	};
}