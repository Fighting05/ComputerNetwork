cmake_minimum_required(VERSION 3.10)
project(ChatGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# ImGui 源文件
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
list(FILTER IMGUI_SOURCES EXCLUDE REGEX ".*imgui_demo.*")

# 自编译的 GLFW
set(GLFW_SOURCE_DIR ${CMAKE_SOURCE_DIR}/glfw)
set(GLFW_BUILD_DIR ${GLFW_SOURCE_DIR}/build)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${GLFW_BUILD_DIR}/src/libglfw3.a"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_SOURCE_DIR}/include"
)

# 主程序 - 添加 WIN32 关键字隐藏控制台窗口
add_executable(chat-gui WIN32 src/client_gui.cpp ${IMGUI_SOURCES})

# 链接所有依赖
target_link_libraries(chat-gui
    glfw
    opengl32
    gdi32
    ws2_32          # Winsock
    imm32           # 输入法支持
    dwmapi          # 桌面窗口管理
)

# 包含目录
target_include_directories(chat-gui PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Windows 特定设置 - 隐藏控制台窗口
if(WIN32)
    # 设置为 Windows GUI 子系统
    set_target_properties(chat-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # MinGW 链接选项 - 隐藏控制台的关键参数
    if(MINGW)
        target_link_options(chat-gui PRIVATE
            -static              # 静态链接
            -static-libgcc       # 静态链接 libgcc
            -static-libstdc++    # 静态链接 libstdc++
            -mwindows            # 隐藏控制台窗口 - 最关键！
        )
    endif()
endif()

# 打印配置信息
message(STATUS "========================================")
message(STATUS "Build Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  ImGui directory: ${IMGUI_DIR}")
message(STATUS "  GLFW directory: ${GLFW_SOURCE_DIR}")
message(STATUS "  Hide console: YES (WIN32 + -mwindows)")
message(STATUS "========================================")