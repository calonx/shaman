
#include <iterator>
#include <stdio.h>

#include <czmq.h>

static void Log(const char* format, ...)
{
	char buffer[4096] = "";
	va_list args;
	va_start(args, format);
	int length = vsnprintf_s(buffer, std::size(buffer), format, args);
	va_end(args);
	assert(length < std::size(buffer));
	OutputDebugString(buffer);
}

static char* s_recv(void* socket)
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	int size = zmq_msg_recv(&msg, socket, 0);
	if (size == -1)
		return nullptr;
	char* string = (char*)malloc(size + 1ull);
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

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		if (!stricmp(argv[1], "--local") || !stricmp(argv[1], "-l"))
		{
			ServiceWorkerThread(nullptr);
			return 0;
		}
	}
	Log("Shaman: Main: Entry");

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{"Shaman", (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{nullptr, nullptr}
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == false)
	{
		Log("Shaman: Main: StartServiceCtrlDispatcher returned error");
		return GetLastError();
	}

	Log("Shaman: Main: Exit");
	return 0;
}

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	DWORD Status = E_FAIL;

	Log("Shaman: ServiceMain: Entry");

	g_StatusHandle = RegisterServiceCtrlHandler("Shaman", ServiceCtrlHandler);

	if (g_StatusHandle == nullptr)
	{
		Log("Shaman: ServiceMain: RegisterServiceCtrlHandler returned error");
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == false)
	{
		Log("Shaman: ServiceMain: SetServiceStatus returned error");
	}

	/*
	 * Perform tasks neccesary to start the service here
	 */
	Log("Shaman: ServiceMain: Performing Service Start Operations");

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(nullptr, true, false, nullptr);
	if (g_ServiceStopEvent == nullptr)
	{
		Log("Shaman: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error");

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == false)
		{
			Log("Shaman: ServiceMain: SetServiceStatus returned error");
		}
		goto EXIT;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == false)
	{
		Log("Shaman: ServiceMain: SetServiceStatus returned error");
	}

	// Start the thread that will perform the main task of the service
	HANDLE hThread = CreateThread(nullptr, 0, ServiceWorkerThread, nullptr, 0, nullptr);

	Log("Shaman: ServiceMain: Waiting for Worker Thread to complete");

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	Log("Shaman: ServiceMain: Worker Thread Stop Event signaled");


	/*
	 * Perform any cleanup tasks
	 */
	Log("Shaman: ServiceMain: Performing Cleanup Operations");

	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == false)
	{
		Log("Shaman: ServiceMain: SetServiceStatus returned error");
	}

EXIT:
	Log("Shaman: ServiceMain: Exit");

	return;

}

void WINAPI ServiceCtrlHandler(DWORD control_code)
{
	Log("Shaman: ServiceCtrlHandler: Entry");

	switch (control_code)
	{
	case SERVICE_CONTROL_STOP:

		Log("Shaman: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request");

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		 * Perform tasks necessary to stop the service here
		 */

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == false)
		{
			Log("Shaman: ServiceCtrlHandler: SetServiceStatus returned error");
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	Log("Shaman: ServiceCtrlHandler: Exit");
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	Log("Shaman: ServiceWorkerThread: Entry");

	//// Periodically check if the service has been requested to stop
	//while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	//{
	//	/*
	//	 * Perform main service function here
	//	 */

	//	 // Simulate some work by sleeping
	//	Sleep(3000);
	//}

	char hostname[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD name_len = std::size(hostname);
	GetComputerName(hostname, &name_len);

	printf("Computer name: %s\n", hostname);

	zsock_t* bcast = zsock_new_pub("tcp://*:4007");

	char nbuf[64];
	char msg[128];
	int i = 100;
	while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0)
	{
		itoa(i++, nbuf, 10);
		strcpy_s(msg, hostname);
		strcat_s(msg, ": Running (");
		strcat_s(msg, nbuf);
		strcat_s(msg, ")");
		printf("Sending: %s\n", msg);
		zstr_send(bcast, msg);
	}

	Log("Shaman: ServiceWorkerThread: Exit");

	return ERROR_SUCCESS;
}
