#ifndef _test_mq_h_
#define _test_mq_h_

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <winsock2.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <ws2tcpip.h>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <thread>
#include <mutex>

using namespace std;
#pragma comment (lib,"ws2_32.lib")
const char PRIORITY_LOW = '1';
const char PRIORITY_CENTRE = '2';
const char PRIORITY_HIGH = '3';


typedef long long ll;

mutex msg_after_mutex;
mutex send_number_mutex;
mutex receive_number_mutex;


//连接工厂
class Factory {
private:
	string host_name = "127.0.0.1";//服务器ip
	int port_number = 9999;//端口号

public:
	Factory() {
		//加载winsock库
		WSADATA  Ws;
		//Init Windows Socket（初始化socket资源）
		if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
			exit(1);
		}
	}
	~Factory() {

	}
	void setHost(string s) {
		host_name = s;
	}
	string getHost() {
		return host_name;
	}
	void setPort(int n) {
		port_number = n;
	}
	int getPort() {
		return port_number;
	}
	void close() {
	}
};

//发送消息
class connectionServer {
private:

	const int MSGLEN_LEN = 8;//消息中消息长度的长度
	const int MSGTYPE_LEN = 2;//消息中消息类型的长度
	const int MSGGUID_LEN = 8;//消息中消息guid的长度
	const int FUNDATE_LEN = 32;//消息中函数参数的长度

	int sockfd;//存储socket成功连接返回的编号

	//Factory factory;

	//十进制和十六进制相互转换
	string getNumStr(int x, int n) {//将十进制 x 转换成 n 位十六进制返回
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//将 n 位十六进制 s 转换成十进制返回
		int ans = 0;
		for (int i = 0; i < n; i++) {
			int t = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
			ans = ans * 16 + t;
		}
		return ans;
	}

public:
	connectionServer() {
		//this->factory = factory;
		//init_connection();
	}
	~connectionServer() {
		printf("connectionServer退出了！\n");
	}
	//初始化建立连接
	void init_connection() {
		struct sockaddr_in server_addr;//存储网络通信的地址
		struct hostent* host;//记录主机各种信息（包括但不限于：主机名、地址列表、地址长度）

		//如果IP地址转换失败
		if ((host = gethostbyname("127.0.0.1")) == NULL) {
			fprintf(stderr, "Gethostname error\n");
			exit(1);
		}

		//int portnumber = factory.getPort();//端口号
		int portnumber;//端口号
		//如果字符串转换失败
		if ((portnumber = atoi("9999")) < 0) {
			fprintf(stderr, "Usage: hostname portnumber\a\n");
			exit(1);
		}

		/* 客户程序开始建立 sockfd描述符  */
		//如果socket建立失败
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
			exit(1);
		}

		/* 客户程序填充服务端的资料       */
		memset(&server_addr, 0, sizeof(server_addr));//清空，重置为 0
		server_addr.sin_family = AF_INET;//地址族；AF_INET：使用ipv4的方式进行通信
		server_addr.sin_port = htons(portnumber);//存储端口号（使用网络字节顺序）；
		server_addr.sin_addr = *((struct in_addr*)host->h_addr);//存储IP地址

		/* 客户程序发起连接请求         */
		if (connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Connect Error:%s\a\n", strerror(errno));
			exit(1);
		}
	}
	//发消息
	bool sendMessage(string msg) {
		//在msg头部加msg长度
		msg = getNumStr(msg.size(), MSGLEN_LEN) + msg;


		//发送msg
		if ((send(sockfd, msg.c_str(), msg.size(), 0)) == -1) {
			printf("the net has a error occured..");
			return false;
		}
		return true;
	}
	//获取建立连接的fd
	int getsockfd() {
		return sockfd;
	}
	//关闭连接
	bool close() {
		closesocket(sockfd);
		return true;
	}
};

//发送者
class publishChannel {
private:
	//struct MESSAGE {
	//	ll send_time = 0;//msg发送的时间
	//	string msg;//msg发送的内容
	//	int msg_send_num = 0;//msg发送的次数
	//	connectionServer* connection_ing = NULL;//msg发送所建立的连接
	//};

	const int MSGLEN_LEN = 8;//消息中消息长度的长度
	const int MSGTYPE_LEN = 2;//消息中消息类型的长度
	const int MSGGUID_LEN = 8;//消息中消息guid的长度
	const int FUNDATE_LEN = 32;//消息中函数参数的长度

	connectionServer publish_server;

	//Factory factory;
	//unordered_map<ll, MESSAGE>msg_send_after;//记录已经发送的msg，通过guid访问msg
	//ll msg_guid = 0;//msg的唯一标识
	//int msg_wait_maxtime = 10000;//ack消息等待上限，超时之后重发。(默认10秒)
	//int msg_retry_maxnum = 0;//msg重发次数上限（默认无上限）
	string msg_store;//消息储存器，用于从网络层接收消息
	unordered_set<string>msg_send_after;//存储已发送但未收到ack的消息
	ll send_number = -2;//存储发送消息个数
	ll receive_number = -2;//存储接收消息个数
	ll start_time = 0;;

