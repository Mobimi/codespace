#pragma once
#include <string>
#include <Windows.h>

enum class Category {
    HUD,
    Visual,
    Utility
};

class Module {
public:
    std::string name;
    std::string description;
    Category category;
    bool enabled = false;
    int keybind = 0; // VK code

    Module(const std::string& name, const std::string& desc, Category cat)
        : name(name), description(desc), category(cat) {}

    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onTick() {}
    virtual void onRender() {} // Gọi mỗi frame để vẽ HUD

    void toggle() {
        enabled = !enabled;
        if (enabled) onEnable();
        else onDisable();
    }

    void setKeybind(int vkKey) { keybind = vkKey; }
    int getKeybind() const { return keybind; }
    std::string getKeybindName() const {
        if (keybind == 0) return "None";
        char buf[32];
        GetKeyNameTextA(MapVirtualKeyA(keybind, MAPVK_VK_TO_VSC) << 16, buf, sizeof(buf));
        return std::string(buf);
    }
};
