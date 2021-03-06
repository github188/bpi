#include "header.h"

// 计算校验和
unsigned short check_sum(unsigned short *addr,int len)
{
	unsigned long cksum = 0;
	while( len > 1 ){
		cksum += *addr++;
		len -= sizeof(unsigned short);
	}

	if(len){
		cksum += *(unsigned char *)addr;
	}

	//将32位数转换成16
	while ( cksum >> 16 )
		cksum = ( cksum >> 16 ) + (cksum & 0xffff);
	
	return (unsigned short)(~cksum);
}

// 计算tcp首部校验和
unsigned short check_sum_tcp(struct ip* ip, struct tcphdr* tcp)
{
	// 计算tcp数据包长度
	unsigned short tcp_len = ntohs(ip->ip_len) - (ip->ip_hl << 2);
	//xyprintf(0, "tcp_len = %u", tcp_len);
	
	// 申请临时空间
	char *buf = malloc(tcp_len + 128);
	memset(buf, 0, tcp_len + 128);
	
	// 校验和值置0
	tcp->check = 0;

	// 临时空间已使用的长度
	int lenght = 0;
	
	// 32位源ip地址
	memcpy(buf, &(ip->ip_src), 4);
	lenght += sizeof(ip->ip_src);

	// 32位目的IP地址
	memcpy(buf + lenght, &(ip->ip_dst), 4);
	lenght += sizeof(ip->ip_dst);
	
	// 0
	lenght += 1;
	
	// 8位协议
	*(buf + lenght) = ip->ip_p;
	lenght += sizeof(ip->ip_p);
	
	// 16位tcp总长度
	*((unsigned short*)(buf + lenght)) = htons(tcp_len);
	lenght += sizeof(tcp_len);
	
	if(tcp_len + lenght > 1492){
		tcp_len = 1492 - lenght;
	}

	// TCP数据包
	memcpy(buf + lenght, tcp, tcp_len);
	lenght += tcp_len;
	
	// 如果是奇数补1
	if(lenght % 2){
		lenght++;
	}	
	
	return check_sum((unsigned short*)buf, lenght);
}

// 计算ip首部校验和
unsigned short check_sum_ip(struct ip* ip)
{
	// 校验和值置0
	ip->ip_sum = 0;

	return check_sum((unsigned short*)ip, ip->ip_hl << 2);
}

// 打印tcp头部信息
void xyprintf_tcphdr(struct tcphdr* tcp)
{
	xyprintf(0, "16位源端口 source = %u\n\
			16位目的端口 dest = %u\n\
			32位序号 seq = 0x%u\n\
			32位确认序号 ack_seq = 0x%u\n\
			4位头部长度 doff = %u\n\
			保留 res1 = %u\n\
			->cwr = %u\n\
			->ece = %u\n\
			->urg = %u\n\
			->ack = %u\n\
			->psh = %u\n\
			->rst = %u\n\
			->syn = %u\n\
			->fin = %u\n\
			16位窗口大小 window = %u\n\
			16位校验和 check = 0x%x\n\
			16位紧急指针 urg_ptr = %u",
			ntohs(tcp->source),
			ntohs(tcp->dest),
			ntohl(tcp->seq),
			ntohl(tcp->ack_seq),
			tcp->doff << 2,
			tcp->res1,
			tcp->cwr,
			tcp->ece,
			tcp->urg,
			tcp->ack,
			tcp->psh,
			tcp->rst,
			tcp->syn,
			tcp->fin,
			ntohs(tcp->window),
			tcp->check,
			ntohs(tcp->urg_ptr));

	int options_len = (tcp->doff << 2) - (int)sizeof(struct tcphdr);
	unsigned char *options = (char*)(tcp + 1);
	while(options_len > 0){
		switch(*options){
			case 0:
				xyprintf(0, "%u:EOL -- 选项列表结束(长度1)", options[0]);
				options++;
				options_len--;
				break;
			case 1:
				xyprintf(0, "%u:NOP -- 无操作(补位填充,长度1)", options[0]);
				options++;
				options_len--;
				break;
			case 2:
				xyprintf(0, "%u:MSS -- 最大Segment长度(长度:%u) -- 0x%0x 0x%0x",
						options[0], options[1],
						options[2], options[3]);
				options_len -= 4;
				options += 4;
				break;
			case 3:
				xyprintf(0, "%u:WSOPT -- 窗口扩大系数(长度:%u) -- 0x%0x",
						options[0], options[1], options[2]);
				options_len -= 3;
				options += 3;
				break;
			case 4:
				xyprintf(0, "%u:SACK-Premitted -- 表明支持SACK(长度:%u)",
						options[0], options[1]);
				options_len -= 2;
				options += 2;
				break;
			case 5:
				xyprintf(0, "%u:SACK -- 收到乱序数据(长度:%u)",
						options[0], options[1]);
				int i = 0;
				for(; i < (options[1] - 2 ) / 4; i++){
					xyprintf(0, "\t%d: %u",
							i, ntohl(*((unsigned int*)options + 2 + i * 4)));
				}
				options_len -= options[1];
				options += options[1];
				break;
			case 8:
				xyprintf(0, "%u:TSPOT -- Timestamps(长度:%u) -- 0x%x, 0x%x",
						options[0], options[1],
						ntohl(*((unsigned int*)(options + 2))),
						ntohl(*((unsigned int*)(options + 6))));
				options_len -= 10;
				options += 10;
				break;
			case 19:
				xyprintf(0, "%u:TCP-MD5 -- MD5认证(长度:%u) -- %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
						options[0], options[1],
						options[2], options[3], options[4], options[5],
						options[6], options[7], options[8], options[9],
						options[10], options[11], options[12], options[13],
						options[14], options[15], options[16], options[17]);
				options_len -= 18;
				options += 18;
				break;
			case 28:
				xyprintf(0, "%u:UTO -- User Timeout(长度:%u) -- %u",
						options[0], options[1],
						ntohs(*((unsigned short*)(options + 2))));
				options_len -= 4;
				options += 4;
				break;
			case 29:
				xyprintf(0, "%u:TAP-AO -- 认证(长度:%u)",
						options[0], options[1]);
				options_len -= options[1];
				options += options[1];
				break;
			case 253:
			case 254:
				xyprintf(0, "%u:Experimental -- 保留，用于科研实验(长度:%u)",
						options[0], options[1]);
				options_len -= options[1];
				options += options[1];
				break;
			default:
				xyprintf(0, "%u:未知 -- 赶快去查资料(长度:%u)",
						options[0], options[1]);
				options_len -= options[1];
				options += options[1];
				break;
		}
	}
}

