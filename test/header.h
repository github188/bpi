/*****************************************************
 *
 * 头文件 宏
 *
 *****************************************************/
#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 14096
#define ADDR "127.0.0.1"

void printf_time();

#endif
