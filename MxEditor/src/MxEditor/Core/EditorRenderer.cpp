#include "MxEditor/Core/EditorRenderer.h"

#include "MxUI/Core/UIManager.h"

MxEditor::Core::EditorRenderer::EditorRenderer(Context& p_context) : m_context(p_context)
{
    
}

void MxEditor::Core::EditorRenderer::InitMaterials()
{
}

void MxEditor::Core::EditorRenderer::RenderScene()
{
}

void MxEditor::Core::EditorRenderer::RenderUI()
{
    m_context.uiManager->Render(m_context.mCommandList.Get());
}

void MxEditor::Core::EditorRenderer::RenderCameras()
{
}

void MxEditor::Core::EditorRenderer::RenderLights()
{
}
