#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <deque>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <sstream>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ============================================================
// GLOBALS
// ============================================================
static bool g_running   = true;
static bool g_guiOpen   = false;
static bool g_imguiInit = false;

static ID3D11Device*           g_device  = nullptr;
static ID3D11DeviceContext*    g_context = nullptr;
static ID3D11RenderTargetView* g_rtv     = nullptr;
static IDXGISwapChain*         g_swapChain = nullptr;
static HWND                    g_hwnd    = nullptr;

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
static Present_t oPresent = nullptr;
typedef LRESULT(CALLBACK* WndProc_t)(HWND, UINT, WPARAM, LPARAM);
static WndProc_t oWndProc = nullptr;

// ============================================================
// MODULE BASE
// ============================================================
enum class Category { HUD, Visual, Utility };

struct Module {
    std::string name, desc;
    Category cat;
    bool enabled = false;
    int  keybind = 0;

    Module(const char* n, const char* d, Category c) : name(n), desc(d), cat(c) {}
    virtual ~Module() = default;

    void toggle() { enabled = !enabled; onToggle(); }
    virtual void onToggle() {}
    virtual void onTick()   {}
    virtual void onRender() {}
    virtual void onSettingsUI() {} // Dropdown settings UI

    std::string getKeyName() const {
        if (!keybind) return "None";
        char buf[32] = {};
        GetKeyNameTextA(MapVirtualKeyA(keybind, MAPVK_VK_TO_VSC) << 16, buf, 32);
        return buf[0] ? buf : "?";
    }
};

// ============================================================
// KEYSTROKES MODULE
// ============================================================
struct Keystrokes : Module {
    std::deque<std::chrono::steady_clock::time_point> lClicks, rClicks;
    bool prevL = false, prevR = false;
    float px = 20, py = 200;
    bool dragging = false;
    float dragOffX = 0, dragOffY = 0;

    Keystrokes() : Module("Keystrokes", "WASD + LMB/RMB + CPS (Shift+Drag de di chuyen)", Category::HUD) { enabled = true; }

    int cps(std::deque<std::chrono::steady_clock::time_point>& q) {
        auto now = std::chrono::steady_clock::now();
        while (!q.empty() && std::chrono::duration_cast<std::chrono::milliseconds>(now - q.front()).count() > 1000)
            q.pop_front();
        return (int)q.size();
    }

    void onTick() override {
        auto now = std::chrono::steady_clock::now();
        bool L = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool R = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
        if (L && !prevL) lClicks.push_back(now);
        if (R && !prevR) rClicks.push_back(now);
        prevL = L; prevR = R;
    }

    void drawKey(ImDrawList* dl, float x, float y, float w, float h, const char* lbl, bool pressed, int c = -1) {
        ImU32 bg  = pressed ? IM_COL32(255,255,255,220) : IM_COL32(30,30,30,180);
        ImU32 fg  = pressed ? IM_COL32(0,0,0,255)       : IM_COL32(255,255,255,255);
        ImU32 brd = pressed ? IM_COL32(255,255,255,255)  : IM_COL32(100,100,100,200);
        dl->AddRectFilled({x,y},{x+w,y+h}, bg, 6);
        dl->AddRect({x,y},{x+w,y+h}, brd, 6, 0, 1.5f);
        auto ts = ImGui::CalcTextSize(lbl);
        dl->AddText({x+(w-ts.x)/2, y+(c>=0?4:(h-ts.y)/2)}, fg, lbl);
        if (c >= 0) {
            char buf[16]; snprintf(buf,16,"%d CPS",c);
            auto cs = ImGui::CalcTextSize(buf);
            dl->AddText({x+(w-cs.x)/2, y+h-cs.y-4},
                pressed ? IM_COL32(0,120,255,255) : IM_COL32(180,180,180,255), buf);
        }
    }

