#include "tcpClient.h"
using namespace std;

tcpClient::tcpClient() {}
tcpClient::~tcpClient() {}

//发送线程
void* sendThread(void *arg)
{
	//取得通信套接字
	int socket_fd = *(int *)arg;
	//发送数据
	string sendData;

	while(1)
	{
		//测试各个请求 
		cout << "please input 1-2: ";
		int num = 0;
		scanf("%d", &num);

		//选择发送的消息类型
		switch(num) {
		case 1:
			{
				//系统验证
				sendData = "{\"req\":\"auth\", \"code\":\"12345678\"}";
				break;
			}

		case 2:
			{
				//请求数据通信
				sendData = "{\"req\":\"data\"}";
				break;
			}

		default:
			{
				cout << "重新输入..." << endl;
			}
		}

		//创建发送数据包
		DATA_PACKAGE p_len;
		//获得包头长度
		p_len.pack_len = sendData.length() + 1;
		cout << "p_len.pack_len: " << p_len.pack_len << endl;

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

		//发送数据到服务器端
		if(send(socket_fd, sendBuf, tolLen, 0) == -1)
		{
			perror("send");
		}
	}
}

void tcpClient::tcpsocket()
{
	//创建socket
	if((socket_fd = socket(AF_INET, SOCK_STREAM,0)) == -1)
	{
		perror("socket");
		exit(-1);
	}

	//准备通信地址  
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8888);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//解决地址被占用问题 
	int reuseaddr = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

	//连接socket
	if(connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		exit(-1);
	}

	//接收客户端发来的数据包长度 
	int package_len = 0;
	char *recvbuf = NULL;

	//新建一个线程用来发送信息
	pthread_t sendID;
		
	if(pthread_create(&sendID, NULL, sendThread, &socket_fd) < 0)
	{
		perror("pthread_create");
	}
	
	//持续接收服务器端发送来的信息
	while(1) {

		//接收数据并解析
		Json::Reader reader;
		Json::Value value;

		int data_len = 0;
		
		//接收数据包头
		if((data_len = recv(socket_fd, &package_len, sizeof(package_len), 0)) == -1)
		{
			perror("recv1");
		}

		cout << "data_len: " << data_len << endl;
		//保护； 如果发送来的信息长度为0则认为服务器已经断开连接
		if(data_len == 0)
		{
			close(socket_fd);
			exit(-1);
		}

		cout << "package_len: " << package_len << endl;

		//创建接收数据缓冲区
		recvbuf = new char[package_len];
		//将缓冲区清零
		memset(recvbuf, 0, package_len);

		//接收数据
		if(recv(socket_fd, recvbuf, package_len, 0) == -1) {
			perror("recv1");
		}

		cout << "recvbuf: " << recvbuf << endl;
		//解析接收到的数据
		if(reader.parse(recvbuf, value))
		{
			cout << "start analysis data..." << endl;

			//判断应答类型
			string resType = value["res"].asString();
			string reqType = value["req"].asString();

			//判断连接是否成功
			if("succ" == resType) {
				cout << "info: " << value["info"].asString() << endl;
			}
			if("failed" == resType)
			{
				cout << "info: " << value["info"].asString() << endl;
			}
			
			//心跳信息回应
			if("heart" == reqType) {
				cout << "req: " << value["req"].asString() << endl;
				
				//心跳回应
				string heartdata = "{\"res\":\"res\"}";
				DATA_PACKAGE h_buf; //长度 
				h_buf.pack_len = heartdata.length() + 1;
				
				int lenheart = sizeof(h_buf) + h_buf.pack_len;
				char sendheart[lenheart];
				memset(sendheart, 0, lenheart);
				memcpy(sendheart, &h_buf, sizeof(h_buf));
				memcpy(sendheart + sizeof(h_buf), heartdata.c_str(), h_buf.pack_len);

				//发送心跳回应给服务器端
				if(send(socket_fd, sendheart, lenheart, 0) == -1)
				{
					perror("send");
				}
			}

			//接收服务器端发送来的数据信息
			if("data" == resType)
			{
				cout << "information: " << value["information"].asString() << endl;
			}
		}

		//回收内存操作
		delete[] recvbuf;
		recvbuf = NULL;

	}
	//关闭socket
	close(socket_fd);
}
