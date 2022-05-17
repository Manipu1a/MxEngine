#pragma once
#include "Interface.hpp"

namespace MxEngine
{
    class IRuntimeModule
    {
    public:
        virtual ~IRuntimeModule(){};

        virtual int Initialize() = 0;
        virtual void Finalize() = 0;

        virtual int Tick() = 0;
    };
}

