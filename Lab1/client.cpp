#include <iostream>
using namespace std;
#include <winsock2.h>
#include <thread>
#include <algorithm>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")



//ai给写的
string gbkToUtf8(const std::string& gbkStr) {
    // 1. 将 GBK 字符串转换为宽字符 (UTF-16)
    int wideCharCount = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, NULL, 0);
    if (wideCharCount == 0) return {}; 

    std::wstring wideStr(wideCharCount - 1, 0); // -1 是为了去掉 null terminator
    MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, &wideStr[0], wideCharCount);

    // 2. 将宽字符 (UTF-16) 转换为 UTF-8 字符串
    int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8ByteCount == 0) return {}; // 转换失败

    std::string utf8Str(utf8ByteCount - 1, 0); // -1 是为了去掉 null terminator
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8ByteCount, NULL, NULL);

    return utf8Str;
}

//接收服务器回显
void rcvFromServer(SOCKET clientSocket)
{
    while(true)
    {
        string rcvBuf(1024, '\0');
            int bytesReceived = recv(clientSocket, &rcvBuf[0], rcvBuf.size(), 0); 
            if (bytesReceived <= 0) 
            {
                cout << "接收失败或连接关闭" << endl;
                return;
            }
            rcvBuf.resize(bytesReceived); // 调整字符串大小为实际接收字节数
            cout << rcvBuf << endl; 
    }
}

//发送消息
void sendServer(SOCKET clientSocket, const string& nickname)
{
    while(true)
    {
        string buffer;
        //发送消息
        cout << "请输入你要发送的消息（输入 /quit 退出）: "<<endl;
        getline(cin, buffer); 

        if (buffer == "/quit") 
        {
            cout << "你已经退出聊天" << endl;
            string goodbye = gbkToUtf8(nickname + " 离开了聊天室\n");
            send(clientSocket, goodbye.c_str(), goodbye.size(), 0);
            closesocket(clientSocket); // 关闭 socket
            return;
        }

        // 发送消息，加上换行符
        string utf8Buffer = gbkToUtf8(nickname + ": " + buffer);
        if (utf8Buffer.empty()) {
            cerr << "编码转换失败，跳过发送。" << endl;
            continue; // 跳过此次发送
        }
        utf8Buffer += "\n";
        send(clientSocket, utf8Buffer.c_str(), utf8Buffer.size(), 0);
    }
}


int main()
{

    SetConsoleOutputCP(CP_UTF8);
    
    //初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cerr << "WSAStartup failed!" << endl;
        return 1;
    }

    //创建一个socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) 
    {
        cout << "创建socket错误" << endl;
        WSACleanup();
        return 0;
    }

    //设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2059);
    serverAddr.sin_addr.s_addr = inet_addr("60.205.14.222");//改为我的服务器地址


    //连接服务器
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        cout << "连接服务器错误" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "连接服务器成功" << endl;
    }

    cout << "你已经成功连接上服务器，请输入你的昵称: ";
    string nickname;
    getline(cin, nickname); 

    string welcome = gbkToUtf8(nickname + " 加入了聊天室\n");
    send(clientSocket, welcome.c_str(), welcome.size(), 0);
    
    string buffer;

    thread(rcvFromServer, clientSocket).detach();//接受
    thread(sendServer, clientSocket,nickname).detach();//发送

    while (true) 
    {
        this_thread::sleep_for(chrono::seconds(1));
    }

    WSACleanup();
    return 0;
}

// g++ -std=c++11 client.cpp -o client.exe -lws2_32 -static -static-libgcc -static-libstdc++