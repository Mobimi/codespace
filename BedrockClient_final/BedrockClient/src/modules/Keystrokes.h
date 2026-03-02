#pragma once
#include "../Module.h"
#include <imgui.h>
#include <deque>
#include <chrono>

class Keystrokes : public Module {
private:
    // CPS tracking
    std::deque<std::chrono::steady_clock::time_point> lmbClicks;
    std::deque<std::chrono::steady_clock::time_point> rmbClicks;

    bool lastLMB = false;
    bool lastRMB = false;

    // Vị trí HUD (có thể kéo thả)
    float posX = 20.0f;
    float posY = 200.0f;

    int getCPS(std::deque<std::chrono::steady_clock::time_point>& clicks) {
        auto now = std::chrono::steady_clock::now();
        // Xóa click cũ hơn 1 giây
        while (!clicks.empty() &&
               std::chrono::duration_cast<std::chrono::milliseconds>(now - clicks.front()).count() > 1000) {
            clicks.pop_front();
        }
        return (int)clicks.size();
    }

    void renderKey(ImDrawList* draw, float x, float y, float w, float h,
                   const char* label, bool pressed, int cps = -1) {
        ImU32 bgColor = pressed
            ? IM_COL32(255, 255, 255, 220)
            : IM_COL32(30, 30, 30, 180);
        ImU32 textColor = pressed
            ? IM_COL32(0, 0, 0, 255)
            : IM_COL32(255, 255, 255, 255);
        ImU32 borderColor = pressed
            ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(100, 100, 100, 200);

        // Background với bo góc
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), bgColor, 6.0f);
        draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), borderColor, 6.0f, 0, 1.5f);

        // Text label
        ImVec2 textSize = ImGui::CalcTextSize(label);
        draw->AddText(
            ImVec2(x + (w - textSize.x) / 2, y + (cps >= 0 ? 4.0f : (h - textSize.y) / 2)),
            textColor, label
        );

        // CPS text bên dưới (chỉ cho LMB/RMB)
        if (cps >= 0) {
            char cpsStr[16];
            snprintf(cpsStr, sizeof(cpsStr), "%d CPS", cps);
            ImVec2 cpsSize = ImGui::CalcTextSize(cpsStr);
            draw->AddText(
                ImVec2(x + (w - cpsSize.x) / 2, y + h - cpsSize.y - 4.0f),
                pressed ? IM_COL32(0, 120, 255, 255) : IM_COL32(180, 180, 180, 255),
                cpsStr
            );
        }
    }

public:
    Keystrokes() : Module("Keystrokes", "Hiện phím WASD, LMB, RMB và CPS", Category::HUD) {}

    void onTick() override {
        auto now = std::chrono::steady_clock::now();

        bool curLMB = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool curRMB = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

        if (curLMB && !lastLMB) lmbClicks.push_back(now);
        if (curRMB && !lastRMB) rmbClicks.push_back(now);

        lastLMB = curLMB;
        lastRMB = curRMB;
    }

    void onRender() override {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        bool W = GetAsyncKeyState('W') & 0x8000;
        bool A = GetAsyncKeyState('A') & 0x8000;
        bool S = GetAsyncKeyState('S') & 0x8000;
        bool D = GetAsyncKeyState('D') & 0x8000;
        bool LMB = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool RMB = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

        int lCPS = getCPS(lmbClicks);
        int rCPS = getCPS(rmbClicks);

        float kW = 40.0f, kH = 40.0f;
        float gap = 4.0f;
        float mouseH = 55.0f; // Cao hơn để hiện CPS

        float baseX = posX;
        float baseY = posY;

        // W key (giữa hàng trên)
        renderKey(draw, baseX + kW + gap, baseY, kW, kH, "W", W);

        // A S D
        renderKey(draw, baseX,               baseY + kH + gap, kW, kH, "A", A);
        renderKey(draw, baseX + kW + gap,    baseY + kH + gap, kW, kH, "S", S);
        renderKey(draw, baseX + (kW + gap)*2, baseY + kH + gap, kW, kH, "D", D);

        // LMB RMB (hàng 3, cao hơn)
        float mouseY = baseY + (kH + gap) * 2;
        float mouseW = (kW * 3 + gap * 2) / 2.0f - gap / 2.0f;

        renderKey(draw, baseX,             mouseY, mouseW, mouseH, "LMB", LMB, lCPS);
        renderKey(draw, baseX + mouseW + gap, mouseY, mouseW, mouseH, "RMB", RMB, rCPS);
    }
};
