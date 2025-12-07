#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<winsock2.h>
#include "protocol.h"
using namespace std;

struct sendPacket
{
    Packet pkt;
    bool acked=false;
    clock_t sentTime;
};


unsigned int SEND_ISN = 1000;

void init_window(vector<sendPacket>& sendWindow, vector<Packet>& all_packets)
{
    for(int i=0;i<all_packets.size();i++)
    {
        sendWindow[i].pkt=all_packets[i];
        sendWindow[i].acked=false;
        sendWindow[i].sentTime=0;
    }
}

void send_fin_packet(SOCKET sendSocket, sockaddr_in recvAddr,int next_ack)
{
    Packet fin;
    make_fin_packet(&fin, ++SEND_ISN);
    fin.ack=next_ack;
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

void get_pkt_from_flie(string filename,vector<Packet>& all_packets,int ack_start)
{
     ifstream file(filename,ios::binary|ios::ate);
    if(!file.is_open())
    {
        cout<<"文件打开失败"<<endl;
        return;
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
        make_data_packet(&pkt, ++SEND_ISN,ack_start, buffer, realsize);
        all_packets.push_back(pkt);
    }
    file.close();
    cout<<filename<<"文件分包完成，共分为"<<all_packets.size()<<"个数据包"<<endl;

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
    // 设置接收超时为3000毫秒（3秒）
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
    ack.seq = syn.seq+1;
    ack.ack=syn_ack.seq+1;
    SEND_ISN=ack.seq;
    ack.flags = FLAG_ACK;
    //发送第三次握手
    send_packet(sendSocket, ack, recvAddr);
    cout<<"ACK第三次握手发送成功"<<endl;

    timeout=1;
    setsockopt(sendSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    //尝试发送一个txt
    string filename1="D:\\大三上\\计算机网络\\实验\\Lab2\\lab2测试环境\\测试文件\\helloworld.txt";
    string filename2="D:\\大三上\\计算机网络\\实验\\Lab2\\lab2测试环境\\测试文件\\1.jpg";
    string filename3="D:\\大三上\\计算机网络\\实验\\Lab2\\lab2测试环境\\测试文件\\2.jpg";
    string filename4="D:\\大三上\\计算机网络\\实验\\Lab2\\lab2测试环境\\测试文件\\3.jpg";
    cout<<"选择你准备发送的："<<endl<<"1.:"<<filename1<<endl<<"2.:"<<filename2<<endl<<"3.:"<<filename3<<endl<<"4.:"<<filename4<<endl<<"请输入你的选择：";
    int choice;
    cin>>choice;
    string filename;
    switch(choice)
    {
        case 1:
            filename=filename1;
            break;
        case 2:
            filename=filename2;
            break;
        case 3:
            filename=filename3;
            break;
        case 4:
            filename=filename4;
            break;
        default:
            cout<<"选择错误"<<endl;
            return 0;
    }


    vector<Packet> all_packets;
    get_pkt_from_flie(filename,all_packets,ack.ack);
    int windowSize=5;
    int base=0;
    int next_ack=ack.ack;
    int next_seq=0;
    vector<sendPacket> sendWindow(all_packets.size());
    init_window(sendWindow, all_packets);
    while(base<all_packets.size())
    {
        //发送窗口内的包
        while(next_seq<base+windowSize&&next_seq<all_packets.size())
        {
            sendWindow[next_seq].pkt.ack=next_ack;
            send_packet(sendSocket, sendWindow[next_seq].pkt, recvAddr);
            sendWindow[next_seq].sentTime=clock();
            next_seq++;
        }
        //接收ACK
        Packet ackPkt;
        memset(&ackPkt, 0, sizeof(ackPkt));
        if (recv_packet_show(sendSocket, &ackPkt, &recvAddr, &clientAddrLen)) 
        {
            if (ackPkt.flags & FLAG_ACK) 
            {
                for(int i=base;i<next_seq;i++)
                {
                    if(ackPkt.ack==sendWindow[i].pkt.seq+1)
                    {
                        sendWindow[i].acked=true;
                        next_ack++;
                        break;
                    }
                }
            }
        }
        //滑动窗口
        while(base<next_seq&&sendWindow[base].acked)
        {
            base++;
        }
        for(int i=base;i<next_seq;i++)
        {
            if(!sendWindow[i].acked)
            {
                if(clock()-sendWindow[i].sentTime>=1000)
                {
                    sendWindow[i].pkt.ack=next_ack;
                    send_packet(sendSocket, sendWindow[i].pkt, recvAddr);
                    sendWindow[i].sentTime=clock();
                }
            }
        }
    }

    send_fin_packet(sendSocket, recvAddr,next_ack);

    //关闭链接
    closesocket(sendSocket);
    WSACleanup();
    system("pause");
    return 0;
}