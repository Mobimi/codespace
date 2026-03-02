#pragma once
#include "../Module.h"
#include <imgui.h>
#include <vector>
#include <map>
#include <string>

class ClickGUI : public Module {
private:
    bool isOpen = false;
    bool waitingForBind = false;
    Module* bindingModule = nullptr;

    // Category colors
    std::map<Category, ImVec4> catColors = {
        { Category::HUD,     { 0.2f, 0.6f, 1.0f, 1.0f } },
        { Category::Visual,  { 0.4f, 1.0f, 0.5f, 1.0f } },
        { Category::Utility, { 1.0f, 0.7f, 0.2f, 1.0f } },
    };

    std::map<Category, std::string> catNames = {
        { Category::HUD,     "HUD" },
        { Category::Visual,  "Visual" },
        { Category::Utility, "Utility" },
    };

    // Vị trí mỗi category panel
    std::map<Category, ImVec2> panelPos = {
        { Category::HUD,     { 50.0f,  100.0f } },
        { Category::Visual,  { 230.0f, 100.0f } },
        { Category::Utility, { 410.0f, 100.0f } },
    };

public:
    ClickGUI() : Module("ClickGUI", "Menu chọn chức năng", Category::Utility) {
        keybind = VK_INSERT;
    }

    void toggle() {
        isOpen = !isOpen;
        // Hiện/ẩn con chuột
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = isOpen;
    }

    bool isVisible() const { return isOpen; }

    // Gọi từ bên ngoài khi nhấn keybind
    void handleKeybind() {
        if (GetAsyncKeyState(keybind) & 1) {
            toggle();
        }
    }

    void renderGUI() {
        handleKeybind();
        if (!isOpen) return;

        extern std::vector<std::shared_ptr<Module>> g_modules; // từ ModuleManager

        ImGuiIO& io = ImGui::GetIO();

        // Dim overlay
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(0, 0), io.DisplaySize,
            IM_COL32(0, 0, 0, 100)
        );

        // Vẽ từng category panel
        for (auto& [cat, pos] : panelPos) {
            ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(170, 400), ImGuiCond_Once);

            ImGui::PushStyleColor(ImGuiCol_TitleBg,       ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg,      ImVec4(0.08f, 0.08f, 0.08f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(catColors[cat].x * 0.3f, catColors[cat].y * 0.3f, catColors[cat].z * 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(catColors[cat].x * 0.5f, catColors[cat].y * 0.5f, catColors[cat].z * 0.5f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

            std::string windowName = "##cat_" + catNames[cat];
            ImGui::Begin(windowName.c_str(), nullptr,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            // Category title
            ImGui::PushStyleColor(ImGuiCol_Text, catColors[cat]);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(catNames[cat].c_str()).x) / 2);
            ImGui::Text("%s", catNames[cat].c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            // Lấy modules của category này từ ModuleManager
            // (dùng include extern hoặc singleton)
            renderModulesForCategory(cat);

            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(5);
        }

        // Waiting for bind popup
        if (waitingForBind && bindingModule) {
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - 120, io.DisplaySize.y / 2 - 40), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(240, 80), ImGuiCond_Always);
            ImGui::Begin("##bindpopup", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
            ImGui::SetCursorPosY(15);
            ImGui::SetCursorPosX(20);
            ImGui::Text("Press a key to bind \"%s\"", bindingModule->name.c_str());
            ImGui::SetCursorPosX(20);
            ImGui::TextDisabled("Press ESC to cancel / DEL to clear");
            ImGui::End();

            // Detect key press
            for (int vk = 0; vk < 256; vk++) {
                if (GetAsyncKeyState(vk) & 1) {
                    if (vk == VK_ESCAPE) {
                        // Cancel
                    } else if (vk == VK_DELETE) {
                        bindingModule->setKeybind(0);
                    } else {
                        bindingModule->setKeybind(vk);
                    }
                    waitingForBind = false;
                    bindingModule = nullptr;
                    break;
                }
            }
        }
    }

    void startBinding(Module* mod) {
        waitingForBind = true;
        bindingModule = mod;
    }

private:
    void renderModulesForCategory(Category cat);
};
