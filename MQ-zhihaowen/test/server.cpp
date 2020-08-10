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

#define LISTEN_PORT 9999//端口号
#define LIATEN_BACKLOG 32
using namespace std;
typedef long long ll;

struct MESSAGE {
	ll send_time = 0;//发送时间
	string msg;//消息内容
	//int msg_send_num = 0;//消息发送的次数
	//int msg_send_maxnum = 10;//消息发送次数上限
};
struct Queue {
	queue<pair<ll, string> >msg_low_wait;//等待发送的msg队列，记录guid和对应的msg的内容
	queue<pair<ll, string> >msg_cen_wait;//等待发送的msg队列，记录guid和对应的msg的内容
	queue<pair<ll, string> >msg_high_wait;//等待发送的msg队列，记录guid和对应的msg的内容
	unordered_map<ll, MESSAGE> msg_after;//已经发送但未ack确认的msg队列，通过guid访问msg的内容
	struct bufferevent* consumer_bev = NULL;//监听者的连接
};
struct Exchange {
	string exchange_type;//交换机属性，可扩展
	unordered_map<string, vector<string> > all_queue_name;//所有队列的名字，通过路由键访问队列名
};

ll server_msg_guid = 0;//每条msg对应唯一的guid
//unordered_set<string>receive_msg;//收到的所有消息
//mutex receive_msg_mutex;//receive_msg锁
unordered_map<string, Queue>all_queue;//所有队列，通过队列名访问队列
mutex all_queue_mutex;
unordered_map<string, Exchange>all_exchange;//所有交换机，通过交换机名访问交换机
unordered_map<ll, string> guid_queue_name;//通过guid访问发送msg的队列名
unordered_map<struct bufferevent*, string>bev_queue_name;//通过bev访问与之建立连接的队列名
int ack_max_time = 10000;//消息等待上限，达到之后重发。
string msg_store;//消息储存器
int consumer_msg_maxnum = 1;//消费者消费信息的上限，0为无上限


const int MSGLEN_LEN = 8;//消息中消息长度的长度
const int MSGTYPE_LEN = 2;//消息中消息类型的长度
const int MSGGUID_LEN = 8;//消息中消息guid的长度
const int FUNDATE_LEN = 32;//消息中函数参数的长度

/**************************************函数声明********************************************/
//十进制和十六进制相互转换
string getNumStr(int x, int n);
int getStrNum(string s, int n);

//accept回调函数
void do_accept_cb(evutil_socket_t listener, short event, void* arg);
//read 回调函数
void read_cb(struct bufferevent* bev, void* arg);
//write 回调函数
void write_cb(struct bufferevent* bev, void* arg);
//error 回调函数
void error_cb(struct bufferevent* bev, short event, void* arg);
//发送消息
void sendMsg(struct bufferevent* bev, string msg);
//超时重发
void timeoutRetryth();

//客户端接口
bool queueDeclare(string queue_name);
bool exchangeDeclare(string exchange_name, string exchange_type);
bool queueBind(string queue_name, string exchange_name, string routing_key);
bool basicPublish(string exchange_name, string routing_key, string msg);
bool basicConsumer(string queue_name, struct bufferevent* bev);
bool basicQos(string queue_name, int num);

/**************************************函数体********************************************/
//十进制和十六进制相互转换
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
//补充函数参数
string addStr(string s) {
	while (s.size() < FUNDATE_LEN) {
		s += '-';
	}
	return s;
}

