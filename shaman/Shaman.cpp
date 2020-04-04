
#include <iterator>
#include <stdio.h>

#include <czmq.h>

static char* s_recv(void* socket)
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	int size = zmq_msg_recv(&msg, socket, 0);
	if (size == -1)
		return nullptr;
	char* string = (char*)malloc(size + 1);
	memcpy(string, zmq_msg_data(&msg), size);
	zmq_msg_close(&msg);
	string[size] = 0;
	return string;
}

static int s_send(void* socket, char* string)
{
	zmq_msg_t msg;
	zmq_msg_init_size(&msg, strlen(string));
	memcpy(zmq_msg_data(&msg), string, strlen(string));
	int size = zmq_msg_send(&msg, socket, 0);
	zmq_msg_close(&msg);
	return size;
}

int main(int argc, char** argv)
{
	zsock_t* bcast = zsock_new_sub("tcp://localhost:4007", "");

	while (true)
	{
		if (char* msg = zstr_recv(bcast))
		{
			printf("Received: %s\n", msg);
			zstr_free(&msg);
		}
		Sleep(100);
	}
	return 0;
}