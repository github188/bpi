#include "../header.h"

int main(int argc, char** argv)
{
	int sockfd;

	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("SOCK_ERROR:%s %s %d -- socket() error!\n", __func__, __FILE__, __LINE__);
		return -1;
	}
    
	//解除IP端口没有及时释放而无法绑定 保证服务器关掉后，立即可以启动
	int reuseaddr = 1;
    setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof (reuseaddr));
	
	struct sockaddr_in sa;
	//变量
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons( PORT );
	
	//绑定端口
	if (bind( sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0){
		printf("SOCK_ERROR:%s %s %d -- bind() error!\n", __func__, __FILE__, __LINE__);
		return -1;
	}
	
	//开启监听
	if (listen(sockfd, 4098) < 0){
		printf("SOCK_ERROR:%s %s %d -- listen() error!\n", __func__, __FILE__, __LINE__);
		return -1;
	}

	int ret;			// 返回值
	unsigned char buf[65535];

	//开始
	for(;;) {
		
		struct sockaddr_in client_address;//存放客户端地址信息
		int client_len = sizeof( client_address );
		int client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
		if(client_sockfd == -1){
			printf("SOCK_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
			continue;
		}
		
		printf("A new client connect, sockfd is %d, client_len = %d\n", client_sockfd, client_len);
		printf("sa_family = %d, sin_port = %d, sin_addr = %s\n\n",
			client_address.sin_family, htons(client_address.sin_port), inet_ntoa(client_address.sin_addr) );

		for(;;){
			memset(buf, 0, 65535);
			//接收
			ret =  recv(client_sockfd, buf, sizeof(buf), 0);
			if (ret <= 0 ) {
				printf("SOCK_ERROR:%s %s %d -- recv() failed!\n\n", __func__, __FILE__, __LINE__);
				break;
			}
			
			printf_time();
			printf(" -- 接收到一个数据包：ret = %d\n", ret);
			printf("\t数据包内容：%s\n\n",buf);
		
			sprintf(buf, "Hi, client!");
			ret = send(client_sockfd, buf, strlen(buf), 0);
			if (ret <= 0) {
				printf("SOCK_ERROR:%s %s %d -- send() failed!\n", __func__, __FILE__, __LINE__);
				printf("%s\n", strerror(errno));
				break;
			}
			
			printf_time();
			printf(" -- 回复数据包 ret = %d\n\n", ret);
		}
		if(client_sockfd > 0){
			close(client_sockfd);
		}
	}

	if (sockfd != -1) {
		close( sockfd );
		sockfd = -1;
	}

	printf("All is over!\n");

	return 0;
}
