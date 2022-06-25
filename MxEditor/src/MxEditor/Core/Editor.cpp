#include "MxEditor/Core/Editor.h"
//#include "MxUI/Core/UIManager.h"


MxEditor::Core::Editor::Editor(MxEditor::Core::Context& p_context) :mContext(p_context),mEditorRenderer(p_context), mPanelsManager(mCanvas)
{
    //SetupUI();
}

MxEditor::Core::Editor::~Editor()
{
    
}

void MxEditor::Core::Editor::SetupUI()
{
    MxEditor::Core::PanelManager pp(mCanvas);
    //mPanelsManager.CreatePanel<MxEditor::Panels::MenuBar>("Menu Bar");
    //mCanvas.MakeDockspace(true);
    //if(mContext.uiManager)
    {
       // mContext.uiManager->SetCanvas(mCanvas);
    }
}

void MxEditor::Core::Editor::Update(float p_deltaTime)
{
    
}

void MxEditor::Core::Editor::RenderEditorUI(float p_deltaTime)
{
    mEditorRenderer.RenderUI();
}
