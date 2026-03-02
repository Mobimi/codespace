#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <Windows.h>
#include "../ModuleManager.h"

class CommandHandler {
private:
    // Map tên key string -> VK code
    static inline std::map<std::string, int> keyMap = {
        {"A",0x41},{"B",0x42},{"C",0x43},{"D",0x44},{"E",0x45},
        {"F",0x46},{"G",0x47},{"H",0x48},{"I",0x49},{"J",0x4A},
        {"K",0x4B},{"L",0x4C},{"M",0x4D},{"N",0x4E},{"O",0x4F},
        {"P",0x50},{"Q",0x51},{"R",0x52},{"S",0x53},{"T",0x54},
        {"U",0x55},{"V",0x56},{"W",0x57},{"X",0x58},{"Y",0x59},
        {"Z",0x5A},
        {"F1",VK_F1},{"F2",VK_F2},{"F3",VK_F3},{"F4",VK_F4},
        {"F5",VK_F5},{"F6",VK_F6},{"F7",VK_F7},{"F8",VK_F8},
        {"F9",VK_F9},{"F10",VK_F10},{"F11",VK_F11},{"F12",VK_F12},
        {"INSERT",VK_INSERT},{"DELETE",VK_DELETE},{"HOME",VK_HOME},
        {"END",VK_END},{"PAGEUP",VK_PRIOR},{"PAGEDOWN",VK_NEXT},
        {"0",0x30},{"1",0x31},{"2",0x32},{"3",0x33},{"4",0x34},
        {"5",0x35},{"6",0x36},{"7",0x37},{"8",0x38},{"9",0x39},
        {"RSHIFT",VK_RSHIFT},{"LSHIFT",VK_LSHIFT},
        {"RCTRL",VK_RCONTROL},{"LCTRL",VK_LCONTROL},
        {"TAB",VK_TAB},{"CAPSLOCK",VK_CAPITAL},
    };

    static std::string toUpper(std::string s) {
        for (auto& c : s) c = toupper(c);
        return s;
    }

    static std::vector<std::string> split(const std::string& str) {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        return tokens;
    }

public:
    static void Init() {
        // Hook vào game chat input
        // Trong thực tế: hook SendPacket hoặc chat submit function
        // Đây là thread giả lập nhận lệnh từ console hoặc hook
    }

    // Gọi hàm này khi người chơi gửi chat message
    // Trả về true nếu là lệnh client (không gửi lên server)
    static bool handleCommand(const std::string& message) {
        if (message.empty() || message[0] != '.') return false;

        std::string cmd = message.substr(1); // Bỏ dấu .
        auto tokens = split(cmd);
        if (tokens.empty()) return false;

        std::string command = toUpper(tokens[0]);

        // === .bind [module] [key] ===
        if (command == "BIND") {
            if (tokens.size() < 3) {
                showMessage("§cUsage: .bind [module] [key]");
                showMessage("§7Example: .bind clickgui INSERT");
                return true;
            }

            std::string modName = tokens[1];
            std::string keyName = toUpper(tokens[2]);

            // Tìm module (case-insensitive)
            std::shared_ptr<Module> target = nullptr;
            for (auto& m : ModuleManager::getAll()) {
                std::string mn = m->name;
                for (auto& c : mn) c = tolower(c);
                std::string input = tokens[1];
                for (auto& c : input) c = tolower(c);
                if (mn == input) { target = m; break; }
            }

            if (!target) {
                showMessage("§cModule \"" + tokens[1] + "\" not found!");
                listModules();
                return true;
            }

            // Tìm VK code
            if (keyMap.find(keyName) == keyMap.end()) {
                showMessage("§cKey \"" + tokens[2] + "\" not recognized!");
                return true;
            }

            int vk = keyMap[keyName];
            target->setKeybind(vk);
            showMessage("§aBound §f" + target->name + " §ato §f" + keyName);
            return true;
        }

        // === .unbind [module] ===
        if (command == "UNBIND") {
            if (tokens.size() < 2) {
                showMessage("§cUsage: .unbind [module]");
                return true;
            }
            for (auto& m : ModuleManager::getAll()) {
                std::string mn = m->name, input = tokens[1];
                for (auto& c : mn) c = tolower(c);
                for (auto& c : input) c = tolower(c);
                if (mn == input) {
                    m->setKeybind(0);
                    showMessage("§aUnbound §f" + m->name);
                    return true;
                }
            }
            showMessage("§cModule not found!");
            return true;
        }

        // === .modules ===
        if (command == "MODULES") {
            listModules();
            return true;
        }

        // === .toggle [module] ===
        if (command == "TOGGLE") {
            if (tokens.size() < 2) {
                showMessage("§cUsage: .toggle [module]");
                return true;
            }
            for (auto& m : ModuleManager::getAll()) {
                std::string mn = m->name, input = tokens[1];
                for (auto& c : mn) c = tolower(c);
                for (auto& c : input) c = tolower(c);
                if (mn == input) {
                    m->toggle();
                    showMessage(std::string("§") + (m->enabled ? "a" : "c") +
                                m->name + (m->enabled ? " §aenabled" : " §cdisabled"));
                    return true;
                }
            }
            showMessage("§cModule not found!");
            return true;
        }

        // === .help ===
        if (command == "HELP") {
            showMessage("§6=== BedrockClient Commands ===");
            showMessage("§f.bind §7[module] [key] §8- Set keybind");
            showMessage("§f.unbind §7[module] §8- Remove keybind");
            showMessage("§f.toggle §7[module] §8- Toggle module");
            showMessage("§f.modules §8- List all modules");
            showMessage("§f.help §8- Show this help");
            return true;
        }

        return false; // Lệnh không nhận ra, gửi lên server bình thường
    }

private:
    static void showMessage(const std::string& msg) {
        // Trong thực tế: hook AddMessage của game để hiện trong chat
        // Placeholder: OutputDebugString
        OutputDebugStringA(("[BedrockClient] " + msg + "\n").c_str());
    }

    static void listModules() {
        showMessage("§6Modules:");
        for (auto& m : ModuleManager::getAll()) {
            std::string bind = m->keybind != 0 ? " [" + m->getKeybindName() + "]" : "";
            showMessage(std::string("  §") + (m->enabled ? "a" : "7") +
                        m->name + bind);
        }
    }
};
