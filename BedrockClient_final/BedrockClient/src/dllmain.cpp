// BedrockClient - DLL Main Entry Point
// Inject vào Minecraft Bedrock Edition (Windows)

#include <Windows.h>
#include <thread>
#include "hooks/DXHook.h"
#include "gui/ClickGUI.h"
#include "utils/CommandHandler.h"
#include "ModuleManager.h"

HMODULE g_hModule = nullptr;

void MainThread(HMODULE hModule) {
    // Chờ game load xong
    Sleep(1000);

    // Khởi tạo các hệ thống
    ModuleManager::Init();
    CommandHandler::Init();
    DXHook::Init();

    // Giữ DLL sống
    while (!GetAsyncKeyState(VK_END)) {
        Sleep(100);
    }

    // Cleanup khi unload
    DXHook::Shutdown();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
