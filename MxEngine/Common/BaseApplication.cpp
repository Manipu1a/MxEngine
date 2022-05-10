#include "BaseApplication.h"

bool MxEngine::BaseApplication::m_bQuit = false;

MxEngine::BaseApplication::BaseApplication()
{
 
}

int MxEngine::BaseApplication::Initialize()
{

    return 0;
}

void MxEngine::BaseApplication::Finalize()
{
    
}

void MxEngine::BaseApplication::Tick()
{
}

bool MxEngine::BaseApplication::IsQuit()
{
    return false;
}
