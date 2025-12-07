// protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <winsock2.h> 

#pragma pack(1)  // 关闭字节对齐，确保结构体大小固定（网络传输必需！）


#define MAX_DATA_SIZE 1000

// 标志位定义
#define FLAG_SYN  1   // 建立连接
#define FLAG_ACK  2   // 确认
#define FLAG_FIN  4   // 关闭连接
#define FLAG_DATA 8   // 数据包

// 协议包结构
struct Packet {
    unsigned int seq;        // 发送序号（从 0 开始递增）
    unsigned int ack;        // 确认号（期望收到的下一个 seq）
    unsigned short checksum; // 校验和
    unsigned short len;      // 数据长度
    unsigned char flags;     // 标志位
    char data[MAX_DATA_SIZE]; // 实际数据
};

#pragma pack()

// 计算校验和
unsigned short calculate_checksum(const void* buf, int len);


void make_syn_packet(Packet* pkt, unsigned int seq);


void make_ack_packet(Packet* pkt, unsigned int ack_seq);


void make_fin_packet(Packet* pkt, unsigned int seq);


void make_data_packet(Packet* pkt, unsigned int seq, const char* payload, int size);

void show_packet(const Packet* pkt);

void send_packet(const SOCKET& sendSocket, const Packet& pkt, const sockaddr_in& recvAddr);

bool recv_packet_show(const SOCKET& recvSocket, Packet* pkt, sockaddr_in* clientAddr, int* clientAddrLen);
#endif // PROTOCOL_H