    void onRender() override {
        auto* dl = ImGui::GetBackgroundDrawList();
        auto& io = ImGui::GetIO();

        bool W = GetAsyncKeyState('W')&0x8000, A = GetAsyncKeyState('A')&0x8000;
        bool S = GetAsyncKeyState('S')&0x8000, D = GetAsyncKeyState('D')&0x8000;
        bool L = GetAsyncKeyState(VK_LBUTTON)&0x8000, R = GetAsyncKeyState(VK_RBUTTON)&0x8000;
        bool Space = GetAsyncKeyState(VK_SPACE)&0x8000;

        float g = 4, k = 40, mh = 55, sh = 25;
        float totalW = k*3 + g*2;
        float totalH = k*2 + g + mh + g + sh + g;

        if (!g_guiOpen) {
            float mx = io.MousePos.x, my = io.MousePos.y;
            bool inBox = mx>=px && mx<=px+totalW && my>=py && my<=py+totalH;
            if (inBox && GetAsyncKeyState(VK_SHIFT)&0x8000 && GetAsyncKeyState(VK_LBUTTON)&0x8000) {
                if (!dragging) { dragging=true; dragOffX=mx-px; dragOffY=my-py; }
            }
            if (!(GetAsyncKeyState(VK_LBUTTON)&0x8000)) dragging = false;
            if (dragging) { px = mx-dragOffX; py = my-dragOffY; }
        }

        drawKey(dl, px+k+g,     py,         k,  k,  "W",   W);
        drawKey(dl, px,          py+k+g,     k,  k,  "A",   A);
        drawKey(dl, px+k+g,     py+k+g,     k,  k,  "S",   S);
        drawKey(dl, px+(k+g)*2, py+k+g,     k,  k,  "D",   D);
        float mw = totalW/2 - g/2;
        drawKey(dl, px,          py+(k+g)*2, mw, mh, "LMB", L, cps(lClicks));
        drawKey(dl, px+mw+g,    py+(k+g)*2, mw, mh, "RMB", R, cps(rClicks));
        
        drawKey(dl, px,          py+(k+g)*2 + mh + g, totalW, sh, "       [ SPACE ]       ", Space);

        if (!g_guiOpen) {
            auto hs = ImGui::CalcTextSize("Shift+Drag");
            dl->AddText({px+totalW/2-hs.x/2, py-14}, IM_COL32(150,150,150,120), "Shift+Drag");
        }
    }
};

// ============================================================
// FPS MODULE
// ============================================================
struct FPSDisplay : Module {
    float px=20, py=20;
    std::deque<float> ft;
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    float smooth = 0;

    FPSDisplay() : Module("FPS", "FPS Counter", Category::HUD) { enabled = true; }

    void onRender() override {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now-last).count();
        last = now;
        if (dt > 0) { ft.push_back(dt); if(ft.size()>60) ft.pop_front(); }
        float avg = 0; for(auto f:ft) avg+=f; if(!ft.empty()) avg/=ft.size();
        smooth = avg > 0 ? 1.f/avg : 0;
        int fps = (int)smooth;

        ImU32 col = fps>=60 ? IM_COL32(0,255,128,255) : fps>=30 ? IM_COL32(255,200,0,255) : IM_COL32(255,60,60,255);
        auto* dl = ImGui::GetBackgroundDrawList();
        char buf[32]; snprintf(buf,32,"FPS: %d",fps);
        auto ts = ImGui::CalcTextSize(buf);
        dl->AddRectFilled({px-8,py-5},{px+ts.x+8,py+ts.y+5}, IM_COL32(20,20,20,180), 6);
        dl->AddRect({px-8,py-5},{px+ts.x+8,py+ts.y+5}, IM_COL32(60,60,60,200), 6, 0, 1);
        dl->AddText({px,py}, IM_COL32(200,200,200,255), "FPS: ");
        auto ls = ImGui::CalcTextSize("FPS: ");
        char nb[8]; snprintf(nb,8,"%d",fps);
        dl->AddText({px+ls.x,py}, col, nb);
    }
};

// ============================================================
// PING MODULE
// ============================================================
struct PingDisplay : Module {
    float px=20, py=55;
    std::atomic<int> ping{20};
    std::atomic<bool> run{false};
    std::thread thr;

    PingDisplay() : Module("Ping", "Ping Display", Category::HUD) { enabled = true; }

    void onToggle() override {
        if (enabled) { run=true; thr=std::thread([this]{ while(run){ ping=10+rand()%40; Sleep(2000); } }); }
        else { run=false; if(thr.joinable()) thr.join(); }
    }

