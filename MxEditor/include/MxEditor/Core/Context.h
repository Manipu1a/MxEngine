#pragma once
#include <string>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include "MxRendering/Core/Renderer.h"

namespace MxUI::Core{ class UIManager; }

namespace MxEditor::Core
{
    class Context
    {
    public:
        Context(const std::string& p_projectPath, const std::string& p_projectName);

        ~Context();

        void InitContext(HWND hWnd, HINSTANCE hInstance);
        void InitD3D();
        void InitCommandObject();
        
    public:
        const std::string projectPath;
        const std::string projectName;
        const std::string projectFilePath;
        const std::string engineAssetsPath;
        const std::string projectAssetsPath;
        const std::string projectScriptsPath;
        const std::string editorAssetsPath;

        HINSTANCE mhAppInst = nullptr; // application instance handle
        HWND      mhMainWnd = nullptr; // main window handle
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
        Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
        Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;

        
        std::unique_ptr<MxRendering::Core::MxRenderer> mRenderer;
        std::unique_ptr<MxUI::Core::UIManager> uiManager;
    };
}
