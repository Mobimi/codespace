#pragma once
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "Module.h"
#include "modules/Keystrokes.h"
#include "modules/FPSDisplay.h"
#include "modules/PingDisplay.h"
#include "gui/ClickGUI.h"

class ModuleManager {
public:
    static inline std::vector<std::shared_ptr<Module>> modules;
    static inline std::shared_ptr<ClickGUI> clickGUI;

    static void Init() {
        // Đăng ký tất cả modules
        modules.push_back(std::make_shared<Keystrokes>());
        modules.push_back(std::make_shared<FPSDisplay>());
        modules.push_back(std::make_shared<PingDisplay>());

        // ClickGUI module đặc biệt
        clickGUI = std::make_shared<ClickGUI>();
        modules.push_back(clickGUI);

        // Bật mặc định
        getModule("Keystrokes")->enabled = true;
        getModule("FPS")->enabled = true;
        getModule("Ping")->enabled = true;

        // Keybind mặc định cho ClickGUI = Insert
        clickGUI->setKeybind(VK_INSERT);
    }

    static std::shared_ptr<Module> getModule(const std::string& name) {
        for (auto& m : modules) {
            if (m->name == name) return m;
        }
        return nullptr;
    }

    static void onTick() {
        for (auto& m : modules) {
            if (m->enabled) m->onTick();
            // Kiểm tra keybind
            if (m->keybind != 0 && GetAsyncKeyState(m->keybind) & 1) {
                m->toggle();
            }
        }
    }

    static void onRender() {
        for (auto& m : modules) {
            if (m->enabled) m->onRender();
        }
        // ClickGUI render riêng (kể cả khi không "enabled" theo nghĩa thông thường)
        clickGUI->renderGUI();
    }

    static std::vector<std::shared_ptr<Module>>& getAll() {
        return modules;
    }
};
