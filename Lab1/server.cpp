#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

vector<SOCKET> clients; // 保存所有客户端（后续广播用）
mutex clientsMutex;     // 保护列表（线程安全）

//多线程处理客户端
void handleClient(SOCKET clientSocket) 
{
    char buf[1024];
    while (true) 
    {
        int n = recv(clientSocket, buf, sizeof(buf)-1, 0);
        if (n <= 0) 
        {
            cout << "客户端断开连接" << endl;
            break;
        }
        buf[n] = '\0';
        cout << "收到客户端消息: " << buf << endl;
        send(clientSocket, buf, n, 0); // 回显
    }
}



int main()
{
    //初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cerr << "WSAStartup failed!" << endl;
        return 1;
    }

    //创建一个socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == INVALID_SOCKET)
    {
        cout << "创建socket错误" << endl;
        WSACleanup();
        return 0;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2059);          // 端口 2059
    addr.sin_addr.s_addr = INADDR_ANY;    // 监听所有网卡
    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
    {
        cout << "绑定错误" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    cout<<"服务器创建成功,已在端口2059监听"<<endl;

    if (listen(serverSocket, 5) == SOCKET_ERROR) 
    {
        cout << "监听错误" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }  // 开始监听，最多排队 5 个连接

    while (true) 
    {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) 
        {
            cout << "新客户端接受连接失败\n";
            continue;
        }

        cout << "新客户端连接成功\n";

        // 启动线程处理该客户端
        thread(handleClient, clientSocket).detach();
    }

    // 关闭连接
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}