	//十进制和十六进制相互转换
	string getNumStr(int x, int n) {//将十进制 x 转换成 n 位十六进制返回
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//将 n 位十六进制 s 转换成十进制返回
		int ans = 0;
		for (int i = 0; i < n; i++) {
			int t = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
			ans = ans * 16 + t;
		}
		return ans;
	}
	//补充参数
	string addStr(string s) {
		while (s.size() < FUNDATE_LEN) {
			s += '-';
		}
		return s;
	}

	//发送消息：msg类型、msg内容
	bool sendServer1(int type, string msg) {
		//将  类型 封装到msg的头部
		msg = getNumStr(type, MSGTYPE_LEN) + msg;

		//定义发送类, 发送msg
		publish_server.sendMessage(msg);

		//加锁，将消息放进队列
		msg_after_mutex.lock();
		msg_send_after.emplace(msg);
		msg_after_mutex.unlock();

		//计数器加1
		send_number_mutex.lock();
		send_number++;
		send_number_mutex.unlock();

		return 0;
	}

	//监听消息线程的函数
	void read_acceptth(int sockfd) {
		const ll msg_maxlen = 0xffff;
		while (1) {
			int ack_msglen;//ack消息实际长度
			char ack_msgch[msg_maxlen];//存储ack消息
			//阻塞等待接收ack消息
			if ((ack_msglen = recv(sockfd, ack_msgch, msg_maxlen, 0)) == -1) {
				fprintf(stderr, "read error:%s\n", strerror(errno));
				exit(1);
			}
			ack_msgch[ack_msglen] = '\0';


			string ack_msgstr = ack_msgch;
			memset(ack_msgch, 0, msg_maxlen);

			//将接收的消息放进储存器中
			msg_store += ack_msgstr;

			while (msg_store.size() >= MSGLEN_LEN) {
				//取出消息长度
				int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
				if (msg_len + MSGLEN_LEN <= msg_store.size()) {
					//取出并删除消息
					string msg = msg_store.substr(MSGLEN_LEN, msg_len);
					msg_store = msg_store.substr(MSGLEN_LEN + msg_len);


					//加锁，将msg从队列中删除
					msg_after_mutex.lock();
					msg_send_after.erase(msg);
					msg_after_mutex.unlock();

					receive_number_mutex.lock();
					receive_number++;
					receive_number_mutex.unlock();

				}
				else {
					break;
				}
			}
		}
	}

	void cout_date() {
		while (1) {
			system("cls");
			send_number_mutex.lock();
			printf("已发送 %lld 条消息。\n", send_number);
			send_number_mutex.unlock();

			receive_number_mutex.lock();
			printf("共收到 %lld 条消息。\n", receive_number);
			receive_number_mutex.unlock();

			printf("总用时：%lld ms。\n", clock() - start_time);
			Sleep(1000);
		}
	}

public:
	publishChannel() {

		start_time = clock();
		thread th1(&publishChannel::cout_date, this);
		th1.detach();

		publish_server.init_connection();
		//创建子线程监听消息，监听到ack后关闭连接
		thread th2(&publishChannel::read_acceptth, this, publish_server.getsockfd());
		th2.detach();

		//创建默认交换机
		exchangeDeclare("", "default");
	}
	~publishChannel() {
		printf("publishchannel 退出了！\n");
	}
	//创建(声明)队列
	bool queueDeclare(string queue_name) {
		sendServer1(0, addStr(queue_name));
		return true;
	}
	//创建（声明）交换机
	bool exchangeDeclare(string exchange_name, string exchange_type) {
		sendServer1(1, addStr(exchange_name) + addStr(exchange_type));
		return true;
	}
	//发送端发送消息给MQ
	bool basicPublish(string exchange_name, string routing_key, char priority, string msg) {
		sendServer1(2, addStr(exchange_name) + addStr(routing_key) + msg + priority);
		return true;
	}
	void close() {
		closesocket(publish_server.getsockfd());
	}
};

//接收者
class consumerChannel {
private:
	//Factory factory;
	connectionServer consumer_server;

	unordered_map<ll, string>all_message;//存储接收的所有消息
	unordered_set<ll>receive_msg;//存储接收的所有消息的guid
	string msg_store;//消息储存器

	ll send_number = 0;//存储发送消息个数
	ll receive_number = 0;//存储接收消息个数
	ll start_time = 0;

	const int MSGLEN_LEN = 8;//消息中消息长度的长度
	const int MSGTYPE_LEN = 2;//消息中消息类型的长度
	const int MSGGUID_LEN = 8;//消息中消息guid的长度
	const int FUNDATE_LEN = 32;//消息中函数参数的长度

	//十进制和十六进制相互转换
	string getNumStr(int x, int n) {//将十进制 x 转换成 n 位十六进制返回
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//将 n 位十六进制 s 转换成十进制返回
		int ans = 0;
		for (int i = 0; i < n; i++) {
			int t = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
			ans = ans * 16 + t;
		}
		return ans;
	}
	//补充函数参数
	string addStr(string s) {
		while (s.size() < FUNDATE_LEN) {
			s += '-';
		}
		return s;
	}

