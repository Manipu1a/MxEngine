#pragma once

#include "Canvas.h"
#include <string>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace MxUI::Core
{
    class UIManager
    {
    public:
        UIManager(ID3D12Device* device, HWND* winHandle);

        ~UIManager();

        void SetCanvas(Canvas& p_canvas);

        void Render(ID3D12GraphicsCommandList* cmdList);
    private:
        Canvas* m_currentCanvas = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mGuiSrvDescriptorHeap = nullptr;
        std::string m_layoutSaveFilename = "imgui.ini";
    };
}
