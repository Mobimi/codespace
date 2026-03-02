cmake_minimum_required(VERSION 3.20)
project(BedrockClient)
set(CMAKE_CXX_STANDARD 17)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

set(SOURCES
    dllmain.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(BedrockClient SHARED ${SOURCES})

target_include_directories(BedrockClient PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(BedrockClient PRIVATE d3d11.lib dxgi.lib)
set_target_properties(BedrockClient PROPERTIES OUTPUT_NAME "BedrockClient" SUFFIX ".dll")
