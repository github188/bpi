#include "header.h"

//初始化数据库
PGconn* sql_init()
{
	//const char *conninfo = "dbname=test user=postgres hostaddr=127.0.0.1";
	const char *conninfo = "dbname=linyidata user=postgres password=postgres hostaddr=127.0.0.1 port=5432";
	PGconn* conn;
	conn = PQconnectdb(conninfo);

	if(PQstatus(conn)!=CONNECTION_OK){
		printf("connect to database failed!\n");
		PQfinish(conn);
		return 0;
	}
	//printf("连接数据库成功！\n");
	return conn;

}

//执行一个不需要返回结果的语句
int sql_exec(PGconn *conn,char *sql_str)
{
	PGresult *res;
	res=PQexec(conn,sql_str);

	if(PQresultStatus(res)!=PGRES_COMMAND_OK){
		printf("errno == %d \n",errno);
		fprintf(stderr, "command failed: %s", PQerrorMessage(conn));
		printf("PQexec failed!\n");
		PQclear(res);
		PQfinish(conn);
		return 0;
	}
	PQclear(res);
	return 1;
}

//执行查询语句
PGresult* sql_select(PGconn *conn,char *sql_str)
{
	PGresult *res;
	res=PQexec(conn,sql_str);	
	if(PQresultStatus(res)!=PGRES_TUPLES_OK){
		fprintf(stderr, "command failed: %s", PQerrorMessage(conn));
		printf("查询失败！\n");
		PQclear(res);
		PQfinish(conn);
		return NULL;
	}
	else if(PQntuples(res)==0)
	{
		printf("数据库中没有查到！\n");
		return NULL;
	}
	return res;
}
/**
 * 返回结果res中第h行第l列字段的值，以字符串形式
 *
 */
char* SelectBind_String(PGresult *res,int h,int l)
{
	char *result;
	result=PQgetvalue(res,h,l);
	return result;
}

/**
 *
 * 返回h行l列的字段值，以数字形式
 *
 */
int SelectBind_Int(PGresult *res,int h,int l)
{
	char *result;
	//result=(char*)malloc(32*sizeof(char));
	if(res == NULL)
	{
		printf("res is NULL !!!!!\n");
		return -1;
	}
	result=PQgetvalue(res,h,l);
	return atoi(result);
}


int sql_destory(PGconn *conn)
{
	PQfinish(conn);
	/*
	if(PQresultStatus(res)!=CONNECTION_BAD){
		return 0;
	}
	*/
	return 1;
}












