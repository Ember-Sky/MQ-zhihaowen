#include <iostream>
#include <string>
#include "..\priority_test_mq.h"

using namespace std;

typedef long long ll;


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
		string queue_name = "queue_name";
		consumer_channel.queueDeclare(queue_name);
		consumer_channel.basicConsumer(queue_name);
	}
	while (1);
	return 0;
}