cmake_minimum_required(VERSION 3.20)
project(BedrockClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ImGui ---
# Tải ImGui từ: https://github.com/ocornut/imgui
# Đặt vào thư mục: vendor/imgui/
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

# --- MinHook ---
# Tải MinHook từ: https://github.com/TsudaKageyu/minhook
# Đặt vào thư mục: vendor/minhook/
set(MINHOOK_DIR ${CMAKE_SOURCE_DIR}/vendor/minhook)

# --- Source files ---
set(SOURCES
    src/dllmain.cpp
    src/gui/ClickGUI.cpp
    ${IMGUI_SOURCES}
)

add_library(BedrockClient SHARED ${SOURCES})

target_include_directories(BedrockClient PRIVATE
    src/
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${MINHOOK_DIR}/include
)

target_link_libraries(BedrockClient PRIVATE
    d3d11.lib
    dxgi.lib
    ws2_32.lib
    iphlpapi.lib
    ${MINHOOK_DIR}/lib/libMinHook.x64.lib
)

# Compile cho x64
set_target_properties(BedrockClient PROPERTIES
    OUTPUT_NAME "BedrockClient"
    SUFFIX ".dll"
)

target_compile_options(BedrockClient PRIVATE /W3 /MP)
