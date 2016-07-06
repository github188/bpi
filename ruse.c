#include "header.h"

struct ruse_node{
	struct list_head node;
	char* url;
	char* js;
};

static unsigned int ruse_list_size = sizeof(struct ruse_node);

LIST_HEAD(ruse_list_head);

//static pthread_mutex_t ruse_list_lock;

static int ruse_list_count = 0;


void ruse_list_add(char* url, char* js)
{
	struct ruse_node *node = malloc(ruse_list_size);
	node->url = url;
	node->js = js;

//	pthread_mutex_lock(&ruse_list_lock);
	list_add(&(node->node), &ruse_list_head);
//	pthread_mutex_unlock(&ruse_list_lock);
	ruse_list_count ++;
	xyprintf(0, "ADD:ruse_list_count = %d\nurl : %s\njs : %s",
			ruse_list_count, node->url, node->js);
}

/** 
 *@brief 查找数据
 *@return js
 */
char* ruse_list_find(char* url)
{
	struct list_head* pos;
	struct ruse_node* node;
//	pthread_mutex_lock(&ruse_list_lock);
	for( pos = ruse_list_head.next; pos != &ruse_list_head; pos = pos->next ){
		node = (struct ruse_node*)pos;
		if( !strcmp(url, node->url) ){
			xyprintf(0, "Find - url: %s\njs: %s", node->url, node->js);
			//	pthread_mutex_unlock(&ruse_list_lock);
			return node->js;
		}
	}
//	pthread_mutex_unlock(&ruse_list_lock);
	
	return NULL;
}

#define RUSE_SERVER_HOST	"ad.yuanzhenginfo.com"
#define RUSE_SERVER_PORT	80
#define GET_RUSE_NUM_URL	"/get_ruse_num"
#define GET_RUSE_LIST_URL	"/get_ruse_list"
#define ONCE_GET_RUSE_NUM	10

// 连接服务器
int conn_ruse_server()
{
	int sockfd;
    struct sockaddr_in addr;	
	
	// socket初始化
	if(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		xyprintf(errno, "ERROR:%s %d -- socket()", __FILE__, __LINE__);
		return -1;
	}

	// 域名解析
	struct hostent* hostent = gethostbyname( RUSE_SERVER_HOST );
	addr.sin_addr.s_addr	= *(unsigned long *)hostent->h_addr;

	xyprintf(0, "GET host of %s -- %s", RUSE_SERVER_HOST, inet_ntoa(addr.sin_addr));

	// 连接 失败的话 休眠3秒 重新连接
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( RUSE_SERVER_PORT );
	if( connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		xyprintf(errno, "ERROR:%s %d -- connect()", __FILE__, __LINE__);
		close(sockfd);
		return -1;
	}
	return sockfd;
}


/** 
 *@brief  发送数据函数
 *@param  sock		类型 int			要发送到的socket套接字
 *@param  buf		类型 unsigned char*	要发送的内容
 *@param  len		类型 int			要发送的内容长度
 *@return success 0 failed -1
 */
int send_block(int sock, unsigned char *buf, int len)
{
	int ret;			//返回值
	int slen = len;		//未发送字节数
	int yelen = 0;		//已发送字节数
	struct timeval tv;	//超时时间结构体

	//错误判断
	if (sock == -1 || !buf || len <= 0){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- I can't believe, why here !!! （╯' - ')╯︵ ┻━┻ ", __func__, __FILE__, __LINE__);
		return -1;
	}

	//设置超时时间
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	//开启发送
	while (slen > 0) {
		
		//发送 -- send -- ┬─┬ ノ( ' - 'ノ)
		ret = send(sock, buf + yelen, slen, 0);		
		
		if (ret < 0) {			// 发送错误
			xyprintf(errno, "SOCK_ERROR:%s %s %d -- send() failed!", __func__, __FILE__, __LINE__);
			if (errno == EAGAIN) {
				continue;
			}
			return -1;
	    }
		else if (ret == 0) {	// 对方关闭了套接字
			xyprintf(errno, "SOCK_ERROR:%s %s %d -- send() failed!", __func__, __FILE__, __LINE__);
			return -1;
		}
		else {					// 成功
			yelen += ret;	//已发送
			slen -= ret;	//未发送
		}

	}
	return 0;
}

/** 
 *@brief  接收数据函数
 *@param  sock		类型 int			要接收数据的socket套接字
 *@param  buf		类型 unsigned char*	要接收的内容
 *@param  len		类型 int			要接收的内容长度
 *@param  block_flag类型 int			1 使用阻塞接收, 0 使用非阻塞接收
 *@return success 0 failed -1
 */
