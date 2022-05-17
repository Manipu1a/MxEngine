// MxEngine.cpp : Defines the entry point for the application.
//

#include "main.h"
#include "Platform/Windows/MxWindowsApplication.h"

using namespace MxEngine;

namespace MxEngine {
    extern IApplication* g_pApp;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
		g_pApp->Initialize();
        return g_pApp->Run();
    }
    catch (DxException& e)
    {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
    }

}
