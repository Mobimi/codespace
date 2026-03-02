#include "ClickGUI.h"
#include "../ModuleManager.h"

void ClickGUI::renderModulesForCategory(Category cat) {
    for (auto& mod : ModuleManager::getAll()) {
        if (mod->category != cat) continue;
        if (mod->name == "ClickGUI") continue; // Không hiện chính nó

        bool enabled = mod->enabled;

        // Toggle button với màu sắc
        if (enabled) {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.45f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.95f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.35f, 0.75f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
        }

        // Tên module + keybind hint
        std::string label = mod->name;
        if (mod->keybind != 0) {
            label += " [" + mod->getKeybindName() + "]";
        }

        float btnW = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button(label.c_str(), ImVec2(btnW, 30.0f))) {
            mod->toggle();
        }

        ImGui::PopStyleColor(3);

        // Right-click context menu để set keybind
        if (ImGui::BeginPopupContextItem(("ctx_" + mod->name).c_str())) {
            ImGui::Text("Module: %s", mod->name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Set Keybind")) {
                startBinding(mod.get());
            }
            if (ImGui::MenuItem("Clear Keybind")) {
                mod->setKeybind(0);
            }
            ImGui::EndPopup();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("ctx_" + mod->name).c_str());
        }

        // Tooltip description
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", mod->description.c_str());
        }

        ImGui::Spacing();
    }
}
