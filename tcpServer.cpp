#include "tcpServer.h"
using namespace std;

tcpServer::tcpServer() 
{
	//初始化包体长度信息
	package_len = 0;

	//初始化接收数据存放区域
	recvBuf = NULL;
}
tcpServer::~tcpServer() {}

//发送信息到客户端
void tcpServer::senddatatoclient(string sendData, int sendsock_i)
{
	//创建发送数据包
	DATA_PACKAGE p_len;
	//获得包头长度
	p_len.pack_len = sendData.length() + 1;

	//获得发送数据包总长度
	int tolLen = sizeof(p_len) + p_len.pack_len;
	//设定发送数据包缓冲区
	char sendBuf[tolLen];
	//将缓冲区清零
	memset(sendBuf, 0, tolLen);
	//将包头信息压入缓冲区
	memcpy(sendBuf, &p_len, sizeof(p_len));
	//将包体信息压入缓冲区
	memcpy(sendBuf + sizeof(p_len), sendData.c_str(), p_len.pack_len);

	if(send(sendsock_i, sendBuf, tolLen, 0) == -1)
	{
		perror("send");
	}

	cout << "send data" << endl;
}

//从客户端接收信息
bool tcpServer::recvdatatoclient(int r_fd, bool test)
{
	//接收数据并解析
	Json::Reader reader;
	Json::Value value;

	int data_len = 0;

	//接收数据包头
	if((data_len = recv(r_fd, &package_len, 4, 0)) == -1)
	{
		perror("recv1");
	}

	cout << "data_len: " << data_len << endl;
	//保护；如果发送来的数据长度为0，则认为客户端断开连接
	if(data_len == 0)
	{
		//关闭socket
		close(r_fd);
		//将r_fd从reads中移除
		FD_CLR(r_fd, &reads);
		//将r_fd从套接字集合总移除
		for(vector<FD_DATA>::iterator del_fd = v_fd_data.begin(); del_fd != v_fd_data.end(); del_fd++)
		{
			if(del_fd->fd == r_fd)
			{
				v_fd_data.erase(del_fd);
				break;
			}
		}
		return false;
	}

	cout << "package_len: " << package_len << endl;

	//创建接收数据缓冲区
	recvBuf =  new char[package_len];
	//清空接收数据缓冲区
	memset(recvBuf, 0, package_len);
	//接收客户端发送过来的数据
	if(recv(r_fd, recvBuf, package_len, 0) == -1)
	{
		perror("recv2");
	}
	
	cout << "recvBuf: " << recvBuf << endl;

	//解析接收到的数据
	if(reader.parse(recvBuf, value))
	{
		cout << "star analysis data..." << endl;

		//判断应答类型
		string resType = value["res"].asString();
		string reqType = value["req"].asString();

		//连接验证
		if("auth" == value["req"].asString())
		{
			if("12345678" == value["code"].asString())
			{
				//返回信息
				sdata = "{\"res\":\"succ\", \"info\":\"auth success\"}";
				senddatatoclient(sdata, r_fd);
				cout << "socket: " << r_fd << " auth success!!" << endl;
				return true;
			}
			else
			{
				//返回信息
				sdata = "{\"res\":\"failed\", \"info\":\"auth failed\"}"; 
				senddatatoclient(sdata, r_fd);
				return false;
			}
		}
		
		//只有当系统验证通过的时候才发送信息
		if(test)
		{
			//发送数据信息
			if("data" == value["req"].asString())
			{
				//返回信息
				sdata = "{\"res\":\"data\", \"information\":\"send data to you\"}";
				senddatatoclient(sdata, r_fd);
			}

			//接收心跳数据包
			if("res" == value["res"].asString())
			{
				return true;
			}
		}

	}

	//回收内存操作
	delete[] recvBuf;
	recvBuf = NULL;

	return false;
}

