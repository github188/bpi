/*****************************************************
 *
 * 策略操作函数
 *
 *****************************************************/
#include "header.h"

// 策略节点
struct apk_node{
	struct list_head node;
	char* url;
	unsigned int url_len;
	unsigned int count;
	unsigned int para_count;
};

static unsigned int apk_list_size = sizeof(struct apk_node);

LIST_HEAD(apk_list_head);					// 策略链表
static int apk_list_count = 0;				// 策略链表计数器
static pthread_mutex_t apk_list_lock;

//处理时间测试 -- 开始时间
static time_t start;

void apk_list_init()
{
	pthread_mutex_init(&apk_list_lock, 0);
	start = time(0);
}

// 策略添加函数
void apk_list_add(char* url, int para_falg)
{
	// 查找数据是否存在 存在直接返回
	struct list_head* pos;
	struct apk_node* node;

	// 开始查找
	pthread_mutex_lock(&apk_list_lock);
	for( pos = apk_list_head.next; pos != &apk_list_head; pos = pos->next ){
		node = (struct apk_node*)pos;
		
		// 判断是不是 如果是 就增加一次
		if( !strncmp(url, node->url, node->url_len) ){
			node->count ++;
			if( para_falg ){
				node->para_count ++;
			}
			pthread_mutex_unlock(&apk_list_lock);
			//找到返回
			return;
		}
	}
	pthread_mutex_unlock(&apk_list_lock);

	// 没找到添加
	// 申请节点空间
	node = malloc(apk_list_size);

	// 申请字符串空间 url
	node->url = malloc(strlen(url) + 1);
	memcpy(node->url, url, strlen(url) + 1);

	// 字符串长度
	node->url_len = strlen(url);
	
	// 其他初始
	node->count = 1;
	if( para_falg ){
		node->para_count = 1;
	}
	else {
		node->para_count = 0;
	}


	// 添加到链表中
	pthread_mutex_lock(&apk_list_lock);
	list_add(&(node->node), &apk_list_head);
	apk_list_count ++;
	pthread_mutex_unlock(&apk_list_lock);
}

/** 
 *@brief 查找数据
 *@return js
 */
void printf_apk_list()
{
	struct list_head* pos;
	struct apk_node* node;
//	PGconn conn* = sql_init();
//int sql_destory(PGconn *conn);

	// 头部信息
	time_t now = time(0);
	xyprintf(0, "now - start = %d", now - start);
	xyprintf(0, "url\tcount\tpara_count\n\t\tapk_list_count = %u", apk_list_count);
	char sql_str[8192];

	// 开始遍历
	pthread_mutex_lock(&apk_list_lock);
	for( pos = apk_list_head.next; pos != &apk_list_head; pos = pos->next ){
		node = (struct apk_node*)pos;
		xyprintf(0, "%s\t%u\t%u", node->url, node->count, node->para_count);
//		if(conn){
//			snprintf(sql_str, 8191, "INSERT INTO ");
//			sql_exec(conn, sql_str);
//		}
	}
	pthread_mutex_unlock(&apk_list_lock);
}
