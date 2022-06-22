#pragma once
#include "Context.h"

namespace MxEditor::Core
{
    class EditorRenderer
    {
    public:
        EditorRenderer(Context& p_context);

        void InitMaterials();

        void RenderScene();

        void RenderUI();

        void RenderCameras();

        void RenderLights();

    private:
        Context& m_context;
    };
}
