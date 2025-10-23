// src/client_gui.cpp
#define IMGUI_IMPL_OPENGL_LOADER_GL3W
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

// 全局变量
vector<string> g_chatHistory;
mutex g_chatMutex;
SOCKET g_clientSocket = INVALID_SOCKET;
bool g_connected = false;
string g_nickname;

// GBK 转 UTF-8（用于发送到服务器）
string gbkToUtf8(const std::string& gbkStr) {
    int wideCharCount = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, NULL, 0);
    if (wideCharCount == 0) return {}; 

    std::wstring wideStr(wideCharCount - 1, 0);
    MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, &wideStr[0], wideCharCount);

    int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8ByteCount == 0) return {};

    std::string utf8Str(utf8ByteCount - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8ByteCount, NULL, NULL);

    return utf8Str;
}

// UTF-8 转 GBK（用于显示）
std::string utf8ToGbk(const std::string& utf8Str) {
    int wideCharCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideCharCount == 0) return utf8Str;

    std::wstring wideStr(wideCharCount - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideCharCount);

    int gbkByteCount = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (gbkByteCount == 0) return utf8Str;

    std::string gbkStr(gbkByteCount - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &gbkStr[0], gbkByteCount, NULL, NULL);

    return gbkStr;
}

// 接收服务器消息线程
void rcvFromServer(SOCKET clientSocket) {
    while (true) {
        string rcvBuf(1024, '\0');
        int bytesReceived = recv(clientSocket, &rcvBuf[0], rcvBuf.size(), 0);
        
        if (bytesReceived <= 0) {
            {
                std::lock_guard<std::mutex> lock(g_chatMutex);
                g_chatHistory.push_back("[系统] 连接已断开或接收失败");
                g_connected = false;
            }
            break;
        }
        
        rcvBuf.resize(bytesReceived);
        string displayMsg = utf8ToGbk(rcvBuf);
        
        {
            std::lock_guard<std::mutex> lock(g_chatMutex);
            g_chatHistory.push_back(displayMsg);
        }
    }
}

// 发送消息到服务器
bool sendToServer(const string& message) {
    if (!g_connected || g_clientSocket == INVALID_SOCKET) 
    {
        return false;
    }

    string utf8Buffer = gbkToUtf8(message);
    if (utf8Buffer.empty()) 
    {
        g_chatHistory.push_back("[系统] 消息编码转换错误");
        return false;
    }
    
    utf8Buffer += "\n";
    int result = send(g_clientSocket, utf8Buffer.c_str(), utf8Buffer.size(), 0);
    
    return result != SOCKET_ERROR;
}

