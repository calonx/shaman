
#include <iterator>
#include <stdio.h>

#include <czmq.h>

#include "SimpleService.h"

static char* s_recv(void* socket)
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	int size = zmq_msg_recv(&msg, socket, 0);
	if (size == -1)
		return nullptr;
	char* string = (char*)malloc(size + 1u);
	if (!string)
		return nullptr;
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

class ShamanService : public Service
{
public:
	ShamanService() : Service("Shaman", /*canStop*/ true, /*canShutdown*/ false, /*canPauseContinue*/ false)
	{
		DWORD name_len = std::size(m_Name);
		GetComputerName(m_Name, &name_len);
	}

	DWORD WINAPI Loop(LPVOID param);

	void OnStart(DWORD argc, LPSTR* argv) override;
	void OnStop() override;
	void OnPause() override;
	void OnContinue() override;
	void OnShutdown() override;

private:
	char m_Name[MAX_COMPUTERNAME_LENGTH + 1];
	HANDLE m_WakeEvent;
	HANDLE m_Thread;
};

void ShamanService::OnStart(DWORD, LPSTR*)
{
	auto callback = [] (void* param) -> DWORD
	{
		return ((ShamanService*)param)->Loop(param);
	};
	m_WakeEvent = CreateEvent(nullptr, 0, 0, nullptr);
	m_Thread = CreateThread(nullptr, 0, callback, this, 0, nullptr);
}

bool IsHandleValid(HANDLE handle)
{
	return handle != INVALID_HANDLE_VALUE;
}

void CloseHandleSafe(HANDLE* handle)
{
	if (handle && IsHandleValid(*handle))
	{
		CloseHandle(*handle);
		*handle = INVALID_HANDLE_VALUE;
	}
}

void ShamanService::OnStop()
{
	SetEvent(m_WakeEvent);
	WaitForSingleObject(m_Thread, 0);
	CloseHandle(m_WakeEvent);
	CloseHandle(m_Thread);
}

void ShamanService::OnPause()
{
	SetEvent(m_WakeEvent);
}

void ShamanService::OnContinue()
{
	SetEvent(m_WakeEvent);
}

void ShamanService::OnShutdown()
{
}

DWORD WINAPI ShamanService::Loop(LPVOID param)
{
	printf("Computer name: %s\n", m_Name);
	
	zsock_t* bcast = zsock_new_pub("tcp://*:4007");

	char nbuf[64];
	char msg[128];
	int i = 100;
	while (m_Status != Status::StopPending && m_Status != Status::Stopped)
	{
		if (m_Status == Status::Running)
		{
			itoa(i++, nbuf, 10);
			strcpy_s(msg, m_Name);
			strcat_s(msg, ": Running (");
			strcat_s(msg, nbuf);
			strcat_s(msg, ")");
			printf("Sending: %s\n", msg);
			zstr_send(bcast, msg);
		}
		static DWORD sleep_duration = 1000;
		HintTime(sleep_duration);
		DWORD ret = WaitForSingleObject(m_WakeEvent, sleep_duration);
		if (ret == WAIT_FAILED)
		{
			Stop();
			break;
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	ShamanService svc;
	return svc.Run();
}