//创建socket通信
void tcpServer::tcpsocket() {
	//设定select超时时间
	struct timeval timeout;
	
	socklen_t adr_sz;
	
	//套接字最大值和select返回值
	int fd_max, fd_num;
	
	//设定通讯地址
	serv_sock=socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr("127.0.0.1");
	serv_adr.sin_port=htons(8888);
	
	//解决地址被占用的问题
	int reuseaddr = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
	
	//绑定套接字
	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
	{
		perror("bind");
	}

	//监听套接字
	if(listen(serv_sock, 5) == -1)
	{
		perror("listen");
	}
	
	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads);
	fd_max=serv_sock;

	//找到套接字集合中的最大值
	int fd_data_max = 0;
	//判断套接字是否接收到心跳回应信息
	int h_res  = 0;
	
	//与客户端循环通信
	while(1)
	{
		fd_data_max = 0;

		//设置延迟时间为5s
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		cout << "v_fd_data.size: " << v_fd_data.size() << endl;
		//发送心跳信息
		vector<FD_DATA>::iterator h_fd = v_fd_data.begin();
		for(; h_fd != v_fd_data.end(); h_fd++)
		{
			//判断套接字是否验证通过
			if(h_fd->validate)
			{
				//获取系统当前时间
				time_t h_now = time((time_t *)NULL);

				//每隔5s发送心跳信息
				if((h_now - h_fd->test_time) >= 5 && h_res == 0)
				{
					string h_data = "{\"req\":\"heart\"}";
					senddatatoclient(h_data, h_fd->fd);
					h_fd->test_time = h_now;
					//心跳发送标志
					h_res = 1;
					break;
				}
				
				//如果10s没有接收到心跳回应，则断开连接删除套接字
				if(h_now - h_fd->test_time >= 10 && h_res == 1)
				{
					//从套接字集合中删除套接字
					v_fd_data.erase(h_fd);
					//重新给迭代器赋值，防止迭代器失效
					h_fd = v_fd_data.begin();
					//将套接字从reads中删除
					FD_CLR(h_fd->fd, &reads);
					//关闭套接字
					close(h_fd->fd);
				}
			}
		}

		//遍历套接字找到套接字集合中的最大值
		for(vector<FD_DATA>::iterator fd_max_i = v_fd_data.begin(); fd_max_i != v_fd_data.end(); fd_max_i++)
		{
			cout << "all connected socket: " << fd_max_i->fd << endl;
			if(fd_max_i->fd > fd_data_max)
			{
				fd_data_max = fd_max_i->fd;
			}
		}
		if(fd_data_max < serv_sock)
			fd_max = serv_sock;
		else
			fd_max = fd_data_max;

		//可用套接字拷贝
		cpy_reads = reads;
		
		cout << "stop" << endl;
		//判断所有套接字中是否有可读的
		if((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout)) == -1)
		{
			perror("select");
		}

		if(fd_num == 0)
		{
			continue;
		}

		//判断是否有新的客户端连接上来
		if(FD_ISSET(serv_sock, &cpy_reads))
		{
			adr_sz=sizeof(clnt_adr);

			//建立通信
			if((clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz)) == -1)
			{
				perror("accept");
			}

			FD_SET(clnt_sock, &reads);
			if(fd_max<clnt_sock)
				fd_max=clnt_sock;
			printf("connected client: %d \n", clnt_sock);
					
			fd_data.fd = clnt_sock;
			fd_data.validate = false;
			//记录套接字创建时间
			fd_data.test_time = time((time_t *)NULL);	
			
			//将套接字放入容器尾部
			v_fd_data.push_back(fd_data);
		}
		
		//遍历套接字集合
		vector<FD_DATA>::iterator fd_i1 = v_fd_data.begin();
		for(; fd_i1 != v_fd_data.end(); fd_i1++)
		{
			//判断套接字是否可读
			if(FD_ISSET(fd_i1->fd, &cpy_reads))
			{
				if(!fd_i1->validate)
				{
					//套接字验证通过
					if(recvdatatoclient(fd_i1->fd, fd_i1->validate))
					{
						int sign = 0;
						for(vector<FD_DATA>::iterator fd_i2 = v_fd_data.begin(); fd_i2 != v_fd_data.end(); fd_i2++)
						{
							if(fd_i2->validate)
							{
								FD_CLR(fd_i2->fd, &reads);
								close(fd_i2->fd);
								fd_i1->validate = true;
								v_fd_data.erase(fd_i2);
								sign = 1;
								fd_i1 = v_fd_data.begin();
								break;
							}
						}
						if(0 == sign)
							fd_i1->validate = true;
					}
					else
					{
						break;
					}
				}
				else	//套接字已经验证通过
				{
					if(recvdatatoclient(fd_i1->fd, fd_i1->validate))
					{
						//将心跳判断标志置为0
						h_res = 0;
					}
					else
					{
						break;
					}
				}
			}	
		}

		//遍历所有套接字，如果超过验证时间，则关闭这些套接字并将它们从套接字集合中移除
		vector<FD_DATA>::iterator unauth_fd = v_fd_data.begin();
		for(; unauth_fd != v_fd_data.end(); unauth_fd++)
		{
			//如果套接字没有验证过
			if(!unauth_fd->validate)
			{
				printf("client: %d still wait auth\n", unauth_fd->fd);
				//取得系统当前时间
				time_t now = time((time_t *)NULL);
				cout << "wait time: " << now - unauth_fd->test_time << endl;
					
				//判断验证时间是否已经超过20s，如果超过20s则关闭套接字
				if((now - unauth_fd->test_time) > 10)
				{
					cout << "close connection: " << unauth_fd->fd << endl;
					FD_CLR(unauth_fd->fd, &reads);
					close(unauth_fd->fd);
					v_fd_data.erase(unauth_fd);
					unauth_fd = v_fd_data.begin();
				}
			}
		}
	}
	//关闭套接字
	close(serv_sock);
}
