#include<iostream>
#include<fstream>
#include<ostream>
#include<string>
#include<map>
#include<winsock2.h>
#include "protocol.h"
using namespace std;

unsigned int RECEIVE_ISN=5000;

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
    SOCKET recvSocket=socket(AF_INET, SOCK_DGRAM, 0); // 改为 UDP (SOCK_DGRAM)
    if (recvSocket == INVALID_SOCKET) 
    {
        cout<<"recvSocket init failed!"<<endl;
        return 0;
    }

    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(1234); //端口号“1234”
    recvAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    
    // 绑定端口 (UDP Server 必须做这一步)
    if (bind(recvSocket, (sockaddr*)&recvAddr, sizeof(recvAddr)) == SOCKET_ERROR) 
    {
        cout<<"recvSocket bind failed!"<<endl;
        closesocket(recvSocket);
        WSACleanup();
        return 0;
    }
    // 设置接收超时为 3000 毫秒（3秒）
    DWORD timeout = 3000;
    setsockopt(recvSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    cout<<"UDP Receiver 已启动，正在监听端口 1234..."<<endl;

    Packet syn;
    memset(&syn, 0, sizeof(syn));
    sockaddr_in clientAddr; // 用于保存发送者的地址
    int clientAddrLen = sizeof(clientAddr);

    //接受第一次握手
    while (!recv_packet_show(recvSocket, &syn, &clientAddr, &clientAddrLen)) {
        cout << "等待第一次握手 SYN..." << endl; 
    }
    cout<<"SYN第一次握手接收成功"<<endl;

    //第二次握手
    Packet syn_ack;
    memset(&syn_ack, 0, sizeof(syn_ack));
    syn_ack.seq = ++RECEIVE_ISN;
    syn_ack.ack=syn.seq+1;
    syn_ack.flags = FLAG_SYN+FLAG_ACK;

    //发送第二次握手
    send_packet(recvSocket, syn_ack, clientAddr);
    cout<<"SYN_ACK第二次握手发送成功"<<endl;

    //接受第三次握手
    Packet ack;
    memset(&ack, 0, sizeof(ack));
    while (!recv_packet_show(recvSocket, &ack, &clientAddr, &clientAddrLen)) {
        // 可能超时，或者校验和错误
    }
    cout<<"ACK第三次握手接收成功"<<endl;

    //准备接受文件
    cout<<"连接建立成功，准备接受数据..."<<endl;
    ofstream file("received_file.txt", ios::binary);
    if (!file.is_open()) 
    {
        cout << "文件打开失败！" << endl;
        closesocket(recvSocket);
        WSACleanup();
        return 0;
    }

    map<int,Packet> recv_buffPacket;
    int expect_seq=ack.seq+1;
    bool isFirstData=true;
    while(1)
    {
        Packet recvPkt;
        if (recv_packet_show(recvSocket, &recvPkt, &clientAddr, &clientAddrLen)) 
        {
            if (recvPkt.flags & FLAG_DATA) 
            {
                if(recvPkt.seq==expect_seq)
                {
                    file.write(recvPkt.data, recvPkt.len);
                    expect_seq++;
                    while(recv_buffPacket.count(expect_seq))
                    {
                        file.write(recv_buffPacket[expect_seq].data, recv_buffPacket[expect_seq].len);
                        expect_seq++;
                        recv_buffPacket.erase(expect_seq-1);
                    }
                    Packet ackPkt;
                    make_ack_packet(&ackPkt, expect_seq);
                    ackPkt.seq=++RECEIVE_ISN;
                    send_packet(recvSocket, ackPkt, clientAddr);
                }
                else if(recvPkt.seq>expect_seq)
                {
                    recv_buffPacket[recvPkt.seq]=recvPkt;
                    Packet ackPkt;
                    make_ack_packet(&ackPkt, recvPkt.seq+1);
                    ackPkt.seq=++RECEIVE_ISN;
                    send_packet(recvSocket, ackPkt, clientAddr);
                }
                else
                {
                    Packet ackPkt;
                    make_ack_packet(&ackPkt, recvPkt.seq+1);
                    ackPkt.seq=++RECEIVE_ISN;
                    send_packet(recvSocket, ackPkt, clientAddr);
                }
            }
            else if(recvPkt.flags & FLAG_FIN)
            {
                cout << "收到 FIN 包，准备关闭连接..." << endl;
                // 回复 ACK
                Packet ackPkt;
                make_ack_packet(&ackPkt, recvPkt.seq+1);
                ackPkt.seq=++RECEIVE_ISN;
                send_packet(recvSocket, ackPkt, clientAddr);
                break;
            }
                
        }
        else
        {

        }
    }
    
    
    file.close();

    system("pause");
    return 0;
}