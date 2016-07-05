#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
	// 打开网卡列表文件
	FILE* file = fopen("/proc/net/dev", "r");
	if(!file){
		perror("file");
		goto FILEED_ERROR;
	}

	// 创建一个socket套接字
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( sockfd < 0){
		perror("socket");
		goto SOCKETED_ERROR;
	}

	// 读取文件缓冲区
	char buf[1024] = {0};
	char* temp;
	
	struct ifreq ifr; 
	
	char mac[32] = {0};
	int flag;
	char ip[32] = {0};
	char broadAddr[32] = {0};
	char subnetMask[32] = {0};
	
	while( fgets(buf, 1023, file) ){
		temp = strchr(buf, ':');
		if(temp){
			*temp = 0;
			temp = buf;

			while( *temp == ' ' ){
				temp++;
			}

			strcpy(ifr.ifr_name, temp);

			//获取mac地址
			if (!ioctl(sockfd, SIOCGIFHWADDR, &ifr)){
				memset(mac, 0, sizeof(mac));
				
				snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
						(unsigned char)ifr.ifr_hwaddr.sa_data[0],
						(unsigned char)ifr.ifr_hwaddr.sa_data[1],
						(unsigned char)ifr.ifr_hwaddr.sa_data[2],
						(unsigned char)ifr.ifr_hwaddr.sa_data[3],
						(unsigned char)ifr.ifr_hwaddr.sa_data[4],
						(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
				printf("%s - %s\n", temp, mac);
			}
			else{
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				goto SOCKETED_ERROR;
			}
			
			// 获取接口标识
			if (!ioctl(sockfd, SIOCGIFFLAGS, &ifr)){
				flag = ifr.ifr_flags;
			}
			else {
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				goto SOCKETED_ERROR;
			}
			
			// 获取ip地址
			if (!ioctl(sockfd, SIOCGIFADDR, &ifr)){
				snprintf(ip, sizeof(ip), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
				printf("device ip: %s\n", ip);
			}
			else{
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(sockfd);
				return -1;
			}
			
			// 获取广播地址
			if (!ioctl(sockfd, SIOCGIFBRDADDR, &ifr)){
				snprintf(broadAddr, sizeof(broadAddr), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_broadaddr))->sin_addr));
				printf("device broadAddr: %s\n", broadAddr);
			}
			else{
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(sockfd);
				return -1;
			}
			
			// 获取子网掩码
			if (!ioctl(sockfd, SIOCGIFNETMASK, &ifr)){
				snprintf(subnetMask, sizeof(subnetMask), "%s",
						(char *)inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));
				printf("device subnetMask: %s\n", subnetMask);
			}
			else{
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(sockfd);
				return -1;
			}

			// 获取mtu
			if( !ioctl(sockfd, SIOCGIFMTU, &ifr) ){
				printf("mtu: %d\n", ifr.ifr_mtu);
			}
			else{
				printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
				close(sockfd);
				return -1;
			}

		} // endif
		memset(buf, 0, sizeof(buf));
	}

	printf("\n\n\n\n");
SOCKETED_ERROR:
	close(sockfd);
FILEED_ERROR:
	fclose(file);
	return -1;
#if 0
	struct ifconf ifc;			// 保存接口信息

	int interfaceNum = 0;		// 网卡数量

	// 数据缓冲区长度 数据缓冲区
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (void *)buf;

	// 获取所有接口列表
	// 因为SIOCGIFCONF只能获取IP层的配置，当网卡没有设置IP，它就获取不到网卡信息
	if (!ioctl(sockfd, SIOCGIFCONF, (char *)&ifc)){
		// 网卡数量
		interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
		printf("interface num = %d\n", interfaceNum);
		
		while (interfaceNum-- > 0){
			
			// 网卡名称
			printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);
			

			
		}
	}
	else{
		printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		close(sockfd);
		return -1;
	}
#endif
}





























#if 0
// 使用ioctl函数虽然可以获取所有的信息，但是使用起来比较麻烦，如果不需要获取MAC地址，那么使用getifaddrs函数来获取更加方便与简洁。
int main(void)
{
	 struct sockaddr_in *sin = NULL;
	 struct ifaddrs *ifa = NULL, *ifList;
	 if (getifaddrs(&ifList) < 0) return -1;
	 for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next)
	 {
		  if(ifa->ifa_addr->sa_family == AF_INET)
		  {
				printf("\n>>> interfaceName: %s\n", ifa->ifa_name);
				sin = (struct sockaddr_in *)ifa->ifa_addr;
				printf(">>> ipAddress: %s\n", inet_ntoa(sin->sin_addr));
				sin = (struct sockaddr_in *)ifa->ifa_dstaddr;
				printf(">>> broadcast: %s\n", inet_ntoa(sin->sin_addr));
				sin = (struct sockaddr_in *)ifa->ifa_netmask;
				printf(">>> subnetMask: %s\n", inet_ntoa(sin->sin_addr));
		  }
	 }
	 freeifaddrs(ifList);
	 return 0;
}

#endif
