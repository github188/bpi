#include "header.h"

int reinjec_test(unsigned char* buf, int len, struct sockaddr_in* toaddr, unsigned int toaddr_len)
{
	// 网络层原始套接字
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if(sockfd <= 0){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		return -1;
	}

	// 设置网卡
	// TODO 网卡需要能智能获取
	struct ifreq if_mon_nic;
	strncpy(if_mon_nic.ifr_name, REINJEC_NIC, strlen(REINJEC_NIC) + 1);
	
	if( setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&if_mon_nic, sizeof(if_mon_nic))){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		goto ERR;
	}

	// 开启IP_HDRINGCL
	const int on = 1;
	if( setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0 ){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		goto ERR;
	}

	// 发送
	int ret = sendto(sockfd, buf, len, 0, (struct sockaddr*)toaddr, toaddr_len);
	if( ret <= 0){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		goto ERR;
	}

	//xyprintf(0, "--> --> --> ret = %d\n", ret);

	close(sockfd);
	return 0;
ERR:
	close(sockfd);
	return -1;
}

//在该函数中构造整个IP报文，最后调用sendto函数将报文发送出去
void send_rst_test(struct ip* s_ip, struct tcphdr* s_tcp, unsigned int data_len)
{
	//整个IP报文的长度
	int len = sizeof_iphdr + sizeof_tcphdr;
	char buf[128] = {0};
    
	//开始填充IP首部
    struct ip *ip = (struct ip*)buf;
	ip->ip_v = IPVERSION;				// 当前协议版本号 4
	ip->ip_hl = sizeof_iphdr >> 2;		// 首部占32bit字的数目 包括选项
	ip->ip_tos = s_ip->ip_tos;			// 服务类型 3bit优先权子字段 4bit Tos子字段 1bit未用位必须为0
	ip->ip_len = htons(len);			// ip数据包总长度
	ip->ip_id = 0;						// 
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;				// 生存时间
	ip->ip_p = IPPROTO_TCP;				// 协议类型
	ip->ip_src = s_ip->ip_src;			// 源地址
	ip->ip_dst = s_ip->ip_dst;			// 目的地址
	ip->ip_sum = check_sum_ip(ip);		// 校验和

	//开始填充TCP首部
	struct tcphdr *tcp = (struct tcphdr*)(buf + (((unsigned int)ip->ip_hl) << 2));
	tcp->source = s_tcp->source;
	tcp->dest = s_tcp->dest;
	tcp->seq = ntohl(ntohl(s_tcp->seq) + data_len);
	tcp->ack_seq = s_tcp->ack_seq;
	tcp->doff = sizeof_tcphdr >> 2;
	//tcp->fin = 1;
	//tcp->syn = 1;
	tcp->rst = 1;
	//tcp->psh = 1;
	//tcp->ack = 1;
	//tcp->urg = 1;
	//tcp->ece = 1;
	//tcp->cwr = 1;
	tcp->window = s_tcp->window;
	//tcp->urg_ptr = ;
	
	// 校验码
	tcp->check = check_sum_tcp(ip, tcp);
	
	// 地址
	struct sockaddr_in toaddr;
	toaddr.sin_family = AF_INET;
	toaddr.sin_port = tcp->source;
	toaddr.sin_addr = s_ip->ip_dst;
	
#if 0
	xyprintf(0, "-->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b");
	xyprintf(0, "sa_family = %u, sin_port = %d, sin_addr = %s",
			toaddr.sin_family, htons(toaddr.sin_port), inet_ntoa(toaddr.sin_addr) );
	xyprintf_iphdr(ip);
	xyprintf_tcphdr(tcp);
	xyprintf(0, "-->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e");
#endif

	reinjec_test(buf, len, &toaddr, sizeof(toaddr));
}

//在该函数中构造整个IP报文，最后调用sendto函数将报文发送出去
void send_test(struct ip* s_ip, struct tcphdr* s_tcp, unsigned int data_len, char* res_str)
{
	//整个IP报文的长度
	int len = sizeof_iphdr + sizeof_tcphdr + strlen(res_str);
	char buf[2048] = {0};
    
	//开始填充IP首部
    struct ip *ip = (struct ip*)buf;
	ip->ip_v = IPVERSION;				// 当前协议版本号 4
	ip->ip_hl = sizeof_iphdr >> 2;		// 首部占32bit字的数目 包括选项
	ip->ip_tos = s_ip->ip_tos;			// 服务类型 3bit优先权子字段 4bit Tos子字段 1bit未用位必须为0
	ip->ip_len = htons(len);			// ip数据包总长度
	ip->ip_id = 0;						// 
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;				// 生存时间
	ip->ip_p = IPPROTO_TCP;				// 协议类型
	ip->ip_src = s_ip->ip_dst;			// 源地址
	ip->ip_dst = s_ip->ip_src;			// 目的地址
	ip->ip_sum = check_sum_ip(ip);		// 校验和

	//开始填充TCP首部
	struct tcphdr *tcp = (struct tcphdr*)(buf + (((unsigned int)ip->ip_hl) << 2));
	tcp->source = s_tcp->dest;
	tcp->dest = s_tcp->source;
	// 序号是对方的确认序号
	tcp->seq = s_tcp->ack_seq;
	// 确认是序号是对方的序号加上此次发送的数据大小
	tcp->ack_seq = htonl( ntohl(s_tcp->seq) + data_len );
	tcp->doff = sizeof_tcphdr >> 2;
	//tcp->fin = 1;
	//tcp->syn = 1;
	//tcp->rst = 1;
	tcp->psh = 1;
	tcp->ack = 1;
	//tcp->urg = 1;
	//tcp->ece = 1;
	//tcp->cwr = 1;
	tcp->window = 33333;
	//tcp->urg_ptr = ;
	
	// 数据
	unsigned char *data = ((unsigned char*)tcp) + (unsigned int)(tcp->doff << 2);
	memcpy(data, res_str, strlen(res_str));
	// 校验码
	tcp->check = check_sum_tcp(ip, tcp);
	
	// 地址
	struct sockaddr_in toaddr;
	toaddr.sin_family = AF_INET;
	toaddr.sin_port = tcp->dest;
	toaddr.sin_addr = s_ip->ip_src;
	
#if 1
	xyprintf(0, "-->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b -->b");
	xyprintf(0, "sa_family = %u, sin_port = %d, sin_addr = %s",
			toaddr.sin_family, htons(toaddr.sin_port), inet_ntoa(toaddr.sin_addr) );
	xyprintf_iphdr(ip);
	xyprintf_tcphdr(tcp);
	xyprintf_data(data, strlen(res_str));
	xyprintf(0, "-->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e -->e");
#endif

	reinjec_test(buf, len, &toaddr, sizeof(toaddr));
}
