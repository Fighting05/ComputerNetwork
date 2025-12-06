#include<iostream>
#include<winsock2.h>
#include "protocol.h"
using namespace std;

unsigned int SEND_ISN = 1000;

int main() 
{
    //1.初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cout<<"WSAStartup failed!"<<endl;
        return 0;
    }
    //2.创建一个socket
    SOCKET sendSocket=socket(AF_INET, SOCK_DGRAM, 0); // 改为 UDP (SOCK_DGRAM)
    if (sendSocket == INVALID_SOCKET) 
    {
        cout<<"sendSocket init failed!"<<endl;
        return 0;
    }
    // 设置接收超时为 3000 毫秒（3秒）
    DWORD timeout = 3000;
    setsockopt(sendSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // 目标地址 (Receiver 的地址)
    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(1234); // 目标端口
    recvAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 目标IP

    cout<<"UDP Socket 创建成功，准备发送数据..."<<endl;

    cout<<"准备三次握手..."<<endl;
    //准备三次握手
    Packet syn;
    memset(&syn, 0, sizeof(syn));
    syn.seq = ++SEND_ISN;
    syn.ack=0;
    syn.flags = FLAG_SYN;

    //第一次握手
    send_packet(sendSocket, syn, recvAddr);
    cout<<"SYN第一次握手发送成功"<<endl;

    //接受第二次握手
    Packet syn_ack;
    memset(&syn_ack, 0, sizeof(syn_ack));
    int clientAddrLen = sizeof(recvAddr);
    
    while (!recv_packet_show(sendSocket, &syn_ack, &recvAddr, &clientAddrLen)) {
        cout << "接收超时或校验错误，重传第一次握手 SYN..." << endl;
        send_packet(sendSocket, syn, recvAddr);
    }
    cout<<"SYN_ACK第二次握手接收成功"<<endl;

    //第三次握手
    Packet ack;
    memset(&ack, 0, sizeof(ack));
    ack.seq = 0;
    ack.ack=syn_ack.seq+1;
    ack.flags = FLAG_ACK;
    //发送第三次握手
    send_packet(sendSocket, ack, recvAddr);
    cout<<"ACK第三次握手发送成功"<<endl;

    //关闭链接
    closesocket(sendSocket);
    WSACleanup();
    system("pause");
    return 0;
}