    void onRender() override {
        int p = ping.load();
        ImU32 col = p<60 ? IM_COL32(0,255,128,255) : p<120 ? IM_COL32(255,200,0,255) : IM_COL32(255,60,60,255);
        auto* dl = ImGui::GetBackgroundDrawList();
        char buf[32]; snprintf(buf,32,"Ping: %dms",p);
        auto ts = ImGui::CalcTextSize(buf);
        dl->AddRectFilled({px-8,py-5},{px+ts.x+8,py+ts.y+5}, IM_COL32(20,20,20,180), 6);
        dl->AddRect({px-8,py-5},{px+ts.x+8,py+ts.y+5}, IM_COL32(60,60,60,200), 6, 0, 1);
        dl->AddText({px,py}, IM_COL32(200,200,200,255), "Ping: ");
        auto ls = ImGui::CalcTextSize("Ping: ");
        char nb[16]; snprintf(nb,16,"%dms",p);
        dl->AddText({px+ls.x,py}, col, nb);
    }
    ~PingDisplay() { run=false; if(thr.joinable()) thr.join(); }
};

// ============================================================
// AUTO CLICK MODULE
// ============================================================
struct AutoClick : Module {
    // Config
    float cps       = 12.0f;   // clicks per second
    int   button    = 0;       // 0=LMB, 1=RMB, 2=Both
    bool  holdOnly  = true;    // chỉ click khi giữ chuột

    // State
    std::atomic<bool> run{false};
    std::thread thr;
    bool showSettings = false;

    const char* buttonNames[3] = { "LMB", "RMB", "Both" };

    AutoClick() : Module("AutoClick", "Tu dong click chuot", Category::Utility) {}

    void onToggle() override {
        if (enabled) {
            run = true;
            thr = std::thread([this] {
                while (run) {
                    float interval = 1000.0f / cps;
                    Sleep((DWORD)interval);

                    if (!enabled) continue;
                    if (g_guiOpen || GetForegroundWindow() != g_hwnd) continue;

                    bool doClick = !holdOnly;
                    if (holdOnly) {
                        if (button == 0 && (GetAsyncKeyState(VK_LBUTTON)&0x8000)) doClick = true;
                        if (button == 1 && (GetAsyncKeyState(VK_RBUTTON)&0x8000)) doClick = true;
                        if (button == 2 && ((GetAsyncKeyState(VK_LBUTTON)|GetAsyncKeyState(VK_RBUTTON))&0x8000)) doClick = true;
                    }

                    if (!doClick) continue;

                    if (button == 0 || button == 2) {
                        mouse_event(MOUSEEVENTF_LEFTDOWN,  0, 0, 0, 0);
                        Sleep(10);
                        mouse_event(MOUSEEVENTF_LEFTUP,    0, 0, 0, 0);
                    }
                    if (button == 1 || button == 2) {
                        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                        Sleep(10);
                        mouse_event(MOUSEEVENTF_RIGHTUP,   0, 0, 0, 0);
                    }
                }
            });
        } else {
            run = false;
            if (thr.joinable()) thr.join();
        }
    }

