#include <iostream>
using namespace std;
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

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

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);//阻塞，等待客户端连接
    if (clientSocket == INVALID_SOCKET) 
    {
        cout << "接受连接错误" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else
    {
        cout << "接受连接成功" << endl;
    }

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

    // 关闭连接
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}