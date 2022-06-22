#include "MxUI/Plugins/ContextualMenu.h"

void MxUI::Plugins::ContextualMenu::Execute()
{
    if (ImGui::BeginPopupContextItem())
    {
        DrawWidgets();
        ImGui::EndPopup();
    }
}

void MxUI::Plugins::ContextualMenu::Close()
{
    ImGui::CloseCurrentPopup();
}
