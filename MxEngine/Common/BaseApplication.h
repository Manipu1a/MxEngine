#pragma once
#include "../Interface/IApplication.hpp"

namespace MxEngine
{
    class BaseApplication : public IApplication
    {
        BaseApplication();
    public:
        virtual int Initialize() override;
        virtual void Finalize() override;
        virtual void Tick() override;

        virtual bool IsQuit() override;

    protected:
        static bool m_bQuit;
    };
}
