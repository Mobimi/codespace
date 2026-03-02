# 🎮 BedrockClient - Minecraft Bedrock HUD Client

Client DLL cho Minecraft Bedrock Edition (Windows) với HUD đẹp và ClickGUI.

---

## ✨ Tính năng

| Module | Mô tả | Mặc định |
|--------|-------|----------|
| **Keystrokes** | Hiện phím WASD + LMB/RMB với CPS counter | ✅ Bật |
| **FPS** | FPS counter với màu (xanh/vàng/đỏ) | ✅ Bật |
| **Ping** | Ping tới server (ms) với màu | ✅ Bật |
| **ClickGUI** | Menu chọn bật/tắt module | INSERT |

---

## ⌨️ Lệnh chat

Gõ trong chat game (bắt đầu bằng dấu `.`):

```
.bind clickgui INSERT      → Bind phím INSERT mở ClickGUI
.bind fps F3               → Bind F3 để toggle FPS
.bind keystrokes K         → Bind K để toggle Keystrokes
.unbind fps                → Xóa keybind của FPS
.toggle fps                → Bật/tắt FPS nhanh
.modules                   → Xem danh sách module + keybind
.help                      → Xem tất cả lệnh
```

### Key names hỗ trợ:
`A-Z`, `0-9`, `F1-F12`, `INSERT`, `DELETE`, `HOME`, `END`, `PAGEUP`, `PAGEDOWN`, `TAB`, `CAPSLOCK`, `RSHIFT`, `LSHIFT`, `RCTRL`, `LCTRL`

---

## 🛠️ Cách build

### Yêu cầu:
- **Visual Studio 2022** (với C++ Desktop workload)
- **CMake 3.20+**
- **Windows SDK 10+**

### Bước 1: Tải dependencies

```bash
# Tạo thư mục vendor
mkdir vendor
cd vendor

# Clone ImGui
git clone https://github.com/ocornut/imgui.git

# Clone MinHook
git clone https://github.com/TsudaKageyu/minhook.git
```

### Bước 2: Build MinHook
1. Mở `vendor/minhook/build/VC17/MinHookVC17.sln` bằng Visual Studio
2. Build **Release x64**
3. Copy `MinHook.x64.lib` vào `vendor/minhook/lib/libMinHook.x64.lib`

### Bước 3: Build DLL

```bash
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

Output: `build/Release/BedrockClient.dll`

---

## 💉 Inject DLL vào game

1. Mở Minecraft Bedrock Edition
2. Vào một world (single player hoặc server)
3. Dùng một trong các injector:
   - **[Xenos Injector](https://github.com/DarthTon/Xenos)** ← Khuyên dùng
   - **Process Hacker** (Manual Map inject)
   - **GH Injector**
4. Chọn process `Minecraft.Windows.exe`
5. Inject `BedrockClient.dll`
6. Nhấn **INSERT** để mở ClickGUI

> ⚠️ Nhấn **END** để unload DLL

---

## 📁 Cấu trúc project

```
BedrockClient/
├── src/
│   ├── dllmain.cpp           ← Entry point
│   ├── Module.h              ← Base class
│   ├── ModuleManager.h       ← Quản lý module
│   ├── modules/
│   │   ├── Keystrokes.h      ← WASD + CPS
│   │   ├── FPSDisplay.h      ← FPS counter
│   │   └── PingDisplay.h     ← Ping display
│   ├── gui/
│   │   ├── ClickGUI.h        ← GUI header
│   │   └── ClickGUI.cpp      ← GUI implementation
│   ├── hooks/
│   │   └── DXHook.h          ← DirectX 11 hook + ImGui
│   └── utils/
│       └── CommandHandler.h  ← .bind .toggle .help
├── vendor/
│   ├── imgui/                ← ImGui source
│   └── minhook/              ← MinHook library
└── CMakeLists.txt
```

---

## 🎨 Giao diện

**HUD overlay:**
- FPS và Ping hiện ở góc trên trái với box bo góc
- Keystrokes hiện ở bên trái màn hình, phím sáng lên khi nhấn
- LMB/RMB hiển thị CPS bên dưới

**ClickGUI (mở bằng INSERT):**
- 3 panel theo category: HUD | Visual | Utility
- Click trái để toggle module
- Click phải để set keybind
- Panel có thể kéo di chuyển

---

## ⚠️ Lưu ý

- Chỉ dùng cho **offline / LAN / server riêng**
- Không vi phạm ToS của server public
- Một số Anti-Cheat có thể phát hiện DLL injection
- Test trên Minecraft Bedrock **1.20.x** (Windows 10/11)
