#ifndef __TCPCLIENT_H
#define __TCPCLIENT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "json/json.h"
using namespace std;

typedef struct __DATA_PACKAGE
{
	int pack_len;
}DATA_PACKAGE;

class tcpClient
{
public:
	tcpClient();
	~tcpClient();

	void tcpsocket();

protected:
private:
	//Í¨ÐÅÌ×½Ó×Ö
	int socket_fd;
	sockaddr_in addr;
};

#endif