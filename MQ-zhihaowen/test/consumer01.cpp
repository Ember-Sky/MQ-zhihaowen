#include <iostream>
#include <string>
#include "..\test_mq.h"

using namespace std;

typedef long long ll;

////һ�Զ�:��ѯ�ַ������ж������ߣ�һ����Ϣ������һ�Σ�
//void roundRobinConsumer() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	consumerChannel consumer_channel = factory.createConsumerChannel();
//
//	string queue_name = "queue_name";
//	consumer_channel.queueDeclare(queue_name);
//
//	Consumer consumer;
//	consumer_channel.add_Consumer(consumer);
//
//	consumer_channel.basicConsumer(queue_name, true, consumer);
//
//	consumer_channel.close();
//	factory.close();
//}
////һ�Զ�:��ƽ�ַ������ж������ߣ�һ����Ϣ������һ�Σ�
//void fairDispatchConsumer() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	consumerChannel consumer_channel = factory.createConsumerChannel();
//
//	int prefetcha_count = 1;
//	consumer_channel.basicQos(prefetcha_count);
//
//	string queue_name = "queue_name";
//	consumer_channel.queueDeclare(queue_name);
//
//	Consumer consumer;
//	consumer_channel.add_Consumer(consumer);
//	consumer_channel.basicAck();
//
//	bool auto_ack = false;
//	consumer_channel.basicConsumer(queue_name, auto_ack, consumer);
//
//	consumer_channel.close();
//	factory.close();
//
//}
////��Զ�:·��ģʽ�����ж������ߣ�һ����Ϣ�����Ѷ�Σ�
//void directConsumer() {
//	string host_ip = "127.0.0.1";
//	int port_number = 9999;
//
//	Factory factory = Factory();
//	factory.setHost(host_ip);
//	factory.setPort(port_number);
//
//	consumerChannel consumer_channel = factory.createConsumerChannel();
//
//	string exchange_name = "exchange_name";
//	consumer_channel.exchangeDeclare(exchange_name, "direct");
//
//	string queue_name = "queue_name";
//	consumer_channel.queueDeclare(queue_name);
//
//	string routing_key1;
//	consumer_channel.queueBind(queue_name, exchange_name, routing_key1);
//	string routing_key2;
//	consumer_channel.queueBind(queue_name, exchange_name, routing_key2);
//	string routing_key3;
//	consumer_channel.queueBind(queue_name, exchange_name, routing_key3);
//
//	int prefetcha_count = 1;
//	consumer_channel.basicQos(prefetcha_count);
//
//	Consumer consumer;
//	consumer_channel.add_Consumer(consumer);
//	consumer_channel.basicAck();
//
//	bool auto_ack = false;
//	consumer_channel.basicConsumer(queue_name, auto_ack, consumer);
//
//	consumer_channel.close();
//	factory.close();
//
//}

int main(int argc, char* argv[]) {
	//ģʽ | ���������� | ��Ϣ���ʱ�� | ��Ϣ����
	int mode = 1;
	string consumer_id = "1";
	if (argc > 2) {
		mode = atoi(argv[1]);
		consumer_id = argv[2];
	}

	Factory factory;
	consumerChannel consumer_channel;

	if (mode == 1) {
		string queue_name = "queue_name" + consumer_id;
		consumer_channel.queueDeclare(queue_name);
		consumer_channel.basicConsumer(queue_name);
	}
	else if (mode == 2) {
		string queue_name = "queue_name" + consumer_id;
		consumer_channel.queueDeclare(queue_name);

		string exchange_name = "exchange_name";
		consumer_channel.exchangeDeclare(exchange_name, "fanout");
		consumer_channel.queueBind(queue_name, exchange_name, "");

		consumer_channel.basicConsumer(queue_name);
	}
	while (1);
	return 0;
}