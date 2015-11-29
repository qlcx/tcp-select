#ifndef __tcpServer_H__
#define __tcpServer_H__

#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include "json/json.h"
using namespace std;

//发送数据包结构体
typedef struct __DATA_PACKAGE
{
	int pack_len;
}DATA_PACKAGE;

//tcp套接字结构体
typedef struct __FD_DATA
{
	int fd;					//套接字
	bool validate;			//判断套接字是否验证通过
	time_t test_time;		//记录心跳时间
}FD_DATA;

class tcpServer
{
public:
	tcpServer();
	~tcpServer();

	//建立socket通讯
	void tcpsocket();

	//发送信息给客户端
	void senddatatoclient(string sendData, int sendsock_i);

	//从客户端接受信息
	bool recvdatatoclient(int r_fd, bool test);

protected:
private:
	//socket通讯套接字
	int serv_sock;
	int clnt_sock;
	sockaddr_in serv_adr;
	sockaddr_in clnt_adr;

	//可用套接字集合
	fd_set reads, cpy_reads;

		
	//设定套接字属性
	vector<FD_DATA> v_fd_data;
	FD_DATA fd_data;

	//发送信息
	string sdata;

	//接收数据
	char *recvBuf;

	//接收包体长度信息
	int package_len;
};

#endif