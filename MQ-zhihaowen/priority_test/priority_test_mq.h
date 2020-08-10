#ifndef _priority_test_mq_h_
#define _priority_test_mq_h_

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

mutex cout_mutex;
mutex msg_after_mutex;
mutex send_number_mutex;
mutex receive_number_mutex;


//���ӹ���
class Factory {
private:
	string host_name = "127.0.0.1";//������ip
	int port_number = 9999;//�˿ں�

public:
	Factory() {
		//����winsock��
		WSADATA  Ws;
		//Init Windows Socket����ʼ��socket��Դ��
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

//������Ϣ
class connectionServer {
private:

	const int MSGLEN_LEN = 8;//��Ϣ����Ϣ���ȵĳ���
	const int MSGTYPE_LEN = 2;//��Ϣ����Ϣ���͵ĳ���
	const int MSGGUID_LEN = 8;//��Ϣ����Ϣguid�ĳ���
	const int FUNDATE_LEN = 32;//��Ϣ�к��������ĳ���

	int sockfd;//�洢socket�ɹ����ӷ��صı��

	//Factory factory;

	//ʮ���ƺ�ʮ�������໥ת��
	string getNumStr(int x, int n) {//��ʮ���� x ת���� n λʮ�����Ʒ���
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//�� n λʮ������ s ת����ʮ���Ʒ���
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
		cout << "connectionServer�˳��ˣ�" << endl;
	}
	//��ʼ����������
	void init_connection() {
		struct sockaddr_in server_addr;//�洢����ͨ�ŵĵ�ַ
		struct hostent* host;//��¼����������Ϣ�������������ڣ�����������ַ�б���ַ���ȣ�

		//���IP��ַת��ʧ��
		if ((host = gethostbyname("127.0.0.1")) == NULL) {
			fprintf(stderr, "Gethostname error\n");
			exit(1);
		}

		//int portnumber = factory.getPort();//�˿ں�
		int portnumber;//�˿ں�
		//����ַ���ת��ʧ��
		if ((portnumber = atoi("9999")) < 0) {
			fprintf(stderr, "Usage: hostname portnumber\a\n");
			exit(1);
		}

		/* �ͻ�����ʼ���� sockfd������  */
		//���socket����ʧ��
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
			exit(1);
		}

		/* �ͻ�����������˵�����       */
		memset(&server_addr, 0, sizeof(server_addr));//��գ�����Ϊ 0
		server_addr.sin_family = AF_INET;//��ַ�壻AF_INET��ʹ��ipv4�ķ�ʽ����ͨ��
		server_addr.sin_port = htons(portnumber);//�洢�˿ںţ�ʹ�������ֽ�˳�򣩣�
		server_addr.sin_addr = *((struct in_addr*)host->h_addr);//�洢IP��ַ

		/* �ͻ���������������         */
		if (connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Connect Error:%s\a\n", strerror(errno));
			exit(1);
		}
	}
	//����Ϣ
	bool sendMessage(string msg) {
		//��msgͷ����msg����
		msg = getNumStr(msg.size(), MSGLEN_LEN) + msg;

		cout_mutex.lock();
		cout << "������ʵ����: ��" << msg << "��\n";
		cout_mutex.unlock();

		//����msg
		if ((send(sockfd, msg.c_str(), msg.size(), 0)) == -1) {
			printf("the net has a error occured..");
			return false;
		}
		return true;
	}
	//��ȡ�������ӵ�fd
	int getsockfd() {
		return sockfd;
	}
	//�ر�����
	bool close() {
		closesocket(sockfd);
		return true;
	}
};

//������
class publishChannel {
private:
	//struct MESSAGE {
	//	ll send_time = 0;//msg���͵�ʱ��
	//	string msg;//msg���͵�����
	//	int msg_send_num = 0;//msg���͵Ĵ���
	//	connectionServer* connection_ing = NULL;//msg����������������
	//};

