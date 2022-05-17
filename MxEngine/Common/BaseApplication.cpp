#include "BaseApplication.h"

bool MxEngine::BaseApplication::m_bQuit = false;


MxEngine::BaseApplication::BaseApplication(EngineConfiguration& config)
{
    m_Config = config;
}

int MxEngine::BaseApplication::Initialize()
{

    return 0;
}

void MxEngine::BaseApplication::Finalize()
{
    
}

int MxEngine::BaseApplication::Tick()
{
    return 1;
}

bool MxEngine::BaseApplication::IsQuit()
{
    return false;
}

void MxEngine::BaseApplication::OnResize()
{

}

int MxEngine::BaseApplication::Run()
{
    return MxEngine::BaseApplication::Tick();
}
