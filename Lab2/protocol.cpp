#include "protocol.h"
#include <iostream>
#include <cstring>

using namespace std;

// 打印包内容的辅助函数
void show_packet(const Packet* pkt) {
    cout << "[Packet] ";
    if (pkt->flags & FLAG_SYN) cout << "SYN ";
    if (pkt->flags & FLAG_ACK) cout << "ACK ";
    if (pkt->flags & FLAG_FIN) cout << "FIN ";
    if (pkt->flags & FLAG_DATA) cout << "DATA ";
    
    cout << "Seq=" << pkt->seq << " Ack=" << pkt->ack << " Checksum=" << pkt->checksum << endl;
    
    // 如果是数据包，打印一部分数据内容
    // if (pkt->flags & FLAG_DATA) {
    //     cout << "  Data: " << pkt->data << endl;
    // }
}

// 构造 SYN 包
void make_syn_packet(Packet* pkt, unsigned int seq) {
    memset(pkt, 0, sizeof(Packet));
    pkt->flags = FLAG_SYN;
    pkt->seq = seq;
    // 暂时不计算 checksum
}

// 构造 ACK 包
void make_ack_packet(Packet* pkt, unsigned int ack_seq) {
    memset(pkt, 0, sizeof(Packet));
    pkt->flags = FLAG_ACK;
    pkt->ack = ack_seq;
}

// 构造 FIN 包
void make_fin_packet(Packet* pkt, unsigned int seq) {
    memset(pkt, 0, sizeof(Packet));
    pkt->flags = FLAG_FIN;
    pkt->seq = seq;
}

// 构造 DATA 包
void make_data_packet(Packet* pkt, unsigned int seq, unsigned int ack,const char* payload, int size) {
    memset(pkt, 0, sizeof(Packet));
    pkt->flags = FLAG_DATA;
    pkt->seq = seq;
    pkt->ack = ack;
    if (size > MAX_DATA_SIZE) size = MAX_DATA_SIZE;
    pkt->len = size;
    memcpy(pkt->data, payload, size);
}

// 计算校验和
unsigned short calculate_checksum(const void* buf, int len) {
    const unsigned short* ptr = (const unsigned short*)buf;
    unsigned int sum = 0;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len > 0) {
        sum += *(const unsigned char*)ptr;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (unsigned short)~sum;
}

void send_packet(const SOCKET& sendSocket, const Packet& pkt, const sockaddr_in& recvAddr)
{
    Packet pkt_copy = pkt; 
    pkt_copy.checksum = 0; // 清零
    pkt_copy.checksum = calculate_checksum(&pkt_copy, sizeof(Packet)); // 计算

    int len = sendto(sendSocket, (const char*)&pkt_copy, sizeof(Packet), 0, (sockaddr*)&recvAddr, sizeof(recvAddr));
    if (len == SOCKET_ERROR) {
        cout << "发送失败" << endl;
    } else {
        cout << "发送成功: ";
        show_packet(&pkt_copy); 
    }
}

bool recv_packet_show(const SOCKET& recvSocket, Packet* pkt, sockaddr_in* clientAddr, int* clientAddrLen)
{
    int n = recvfrom(recvSocket, (char*)pkt, sizeof(Packet), 0, (sockaddr*)clientAddr, clientAddrLen);
    if (n <= 0) 
    {
        // 接收超时或出错
        cout << "接收失败或超时" << endl;
        return false;
    }
    
    // 校验和验证
    // 注意：接收端计算校验和时，包含头部原有的checksum。
    // 如果传输无误，计算结果应该为 0。
    if (calculate_checksum(pkt, sizeof(Packet)) != 0) {
        cout << "[Error] Checksum validation failed! Packet discarded." << endl;
        return false;
    }

    show_packet(pkt);
    return true;
}

