#include <iostream>
#include <map>
#include "..\test_mq.h"

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
string getmsg(int n) {
	string ans;
	int i = 1;
	while (n) {
		if (n & 1) {
			ans += msg0[i];
		}
		i++;
		n /= 2;
	}
	return ans;
}

////一对多:轮询分发（队列对消费者，一条消息被消费一次）
//void roundRobinClient() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	publishChannel publish_channel = factory.createPublishChannel();
//
//	string queue_name = "queue_name";
//	publish_channel.queueDeclare(queue_name);
//
//	string msg = "轮询模式发送的数据";
//	publish_channel.basicPublish("", queue_name, false, msg);
//
//	publish_channel.close();
//	factory.close();
//
//}
////一对多:公平分发（队列对消费者，一条消息被消费一次）
//void fairDispatchClient() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	publishChannel publish_channel = factory.createPublishChannel();
//
//	string queue_name = "queue_name";
//	publish_channel.queueDeclare(queue_name);
//
//	string msg = "公平模式发送的数据";
//	publish_channel.basicPublish("", queue_name, false, msg);
//
//	publish_channel.close();
//	factory.close();
//
//}
////多对多:路由模式（队列对消费者，一条消息被消费多次）
//void directClient() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	publishChannel publish_channel = factory.createPublishChannel();
//
//	string exchange_name = "exchange_name";
//	publish_channel.exchangeDeclare(exchange_name, "direct");
//
//	string msg1 = "路由模式发送到 routing key 1 的数据";
//	string routing_key1 = "routing_key1";
//	publish_channel.basicPublish(exchange_name, routing_key1, false, msg1);
//	string msg2 = "路由模式发送到 routing key 2 的数据";
//	string routing_key2 = "routing_key2";
//	publish_channel.basicPublish(exchange_name, routing_key2, false, msg2);
//	string msg3 = "路由模式发送到 routing key 3 的数据";
//	string routing_key3 = "routing_key3";
//	publish_channel.basicPublish(exchange_name, routing_key3, false, msg3);
//
//	publish_channel.close();
//	factory.close();
//}
//

int main(int argc, char* argv[]) {
	//模式 | 发布者数量 | 消息间隔时间 | 消息长度



	int mode = 1;
	string publish_id = "1";
	int msg_time = 10;
	int msg_len = 65500;
	if (argc > 4) {
		mode = atoi(argv[1]);
		publish_id = argv[2];
		msg_time = atoi(argv[3]);
		msg_len = atoi(argv[4]);
	}

	getmsg0();
	string msg = getmsg(msg_len);

	Factory factory;
	publishChannel publish_channel;

	if (mode == 1) {
		string queue_name = "queue_name" + publish_id;
		publish_channel.queueDeclare(queue_name);
		while (1) {
			publish_channel.basicPublish("", queue_name, PRIORITY_LOW, msg);
			Sleep(msg_time);
		}
	}
	else if (mode == 2) {
		string exchange_name = "exchange_name";
		publish_channel.exchangeDeclare(exchange_name, "fanout");
		while (1) {
			publish_channel.basicPublish(exchange_name, "", PRIORITY_LOW, msg);
			Sleep(msg_time);
		}
	}

	while (1);
	return 0;
}