	const int MSGLEN_LEN = 8;//��Ϣ����Ϣ���ȵĳ���
	const int MSGTYPE_LEN = 2;//��Ϣ����Ϣ���͵ĳ���
	const int MSGGUID_LEN = 8;//��Ϣ����Ϣguid�ĳ���
	const int FUNDATE_LEN = 32;//��Ϣ�к��������ĳ���

	connectionServer publish_server;

	//Factory factory;
	//unordered_map<ll, MESSAGE>msg_send_after;//��¼�Ѿ����͵�msg��ͨ��guid����msg
	//ll msg_guid = 0;//msg��Ψһ��ʶ
	//int msg_wait_maxtime = 10000;//ack��Ϣ�ȴ����ޣ���ʱ֮���ط���(Ĭ��10��)
	//int msg_retry_maxnum = 0;//msg�ط��������ޣ�Ĭ�������ޣ�
	string msg_store;//��Ϣ�����������ڴ�����������Ϣ
	unordered_set<string>msg_send_after;//�洢�ѷ��͵�δ�յ�ack����Ϣ
	ll send_number = -2;//�洢������Ϣ����
	ll receive_number = -2;//�洢������Ϣ����
	ll start_time = 0;;

	//ʮ���ƺ�ʮ�������໥ת��
	string getNumStr(int x, int n) {//��ʮ���� x ת���� n λʮ�����Ʒ���
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//�� n λʮ������ s ת����ʮ���Ʒ���
		int ans = 0;
		for (int i = 0; i < n; i++) {
			int t = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
			ans = ans * 16 + t;
		}
		return ans;
	}
	//�������
	string addStr(string s) {
		while (s.size() < FUNDATE_LEN) {
			s += '-';
		}
		return s;
	}

	//������Ϣ��msg���͡�msg����
	bool sendServer1(int type, string msg) {
		////��ȡmsg��guid
		//ll guidt = msg_guid++;
		////��  ���� guid  ��װ��msg��ͷ��
		//msg = getNumStr(type, MSGTYPE_LEN) + getNumStr(guidt, MSGGUID_LEN) + msg;
		msg = getNumStr(type, MSGTYPE_LEN) + msg;

		cout_mutex.lock();
		cout << "����: ��" << msg << "��" << endl;
		cout_mutex.unlock();

		//���巢����, ����msg
		publish_server.sendMessage(msg);


		////��¼msg��Ϣ������ʱ�䣬�������ӣ����͵����ݣ����ʹ�����
		//MESSAGE guidt_msg;
		//guidt_msg.send_time = clock();
		//guidt_msg.connection_ing = publish_server;
		//guidt_msg.msg = msg;
		//guidt_msg.msg_send_num++;
		////������ͨ��guid����Ϣ�Ž��ط�����
		//msg_after_mutex.lock();
		//msg_send_after[guidt] = guidt_msg;
		//msg_after_mutex.unlock();

		//����������Ϣ�Ž�����
		msg_after_mutex.lock();
		msg_send_after.emplace(msg);
		msg_after_mutex.unlock();

		//��������1
		send_number_mutex.lock();
		send_number++;
		send_number_mutex.unlock();

		return 0;
	}

