#pragma once
#include "../Interface/IApplication.hpp"
#include "EngineConfiguration.h"

namespace MxEngine
{
    class BaseApplication : public IApplication
    {
    public:
        BaseApplication(EngineConfiguration& config);

        virtual int Initialize() override;
        virtual void Finalize() override;
        virtual int Tick() override;

        virtual bool IsQuit() override;
        virtual void OnResize() override;
        virtual int Run() override;
    protected:
        static bool m_bQuit;
        EngineConfiguration m_Config;
    };


}
