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

////һ�Զ�:��ѯ�ַ������ж������ߣ�һ����Ϣ������һ�Σ�
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
//	string msg = "��ѯģʽ���͵�����";
//	publish_channel.basicPublish("", queue_name, false, msg);
//
//	publish_channel.close();
//	factory.close();
//
//}
////һ�Զ�:��ƽ�ַ������ж������ߣ�һ����Ϣ������һ�Σ�
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
//	string msg = "��ƽģʽ���͵�����";
//	publish_channel.basicPublish("", queue_name, false, msg);
//
//	publish_channel.close();
//	factory.close();
//
//}
////��Զ�:·��ģʽ�����ж������ߣ�һ����Ϣ�����Ѷ�Σ�
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
//	string msg1 = "·��ģʽ���͵� routing key 1 ������";
//	string routing_key1 = "routing_key1";
//	publish_channel.basicPublish(exchange_name, routing_key1, false, msg1);
//	string msg2 = "·��ģʽ���͵� routing key 2 ������";
//	string routing_key2 = "routing_key2";
//	publish_channel.basicPublish(exchange_name, routing_key2, false, msg2);
//	string msg3 = "·��ģʽ���͵� routing key 3 ������";
//	string routing_key3 = "routing_key3";
//	publish_channel.basicPublish(exchange_name, routing_key3, false, msg3);
//
//	publish_channel.close();
//	factory.close();
//}
//

int main(int argc, char* argv[]) {
	//ģʽ | ���������� | ��Ϣ���ʱ�� | ��Ϣ����



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