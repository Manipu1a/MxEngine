#include "MxEditor/Panels/MenuBar.h"

using namespace MxUI::Widgets::Menu;

MxEditor::Panels::MenuBar::MenuBar()
{
    CreateFileMenu();
    CreateBuildMenu();
    CreateWindowMenu();
    CreateActorsMenu();
    CreateResourcesMenu();
    CreateSettingsMenu();
    CreateLayoutMenu();
    CreateHelpMenu();
    
}

void MxEditor::Panels::MenuBar::HandleShortcuts(float p_deltaTime)
{
}

void MxEditor::Panels::MenuBar::RegisterPanel(const std::string& p_name, MxUI::Panels::PanelWindow& p_panel)
{

}

void MxEditor::Panels::MenuBar::CreateFileMenu()
{
    auto& fileMenu = CreateWidget<MenuList>("File");
    fileMenu.CreateWidget<MenuItem>("New Scene", "CTRL + N");
    fileMenu.CreateWidget<MenuItem>("Save Scene", "CTRL + S");
    fileMenu.CreateWidget<MenuItem>("Save Scene As...", "CTRL + SHIFT + S");
    fileMenu.CreateWidget<MenuItem>("Exit", "ALT + F4");
}

void MxEditor::Panels::MenuBar::CreateBuildMenu()
{
    auto& buildMenu = CreateWidget<MenuList>("Build");
    buildMenu.CreateWidget<MenuItem>("Build game");
    buildMenu.CreateWidget<MenuItem>("Build game and run");
    //buildMenu.CreateWidget<Visual::Separator>();
    buildMenu.CreateWidget<MenuItem>("Temporary build");
}

void MxEditor::Panels::MenuBar::CreateWindowMenu()
{
    m_windowMenu = &CreateWidget<MenuList>("Window");
    m_windowMenu->CreateWidget<MenuItem>("Close all");
    m_windowMenu->CreateWidget<MenuItem>("Open all");
    //m_windowMenu->CreateWidget<Visual::Separator>();

    /* When the menu is opened, we update which window is marked as "Opened" or "Closed" */
    //m_windowMenu->ClickedEvent += std::bind(&MenuBar::UpdateToggleableItems, this);
}

void MxEditor::Panels::MenuBar::CreateActorsMenu()
{
    //auto& actorsMenu = CreateWidget<MenuList>("Actors");
    //Utils::ActorCreationMenu::GenerateActorCreationMenu(actorsMenu);
}

void MxEditor::Panels::MenuBar::CreateResourcesMenu()
{
}

void MxEditor::Panels::MenuBar::CreateSettingsMenu()
{
}

void MxEditor::Panels::MenuBar::CreateLayoutMenu()
{
}

void MxEditor::Panels::MenuBar::CreateHelpMenu()
{
}

void MxEditor::Panels::MenuBar::UpdateToggleableItems()
{
}

void MxEditor::Panels::MenuBar::OpenEveryWindows(bool p_state)
{
}