// 打印ip首部数据
void xyprintf_iphdr(struct ip *ip)
{

	char ip_src_str[32] = {0};
	strcpy(ip_src_str, inet_ntoa(ip->ip_src));
	char ip_dst_str[32] = {0};
	strcpy(ip_dst_str, inet_ntoa(ip->ip_dst));

	xyprintf(0, "4位版本号 ip_v = %u\n\
			4位IP头部长度 ip_hl = %u\n\
			8位服务类型 ip_tos = %u\n\
			16位数据包总长度 ip_len = %u\n\
			16位表示符 ip_id = %u\n\
			3位标志 13位片偏移 ip_off = 0x%x\n\
			8位生存时间 ip_ttl = %u\n\
			8位协议号 ip_p = %u\n\
			16位首部校验和 ip_sum = 0x%x\n\
			32位源地址 ip_src = 0x%x - %s\n\
			32位目的地址 ip_dst = 0x%x - %s",
			ip->ip_v,
			ip->ip_hl << 2,
			ip->ip_tos,
			ntohs(ip->ip_len),
			ntohs(ip->ip_id),
			ntohs(ip->ip_off),
			ip->ip_ttl,
			ip->ip_p,
			ip->ip_sum,
			ntohl(ip->ip_src.s_addr), ip_src_str, 
			ntohl(ip->ip_dst.s_addr), ip_dst_str);
	
	if( (ip->ip_hl << 2) > sizeof_iphdr ){
		xyprintf(0, "ip options %u - %ld = %d ",
				ip->ip_hl << 2, sizeof_iphdr, (ip->ip_hl << 2) - sizeof_iphdr);
	}
}

// 打印eth头部
void xyprintf_ethhdr(struct ethhdr* ethhdr)
{
	unsigned char* h_dest = ethhdr->h_dest;
	unsigned char* h_source = ethhdr->h_source;
	xyprintf(0, "目的地址 h_dest = %02x:%02x:%02x:%02x:%02x:%02x\n\
			源地址 h_source = %02x:%02x:%02x:%02x:%02x:%02x\n\
			协议类型 h_proto = 0x%04x",
			h_dest[0], h_dest[1], h_dest[2], h_dest[3], h_dest[4], h_dest[5], 
			h_source[0], h_source[1], h_source[2], h_source[3], h_source[4], h_source[5], 
			ntohs(ethhdr->h_proto));
}

// 打印sockaddr_ll
void xyprintf_sockaddr_ll(struct sockaddr_ll* ll)
{
	xyprintf(0, "->sll_family = %u, AF_PACKET = %d\n\
			->sll_protocol = %u\n\
			->sll_ifindex = %d\n\
			->sll_hatype = %u\n\
			->sll_pkttype = %u\n\
			->sll_halen = %u\n\
			->sll_addr = %02x:%02x:%02x:%02x:%02x:%02x",
			ll->sll_family, AF_PACKET,
			ll->sll_protocol,
			ll->sll_ifindex,
			ll->sll_hatype,
			ll->sll_pkttype,
			ll->sll_halen,
			ll->sll_addr[0], ll->sll_addr[1], ll->sll_addr[2], 
			ll->sll_addr[3], ll->sll_addr[4], ll->sll_addr[5]);
}


