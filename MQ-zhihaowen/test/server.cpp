#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <ws2tcpip.h>
#include <thread>
#pragma comment (lib,"ws2_32.lib")

#define LISTEN_PORT 9999//�˿ں�
#define LIATEN_BACKLOG 32
using namespace std;
typedef long long ll;

struct MESSAGE {
	ll send_time = 0;//����ʱ��
	string msg;//��Ϣ����
	//int msg_send_num = 0;//��Ϣ���͵Ĵ���
	//int msg_send_maxnum = 10;//��Ϣ���ʹ�������
};
struct Queue {
	queue<pair<ll, string> >msg_low_wait;//�ȴ����͵�msg���У���¼guid�Ͷ�Ӧ��msg������
	queue<pair<ll, string> >msg_cen_wait;//�ȴ����͵�msg���У���¼guid�Ͷ�Ӧ��msg������
	queue<pair<ll, string> >msg_high_wait;//�ȴ����͵�msg���У���¼guid�Ͷ�Ӧ��msg������
	unordered_map<ll, MESSAGE> msg_after;//�Ѿ����͵�δackȷ�ϵ�msg���У�ͨ��guid����msg������
	struct bufferevent* consumer_bev = NULL;//�����ߵ�����
};
struct Exchange {
	string exchange_type;//���������ԣ�����չ
	unordered_map<string, vector<string> > all_queue_name;//���ж��е����֣�ͨ��·�ɼ����ʶ�����
};

ll server_msg_guid = 0;//ÿ��msg��ӦΨһ��guid
//unordered_set<string>receive_msg;//�յ���������Ϣ
//mutex receive_msg_mutex;//receive_msg��
unordered_map<string, Queue>all_queue;//���ж��У�ͨ�����������ʶ���
mutex all_queue_mutex;
unordered_map<string, Exchange>all_exchange;//���н�������ͨ�������������ʽ�����
unordered_map<ll, string> guid_queue_name;//ͨ��guid���ʷ���msg�Ķ�����
unordered_map<struct bufferevent*, string>bev_queue_name;//ͨ��bev������֮�������ӵĶ�����
int ack_max_time = 10000;//��Ϣ�ȴ����ޣ��ﵽ֮���ط���
string msg_store;//��Ϣ������
int consumer_msg_maxnum = 1;//������������Ϣ�����ޣ�0Ϊ������


const int MSGLEN_LEN = 8;//��Ϣ����Ϣ���ȵĳ���
const int MSGTYPE_LEN = 2;//��Ϣ����Ϣ���͵ĳ���
const int MSGGUID_LEN = 8;//��Ϣ����Ϣguid�ĳ���
const int FUNDATE_LEN = 32;//��Ϣ�к��������ĳ���

/**************************************��������********************************************/
//ʮ���ƺ�ʮ�������໥ת��
string getNumStr(int x, int n);
int getStrNum(string s, int n);

//accept�ص�����
void do_accept_cb(evutil_socket_t listener, short event, void* arg);
//read �ص�����
void read_cb(struct bufferevent* bev, void* arg);
//write �ص�����
void write_cb(struct bufferevent* bev, void* arg);
//error �ص�����
void error_cb(struct bufferevent* bev, short event, void* arg);
//������Ϣ
void sendMsg(struct bufferevent* bev, string msg);
//��ʱ�ط�
void timeoutRetryth();

//�ͻ��˽ӿ�
bool queueDeclare(string queue_name);
bool exchangeDeclare(string exchange_name, string exchange_type);
bool queueBind(string queue_name, string exchange_name, string routing_key);
bool basicPublish(string exchange_name, string routing_key, string msg);
bool basicConsumer(string queue_name, struct bufferevent* bev);
bool basicQos(string queue_name, int num);

