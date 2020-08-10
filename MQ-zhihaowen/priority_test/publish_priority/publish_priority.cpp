#include <iostream>
#include <map>
#include "..\priority_test_mq.h"

using namespace std;

typedef long long ll;

map<int, string> msg0;
void getmsg0() {
	int n = 30;
	msg0[1] = "*";
	for (int i = 2; i < n; i++) {
		msg0[i] = msg0[i - 1] + msg0[i - 1];
	}
}
string ans;
string msg;
void getmsg(int n) {
	ans.clear();
	int i = 1;
	while (n) {
		if (n & 1) {
			ans += msg0[i];
		}
		i++;
		n /= 2;
	}
	msg = ans;
}


int main(int argc, char* argv[]) {
	//模式 | 发布者数量 | 消息间隔时间 | 消息长度



	int mode = 1;
	string publish_id = "1";
	int msg_time = 1000;
	int msg_len = 65500;
	if (argc > 5) {
		mode = atoi(argv[1]);
		publish_id = argv[2];
		msg_time = atoi(argv[3]);
		msg_len = atoi(argv[5]);
	}

	getmsg0();
	getmsg(msg_len);

	Factory factory;
	publishChannel publish_channel;

	if (mode == 1) {
		string queue_name = "queue_name";
		publish_channel.queueDeclare(queue_name);
		while (1) {
			publish_channel.basicPublish("", queue_name, publish_id, msg + publish_id);
			Sleep(msg_time);
		}
	}

	while (1);
	return 0;
}