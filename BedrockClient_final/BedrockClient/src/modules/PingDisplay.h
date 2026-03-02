#pragma once
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include "../Module.h"
#include <imgui.h>
#include <chrono>
#include <thread>
#include <atomic>

class PingDisplay : public Module {
private:
    float posX = 20.0f;
    float posY = 55.0f;

    std::atomic<int> currentPing{ -1 };
    std::thread pingThread;
    std::atomic<bool> running{ false };

    void pingLoop() {
        while (running) {
            // Dùng GetTickCount để estimate ping
            // Thực tế: hook network packet timing từ game
            currentPing = (rand() % 30 + 5); // placeholder 5-35ms
            Sleep(2000);
        }
    }

public:
    PingDisplay() : Module("Ping", "Hiện ping tới server", Category::HUD) {}

    void onEnable() override {
        running = true;
        pingThread = std::thread(&PingDisplay::pingLoop, this);
    }

    void onDisable() override {
        running = false;
        if (pingThread.joinable()) pingThread.join();
    }

    void onRender() override {
        int ping = currentPing.load();

        ImU32 pingColor;
        if (ping < 0)         pingColor = IM_COL32(150, 150, 150, 255);  // Gray (unknown)
        else if (ping < 60)   pingColor = IM_COL32(0, 255, 128, 255);    // Xanh lá (tốt)
        else if (ping < 120)  pingColor = IM_COL32(255, 200, 0, 255);    // Vàng (trung bình)
        else if (ping < 200)  pingColor = IM_COL32(255, 128, 0, 255);    // Cam (cao)
        else                  pingColor = IM_COL32(255, 60, 60, 255);     // Đỏ (tệ)

        char buf[32];
        if (ping < 0)
            snprintf(buf, sizeof(buf), "Ping: --");
        else
            snprintf(buf, sizeof(buf), "Ping: %dms", ping);

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImVec2 textSize = ImGui::CalcTextSize(buf);

        float padX = 8.0f, padY = 5.0f;

        // Background
        draw->AddRectFilled(
            ImVec2(posX - padX, posY - padY),
            ImVec2(posX + textSize.x + padX, posY + textSize.y + padY),
            IM_COL32(20, 20, 20, 180), 6.0f
        );
        draw->AddRect(
            ImVec2(posX - padX, posY - padY),
            ImVec2(posX + textSize.x + padX, posY + textSize.y + padY),
            IM_COL32(60, 60, 60, 200), 6.0f, 0, 1.0f
        );

        // "Ping: " trắng, số màu theo ping
        draw->AddText(ImVec2(posX, posY), IM_COL32(200, 200, 200, 255), "Ping: ");
        ImVec2 labelSize = ImGui::CalcTextSize("Ping: ");
        char valBuf[16];
        if (ping < 0) snprintf(valBuf, sizeof(valBuf), "--");
        else snprintf(valBuf, sizeof(valBuf), "%dms", ping);
        draw->AddText(ImVec2(posX + labelSize.x, posY), pingColor, valBuf);
    }
};
