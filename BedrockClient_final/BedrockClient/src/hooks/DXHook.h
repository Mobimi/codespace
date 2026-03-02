#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#pragma comment(lib, "d3d11.lib")

// Forward declare ImGui WndProc handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef LRESULT(CALLBACK* WndProc_t)(HWND, UINT, WPARAM, LPARAM);

class DXHook {
public:
    static inline Present_t oPresent = nullptr;
    static inline WndProc_t oWndProc = nullptr;

    static inline IDXGISwapChain*  swapChain  = nullptr;
    static inline ID3D11Device*    device      = nullptr;
    static inline ID3D11DeviceContext* context = nullptr;
    static inline ID3D11RenderTargetView* mainRTV = nullptr;

    static inline HWND gameWindow = nullptr;
    static inline bool imguiInit = false;
    static inline void* hookPresent = nullptr; // MinHook handle

    static void Init() {
        // Tìm SwapChain bằng cách tạo dummy device
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = FindWindowA(nullptr, "Minecraft");
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        if (!sd.OutputWindow) {
            // Thử tìm window Bedrock
            sd.OutputWindow = FindWindowA("Windows.UI.Core.CoreWindow", nullptr);
        }

        ID3D11Device* dummyDevice = nullptr;
        IDXGISwapChain* dummySwapChain = nullptr;
        ID3D11DeviceContext* dummyCtx = nullptr;

        D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            &fl, 1, D3D11_SDK_VERSION, &sd,
            &dummySwapChain, &dummyDevice, nullptr, &dummyCtx)))
        {
            // Lấy VMT của SwapChain (Present = index 8)
            void** vmt = *(void***)dummySwapChain;
            void* presentPtr = vmt[8];

            // Hook Present với MinHook
            // MH_Initialize();
            // MH_CreateHook(presentPtr, &HookedPresent, (void**)&oPresent);
            // MH_EnableHook(presentPtr);

            // Cleanup dummy objects
            dummySwapChain->Release();
            dummyDevice->Release();
            dummyCtx->Release();

            // Lưu present pointer để hook thủ công nếu không dùng MinHook
            oPresent = (Present_t)presentPtr;
        }
    }

    static HRESULT __stdcall HookedPresent(IDXGISwapChain* pChain, UINT SyncInterval, UINT Flags) {
        if (!imguiInit) {
            // Lấy device từ swapchain
            pChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);
            device->GetImmediateContext(&context);

            // Tạo RenderTargetView
            ID3D11Texture2D* backBuffer = nullptr;
            pChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            device->CreateRenderTargetView(backBuffer, nullptr, &mainRTV);
            backBuffer->Release();

            // Lấy game window
            DXGI_SWAP_CHAIN_DESC desc;
            pChain->GetDesc(&desc);
            gameWindow = desc.OutputWindow;

            // Hook WndProc
            oWndProc = (WndProc_t)SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

            // Init ImGui
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            // Style
            ApplyStyle();

            ImGui_ImplWin32_Init(gameWindow);
            ImGui_ImplDX11_Init(device, context);

            imguiInit = true;
            swapChain = pChain;
        }

        // Render ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Tick & Render tất cả modules
        ModuleManager::onTick();
        ModuleManager::onRender();

        ImGui::Render();
        context->OMSetRenderTargets(1, &mainRTV, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return oPresent(pChain, SyncInterval, Flags);
    }

    static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Chặn input vào game khi GUI mở
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

        if (ModuleManager::clickGUI && ModuleManager::clickGUI->isVisible()) {
            // Chặn mouse/keyboard khi clickgui mở
            switch (msg) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP:
                case WM_RBUTTONDOWN: case WM_RBUTTONUP:
                case WM_MOUSEMOVE:
                case WM_MOUSEWHEEL:
                case WM_KEYDOWN: case WM_KEYUP:
                case WM_CHAR:
                    return 0;
            }
        }

        return CallWindowProcA(oWndProc, hWnd, msg, wParam, lParam);
    }

    static void ApplyStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        style.WindowRounding    = 8.0f;
        style.FrameRounding     = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 4.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;

        colors[ImGuiCol_WindowBg]        = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
        colors[ImGuiCol_TitleBg]         = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_Border]          = ImVec4(0.25f, 0.25f, 0.25f, 0.60f);
        colors[ImGuiCol_FrameBg]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_Button]          = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ButtonHovered]   = ImVec4(0.25f, 0.45f, 0.85f, 1.00f);
        colors[ImGuiCol_ButtonActive]    = ImVec4(0.15f, 0.35f, 0.75f, 1.00f);
        colors[ImGuiCol_Header]          = ImVec4(0.20f, 0.40f, 0.80f, 0.50f);
        colors[ImGuiCol_HeaderHovered]   = ImVec4(0.20f, 0.45f, 0.85f, 0.80f);
        colors[ImGuiCol_Separator]       = ImVec4(0.30f, 0.30f, 0.30f, 0.60f);
        colors[ImGuiCol_Text]            = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]     = ImVec4(0.05f, 0.05f, 0.05f, 0.80f);
        colors[ImGuiCol_ScrollbarGrab]   = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    }

    static void Shutdown() {
        if (imguiInit) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            if (oWndProc) SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc);
            if (mainRTV) mainRTV->Release();
        }
    }
};