// 连接服务器
bool connectToServer(const char* ip, int port, const string& nickname) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        g_chatHistory.push_back("[系统] WSAStartup错误");
        return false;
    }

    g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_clientSocket == INVALID_SOCKET) 
    {
        g_chatHistory.push_back("[系统] 创建socket错误");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    if (connect(g_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        g_chatHistory.push_back("[系统] 连接服务器错误");
        closesocket(g_clientSocket);
        WSACleanup();
        return false;
    }

    string nickMsg = nickname + "\n";
    send(g_clientSocket, nickMsg.c_str(), nickMsg.size(), 0);
    
    g_connected = true;
    g_nickname = nickname;
    
    thread(rcvFromServer, g_clientSocket).detach();
    
    {
        std::lock_guard<std::mutex> lock(g_chatMutex);
        g_chatHistory.push_back("[系统] 已成功连接到服务器");
        g_chatHistory.push_back("[系统] 你的昵称: " + nickname);
        g_chatHistory.push_back("通过输入“/msg加空格加目标昵称加空格加消息内容”来私聊\n例如：/msg 张三 你好！");
    }
    
    return true;
}

// Windows程序入口点（隐藏控制台窗口）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    if (!glfwInit()) 
    {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建更大的初始窗口
    GLFWwindow* window = glfwCreateWindow(1200, 800, "聊天室客户端", nullptr, nullptr);
    if (window == nullptr) 
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 加载中文字体
    ImFont* font = io.Fonts->AddFontFromFileTTF("../fonts/SimHei.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    if (font == nullptr) {
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    }

    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 界面状态变量
    char nicknameInput[128] = "";
    char serverIP[64] = "60.205.14.222";
    int serverPort = 2059;
    char messageInput[512] = "";
    bool showConnectDialog = true;
    bool autoScroll = true;

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 获取窗口大小
        int window_width, window_height;
        glfwGetWindowSize(window, &window_width, &window_height);

        // 连接对话框 - 全屏背景 + 居中对话框
        if (showConnectDialog && !g_connected) {
            // 创建全屏背景窗口
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
            
            ImGui::Begin("##ConnectionBackground", nullptr,
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoBringToFrontOnFocus);
            
            // 对话框内容居中
            ImVec2 dialogSize(1056, 768);
            ImVec2 dialogPos((window_width - dialogSize.x) * 0.5f, (window_height - dialogSize.y) * 0.5f);
            
            ImGui::SetCursorPos(dialogPos);
            
            // 创建对话框子窗口
            ImGui::BeginChild("ConnectionDialog", dialogSize, true, ImGuiWindowFlags_NoScrollbar);
            
            // 标题
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::PushFont(io.Fonts->Fonts[0]); // 使用默认字体
            float titleWidth = ImGui::CalcTextSize("连接到服务器").x;
            ImGui::SetCursorPosX((dialogSize.x - titleWidth) * 0.5f);
            ImGui::Text("连接到服务器");
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(0.0f, 15.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 25.0f));
            
            // 输入区域
            float leftPadding = 40.0f;
            float labelWidth = 100.0f;
            float inputWidth = dialogSize.x - leftPadding - labelWidth - 40.0f;
            
            // 服务器IP
            ImGui::Dummy(ImVec2(leftPadding, 0.0f));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("服务器IP:");
            ImGui::SameLine(leftPadding + labelWidth);
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputText("##ServerIP", serverIP, sizeof(serverIP));
            ImGui::PopItemWidth();
            
            ImGui::Dummy(ImVec2(0.0f, 18.0f));
            
            // 端口
            ImGui::Dummy(ImVec2(leftPadding, 0.0f));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("端口:");
            ImGui::SameLine(leftPadding + labelWidth);
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputInt("##Port", &serverPort);
            ImGui::PopItemWidth();
            
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            
            // 昵称
            ImGui::Dummy(ImVec2(leftPadding, 0.0f));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("昵称:");
            ImGui::SameLine(leftPadding + labelWidth);
            ImGui::PushItemWidth(inputWidth);
            ImGui::InputText("##Nickname", nicknameInput, sizeof(nicknameInput));
            ImGui::PopItemWidth();
            
            ImGui::Dummy(ImVec2(0.0f, 45.0f));
            
            // 按钮
            float buttonWidth = 180.0f;
            float buttonHeight = 45.0f;
            float spacing = 30.0f;
            float totalWidth = buttonWidth * 2 + spacing;
            ImGui::SetCursorPosX((dialogSize.x - totalWidth) * 0.5f);
            
            if (ImGui::Button("连接", ImVec2(buttonWidth, buttonHeight))) {
                if (strlen(nicknameInput) > 0) {
                    if (connectToServer(serverIP, serverPort, std::string(nicknameInput))) {
                        showConnectDialog = false;
                    } else {
                        std::lock_guard<std::mutex> lock(g_chatMutex);
                        g_chatHistory.push_back("[错误] 连接失败，请检查服务器地址和端口");
                    }
                }
            }
            
            ImGui::SameLine(0.0f, spacing);
            
            if (ImGui::Button("取消", ImVec2(buttonWidth, buttonHeight))) {
                glfwSetWindowShouldClose(window, true);
            }
            
            ImGui::EndChild();
            ImGui::End();
        }

        // 主聊天窗口
        if (g_connected || !showConnectDialog) {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
            
            ImGui::Begin("聊天室", nullptr, 
                        ImGuiWindowFlags_NoResize | 
                        ImGuiWindowFlags_NoMove | 
                        ImGuiWindowFlags_NoCollapse | 
                        ImGuiWindowFlags_MenuBar |
                        ImGuiWindowFlags_NoBringToFrontOnFocus);

            // 菜单栏
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("连接")) {
                    if (ImGui::MenuItem("重新连接", nullptr, false, !g_connected)) {
                        showConnectDialog = true;
                    }
                    if (ImGui::MenuItem("断开连接", nullptr, false, g_connected)) {
                        if (g_connected && g_clientSocket != INVALID_SOCKET) 
                        {
                            closesocket(g_clientSocket);
                            g_clientSocket = INVALID_SOCKET;
                            g_connected = false;
                            
                            std::lock_guard<std::mutex> lock(g_chatMutex);
                            g_chatHistory.push_back("[系统] 已断开连接");
                        }
                        WSACleanup();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("退出")) {
                        glfwSetWindowShouldClose(window, true);
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("选项")) {
                    ImGui::MenuItem("自动滚动", nullptr, &autoScroll);
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenuBar();
            }

            // 状态栏
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            if (g_connected) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● 已连接");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "● 未连接");
            }
            ImGui::SameLine();
            ImGui::Text("| 昵称: %s | 服务器: %s:%d", 
                       g_nickname.c_str(), serverIP, serverPort);
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Separator();

            // 聊天记录区域
            float statusBarHeight = 85.0f;
            float inputAreaHeight = 110.0f;
            float chatHeight = window_height - statusBarHeight - inputAreaHeight;
            
            ImGui::BeginChild("ChatHistory", ImVec2(0, chatHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::lock_guard<std::mutex> lock(g_chatMutex);
                for (const auto& msg : g_chatHistory) {
                    ImGui::TextWrapped("%s", msg.c_str());
                }
            }
            
            if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            // 输入区域
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            
            // 使用表格布局来对齐
            ImGui::Text("输入消息:");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            
            float buttonWidth = 100.0f;
            float buttonHeight = 38.0f;
            float inputWidth = window_width - buttonWidth - 50.0f;
            
            // 输入框
            ImGui::PushItemWidth(inputWidth);
            // 增加输入框高度
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 10));
            
            // 设置输入框自动获取焦点
            static bool needFocus = true;  // 标记是否需要设置焦点
            if (needFocus) {
                ImGui::SetKeyboardFocusHere();  // 设置焦点到下一个控件
                needFocus = false;
            }
            
            bool enterPressed = ImGui::InputText("##MessageInput", messageInput, sizeof(messageInput), 
                                                ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleVar();
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            bool sendClicked = ImGui::Button("发送", ImVec2(buttonWidth, buttonHeight));

            if ((enterPressed || sendClicked) && strlen(messageInput) > 0) {
                if (g_connected) {
                    std::string msg(messageInput);
                    
                    if (sendToServer(msg)) {
                        messageInput[0] = '\0';
                        needFocus = true;  // 发送后重新设置焦点
                    } else {
                        std::lock_guard<std::mutex> lock(g_chatMutex);
                        g_chatHistory.push_back("[错误] 发送失败");
                    }
                } else {
                    std::lock_guard<std::mutex> lock(g_chatMutex);
                    g_chatHistory.push_back("[错误] 未连接到服务器");
                }
            }

            ImGui::End();
        }

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 清理资源
    if (g_connected && g_clientSocket != INVALID_SOCKET) 
    {
        closesocket(g_clientSocket);
        g_clientSocket = INVALID_SOCKET;
        g_connected = false;
    }
    WSACleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}