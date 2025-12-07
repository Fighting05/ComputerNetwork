#include<iostream>
#include<fstream>
#include<string>
#include<winsock2.h>
#include "protocol.h"
using namespace std;

unsigned int SEND_ISN = 1000;

void send_fin_packet(SOCKET sendSocket, sockaddr_in recvAddr)
{
    Packet fin;
    make_fin_packet(&fin, SEND_ISN);
    bool finAck=false;
    int time=0;
    while(!finAck&&time<=5)
    {
        send_packet(sendSocket, fin, recvAddr);
        cout<<"FIN发送成功"<<endl;
        time++;
        // 等待 ACK
        Packet ackPkt;
        memset(&ackPkt, 0, sizeof(ackPkt));
        int clientAddrLen = sizeof(recvAddr);
        if (recv_packet_show(sendSocket, &ackPkt, &recvAddr, &clientAddrLen)) 
        {
            if (ackPkt.flags & FLAG_ACK && ackPkt.ack == fin.seq + 1) 
            {
                finAck = true;
                cout<<"FIN_ACK接收成功"<<endl;
            }
        }
    }
}

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
    SEND_ISN=ack.ack;
    ack.flags = FLAG_ACK;
    //发送第三次握手
    send_packet(sendSocket, ack, recvAddr);
    cout<<"ACK第三次握手发送成功"<<endl;

    //尝试发送一个txt
    string filename="D:\\大三上\\计算机网络\\实验\\Lab2\\lab2测试环境\\测试文件\\helloworld.txt";
    ifstream file(filename,ios::binary|ios::ate);
    if(!file.is_open())
    {
        cout<<"文件打开失败"<<endl;
        return 0;
    }
    streamsize fileSize=file.tellg();
    cout<<filename<<"文件打开成功，大小为:"<<fileSize<<"字节"<<endl;

    //文件指针移回开头
    file.seekg(0, ios::beg);

    char buffer[MAX_DATA_SIZE];
    while(!file.eof())
    {
        file.read(buffer, MAX_DATA_SIZE);
        int realsize=file.gcount();
        if(realsize<=0)
        {
            //读完了
            file.close();
            break;
        }
        Packet pkt;
        make_data_packet(&pkt, SEND_ISN++, buffer, realsize);
        cout<<"读取了"<<realsize<<"字节,准备发送"<<endl;
        send_packet(sendSocket, pkt, recvAddr);
        while (!recv_packet_show(sendSocket, &ack, &recvAddr, &clientAddrLen)) 
        {
            cout << "接收超时或校验错误，重传..." << endl;
            send_packet(sendSocket, pkt, recvAddr);
        }
    }

    send_fin_packet(sendSocket, recvAddr);

    //关闭链接
    closesocket(sendSocket);
    WSACleanup();
    system("pause");
    return 0;
}