#pragma once

#include "Interface.hpp"
#include "IRuntimeModule.hpp"

namespace MxEngine
{
    class IApplication : public IRuntimeModule
    {
    public:
        virtual int Initialize() = 0;
        virtual void Finalize() = 0;

        virtual int Tick() = 0;

        virtual bool IsQuit() = 0;

        virtual void OnResize() = 0; 

        virtual int Run() = 0;
    };
}