// 打印数据包
void xyprintf_data(unsigned char* data, int len)
{
	if(len < 1){
		return;
	}

	xyprintf(0, "数据部分(%d)：", len);

	int line_of_num = 32;

	char x_buf[256] = {0};
	char s_buf[64] = {0};
	
	int i = 0;
	int count;
	for(; i < len; i++){
		count = i % line_of_num;
		sprintf(x_buf + count * 3, "%02x ", data[i]);
		sprintf(s_buf + count, "%c", data[i]);
		
		if(count == line_of_num - 1){
			xyprintf(0, "%s\t%s", x_buf, s_buf);
			memset(x_buf, 0, 256);
			memset(s_buf, 0, 64);
		}
	}
	
	if(count != line_of_num - 1){
		xyprintf(0, "%s\t%s", x_buf, s_buf);
	}
}

// 从/proc/net/dev中读取网卡列表
// 这也可以使用SIOCGIFCONF获取，但获取到的只能是有ip的网卡
// 如果不需要获取mac地址，可以使用getifaddrs函数获取

// 初始化网卡
int init_nic(char *reinjec_nic)
{
	// 打开网卡列表文件
	FILE* file = fopen("/proc/net/dev", "r");
	if(!file){
		xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
		goto FILEED_ERROR;
	}

	// 创建一个socket套接字
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sockfd < 0){
		xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
		goto SOCKETED_ERROR;
	}

	// 读取文件缓冲区
	char buf[1024] = {0};
	char* temp;
	
	struct ifreq ifr;

	// 逐行读取
	while( fgets(buf, 1023, file) ){
		// 根据冒号判断此行是否有网卡名称
		temp = strchr(buf, ':');
		if(temp){
			// 将冒号改成结束符
			*temp = 0;
			temp = buf;

			// 过滤掉前面的空格
			while( *temp == ' ' ){
				temp++;
			}

			// 名字
			memset(&ifr, 0, sizeof(ifr));
			strcpy(ifr.ifr_name, temp);

			// 如果是回注口
			if( !strcmp(temp, reinjec_nic) ){
				// 获取mac地址 存放到全局变量reinjec_mac中
				if ( ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1){
					xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
					goto SOCKETED_ERROR;
				}
				memcpy(reinjec_mac, ifr.ifr_hwaddr.sa_data, 6);
				
				// 获取mtu
				if( ioctl(sockfd, SIOCGIFMTU, &ifr) == -1 ){
					xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
					goto SOCKETED_ERROR;
				}
				reinjec_mtu = ifr.ifr_mtu;
				
				xyprintf(0, "Get reinjec nic %s's mtu is %d, mac is %02x:%02x:%02x:%02x:%02x:%02x",
						reinjec_nic, reinjec_mtu,
						reinjec_mac[0],	reinjec_mac[1],	reinjec_mac[2],
						reinjec_mac[3],	reinjec_mac[4],	reinjec_mac[5]);
			}
			// 如果不是回注口
			else {
				// 获取接口标识
				if ( ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1 ){
					xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
					goto SOCKETED_ERROR;
				}
			
				// 判断是不是本地环路接口
				if( ifr.ifr_flags & IFF_LOOPBACK ){
					xyprintf(0, "Nic %s is the loopback interface", ifr.ifr_name);
				}
				else {
					// 启用
					ifr.ifr_flags |= IFF_UP;
					// 混乱模式
					ifr.ifr_flags |= IFF_PROMISC;
					
					// 设置接口标识
					if( ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1 ) { 
						xyprintf(errno, "This a error at file %s line %d", __FILE__, __LINE__);
						goto SOCKETED_ERROR;
					}
					xyprintf(0, "Nic %s set up & promisc success!", ifr.ifr_name);
				}
			} //end if
		} // end if
		memset(buf, 0, sizeof(buf));
	} // end while

	xyprintf(0, "Init Nic success!\n");

	close(sockfd);
	fclose(file);
	return 0;

SOCKETED_ERROR:
	close(sockfd);
FILEED_ERROR:
	fclose(file);
	return -1;
}

// 字符串替换
void str_replace(char* src, char* match, char* replace)
{
	char temp[4096];			// 临时空间
	int befor_len = 0;			// 标志前字符长度

	// 查找匹配关键字
	char* find_pos = strstr(src, match);

	// 循环替换
	while( find_pos ){
		// 清空临时空间 将字符串组装到临时空间内
		memset(temp, 0, sizeof(temp));

		befor_len = find_pos - src;

		// 匹配关键字前字符
		strncpy(temp, src, befor_len);
		// 匹配关键字
		strcat(temp, replace);
		// 匹配关键字后字符
		strcat(temp, find_pos + strlen(match));
		
		strcpy(src, temp);

		find_pos = strstr(src, match);
	}
}





// 打印线程
void* print_thread(void *fd)
{
	pthread_detach(pthread_self());

	xyprintf(0, "Print thread start ......");

	while(1){
		sleep(60);
		xyprintf(0, "Print thread running ......");
		printf_apk_list();
	}// end while(1)
	pthread_exit(NULL);
}