    void onSettingsUI() override {
        ImGui::Spacing();
        ImGui::Indent(8);

        // CPS slider
        ImGui::Text("CPS:");
        ImGui::SetNextItemWidth(140);
        if (ImGui::SliderFloat("##cps", &cps, 0.1f, 20.0f, "%.1f CPS"))
            cps = std::max(0.1f, std::min(20.0f, cps));

        // Button selector
        ImGui::Text("Button:");
        ImGui::SetNextItemWidth(140);
        if (ImGui::BeginCombo("##btn", buttonNames[button])) {
            for (int i = 0; i < 3; i++) {
                bool sel = (button == i);
                if (ImGui::Selectable(buttonNames[i], sel))
                    button = i;
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Hold only checkbox
        ImGui::Checkbox("Hold to activate##ac", &holdOnly);
        ImGui::SetItemTooltip("Chi click khi dang giu phim chuot");

        // Keybind
        ImGui::Text("Keybind: %s", getKeyName().c_str());

        ImGui::Unindent(8);
        ImGui::Spacing();
    }

    ~AutoClick() { run=false; if(thr.joinable()) thr.join(); }
};

// ============================================================
// MODULE MANAGER
// ============================================================
static std::vector<Module*> g_modules;

void ModuleManager_Init() {
    g_modules.push_back(new Keystrokes());
    g_modules.push_back(new FPSDisplay());
    g_modules.push_back(new PingDisplay());
    g_modules.push_back(new AutoClick());
    for(auto* m : g_modules) if(m->enabled) m->onToggle();
}

void ModuleManager_Tick() {
    for(auto* m : g_modules) {
        if(m->keybind && (GetAsyncKeyState(m->keybind)&1)) m->toggle();
        if(m->enabled) m->onTick();
    }
}

void ModuleManager_Render() {
    for(auto* m : g_modules) if(m->enabled) m->onRender();
}

Module* ModuleManager_Find(const std::string& name) {
    for(auto* m : g_modules) {
        std::string a=m->name, b=name;
        for(auto& c:a) c=tolower(c);
        for(auto& c:b) c=tolower(c);
        if(a==b) return m;
    }
    return nullptr;
}

// ============================================================
// CLICK GUI
// ============================================================
static bool    g_waitBind   = false;
static Module* g_bindTarget = nullptr;
static std::map<Module*, bool> g_expanded; // track expanded settings

static std::map<Category, ImVec4>      catColor = {
    {Category::HUD,     {0.2f,0.6f,1.f,1.f}},
    {Category::Visual,  {0.4f,1.f,0.5f,1.f}},
    {Category::Utility, {1.f,0.7f,0.2f,1.f}},
};
static std::map<Category, const char*> catName = {
    {Category::HUD,"HUD"},{Category::Visual,"Visual"},{Category::Utility,"Utility"}
};
static std::map<Category, ImVec2> catPos = {
    {Category::HUD,{50,100}},{Category::Visual,{230,100}},{Category::Utility,{410,100}}
};

void RenderClickGUI() {
    if (!g_guiOpen) return;

    auto& io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled({0,0}, io.DisplaySize, IM_COL32(0,0,0,100));

    for (auto& [cat, pos] : catPos) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize({185,450}, ImGuiCond_Once);
        ImGui::PushStyleColor(ImGuiCol_WindowBg,      {0.08f,0.08f,0.08f,0.95f});
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, {0.1f,0.1f,0.1f,1.f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  4.f);

        std::string wid = std::string("##") + catName[cat];
        ImGui::Begin(wid.c_str(), nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        // Category title
        ImGui::PushStyleColor(ImGuiCol_Text, catColor[cat]);
        float tw = ImGui::CalcTextSize(catName[cat]).x;
        ImGui::SetCursorPosX((185-tw)/2);
        ImGui::Text("%s", catName[cat]);
        ImGui::PopStyleColor();
        ImGui::Separator(); ImGui::Spacing();

        for (auto* m : g_modules) {
            if (m->cat != cat) continue;
            bool en = m->enabled;
            bool hasSettings = false;
            // Check if module has settings (AutoClick does)
            if (dynamic_cast<AutoClick*>(m)) hasSettings = true;

            // Toggle button
            if (en) {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.45f,0.85f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.20f,0.55f,0.95f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.10f,0.35f,0.75f,1.f});
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.18f,0.18f,0.18f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f,0.25f,0.25f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.12f,0.12f,0.12f,1.f});
            }

            std::string lbl = m->name;
            if (m->keybind) lbl += " [" + m->getKeyName() + "]";

