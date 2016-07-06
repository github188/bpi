#include "header.h"

#define MAX_LINK_PACKET_SIZE	65535

static unsigned int count = 0;

// 监听线程
void monitor_thread()
{
	// 创建链路层监听socket
	int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if(sockfd <= 0){
		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
		pthread_exit(NULL);
	}

	// TODO 这里不起作用
	// 绑定网卡对链路层不起作用
//	if( setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&if_mon_nic, sizeof(if_mon_nic))){
//		xyprintf(errno, "There is an error in %s(%d) : ", __FILE__, __LINE__);
//		goto ERR;
//	}

	struct sockaddr_in from = {0};
	socklen_t from_len = sizeof(from);
	unsigned char buf[65535];
	pthread_t pt;


	while(1){
		// 申请空间存放接收到的数据包
		struct link_packet* lp = malloc(sizeof(struct link_packet));
		if(!lp){
			xyprintf(errno, "An error in file %s line %d.", __FILE__, __LINE__);
			continue;
		}
		memset(lp, sizeof(struct link_packet), 0);
		
		lp->packet = malloc(MAX_LINK_PACKET_SIZE);
		if(!lp->packet){
			xyprintf(errno, "An error in file %s line %d.", __FILE__, __LINE__);
			if(lp){
				free(lp);
			}
		}
		memset(lp->packet, MAX_LINK_PACKET_SIZE, 0);
		
		// 接收数据	
		lp->packet_len = 
			recvfrom(sockfd, lp->packet, MAX_LINK_PACKET_SIZE, 0, (struct sockaddr*)&from, &from_len);
		
		// 错误处理
		if(lp->packet_len < 0){
			// 释放申请的空间
			if(lp->packet){
				free(lp->packet);
			}
			if(lp){
				free(lp);
			}
		
			// 是否需要重试
			if(errno == EINTR){
				continue;
			}
			else {
				xyprintf(errno, "An error in file %s line %d.", __FILE__, __LINE__);
				break;
			}
		}

		// 收包计数器
		if(++count % 10000 == 0){
			xyprintf(0, "<-- <-- <-- Recv the %u packet!", count);
		}
		//xyprintf_sockaddr_ll(&from);

		// 新线程处理数据包
		if( pthread_create(&pt, NULL, filter_thread, (void*)lp) != 0 ){
			xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
			
			// 释放申请的空间
			if(lp->packet){
				free(lp->packet);
			}
			if(lp){
				free(lp);
			}
		}
	} //end for
ERR:
	close(sockfd);
	return;
}

