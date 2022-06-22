#pragma once
#include <unordered_map>

#include "MxUI/Panels/PanelMenuBar.h"
#include "MxUI/Panels/PanelWindow.h"
#include "MxUI/Widgets/Menu/MenuItem.h"

namespace MxEditor::Panels
{
    class MenuBar : public MxUI::Panels::PanelMenuBar
    {
        using PanelMap = std::unordered_map<std::string, std::pair<std::reference_wrapper<MxUI::Panels::PanelWindow>, std::reference_wrapper<MxUI::Widgets::Menu::MenuItem>>>;

    public:
        /**
        * Constructor
        */
        MenuBar();
		
        /**
        * Check inputs for menubar shortcuts
        * @param p_deltaTime
        */
        void HandleShortcuts(float p_deltaTime);

        /**
        * Register a panel to the menu bar window menu
        */
        void RegisterPanel(const std::string& p_name, MxUI::Panels::PanelWindow& p_panel);

    private:
        void CreateFileMenu();
        void CreateBuildMenu();
        void CreateWindowMenu();
        void CreateActorsMenu();
        void CreateResourcesMenu();
        void CreateSettingsMenu();
        void CreateLayoutMenu();
        void CreateHelpMenu();

        void UpdateToggleableItems();
        void OpenEveryWindows(bool p_state);

    private:
        PanelMap m_panels;

        MxUI::Widgets::Menu::MenuList* m_windowMenu = nullptr;
    };
}