	//主动发送msg并阻塞等待ack消息
	bool sendServer(int type, string msg) {

		msg = getNumStr(type, MSGTYPE_LEN) + msg;

		//通过发送类发送函数，发送msg
		consumer_server.sendMessage(msg);

		send_number_mutex.lock();
		send_number++;
		send_number_mutex.unlock();

		//阻塞，监听ack消息
		int sockfd = consumer_server.getsockfd();
		int ack_msglen;//ack消息实际长度
		char ack_msgch[128];//存储ack消息
		//阻塞，等待接收ack消息
		if ((ack_msglen = recv(sockfd, ack_msgch, 128, 0)) == -1) {
			fprintf(stderr, "read error:%s\n", strerror(errno));
			exit(1);
		}
		ack_msgch[ack_msglen] = '\0';

		string ack_msgstr = ack_msgch;
		memset(ack_msgch, 0, 128);

		//将接收的消息放进储存器中
		msg_store += ack_msgstr;

		while (msg_store.size() >= MSGLEN_LEN) {
			//取出消息长度
			int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
			if (msg_len + MSGLEN_LEN <= msg_store.size()) {
				//取出消息，删除消息
				string msg = msg_store.substr(MSGLEN_LEN, msg_len);
				msg_store = msg_store.substr(MSGLEN_LEN + msg_len);

				receive_number_mutex.lock();
				receive_number++;
				receive_number_mutex.unlock();

			}
		}
		return 0;
	}

	//监听接收消息
	void read_accept(int sockfd) {
		const ll msg_maxlen = 0xffff;
		while (1) {
			int msg_len;//msg实际长度
			char msg_char[msg_maxlen];//存储msg


			//阻塞等待接收消息
			if ((msg_len = recv(sockfd, msg_char, msg_maxlen, 0)) == -1) {
				printf("read error:%s\n", strerror(errno));
				exit(1);
			}
			if (msg_len == 0) break;
			msg_char[msg_len] = '\0';


			string msg_str = msg_char;
			memset(msg_char, 0, msg_maxlen);

			//将接收的消息放进储存器中
			msg_store += msg_str;

			while (msg_store.size() >= MSGLEN_LEN) {
				//取出消息长度
				int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
				if (msg_len + MSGLEN_LEN <= msg_store.size()) {
					//取出消息，删除消息
					string msg = msg_store.substr(MSGLEN_LEN, msg_len);
					msg_store = msg_store.substr(MSGLEN_LEN + msg_len);

					//取出guid
					ll msg_guid = getStrNum(msg.substr(0, MSGGUID_LEN), MSGGUID_LEN);

					//避免重复消费
					if (receive_msg.count(msg_guid) == 0) {
						//记录消息guid
						receive_msg.emplace(msg_guid);
						//消费函数
						all_message[msg_guid] = msg.substr(MSGGUID_LEN);
					}

					receive_number_mutex.lock();
					receive_number++;
					receive_number_mutex.unlock();

					//发送ack消息
					string consumerAck = getNumStr(132, MSGTYPE_LEN) + getNumStr(msg_guid, MSGGUID_LEN);
					consumer_server.sendMessage(consumerAck);

					send_number_mutex.lock();
					send_number++;
					send_number_mutex.unlock();

				}
				else {
					break;
				}
			}
		}
	}

	void cout_date() {
		while (1) {
			system("cls");
			send_number_mutex.lock();
			printf("已发送 %lld 条消息。\n", send_number);
			send_number_mutex.unlock();

			receive_number_mutex.lock();
			printf("共收到 %lld 条消息。\n", receive_number);
			receive_number_mutex.unlock();

			printf("总用时：%lld ms。\n", clock() - start_time);
			Sleep(1000);
		}
	}


public:
	consumerChannel() {
		start_time = clock();
		thread th(&consumerChannel::cout_date, this);
		th.detach();

		consumer_server.init_connection();

		exchangeDeclare("", "default");
	}
	~consumerChannel() {
		printf("consuemrChannel退出了！\n");
	}

	//创建(声明)队列
	bool queueDeclare(string queue_name) {
		sendServer(128, addStr(queue_name));
		return 0;
	}
	//创建（声明）交换机
	bool exchangeDeclare(string exchange_name, string exchange_type) {
		sendServer(129, addStr(exchange_name) + addStr(exchange_type));
		return 0;
	}
	//绑定队列与交换机
	bool queueBind(string queue_name, string exchange_name, string routing_key) {
		sendServer(130, addStr(queue_name) + addStr(exchange_name) + addStr(routing_key));
		return 0;
	}
	//发送需要监听队列中的消息，创建线程，开始监听
	bool basicConsumer(string queue_name) {
		sendServer(131, addStr(queue_name));

		//创建子线程，接收server端消息
		thread th(&consumerChannel::read_accept, this, consumer_server.getsockfd());
		th.detach();
		return 0;
	}
	bool basicQos(int num) {
		sendServer(131, getNumStr(num, 4));
		return 0;
	}
	void close() {
		closesocket(consumer_server.getsockfd());
	}
};
#endif // !_test_mq_h_
