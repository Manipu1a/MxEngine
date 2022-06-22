#pragma once
#include "EditorRenderer.h"
#include "MxEditor/Core/PanelManager.h"
#include "MxUI/Core/Canvas.h"

namespace MxEditor::Core
{
    class Editor
    {
    public:
        Editor(MxEditor::Core::Context& p_context);

        ~Editor();

        void SetupUI();

        void Update(float p_deltaTime);

        void RenderEditorUI(float p_deltaTime);
        
    private:
        MxUI::Core::Canvas mCanvas;
        MxEditor::Core::Context& mContext;
        MxEditor::Core::EditorRenderer mEditorRenderer;
       // MxEditor::Core::PanelManager mPanelsManager;
    };
}