int recv_block(int sock, unsigned char *buf, int len/*, int block_flag*/)
{
	int ret;			// 返回值
	int rlen = 0;		// 已接收到的字节数
	struct timeval tv;	// 超时时间结构体

	//错误判断
	if (sock == -1 || !buf || len <= 0){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- I can't believe, why here !!! （╯' - ')╯︵ ┻━┻ ", __func__, __FILE__, __LINE__);
		return -1;
	}

	//设置超时时间
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	//开始接收
	while (rlen < len) {
		//接收
		ret =  recv(sock, buf + rlen, len - rlen, 0);
		if (ret < 0 ) {				// 读取错误
			xyprintf(errno, "SOCK_ERROR:%s %s %d -- recv() failed!", __func__, __FILE__, __LINE__);
//			if (errno == EAGAIN) {
//				continue;
//			}
			return -1;
		}
		else if (ret == 0) {		// 对方关闭了套接字
			xyprintf(errno, "SOCK_ERROR:%s %s %d -- recv() failed!", __func__, __FILE__, __LINE__);
			return -1;
		}
		else {						// 读取正常
			rlen += ret;
		}
	}
	return 0;
}


char* get_http_res(int sockfd, char* ques)
{
	xyprintf(0,"ques: %s", ques);
	// 发送请求
	if( send_block(sockfd, ques, strlen(ques)) ){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- send get ruse num failed!", __func__, __FILE__, __LINE__);
		return NULL;
	}

	// 接收回复
	char res[2048] = {0};
	
	// 先最多接收1024个字符
	int ret = recv(sockfd, res, 2048, 0);
	if( ret < 0 ){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- send get ruse num failed!", __func__, __FILE__, __LINE__);
		return NULL;
	}

	xyprintf(0, "%s", res);

	// 获取内容长度
	char *res_content_length = strstr(res, "Content-Length:");
	res_content_length += strlen("Content-Length:");
	if(*res_content_length == ' '){
		res_content_length++;
	}
	int content_length = atoi(res_content_length);

	if( !content_length ){
		xyprintf(errno, "Get Content-Length error at file %s line %d!", __FILE__, __LINE__);
		return NULL;
	}
	
	xyprintf(0, "content_length : %d", content_length);

	// 找到数据头
	char *res_content = strstr(res, "\r\n\r\n");
	res_content += 4;

	// 计算已经接收到的数据长度
	int content_recved_num = ret - (res_content  - res);
	// 没有接收到的数据长度
	int content_recv_num = content_length - content_recved_num;
	
	char *content = malloc(content_length + 32);
	memset(content, 0, content_length + 32);

	// 拷贝已接收到的内容数据
	if( content_recved_num > 0 ){
		memcpy(content, res_content, content_recved_num);
	}
	else if( content_recved_num < 0 ) {
		xyprintf(0, "??? Is this a error url, content_recved_num is %d!", content_recved_num);
		goto MALLOCED_ERROR;
	}

	// 如果还有数据需要接收
	if( content_recv_num > 0 ){
		// 继续接收剩余的数据
		if( recv_block(sockfd, content + content_recved_num, content_recv_num) ){
			xyprintf(0, "Recv the content error, file at %s, line is %d", __FILE__, __LINE__);
			goto MALLOCED_ERROR;
		}
	}

	return content;
MALLOCED_ERROR:
	if(content){
		free(content);
	}
	return NULL;
}

// 初始化策略库
int init_ruse()
{
	// 初始化socket
	int sockfd = conn_ruse_server();
	if( sockfd < 0 ){
		return -1;
	}
	
	char ques[1024] = {0};
	snprintf(ques, sizeof(ques) - 1, "GET %s HTTP1.1\r\nHost: %s\r\n\r\n", GET_RUSE_NUM_URL, RUSE_SERVER_HOST);

	// 发送请求并获取内容
	char* res = get_http_res(sockfd, ques);
	if(!res){
		xyprintf(0, "get ruse num error!");
		goto SOCKED_ERROR;
	}

	xyprintf(0, "res connect : %s", res);

	// 获得ruse数量
	int ruse_num = atoi(res);
	if( ruse_num <= 0 ){
		xyprintf(0, "ruse is error:%d", ruse_num);
		free(res);
		goto SOCKED_ERROR;
	}
	
	// 处理完获取的http内容后 记得释放内存
	free(res);
	res = NULL;


	// 循环读取策略库
	int curr = 0;
	for( ;curr < ruse_num; curr += ONCE_GET_RUSE_NUM ){
		
		// 获取 curr+1 到 curr+1+ONCE_GET_RUSE_NUM 或者是末尾的条目
		int num = (ruse_num - curr >= ONCE_GET_RUSE_NUM) ? ONCE_GET_RUSE_NUM : ruse_num - curr;
		snprintf(ques, sizeof(ques) - 1, "GET %s?begin=%d&num=%d HTTP1.1\r\nHost: %s\r\n\r\n",
				GET_RUSE_LIST_URL, curr + 1, num, RUSE_SERVER_HOST);
		
		// 获取到回复
		res = get_http_res(sockfd, ques);
		if(!res){
			xyprintf(0, "get ruse list of %d to %d error!", curr + 1, curr + 1 + num);
			goto SOCKED_ERROR;
		}
		
		xyprintf(0, "%s", res);

	} //end for



	close(sockfd);
	exit(1);
	return 0;
SOCKED_ERROR:
	close( sockfd );		//出现错误关闭sockfd 并重新连接
	return -1;
}
