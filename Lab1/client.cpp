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
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

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

    string msg="hello server";
    send(clientSocket, msg.c_str(), msg.size(), 0);
    cout<<"你已经成功连接上服务器"<<endl;
    
    string buffer;
    string rcvBuf;
    while (true) 
    {
        //接受回显，不过目前的问题就是没接收到回显就不会继续循环发送消息，之后改多线程
        rcvBuf.resize(1024); 
        int bytesReceived = recv(clientSocket, &rcvBuf[0], rcvBuf.size(), 0); 
        if (bytesReceived <= 0) 
        {
            cerr << "接收失败或连接关闭" << endl;
            break;
        }
        rcvBuf.resize(bytesReceived); // 调整字符串大小为实际接收字节数
        cout << "服务器回显示: " << rcvBuf << endl;

        //发送消息
        cout << "请输入你要发送的消息（输入 /quit 退出）: ";
        getline(cin, buffer); 

        if (buffer == "/quit") {
            break;
        }

        // 发送消息，加上换行符
        buffer += "\n";
        send(clientSocket, buffer.c_str(), buffer.size(), 0);
         
        
    }

    //关闭socket
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}