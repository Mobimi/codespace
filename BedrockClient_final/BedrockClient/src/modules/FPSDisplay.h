#pragma once
#include "../Module.h"
#include <imgui.h>
#include <chrono>
#include <deque>

class FPSDisplay : public Module {
private:
    float posX = 20.0f;
    float posY = 20.0f;

    std::deque<float> frameTimes;
    std::chrono::steady_clock::time_point lastFrame;
    float smoothFPS = 0.0f;

public:
    FPSDisplay() : Module("FPS", "Hiện FPS hiện tại", Category::HUD) {}

    void onRender() override {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;

        if (dt > 0.0f) {
            frameTimes.push_back(dt);
            if (frameTimes.size() > 60) frameTimes.pop_front();

            float avg = 0.0f;
            for (float f : frameTimes) avg += f;
            avg /= frameTimes.size();
            smoothFPS = 1.0f / avg;
        }

        int fps = (int)smoothFPS;

        // Màu theo FPS
        ImU32 fpsColor;
        if (fps >= 60)       fpsColor = IM_COL32(0, 255, 128, 255);   // Xanh lá
        else if (fps >= 30)  fpsColor = IM_COL32(255, 200, 0, 255);   // Vàng
        else                 fpsColor = IM_COL32(255, 60, 60, 255);    // Đỏ

        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        // Background box
        char buf[32];
        snprintf(buf, sizeof(buf), "FPS: %d", fps);
        ImVec2 textSize = ImGui::CalcTextSize(buf);

        float padX = 8.0f, padY = 5.0f;
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

        // "FPS: " trắng, số màu theo FPS
        draw->AddText(ImVec2(posX, posY), IM_COL32(200, 200, 200, 255), "FPS: ");
        ImVec2 labelSize = ImGui::CalcTextSize("FPS: ");
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", fps);
        draw->AddText(ImVec2(posX + labelSize.x, posY), fpsColor, numBuf);
    }
};
