/*****************************************************
 *
 * 头文件 宏
 *
 *****************************************************/
#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

#include <pthread.h>

#include <netdb.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>	// struct ethhdr
#include <net/if.h>			// struct ifreq
#include <netpacket/packet.h>	// struct sockaddr_ll
#include <linux/sockios.h>		// SIOCGIFFLAGS 

#include "list.h"

#define WRITE_LOG	0

// TODO 网卡需要在配置文件中读取
#define REINJEC_NIC		"p32p1" 

// 全局变量
extern unsigned int sizeof_iphdr;
extern unsigned int sizeof_tcphdr;
extern unsigned int sizeof_ethhdr;
extern unsigned char reinjec_mac[6];
extern short reinjec_mtu;

// log函数
int logs_init(char* prefix);
void logs_destroy();
int xyprintf(int err_no, char* format, ...);

// utils
unsigned short check_sum(unsigned short *addr,int len);				// 计算校验和
unsigned short check_sum_tcp(struct ip* ip, struct tcphdr* tcp);	// 计算tcp首部校验和
unsigned short check_sum_ip(struct ip* ip);							// 计算ip首部校验和

void xyprintf_tcphdr(struct tcphdr* tcp);
void xyprintf_iphdr(struct ip *ip);
void xyprintf_ethhdr(struct ethhdr* ethhdr);
void xyprintf_data(unsigned char* data, int len);

// send.c
void send_http(struct ip* s_ip, struct tcphdr* s_tcp, unsigned int data_len, char* res_str);
void send_rst_test(struct ip* s_ip, struct tcphdr* s_tcp, unsigned int data_len);



// filter.c
struct link_packet{
	unsigned char* packet;
	unsigned int packet_len;
};
void* filter_thread(void* lp);

// monitor.c
void monitor_thread();

// 

#endif

#if 0
#define ETH_ALEN 6  //定义了以太网接口的MAC地址的长度为6个字节
#define ETH_HLAN 14  //定义了以太网帧的头长度为14个字节
#define ETH_ZLEN 60  //定义了以太网帧的最小长度为 ETH_ZLEN + ETH_FCS_LEN = 64个字节
#define ETH_DATA_LEN 1500  //定义了以太网帧的最大负载为1500个字节
#define ETH_FRAME_LEN 1514  //定义了以太网正的最大长度为ETH_DATA_LEN + ETH_FCS_LEN = 1518个字节
#define ETH_FCS_LEN 4   //定义了以太网帧的CRC值占4个字节
struct ethhdr
{
unsigned char h_dest[ETH_ALEN]; //目的MAC地址
unsigned char h_source[ETH_ALEN]; //源MAC地址
__u16 h_proto ; //网络层所使用的协议类型
}__attribute__((packed))  //用于告诉编译器不要对这个结构体中的缝隙部分进行填充操作；


#define  ETH_P_IP 0x0800 //IP协议
#define  ETH_P_ARP 0x0806  //地址解析协议(Address Resolution Protocol)
#define  ETH_P_RARP 0x8035  //返向地址解析协议(Reverse Address Resolution Protocol)
#define  ETH_P_IPV6 0x86DD  //IPV6协议

#endif

#if 0
struct ip
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4;		/* header length */
    unsigned int ip_v:4;		/* version */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;		/* version */
    unsigned int ip_hl:4;		/* header length */
#endif
    u_int8_t ip_tos;			/* type of service */
    u_short ip_len;			/* total length */
    u_short ip_id;			/* identification */
    u_short ip_off;			/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
    u_int8_t ip_ttl;			/* time to live */
    u_int8_t ip_p;			/* protocol */
    u_short ip_sum;			/* checksum */
    struct in_addr ip_src, ip_dst;	/* source and dest address */
  };
#endif

#if 0
struct tcphdr {
	__be16	source;
	__be16	dest;
	__be32	seq;
	__be32	ack_seq;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u16	res1:4,
		doff:4,
		fin:1,
		syn:1,
		rst:1,
		psh:1,
		ack:1,
		urg:1,
		ece:1,
		cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u16	doff:4,
		res1:4,
		cwr:1,
		ece:1,
		urg:1,
		ack:1,
		psh:1,
		rst:1,
		syn:1,
		fin:1;
#else
#error	"Adjust your <asm/byteorder.h> defines"
#endif	
	__be16	window;
	__sum16	check;
	__be16	urg_ptr;
};
#endif
/*
struct sockaddr_ll
  {
    unsigned short int sll_family;
    unsigned short int sll_protocol;
    int sll_ifindex;
    unsigned short int sll_hatype;
    unsigned char sll_pkttype;
    unsigned char sll_halen;
    unsigned char sll_addr[8];
  };
  */