/**************************************������********************************************/
//ʮ���ƺ�ʮ�������໥ת��
string getNumStr(int x, int n) {
	string ans;
	for (int i = 0; i < n; i++) {
		int t = x % 16;
		ans = (char)(t > 9 ? (t - 10 + 'A') : (t + '0')) + ans;
		x /= 16;
	}
	return ans;
}
int getStrNum(string s, int n) {
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

//accept�ص�����
void do_accept_cb(evutil_socket_t listener, short event, void* arg) {
	//�����event_baseָ��
	struct event_base* base = (struct event_base*)arg;
	//socket������
	evutil_socket_t fd;

	//������ַ
	struct sockaddr_in sin;
	//��ַ��������
	socklen_t slen = sizeof(sin);

	//���տͻ���
	fd = accept(listener, (struct sockaddr*)&sin, &slen);
	if (fd < 0) {
		cout << "accept error!" << endl;
		return;
	}
	cout << "--------------------------------------" << endl;
	cout << "Client connection successful: fd = " << fd << endl;

	//ע��һ��bufferevent_socket_new�¼� //����bufferevent����
	struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

	//���ûص�����
	bufferevent_setcb(bev, read_cb, write_cb, error_cb, arg);
	//���ø��¼�������
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);
}
//read �ص�����
void read_cb(struct bufferevent* bev, void* arg) {
#define MAX_LINE 1024
	char line[MAX_LINE + 1];
	//ͨ���������bev�ҵ�socket fd
	evutil_socket_t fd = bufferevent_getfd(bev);
	int n = bufferevent_read(bev, line, MAX_LINE);
	if (n == 0) {
		return;
	}
	line[n] = '\0';
	//cout << "fd=" << fd << ", �յ���ʵ����: ��" << line << "��\n";

	string linestr = line;
	msg_store += linestr;

	while (msg_store.size() > MSGLEN_LEN) {
		//ȡ����Ϣ����
		int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
		if (msg_len + MSGLEN_LEN <= msg_store.size()) {
			//ȡ����Ϣ��ɾ����Ϣ
			string msg = msg_store.substr(MSGLEN_LEN, msg_len);
			msg_store = msg_store.substr(MSGLEN_LEN + msg_len);
			cout << "fd=" << fd << ", �յ�: ��" << msg << "��\n";

			//ȡ������
			int  msg_type = getStrNum(msg.substr(0, MSGTYPE_LEN), MSGTYPE_LEN);
			////ȡ��guid
			//ll msg_guid = getStrNum(msg.substr(MSGTYPE_LEN, MSGGUID_LEN), MSGGUID_LEN);

			ll msg_guid;
			//����ack
			if (msg_type == 132) {
				//ȡ��guid
				msg_guid = getStrNum(msg.substr(MSGTYPE_LEN, MSGGUID_LEN), MSGGUID_LEN);
			}
			else {
				cout << "fd=" << fd << ", ����: ��" << msg << "�� ��Ack" << endl;
				sendMsg(bev, msg);
			}


			switch (msg_type) {
			case 0:
			case 128://��������
				queueDeclare(msg.substr(MSGTYPE_LEN));
				break;
			case 1:
			case 129://����������
				exchangeDeclare(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN));
				break;
			case 2://server����publish����Ϣ
				basicPublish(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + 2 * FUNDATE_LEN));
				break;
			case 130://�󶨽������Ͷ���
				queueBind(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + 2 * FUNDATE_LEN));
				break;
			case 131://consumer�˼�������
				basicConsumer(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), bev);
				break;
			case 132://server����consumer��ack
				all_queue_mutex.lock();
				all_queue[guid_queue_name[msg_guid]].msg_after.erase(msg_guid);
				all_queue_mutex.unlock();
				write_cb(bev, arg);
				break;
			case 133://���ö�������
				basicQos(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), getStrNum(msg.substr(MSGTYPE_LEN + FUNDATE_LEN), 4));
				break;
			case 134://����ack
				break;
			}
		}
		else {
			break;
		}
	}
}
//write �ص�����
void write_cb(struct bufferevent* bev, void* arg) {
	//ѭ�����ж��У�����������Ϣ��ֱ����ǰ���в��ܷ�����Ϣ������һ������
	all_queue_mutex.lock();
	for (auto it = all_queue.begin(); it != all_queue.end(); ) {

		if (it->second.consumer_bev != NULL//if���д��ڼ�����������
			&& (it->second.msg_after.size() < consumer_msg_maxnum
				|| consumer_msg_maxnum == 0)) {//������û�дﵽ��������
			if (it->second.msg_high_wait.size()) {
				//��msg��wait������ȡ��
				pair<ll, string>msg_front = it->second.msg_high_wait.front();
				it->second.msg_high_wait.pop();

				//�Ž�after����
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", ����: ��" << send_msg << "��" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
			if (it->second.msg_cen_wait.size()) {
				//��msg��wait������ȡ��
				pair<ll, string>msg_front = it->second.msg_cen_wait.front();
				it->second.msg_cen_wait.pop();

				//�Ž�after����
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", ����: ��" << send_msg << "��" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
			if (it->second.msg_low_wait.size()) {
				//��msg��wait������ȡ��
				pair<ll, string>msg_front = it->second.msg_low_wait.front();
				it->second.msg_low_wait.pop();

				//�Ž�after����
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", ����: ��" << send_msg << "��" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
		}
		it++;
	}
	all_queue_mutex.unlock();
}
//error�ص�����
void error_cb(struct bufferevent* bev, short event, void* arg) {
	//ͨ���������bev�ҵ�socket fd
	evutil_socket_t fd = bufferevent_getfd(bev);
	cout << "fd = " << fd << "��";
	if (event & BEV_EVENT_TIMEOUT) {
		cout << "��ʱ��" << endl;//if bufferevent_set_timeouts() called
	}
	else if (event & BEV_EVENT_EOF) {
		cout << "���ӹرգ�" << endl;
	}
	else if (event & BEV_EVENT_ERROR) {
		cout << "��������" << endl;
	}

	//ɾ��������������
	string queue_name = bev_queue_name[bev];
	bev_queue_name.erase(bev);
	all_queue_mutex.lock();
	if (all_queue.count(queue_name)) {
		all_queue[queue_name].consumer_bev = NULL;
	}
	all_queue_mutex.unlock();

	//�ͷ�Bufferevent
	bufferevent_free(bev);
}
//�ӱ�ͷ��������Ϣ
void sendMsg(struct bufferevent* bev, string msg) {
	evutil_socket_t fd = bufferevent_getfd(bev);

	msg = getNumStr(msg.size(), MSGLEN_LEN) + msg;
	cout << "fd=" << fd << ", ������ʵ����: ��" << msg << "��\n";
	bufferevent_write(bev, msg.c_str(), msg.size());
}
//��ʱ�ط�
void timeoutRetryth() {
	while (1) {
		//��ʱ�ط�
		all_queue_mutex.lock();
		auto all_queuet = all_queue;
		all_queue_mutex.unlock();

		int cnt = 0;
		for (auto it = all_queuet.begin(); it != all_queuet.end(); it++) {
			if (it->second.consumer_bev != NULL//���д��ڼ�����������
				&& it->second.msg_after.size()) {//�ط������д�������
				ll now_time = clock();
				for (auto x : it->second.msg_after) {
					if (now_time - x.second.send_time > ack_max_time) {
						cout << "���̷߳��ͳ�ʱ�ط���Ϣ: ��" << x.second.msg << "��" << endl;

						sendMsg(it->second.consumer_bev, x.second.msg);
						cnt++;
					}
				}
			}
		}
		Sleep(ack_max_time / 10);
	}
}

//�ͻ��˽ӿ�
bool queueDeclare(string queue_name) {
	all_queue_mutex.lock();
	if (all_queue.count(queue_name)) {
		all_queue_mutex.unlock();
		return false;
	}
	else {
		all_queue[queue_name];
		all_queue_mutex.unlock();
		return queueBind(addStr(queue_name), addStr(""), addStr(queue_name));
	}
}
bool exchangeDeclare(string exchange_name, string exchange_type) {
	if (all_exchange.count(exchange_name)) {
		return false;
	}
	else {
		all_exchange[exchange_name].exchange_type = exchange_type;
		return true;
	}
}
bool queueBind(string queue_name, string exchange_name, string routing_key) {
	if (all_exchange.count(exchange_name)) {
		all_exchange[exchange_name].all_queue_name[routing_key].push_back(queue_name);
		return true;
	}
	else {
		return false;
	}
}
bool basicPublish(string exchange_name, string routing_key, string msg) {
	char priority = msg.back();
	msg.pop_back();
	for (auto x : all_exchange[exchange_name].all_queue_name[routing_key]) {
		//��ȡ���͵�consumer����Ϣ��guid
		ll server_guid_t = server_msg_guid++;

		//��guid��Ӧ��queue_name��¼����������ack����
		guid_queue_name[server_guid_t] = x;

		//����Ϣ�Ž�wait����
		all_queue_mutex.lock();
		switch (priority) {
		case '1':
			all_queue[x].msg_low_wait.push({ server_guid_t,msg });
			break;
		case '2':
			all_queue[x].msg_cen_wait.push({ server_guid_t,msg });
			break;
		case '3':
			all_queue[x].msg_high_wait.push({ server_guid_t,msg });
			break;
		}
		all_queue_mutex.unlock();
	}
	return 0;
}
bool basicConsumer(string queue_name, struct bufferevent* bev) {
	all_queue_mutex.lock();
	if (all_queue[queue_name].consumer_bev == NULL) {
		all_queue[queue_name].consumer_bev = bev;
		bev_queue_name[bev] = queue_name;
		all_queue_mutex.unlock();
		return true;
	}
	else {
		all_queue_mutex.unlock();
		return false;
	}
}
bool basicQos(string queue_name, int num) {
	consumer_msg_maxnum = num;
	return 0;
}


int main() {

	//����winsock��
	WSADATA  Ws;
	//Init Windows Socket����ʼ��socket��Դ��
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
		return -1;
	}

	//�洢socket�ɹ����ӷ��صı��
	evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
	assert(listener > 0);//����Ϊ�ٵ�ʱ����ֹ����ִ��
	//�˿�����
	evutil_make_listen_socket_reuseable(listener);

	//�洢����ͨ�ŵĵ�ַ
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(LISTEN_PORT);

	//socket�������Ͷ˿ں�
	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind error!\n");//��������һ���������������ԭ���������׼�豸
		return 1;
	}
	//server���ü���
	if (listen(listener, 1000) < 0) {
		perror("listen");
		return 1;
	}
	cout << "Listening...\n";
	//����socketΪ������ģʽ
	evutil_make_socket_nonblocking(listener);



#ifdef WIN32
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif

	//����һ��event_base���¼���������
	struct event_base* base = event_base_new();
	assert(base != NULL);

	evthread_make_base_notifiable(base);

	//��������һ��event
	struct event* listen_event = event_new(base, listener, EV_READ | EV_PERSIST, do_accept_cb, (void*)base);

	//������ʱ�ش����߳�
	thread th(timeoutRetryth);
	th.detach();

	//��event��ӵ���Ϣѭ�������С�
	event_add(listen_event, NULL);

	//�����¼�ѭ��
	event_base_dispatch(base);

	cout << "����!" << endl;
	getchar();
	return 0;
}
