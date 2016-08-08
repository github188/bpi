#include "header.h"

// 各结构体的长度
unsigned int sizeof_iphdr = sizeof(struct ip);
unsigned int sizeof_tcphdr = sizeof(struct tcphdr);
unsigned int sizeof_ethhdr = sizeof(struct ethhdr);

unsigned char reinjec_mac[6] = {0};
short reinjec_mtu = 0;

static unsigned int no80bag = 0;

// 获取http头部信息
int get_http_head(char *data, char* domain, char* value)
{
	char *temp = strstr(data, "\r\n");
	
	while(temp != NULL && temp != data){
		if( !strncasecmp(data, domain, strlen(domain) ) ){
			data += strlen(domain);
			memcpy( value, data, temp - data );
			return 0;
		}
		
		data = temp + 2;
		temp = strstr(data, "\r\n");
	}
	return -1;
}

// http过滤 返回值0为可操作域名 其他为不可操作
int filter_http(char* data, char* domain, int data_len)
{
	// 判断是不是get请求
	if( strncmp(data, "GET", 3) ){
		return -1;
	}
	
	// 取出路径
	char *data_path = data + 4;
	if( *data_path != '/' ){
		//xyprintf(0, "There is an error in %s(%d) : Can not get path, *data_path = %c", __FILE__, __LINE__, *data_path);
		//xyprintf_data(data, data_len);
		return -1;
	}
	
	char *temp = strchr(data_path, ' ');
	char path[1024] = {0};
	if(temp - data_path > 1000){
		return -1;
	}
	memcpy(path, data_path, temp - data_path);

	// 取出host
	char host[128] = {0};
	if( get_http_head(data, "Host: ", host) ){ // host: 
		//xyprintf(0, "There is an error in %s(%d) : Can not find host!", __FILE__, __LINE__);
		//xyprintf_data(data, data_len);
		return -1;
	}

	// 组装url
	sprintf(domain, "%s%s", host, path);
	
	return 0;
}


// 数据过滤
void* filter_thread(void* lp)
{
	pthread_detach(pthread_self());
	
	// 取出数据
	unsigned char* packet = ((struct link_packet*)lp)->packet;
	unsigned int packet_len = ((struct link_packet*)lp)->packet_len;

	// 判断数据长度 如果包的长度小于 14 + 20 + 8
	if(packet_len < 42){
		xyprintf(0, "A bad packet of len is %d", packet_len);
		goto DATA_ERR;
	}
	
// ethhdr

	// 取ethhdr 检验是否是ip数据包
	struct ethhdr* ethhdr = (struct ethhdr*)packet;
	
	if( ntohs(ethhdr->h_proto) != ETH_P_IP ){
		xyprintf(0, "A packet of type is not IP!");
		goto DATA_ERR;
	}

// iphdr

	// 取iphdr 检验是否是tcp数据包
	struct ip* ip = (struct ip*)(packet + sizeof_ethhdr);
	
	if( ip->ip_p != IPPROTO_TCP ){
		//xyprintf(0, "A packet of type is not TCP!");
		goto DATA_ERR;
	}

	// 检查数据长度 去除 ethhdr长度应该和ip报文的长度一样
	if( packet_len - 14 != ntohs(ip->ip_len) ){
		//xyprintf(0, "A bad packet of len is %d, but ip_len is %u!", packet_len, ntohs(ip->ip_len));
		goto DATA_ERR;
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
		goto DATA_ERR;
	}
	src_check = tcp->check;
	check = check_sum_tcp(ip, tcp);
	if(check != src_check){
		xyprintf(0, "A bad packet of ip_sum is wrong, 0x%x -- 0x%x", check, src_check);
		goto DATA_ERR;
	}
*/
	
	// 判断数据类型 过滤出来不是传输数据的包
	if( tcp->ack != 1 || tcp->psh != 1 ){
		goto DATA_ERR;
	}
	
	// 判断端口
	if( 80 != ntohs(tcp->dest)){ // && 14096 != ntohs(tcp->source) ){
		no80bag ++;
		xyprintf(0, "no80bag = %u", no80bag);
		goto DATA_ERR;
	}

// data
	
	// 数据
	unsigned char *data = ((unsigned char*)tcp) + (unsigned int)(tcp->doff << 2);
	int data_len = packet_len - sizeof_ethhdr - (int)(ip->ip_hl << 2) - (int)(tcp->doff << 2);

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
	if( filter_http(data, domain, data_len) ){
		goto DATA_ERR;
	}

	char* str_para = NULL;
	int para_flag = 0;
	str_para = strrchr(domain, '?');
	if( str_para ){
		*str_para = 0;
		para_flag = 1;
	}
	if( strstr(domain, ".apk") ){
		apk_list_add(domain, para_flag);
	}

DATA_ERR:
	if(packet){
		free(packet);
	}
	if(lp){
		free(lp);
	}
	pthread_exit(NULL);
}
