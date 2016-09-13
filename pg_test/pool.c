/*****************************************************
 *
 * 线程池操作函数
 *
 *****************************************************/
#include "header.h"

/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/
struct pool_worker
{
    void				*(*process) (void* arg, PGconn* conn);	// 回调函数，任务运行时会调用此函数，注意也可声明成其它形式
    void				*arg;						// 回调函数的参数
    struct pool_worker	*next;
};

/*线程池结构*/
struct pool_para
{
    pthread_mutex_t		queue_lock;				// 线程互斥锁
    pthread_cond_t		queue_ready;			// 条件变量
	struct pool_worker	*queue_head;			// 链表结构，线程池中所有等待任务
	int					thread_sleep_num;		// 在睡觉的线程数量
	unsigned int		cur_queue_size;			// 等待队列中的任务数目
};

struct pool_para *pool = NULL;			//线程池对象

/** 
 *@brief  线程执行函数 线程执行的主体函数 这里有这一个线程的一生 它生在这里 在这里执行任务 和 睡觉 吃饭 ,,,, 死亡...
 *@param  arg		类型 void*	线程函数必须，无意义
 *@return 无意义
 */
void * thread_routine (void *arg)
{
	int thread_sn = (long)arg;	// 线程编号

	// 打印一下 告诉程序 我执行正常
	xyprintf(0, "THREAD_ADJUST:** O(∩ _∩ )O ~~ Starting thread success, sn is %04d, id is %lu!", thread_sn, pthread_self());

	// 第一层while循环 如果sql出现错误 会被循环执行
	while(1){
		// 获取数据库链接
		PGconn* conn;
		
		while(1){
			conn = PQconnectdb("dbname=test user=postgres password=postgres hostaddr=192.168.25.11");
			xyprintf(0, "pool sn %d conn pg success!", thread_sn);
			// 检查链接状态
			if(PQstatus(conn) != CONNECTION_OK){
				xyprintf(0, "SQL_ERROR:connect to database failed in pool sn %04d!\n\t%s", thread_sn, PQerrorMessage(conn));
				PQfinish(conn);
				sleep(5);
				continue;
			}
			break;
		}
		
		// 第二层while循环,在sql出现错误的时候会跳出
		while(1){
			// 先申请拿锁
			pthread_mutex_lock (&(pool->queue_lock));
			
		//	xyprintf(0, "pool sn %d get lock!", thread_sn);

			// 如果等待队列为0，则处于阻塞状态; 注意pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁
			while(pool->cur_queue_size == 0){

		//		xyprintf(0, "pool sn %d sleep!", thread_sn);
				
				pool->thread_sleep_num++;										// 线程睡觉
				pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));	// 睡觉等待
				pool->thread_sleep_num--;										// 线程睡醒
				
		//		xyprintf(0, "pool sn %d getup!", thread_sn);
			}

			// 如果当前任务队列没有任务了 则返回等待
			if( pool->cur_queue_size == 0 || pool->queue_head == NULL){
				xyprintf(0, "pool sn %d not get cur!", thread_sn);
				continue;
			}

			// 等待队列长度减去1，并取出链表中的头元素
			pool->cur_queue_size--;
			struct pool_worker *worker = pool->queue_head;
			pool->queue_head = worker->next;
			
		//	xyprintf(0, "pool sn %d get a cur, cur_queue_size is %d!", thread_sn, pool->cur_queue_size);
		
			pthread_mutex_unlock (&(pool->queue_lock));			// 释放互斥锁

			// 调用回调函数，执行任务
			void* res = (*(worker->process)) (worker->arg, conn);
			
		//	xyprintf(0, "pool sn %d finish!", thread_sn);
			// 任务执行完成 释放任务资源
			free (worker);
			worker = NULL;
			// 判断任务是否处理正常 -- sql连接是否运行正常
			if((long)res == POOL_SQL_ERROR){
				break;
			}
		}
		// 因为sql连接异常 而跳出第二层循环 所以先销毁sql资源,然后重新初始化
		PQfinish(conn);
	}
}

/** 
 *@brief  线程池初始化
 *@param  thread_num	类型 unsigned int	线程池大小
 *@param  max			类型 unsigned int	线程最大数量 不自动调节 则将最大 最小数 和初始数一样
 *@param  min			类型 unsigned int	线程最小数量
 *@param  sql_name		类型 char*
 *@param  sql_user		类型 char*
 *@param  sql_pass		类型 char*
 *@return 无
 */
void pool_init (unsigned int thread_num)
{
	// 申请线程池对象空间
	pool = (struct pool_para *) malloc( sizeof(struct pool_para) );
	memset( pool, 0, sizeof(struct pool_para) );
	
	// 变量初始化
    pthread_mutex_init( &(pool->queue_lock), NULL);		// 初始化互斥锁
    pthread_cond_init( &(pool->queue_ready), NULL);		// 初始化条件变量
    pool->queue_head		= NULL;						// 任务队列头置空
	pool->thread_sleep_num	= 0;						// 在睡觉的线程数量
    pool->cur_queue_size	= 0;						// 任务队列任务数

	// 线程创建
	int i = 0;
	pthread_t threadid;
	for(; i < thread_num; i++){
		pthread_create(&threadid, NULL, thread_routine, (void*)((long)(i+1000)) );
    }
}

/** 
 *@brief  向线程池中添加任务
 *@param  process	类型 void*(*)(void*,wt_sql_handle*)		任务执行函数
 *@param  arg		类型 void*								传入任务执行函数的参数
 *@return 无意义
 */
void pool_add_worker (void *(*process) (void *arg,  PGconn *conn), void *arg)
{
    // 构造一个新任务
	struct pool_worker *newworker = (struct pool_worker*) malloc(sizeof (struct pool_worker));
	newworker->process = process;
	newworker->arg = arg;
	newworker->next = NULL;			// 别忘置空

	pthread_mutex_lock(&(pool->queue_lock));	// 加锁
	 
	// 将任务加入到等待队列中
	struct pool_worker *member = pool->queue_head;
	if (member != NULL){
		while (member->next != NULL){
			member = member->next;
		}
		member->next = newworker;				// 存放到队列尾
    }
	else {
		pool->queue_head = newworker;
	}

	pool->cur_queue_size++;						// 任务数量

	// 在任务数量大于10的时候 打印报警语句
	if(pool->cur_queue_size >= 20){
		if(pool->cur_queue_size % 1000 == 0){
			xyprintf(0, "THREAD_ADJUST:WARNING!!! cur_queue_size is %u, thread_sleep_num is %u",
					pool->cur_queue_size, pool->thread_sleep_num);
		}
	}
	
	pthread_mutex_unlock (&(pool->queue_lock));	//释放锁

	//好了，等待队列中有任务了，唤醒一个等待线程；注意如果所有线程都在忙碌，这句没有任何作用
	pthread_cond_signal (&(pool->queue_ready));
}
