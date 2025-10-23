#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
using namespace std;
#pragma comment(lib, "ws2_32.lib")


vector<SOCKET> clients; // 保存所有客户端（后续广播用）
unordered_map<SOCKET, string> clientNames;
unordered_map<string, SOCKET> nameToSocket;
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
            // 从列表中移除客户端
            {
                lock_guard<mutex> lock(clientsMutex);
                string name = clientNames[clientSocket];
                clientNames.erase(clientSocket);
                nameToSocket.erase(name);
                clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
                string goodbye = "[" + name + "] 离开了聊天室\n";
                for (SOCKET s : clients) 
                {
                    send(s, goodbye.c_str(), goodbye.size(), 0);
                }
            }
            break;
        }
        buf[n] = '\0';
        cout << "收到客户端消息: " <<"["<< clientNames[clientSocket] << "] " << buf << endl;

        string msg(buf);
        // 判断是否是私聊
        if (msg.length() >= 5 && msg.substr(0, 5) == "/msg ") 
        {
            
            string nickname = clientNames[clientSocket];
            size_t firstSpace = msg.find(' ', 5); 
            string sendToMyself=nickname+":[私聊] TO"+msg.substr(5, firstSpace - 5)+":"+msg.substr(firstSpace + 1);
            send(clientSocket, sendToMyself.c_str(), sendToMyself.size(), 0);
            if (firstSpace != string::npos) 
            {
                string target = msg.substr(5, firstSpace - 5);
                string content = msg.substr(firstSpace + 1);
                string privateMsg = "[私聊] " + nickname + ": " + content;
                {
                    lock_guard<mutex> lock(clientsMutex);
                    if (nameToSocket.count(target)) {
                        send(nameToSocket[target], privateMsg.c_str(), privateMsg.size(), 0);
                    } else {
                        string err = "用户 [" + target + "] 不在线\n";
                        send(clientSocket, err.c_str(), err.size(), 0);
                    }
                }
            }
        }
        else
        {
            string nickname = clientNames[clientSocket];
            string broadcastMsg = "[" + nickname + "] " + msg;
            {
                lock_guard<mutex> lock(clientsMutex);
                for (SOCKET s : clients) 
                {
                    send(s, broadcastMsg.c_str(), broadcastMsg.size(), 0);
                }
            }
       }
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
    addr.sin_port = htons(1023);          // 端口 1023
    addr.sin_addr.s_addr = INADDR_ANY;    // 监听所有网卡
    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
    {
        cout << "绑定错误" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    cout<<"服务器创建成功,已在端口1023监听"<<endl;

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

        // 读取昵称
        char buf[1024];
        int n = recv(clientSocket, buf, sizeof(buf)-1, 0);
        if (n <= 0) 
        {
            cout << "客户端未发送昵称就断开" << endl;
            break;
        }
        buf[n] = '\0';
        string nickname(buf);
        if (!nickname.empty() && nickname.back() == '\n') {
            nickname.pop_back();
        }

        // 保存昵称映射
        {
            lock_guard<mutex> lock(clientsMutex);
            clientNames[clientSocket] = nickname;
            nameToSocket[nickname] = clientSocket;
            clients.push_back(clientSocket); 
            string welcome = "[" + nickname + "] 加入了聊天室\n";
            string clientList = "当前在线用户: "+nickname;
            for (SOCKET s:clients)
            {
                if(s!=clientSocket)
                {
                    clientList +=","+clientNames[s];
                }
            }
            for (SOCKET s : clients) 
            {
                send(s, welcome.c_str(), welcome.size(), 0);
                send(s, clientList.c_str(), clientList.size(), 0);
            }
        }

        
        // 启动线程处理该客户端
        thread(handleClient, clientSocket).detach();
    }

    // 关闭连接
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}