//accept回调函数
void do_accept_cb(evutil_socket_t listener, short event, void* arg) {
	//传入的event_base指针
	struct event_base* base = (struct event_base*)arg;
	//socket描述符
	evutil_socket_t fd;

	//声明地址
	struct sockaddr_in sin;
	//地址长度声明
	socklen_t slen = sizeof(sin);

	//接收客户端
	fd = accept(listener, (struct sockaddr*)&sin, &slen);
	if (fd < 0) {
		cout << "accept error!" << endl;
		return;
	}
	cout << "--------------------------------------" << endl;
	cout << "Client connection successful: fd = " << fd << endl;

	//注册一个bufferevent_socket_new事件 //创建bufferevent对象
	struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

	//设置回调函数
	bufferevent_setcb(bev, read_cb, write_cb, error_cb, arg);
	//设置该事件的属性
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);
}
//read 回调函数
void read_cb(struct bufferevent* bev, void* arg) {
#define MAX_LINE 1024
	char line[MAX_LINE + 1];
	//通过传入参数bev找到socket fd
	evutil_socket_t fd = bufferevent_getfd(bev);
	int n = bufferevent_read(bev, line, MAX_LINE);
	if (n == 0) {
		return;
	}
	line[n] = '\0';
	//cout << "fd=" << fd << ", 收到真实数据: “" << line << "”\n";

	string linestr = line;
	msg_store += linestr;

	while (msg_store.size() > MSGLEN_LEN) {
		//取出消息长度
		int msg_len = getStrNum(msg_store.substr(0, MSGLEN_LEN), MSGLEN_LEN);
		if (msg_len + MSGLEN_LEN <= msg_store.size()) {
			//取出消息，删除消息
			string msg = msg_store.substr(MSGLEN_LEN, msg_len);
			msg_store = msg_store.substr(MSGLEN_LEN + msg_len);
			cout << "fd=" << fd << ", 收到: “" << msg << "”\n";

			//取出类型
			int  msg_type = getStrNum(msg.substr(0, MSGTYPE_LEN), MSGTYPE_LEN);
			////取出guid
			//ll msg_guid = getStrNum(msg.substr(MSGTYPE_LEN, MSGGUID_LEN), MSGGUID_LEN);

			ll msg_guid;
			//返回ack
			if (msg_type == 132) {
				//取出guid
				msg_guid = getStrNum(msg.substr(MSGTYPE_LEN, MSGGUID_LEN), MSGGUID_LEN);
			}
			else {
				cout << "fd=" << fd << ", 发送: “" << msg << "” 的Ack" << endl;
				sendMsg(bev, msg);
			}


			switch (msg_type) {
			case 0:
			case 128://创建队列
				queueDeclare(msg.substr(MSGTYPE_LEN));
				break;
			case 1:
			case 129://创建交换机
				exchangeDeclare(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN));
				break;
			case 2://server接收publish端消息
				basicPublish(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + 2 * FUNDATE_LEN));
				break;
			case 130://绑定交换机和队列
				queueBind(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + FUNDATE_LEN, FUNDATE_LEN), msg.substr(MSGTYPE_LEN + 2 * FUNDATE_LEN));
				break;
			case 131://consumer端监听队列
				basicConsumer(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), bev);
				break;
			case 132://server接收consumer端ack
				all_queue_mutex.lock();
				all_queue[guid_queue_name[msg_guid]].msg_after.erase(msg_guid);
				all_queue_mutex.unlock();
				write_cb(bev, arg);
				break;
			case 133://设置队列上限
				basicQos(msg.substr(MSGTYPE_LEN, FUNDATE_LEN), getStrNum(msg.substr(MSGTYPE_LEN + FUNDATE_LEN), 4));
				break;
			case 134://设置ack
				break;
			}
		}
		else {
			break;
		}
	}
}
//write 回调函数
void write_cb(struct bufferevent* bev, void* arg) {
	//循环所有队列，主动发送信息：直到当前队列不能发送信息，换下一个队列
	all_queue_mutex.lock();
	for (auto it = all_queue.begin(); it != all_queue.end(); ) {

		if (it->second.consumer_bev != NULL//if队列存在监听的消费者
			&& (it->second.msg_after.size() < consumer_msg_maxnum
				|| consumer_msg_maxnum == 0)) {//消费者没有达到消费上限
			if (it->second.msg_high_wait.size()) {
				//将msg从wait队列中取出
				pair<ll, string>msg_front = it->second.msg_high_wait.front();
				it->second.msg_high_wait.pop();

				//放进after队列
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", 发送: “" << send_msg << "”" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
			if (it->second.msg_cen_wait.size()) {
				//将msg从wait队列中取出
				pair<ll, string>msg_front = it->second.msg_cen_wait.front();
				it->second.msg_cen_wait.pop();

				//放进after队列
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", 发送: “" << send_msg << "”" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
			if (it->second.msg_low_wait.size()) {
				//将msg从wait队列中取出
				pair<ll, string>msg_front = it->second.msg_low_wait.front();
				it->second.msg_low_wait.pop();

				//放进after队列
				it->second.msg_after[msg_front.first].send_time = clock();
				string send_msg = getNumStr(msg_front.first, MSGGUID_LEN) + msg_front.second;
				it->second.msg_after[msg_front.first].msg = send_msg;

				cout << "fd=" << bufferevent_getfd(it->second.consumer_bev);
				cout << ", 发送: “" << send_msg << "”" << endl;
				sendMsg(it->second.consumer_bev, send_msg);
				continue;
			}
		}
		it++;
	}
	all_queue_mutex.unlock();
}
//error回调函数
void error_cb(struct bufferevent* bev, short event, void* arg) {
	//通过传入参数bev找到socket fd
	evutil_socket_t fd = bufferevent_getfd(bev);
	cout << "fd = " << fd << "，";
	if (event & BEV_EVENT_TIMEOUT) {
		cout << "超时！" << endl;//if bufferevent_set_timeouts() called
	}
	else if (event & BEV_EVENT_EOF) {
		cout << "连接关闭！" << endl;
	}
	else if (event & BEV_EVENT_ERROR) {
		cout << "其他错误" << endl;
	}

	//删除监听的消费者
	string queue_name = bev_queue_name[bev];
	bev_queue_name.erase(bev);
	all_queue_mutex.lock();
	if (all_queue.count(queue_name)) {
		all_queue[queue_name].consumer_bev = NULL;
	}
	all_queue_mutex.unlock();

	//释放Bufferevent
	bufferevent_free(bev);
}
//加报头，发送消息
void sendMsg(struct bufferevent* bev, string msg) {
	evutil_socket_t fd = bufferevent_getfd(bev);

	msg = getNumStr(msg.size(), MSGLEN_LEN) + msg;
	cout << "fd=" << fd << ", 发送真实数据: “" << msg << "”\n";
	bufferevent_write(bev, msg.c_str(), msg.size());
}
//超时重发
void timeoutRetryth() {
	while (1) {
		//超时重发
		all_queue_mutex.lock();
		auto all_queuet = all_queue;
		all_queue_mutex.unlock();

		int cnt = 0;
		for (auto it = all_queuet.begin(); it != all_queuet.end(); it++) {
			if (it->second.consumer_bev != NULL//队列存在监听的消费者
				&& it->second.msg_after.size()) {//重发队列中存在数据
				ll now_time = clock();
				for (auto x : it->second.msg_after) {
					if (now_time - x.second.send_time > ack_max_time) {
						cout << "子线程发送超时重发信息: “" << x.second.msg << "”" << endl;

						sendMsg(it->second.consumer_bev, x.second.msg);
						cnt++;
					}
				}
			}
		}
		Sleep(ack_max_time / 10);
	}
}

//客户端接口
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
		//获取发送到consumer端消息的guid
		ll server_guid_t = server_msg_guid++;

		//将guid对应的queue_name记录下来，方便ack处理
		guid_queue_name[server_guid_t] = x;

		//将消息放进wait队列
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

	//加载winsock库
	WSADATA  Ws;
	//Init Windows Socket（初始化socket资源）
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
		return -1;
	}

	//存储socket成功连接返回的编号
	evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
	assert(listener > 0);//参数为假的时候，终止程序执行
	//端口重用
	evutil_make_listen_socket_reuseable(listener);

	//存储网络通信的地址
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(LISTEN_PORT);

	//socket绑定主机和端口号
	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind error!\n");//用来将上一个函数发生错误的原因输出到标准设备
		return 1;
	}
	//server设置监听
	if (listen(listener, 1000) < 0) {
		perror("listen");
		return 1;
	}
	cout << "Listening...\n";
	//设置socket为非阻塞模式
	evutil_make_socket_nonblocking(listener);



#ifdef WIN32
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif

	//创建一个event_base（事件管理器）
	struct event_base* base = event_base_new();
	assert(base != NULL);

	evthread_make_base_notifiable(base);

	//创建并绑定一个event
	struct event* listen_event = event_new(base, listener, EV_READ | EV_PERSIST, do_accept_cb, (void*)base);

	//创建超时重传子线程
	thread th(timeoutRetryth);
	th.detach();

	//将event添加到消息循环队列中。
	event_add(listen_event, NULL);

	//启动事件循环
	event_base_dispatch(base);

	cout << "结束!" << endl;
	getchar();
	return 0;
}