	//������Ϣ�̵߳ĺ���
	void read_acceptth(int sockfd) {
		const ll msg_maxlen = 0xffff;
		while (1) {
			int ack_msglen;//ack��Ϣʵ�ʳ���
			char ack_msgch[msg_maxlen];//�洢ack��Ϣ
			//�����ȴ�����ack��Ϣ
			if ((ack_msglen = recv(sockfd, ack_msgch, msg_maxlen, 0)) == -1) {
				fprintf(stderr, "read error:%s\n", strerror(errno));
				exit(1);
			}
			ack_msgch[ack_msglen] = '\0';

			cout_mutex.lock();
			cout << "�յ���ʵ����: ��" << ack_msgch << "��\n";
			cout_mutex.unlock();

			string ack_msgstr = ack_msgch;
			memset(ack_msgch, 0, msg_maxlen);

			//�����յ���Ϣ�Ž���������
			msg_store += ack_msgstr;

			while (msg_store.size() >= MSGLEN_LEN) {
				//ȡ����Ϣ����
				int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
				if (msg_len + MSGLEN_LEN <= msg_store.size()) {
					//ȡ����ɾ����Ϣ
					string msg = msg_store.substr(MSGLEN_LEN, msg_len);
					msg_store = msg_store.substr(MSGLEN_LEN + msg_len);

					cout_mutex.lock();
					cout << "�յ� ��" << msg << "�� ��ack��Ϣ" << endl;
					cout_mutex.unlock();

					////����Ϣ�л�ȡguid
					//ll ack_guid = getStrNum(msg, MSGGUID_LEN);
					////��������msg���ط�������ɾ��
					//msg_after_mutex.lock();
					//msg_send_after.erase(ack_guid);//�����߳��������ⲿ�������豣֤����publishchannelһֱ����
					//msg_after_mutex.unlock();

					//��������msg�Ӷ�����ɾ��
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
	////��ʱ�ط�
	//void timeoutRetryth() {
	//	while (1) {
	//		ll now_time = clock();
	//		msg_after_mutex.lock();
	//		//���������ѷ��͵�msg
	//		for (auto it = msg_send_after.begin(); it != msg_send_after.end(); it++) {
	//			if (now_time - it->second.send_time > msg_wait_maxtime //if��ʱ
	//				&& (it->second.msg_send_num < msg_retry_maxnum //if�ط�����δ�ﵽ����
	//					|| msg_retry_maxnum == 0)) { //if���ط�����
	//				cout_mutex.lock();
	//				cout << "���ͳ�ʱ�ط���Ϣ: ��" << it->second.msg << "��" << endl;
	//				cout_mutex.unlock();
	//				//������Ϣ
	//				it->second.connection_ing->sendMessage(it->second.msg);
	//				//���·���ʱ��/���ʹ�������vis���
	//				it->second.send_time = now_time;
	//				it->second.msg_send_num++;
	//			}
	//		}
	//		msg_after_mutex.unlock();
	//		Sleep(msg_wait_maxtime / 10);
	//	}
	//}

	void cout_date() {
		while (1) {
			system("cls");
			send_number_mutex.lock();
			cout << "�ѷ��� " << send_number << " ����Ϣ��" << endl;
			send_number_mutex.unlock();

			receive_number_mutex.lock();
			cout << "���յ� " << receive_number << " ��ack��Ϣ���ݡ�" << endl;
			receive_number_mutex.unlock();

			cout << "����ʱ��" << clock() - start_time << " ms��" << endl;
			Sleep(1000);
		}
	}

public:
	publishChannel() {
		////�������̣߳����г�ʱ�ط�����
		//thread th(&publishChannel::timeoutRetryth, this);
		//th.detach();

		//start_time = clock();
		//thread th(&publishChannel::cout_date, this);
		//th.detach();



		publish_server.init_connection();
		//�������̼߳�����Ϣ��������ack��ر�����
		thread th(&publishChannel::read_acceptth, this, publish_server.getsockfd());
		th.detach();

		//����Ĭ�Ͻ�����
		exchangeDeclare("", "default");
	}
	~publishChannel() {
		cout << "publishchannel �˳��ˣ�" << endl;
	}
	//����(����)����
	bool queueDeclare(string queue_name) {
		sendServer1(0, addStr(queue_name));
		return true;
	}
	//������������������
	bool exchangeDeclare(string exchange_name, string exchange_type) {
		sendServer1(1, addStr(exchange_name) + addStr(exchange_type));
		return true;
	}
	//���Ͷ˷�����Ϣ��MQ
	bool basicPublish(string exchange_name, string routing_key, string priority, string msg) {
		sendServer1(2, addStr(exchange_name) + addStr(routing_key) + msg + priority);
		return true;
	}
	////�����ش����ʱ��
	//bool setRetryTime(int n) {
	//	msg_wait_maxtime = n;
	//	return true;
	//}
	////�����ط�����
	//bool setRetryNum(int n) {
	//	msg_retry_maxnum = n;
	//	return true;
	//}
	void close() {
		closesocket(publish_server.getsockfd());
	}
};

//������
class consumerChannel {
private:
	//Factory factory;
	connectionServer consumer_server;

	unordered_map<ll, string>all_message;//�洢���յ�������Ϣ
	unordered_set<ll>receive_msg;//�洢���յ�������Ϣ��guid
	string msg_store;//��Ϣ������

	ll send_number = 0;//�洢������Ϣ����
	ll receive_number = 0;//�洢������Ϣ����
	ll start_time = 0;

	const int MSGLEN_LEN = 8;//��Ϣ����Ϣ���ȵĳ���
	const int MSGTYPE_LEN = 2;//��Ϣ����Ϣ���͵ĳ���
	const int MSGGUID_LEN = 8;//��Ϣ����Ϣguid�ĳ���
	const int FUNDATE_LEN = 32;//��Ϣ�к��������ĳ���

	//ʮ���ƺ�ʮ�������໥ת��
	string getNumStr(int x, int n) {//��ʮ���� x ת���� n λʮ�����Ʒ���
		string ans;
		for (int i = 0; i < n; i++) {
			int t = x % 16;
			ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
			x /= 16;
		}
		return ans;
	}
	int getStrNum(string s, int n) {//�� n λʮ������ s ת����ʮ���Ʒ���
		int ans = 0;
		for (int i = 0; i < n; i++) {
			int t = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
			ans = ans * 16 + t;
		}
		return ans;
	}
	//���亯������
	string addStr(string s) {
		while (s.size() < FUNDATE_LEN) {
			s += '-';
		}
		return s;
	}

	//��������msg�������ȴ�ack��Ϣ
	bool sendServer(int type, string msg) {

		msg = getNumStr(type, MSGTYPE_LEN) + msg;

		cout_mutex.lock();
		cout << "����: ��" << msg << "��" << endl;
		cout_mutex.unlock();

		//ͨ�������෢�ͺ���������msg
		consumer_server.sendMessage(msg);

		send_number_mutex.lock();
		send_number++;
		send_number_mutex.unlock();

		//����������ack��Ϣ
		int sockfd = consumer_server.getsockfd();
		int ack_msglen;//ack��Ϣʵ�ʳ���
		char ack_msgch[128];//�洢ack��Ϣ
		//�������ȴ�����ack��Ϣ
		if ((ack_msglen = recv(sockfd, ack_msgch, 128, 0)) == -1) {
			fprintf(stderr, "read error:%s\n", strerror(errno));
			exit(1);
		}
		ack_msgch[ack_msglen] = '\0';

		cout_mutex.lock();
		cout << "�յ���ʵ����: ��" << ack_msgch << "��\n";
		cout_mutex.unlock();


		string ack_msgstr = ack_msgch;
		memset(ack_msgch, 0, 128);

		//�����յ���Ϣ�Ž���������
		msg_store += ack_msgstr;

		while (msg_store.size() >= MSGLEN_LEN) {
			//ȡ����Ϣ����
			int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
			if (msg_len + MSGLEN_LEN <= msg_store.size()) {
				//ȡ����Ϣ��ɾ����Ϣ
				string msg = msg_store.substr(MSGLEN_LEN, msg_len);
				msg_store = msg_store.substr(MSGLEN_LEN + msg_len);

				//���
				cout_mutex.lock();
				cout << "�յ� ��" << msg << "�� ��ack��Ϣ\n";
				cout_mutex.unlock();

				receive_number_mutex.lock();
				receive_number++;
				receive_number_mutex.unlock();

			}
		}
		return 0;
	}

	//����������Ϣ
	void read_accept(int sockfd) {
		const ll msg_maxlen = 0xffff;
		while (1) {
			int msg_len;//msgʵ�ʳ���
			char msg_char[msg_maxlen];//�洢msg

			cout_mutex.lock();
			cout << "����" << sockfd << "����Ϣ\n";
			cout_mutex.unlock();

			//�����ȴ�������Ϣ
			if ((msg_len = recv(sockfd, msg_char, msg_maxlen, 0)) == -1) {
				printf("read error:%s\n", strerror(errno));
				exit(1);
			}
			if (msg_len == 0) break;
			msg_char[msg_len] = '\0';

			cout_mutex.lock();
			cout << "�յ�pub���͵���ʵ����: ��" << msg_char << "��\n";
			cout_mutex.unlock();

			string msg_str = msg_char;
			memset(msg_char, 0, msg_maxlen);

			//�����յ���Ϣ�Ž���������
			msg_store += msg_str;

			while (msg_store.size() >= MSGLEN_LEN) {
				//ȡ����Ϣ����
				int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
				if (msg_len + MSGLEN_LEN <= msg_store.size()) {
					//ȡ����Ϣ��ɾ����Ϣ
					string msg = msg_store.substr(MSGLEN_LEN, msg_len);
					msg_store = msg_store.substr(MSGLEN_LEN + msg_len);

					//ȡ��guid
					ll msg_guid = getStrNum(msg.substr(0, MSGGUID_LEN), MSGGUID_LEN);

					//�����ظ�����
					if (receive_msg.count(msg_guid) == 0) {
						//��¼��Ϣguid
						receive_msg.emplace(msg_guid);
						//���Ѻ���
						all_message[msg_guid] = msg.substr(MSGGUID_LEN);
					}

					receive_number_mutex.lock();
					receive_number++;
					receive_number_mutex.unlock();



					Sleep(5000);



					cout_mutex.lock();
					cout << "���ͣ���" << msg << "�� ��ack" << endl;
					cout_mutex.unlock();

					//����ack��Ϣ
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

	////���Ѻ���
	//void consumerMsg(string msg) {
	//	//cout_mutex.lock();
	//	cout << "ԭʼ���Ѻ���" << endl;
	//	cout << "�յ�pub���͵���Ϣ����" << msg << "��" << endl;
	//	cout << clock() << ": ��ʼ������Ϣ: ��" << msg.substr(MSGGUID_LEN) << "��" << endl;
	//	//Sleep(rand() % 1000 + 1000);
	//	cout << clock() << ": ����������Ϣ: ��" << msg.substr(MSGGUID_LEN) << "��" << endl;
	//	//cout_mutex.unlock();
	//}
	void cout_date() {
		while (1) {
			system("cls");
			send_number_mutex.lock();
			cout << "�ѷ��� " << send_number << " ����Ϣ��" << endl;
			send_number_mutex.unlock();

			receive_number_mutex.lock();
			cout << "���յ� " << receive_number << " ��ack��Ϣ���ݡ�" << endl;
			receive_number_mutex.unlock();

			cout << "����ʱ��" << clock() - start_time << " ms��" << endl;
			Sleep(1000);
		}
	}


public:
	consumerChannel() {
		//this->factory = factory;

		//start_time = clock();
		//thread th(&consumerChannel::cout_date, this);
		//th.detach();

		consumer_server.init_connection();

		exchangeDeclare("", "default");
	}
	~consumerChannel() {
		cout << "consuemrChannel�˳��ˣ�" << endl;
	}

	//����(����)����
	bool queueDeclare(string queue_name) {
		sendServer(128, addStr(queue_name));
		return 0;
	}
	//������������������
	bool exchangeDeclare(string exchange_name, string exchange_type) {
		sendServer(129, addStr(exchange_name) + addStr(exchange_type));
		return 0;
	}
	//�󶨶����뽻����
	bool queueBind(string queue_name, string exchange_name, string routing_key) {
		sendServer(130, addStr(queue_name) + addStr(exchange_name) + addStr(routing_key));
		return 0;
	}
	//������Ҫ���������е���Ϣ�������̣߳���ʼ����
	bool basicConsumer(string queue_name) {
		sendServer(131, addStr(queue_name));

		//�������̣߳�����server����Ϣ
		thread th(&consumerChannel::read_accept, this, consumer_server.getsockfd());
		th.detach();
		return 0;
	}
	void close() {
		closesocket(consumer_server.getsockfd());
	}
};
#endif // !_priority_test_mq_h_
