#include "header.h"

#define IS_DAEMON_EXIST			0					// 精灵线程

/** 
 *@brief  程序主体函数 由子进程运行
 */
void run()
{
#if IS_DAEMON_EXIST 
	//关闭父进程创建的log文件
	logs_destroy();
#endif
	//创建子进程log文件
	if( logs_init("logs") ){
		printf("logs_init error!!!!\n");
		exit(-1);
	}
	
	//打印程序开启信息
	xyprintf(0, "\n\t\t\t*************************************************************************\n\
			*************************************************************************\n\
			****                                                                 ****\n\
			****                      The BPI is running!                        ****\n\
			****                          Pid is %d                            ****\n\
			****                                                                 ****\n\
			*************************************************************************\n\
			*************************************************************************\n",
			getpid() );

//	pthread_mutex_init(&gv_authenticate_list_lock, 0);

	/****************平台连接监视线程**************************************************/
//	pthread_t pt;
//	if( pthread_create(&pt, NULL, platform_conn_thread, NULL) != 0 ){
//		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
//	}
	/**********************************************************************************/

	while(1){
		monitor_thread(0);
	}

	//不会执行到的一步
	//pthread_mutex_destroy(&gv_authenticate_list_lock);
}

/** 
 *@brief  主函数 启动子进程中运行程序主体 父进程监控主进程的运行状态
 */
int main(int argc, char** argv)
{
#if IS_DAEMON_EXIST 
	if( logs_init("wizard_logs") ){
		printf("DATA_ERROR:%s %d -- logs_init error!!!!\n",__FILE__, __LINE__);
		exit(-1);
	}

	// 守护进程（父进程）
	int status;
	for(;;){
		if( 0 == fork() ){
			run();
		}
		xyprintf(0, "ELF_THREAD:The child process is running!");
	    waitpid(-1, &status, 0);
		xyprintf(0, "ELF_THREAD:The child process is dead!");
		if(WIFSIGNALED(status)) {
			xyprintf(0, "ELF_THREAD:The child process is dead! Signal is %d", WTERMSIG(status) );
        }
	}
#else
	run();
#endif
}
