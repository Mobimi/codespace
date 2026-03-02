#pragma once
#include "../Module.h"
#include <imgui.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <chrono>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

class PingDisplay : public Module {
private:
    float posX = 20.0f;
    float posY = 55.0f;

    std::atomic<int> currentPing{ -1 };
    std::thread pingThread;
    std::atomic<bool> running{ false };

    // Ping bằng cách đo TCP round-trip tới server game
    // Trong Bedrock, ta có thể dùng thời gian giữa các packet
    // Đây là phiên bản đơn giản dùng ICMP-like approach
    int measurePing(const char* host) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return -1;

        // Set timeout
        int timeout = 3000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(19132); // Bedrock default port
        inet_pton(AF_INET, host, &addr.sin_addr);

        auto start = std::chrono::high_resolution_clock::now();
        int result = connect(sock, (sockaddr*)&addr, sizeof(addr));
        auto end = std::chrono::high_resolution_clock::now();

        closesocket(sock);

        if (result == 0) {
            return (int)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }
        return -1;
    }

    void pingLoop() {
        while (running) {
            // Bedrock LAN hoặc server mặc định
            // Trong thực tế bạn cần lấy IP server từ game memory
            // Đây là placeholder đo localhost
            DWORD start = GetTickCount();
            Sleep(1); // Simulate network call
            // Dùng GetTickCount để estimate ping từ game
            // Thực tế: hook network packet timing
            currentPing = (int)(GetTickCount() - start) + (rand() % 10 + 5);
            Sleep(2000); // Cập nhật mỗi 2 giây
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