            // Nếu có settings: nút toggle nhỏ hơn + nút mũi tên
            if (hasSettings) {
                float arrowW = 28.f;
                float btnW = ImGui::GetContentRegionAvail().x - arrowW - 4;
                if (ImGui::Button(lbl.c_str(), {btnW, 30})) m->toggle();
                ImGui::SameLine(0, 4);

                bool& exp = g_expanded[m];
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.22f,0.22f,0.22f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.35f,0.35f,0.35f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.15f,0.15f,0.15f,1.f});
                std::string arrowLbl = std::string(exp ? "v" : ">") + "##arr_" + m->name;
                if (ImGui::Button(arrowLbl.c_str(), {arrowW, 30})) exp = !exp;
                ImGui::PopStyleColor(3);

                // Dropdown settings
                if (exp) {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.12f,0.12f,0.12f,1.f});
                    ImGui::BeginChild(("##set_"+m->name).c_str(), {ImGui::GetContentRegionAvail().x, 130}, true);
                    m->onSettingsUI();
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                }
            } else {
                if (ImGui::Button(lbl.c_str(), {ImGui::GetContentRegionAvail().x, 30}))
                    m->toggle();
            }

            ImGui::PopStyleColor(3);

            // Right click context menu
            std::string pid = "ctx_" + m->name;
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup(pid.c_str());
            if (ImGui::BeginPopup(pid.c_str())) {
                if (ImGui::MenuItem("Set Keybind"))   { g_waitBind=true; g_bindTarget=m; }
                if (ImGui::MenuItem("Clear Keybind")) { m->keybind=0; }
                ImGui::EndPopup();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                ImGui::SetTooltip("%s", m->desc.c_str());

            ImGui::Spacing();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    // Bind popup
    if (g_waitBind && g_bindTarget) {
        ImGui::SetNextWindowPos({io.DisplaySize.x/2-120, io.DisplaySize.y/2-40}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({240,80}, ImGuiCond_Always);
        ImGui::Begin("##bind", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove);
        ImGui::SetCursorPos({10,15});
        ImGui::Text("Press key for \"%s\"", g_bindTarget->name.c_str());
        ImGui::SetCursorPosX(10);
        ImGui::TextDisabled("ESC = cancel | DEL = clear");
        ImGui::End();
        for (int vk=8; vk<256; vk++) {
            if (GetAsyncKeyState(vk)&1) {
                if (vk==VK_ESCAPE) {}
                else if (vk==VK_DELETE) g_bindTarget->keybind=0;
                else g_bindTarget->keybind=vk;
                g_waitBind=false; g_bindTarget=nullptr; break;
            }
        }
    }
}

// ============================================================
// COMMAND HANDLER
// ============================================================
static std::map<std::string,int> keyMap = {
    {"A",0x41},{"B",0x42},{"C",0x43},{"D",0x44},{"E",0x45},{"F",0x46},{"G",0x47},
    {"H",0x48},{"I",0x49},{"J",0x4A},{"K",0x4B},{"L",0x4C},{"M",0x4D},{"N",0x4E},
    {"O",0x4F},{"P",0x50},{"Q",0x51},{"R",0x52},{"S",0x53},{"T",0x54},{"U",0x55},
    {"V",0x56},{"W",0x57},{"X",0x58},{"Y",0x59},{"Z",0x5A},
    {"F1",VK_F1},{"F2",VK_F2},{"F3",VK_F3},{"F4",VK_F4},{"F5",VK_F5},{"F6",VK_F6},
    {"F7",VK_F7},{"F8",VK_F8},{"F9",VK_F9},{"F10",VK_F10},{"F11",VK_F11},{"F12",VK_F12},
    {"INSERT",VK_INSERT},{"DELETE",VK_DELETE},{"HOME",VK_HOME},{"END",VK_END},
    {"TAB",VK_TAB},{"0",0x30},{"1",0x31},{"2",0x32},{"3",0x33},{"4",0x34},
    {"5",0x35},{"6",0x36},{"7",0x37},{"8",0x38},{"9",0x39},
};

bool HandleCommand(const std::string& msg) {
    if (msg.empty() || msg[0] != '.') return false;
    std::istringstream ss(msg.substr(1));
    std::vector<std::string> tok; std::string t;
    while(ss>>t) tok.push_back(t);
    if(tok.empty()) return false;
    std::string cmd=tok[0]; for(auto& c:cmd) c=toupper(c);
    auto showMsg=[](const std::string& s){ OutputDebugStringA(("[Client] "+s+"\n").c_str()); };
    if(cmd=="BIND"){
        if(tok.size()<3){ showMsg("Usage: .bind [module] [key]"); return true; }
        auto* m=ModuleManager_Find(tok[1]);
        if(!m){ showMsg("Module not found!"); return true; }
        std::string key=tok[2]; for(auto& c:key) c=toupper(c);
        if(keyMap.find(key)==keyMap.end()){ showMsg("Key not found!"); return true; }
        m->keybind=keyMap[key]; showMsg("Bound "+m->name+" to "+key); return true;
    }
    if(cmd=="UNBIND"){
        if(tok.size()<2) return true;
        auto* m=ModuleManager_Find(tok[1]);
        if(m){ m->keybind=0; showMsg("Unbound "+m->name); } return true;
    }
    if(cmd=="TOGGLE"){
        if(tok.size()<2) return true;
        auto* m=ModuleManager_Find(tok[1]);
        if(m){ m->toggle(); showMsg(m->name+(m->enabled?" enabled":" disabled")); } return true;
    }
    if(cmd=="HELP"){
        showMsg(".bind [module] [key]");
        showMsg(".unbind [module]");
        showMsg(".toggle [module]");
        return true;
    }
    return false;
}

// ============================================================
// DIRECTX HOOK
// ============================================================
static int guiKeybind = VK_INSERT;

static LRESULT CALLBACK HookedWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    if (g_guiOpen) switch(m) {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
        case WM_MOUSEMOVE:   case WM_MOUSEWHEEL:
        case WM_KEYDOWN:     case WM_KEYUP: case WM_CHAR: return 0;
    }
    return CallWindowProcA(oWndProc, h, m, w, l);
}

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding=8; s.FrameRounding=4; s.ItemSpacing={8,6};
    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]      = {0.08f,0.08f,0.08f,0.95f};
    c[ImGuiCol_ChildBg]       = {0.10f,0.10f,0.10f,1.f};
    c[ImGuiCol_TitleBgActive] = {0.12f,0.12f,0.12f,1.f};
    c[ImGuiCol_Border]        = {0.25f,0.25f,0.25f,0.6f};
    c[ImGuiCol_Button]        = {0.18f,0.18f,0.18f,1.f};
    c[ImGuiCol_ButtonHovered] = {0.25f,0.45f,0.85f,1.f};
    c[ImGuiCol_Text]          = {0.9f,0.9f,0.9f,1.f};
    c[ImGuiCol_SliderGrab]    = {0.25f,0.45f,0.85f,1.f};
    c[ImGuiCol_SliderGrabActive] = {0.3f,0.55f,0.95f,1.f};
    c[ImGuiCol_FrameBg]       = {0.15f,0.15f,0.15f,1.f};
    c[ImGuiCol_FrameBgHovered]= {0.20f,0.20f,0.20f,1.f};
    c[ImGuiCol_Header]        = {0.15f,0.40f,0.80f,0.5f};
    c[ImGuiCol_HeaderHovered] = {0.20f,0.45f,0.85f,0.8f};
    c[ImGuiCol_CheckMark]     = {0.25f,0.55f,0.95f,1.f};
    c[ImGuiCol_Separator]     = {0.3f,0.3f,0.3f,0.6f};
}

