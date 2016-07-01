#include "../header.h"

int main(int argc, char** argv)
{
	int sockfd;
    struct sockaddr_in addr;	
	unsigned char buf[65535];
	int ret;			// 返回值

	while(1){
		printf("A new loop starting, connect to %s!!\n", ADDR);
		//socket初始化
		if(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
			printf("GUIDE_ERROR:%s %d -- socket()\n", __FILE__, __LINE__);
			goto ERR;
		}
	
		//连接服务器
		addr.sin_family			= AF_INET;
		addr.sin_port			= htons( PORT );
		addr.sin_addr.s_addr	= inet_addr( ADDR );
		if( connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
			printf("GUIDE_ERROR:%d %s %d -- connect: %s\n", sockfd, __FILE__, __LINE__, strerror(errno));
			goto ERR;
		}

		printf("Connect %s - %d success!\n\n", ADDR, PORT);


		//开始
		for(;;) {
			sprintf(buf, "Hello server!");
			ret = send(sockfd, buf, strlen(buf), 0);		
			if (ret <= 0) {
				printf("SOCK_ERROR:%s %s %d -- send() failed!\n", __func__, __FILE__, __LINE__);
				printf("%s\n", strerror(errno));
				break;
			}
			
			printf_time();
			printf(" -- 发送数据包：ret = %d\n\n", ret);
		
			//接收
			memset(buf, 0, 65535);
			ret =  recv(sockfd, buf, 65535, 0);
			if (ret <= 0 ) {
				printf("SOCK_ERROR:%s %s %d -- recv() failed!\n", __func__, __FILE__, __LINE__);
				break;
			}

			printf_time();
			printf(" -- 接收到数据包：ret = %d\n", ret);
			printf("\t数据包内容：%s\n\n", buf);
			
			sleep(3);
			//usleep(100);
		}
ERR:
		if (sockfd != -1) {
			close( sockfd );
			sockfd = -1;
		}

		sleep(1);
	}
	return 0;
}