HRESULT __stdcall HookedPresent(IDXGISwapChain* chain, UINT sync, UINT flags) {
    if (!g_imguiInit) {
        chain->GetDevice(__uuidof(ID3D11Device),(void**)&g_device);
        g_device->GetImmediateContext(&g_context);
        ID3D11Texture2D* bb=nullptr;
        chain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&bb);
        g_device->CreateRenderTargetView(bb,nullptr,&g_rtv);
        bb->Release();
        DXGI_SWAP_CHAIN_DESC desc; chain->GetDesc(&desc);
        g_hwnd = desc.OutputWindow;
        oWndProc = (WndProc_t)SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ApplyStyle();
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_device,g_context);
        g_swapChain = chain;
        g_imguiInit = true;
    }

    if (GetAsyncKeyState(guiKeybind)&1) {
        g_guiOpen = !g_guiOpen;
        ImGui::GetIO().MouseDrawCursor = g_guiOpen;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ModuleManager_Tick();
    ModuleManager_Render();
    RenderClickGUI();

    ImGui::Render();
    g_context->OMSetRenderTargets(1,&g_rtv,nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return oPresent(chain,sync,flags);
}

void HookInit() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount=1; sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow=GetForegroundWindow();
    sd.SampleDesc.Count=1; sd.Windowed=TRUE;
    sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    ID3D11Device* dd=nullptr; IDXGISwapChain* ds=nullptr; ID3D11DeviceContext* dc=nullptr;
    D3D_FEATURE_LEVEL fl=D3D_FEATURE_LEVEL_11_0;
    if(FAILED(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,&fl,1,D3D11_SDK_VERSION,&sd,&ds,&dd,nullptr,&dc))) return;
    void** vmt=*(void***)ds;
    DWORD old; VirtualProtect(&vmt[8],8,PAGE_EXECUTE_READWRITE,&old);
    oPresent=(Present_t)vmt[8]; vmt[8]=HookedPresent;
    VirtualProtect(&vmt[8],8,old,&old);
    ds->Release(); dd->Release(); dc->Release();
}

// ============================================================
// DLL MAIN
// ============================================================
void MainThread(HMODULE hMod) {
    Sleep(500);
    ModuleManager_Init();
    HookInit();
    while(g_running && !(GetAsyncKeyState(VK_END)&1)) Sleep(100);
    if(g_imguiInit){
        ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
        if(g_hwnd && oWndProc) SetWindowLongPtrA(g_hwnd,GWLP_WNDPROC,(LONG_PTR)oWndProc);
        if(g_rtv) g_rtv->Release();
    }
    for(auto* m:g_modules) delete m;
    FreeLibraryAndExitThread(hMod,0);
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){ DisableThreadLibraryCalls(hMod); CreateThread(nullptr,0,(LPTHREAD_START_ROUTINE)MainThread,hMod,0,nullptr); }
    return TRUE;
}
