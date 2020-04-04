/* ---------------------- SimpleService.cpp ---------------------- */
/*++
Copyright (c) 2016 Microsoft Corporation
--*/

#include "SimpleService.h"
#include <assert.h>

#if false
class ScopeCritical
{
public:
    ScopeCritical(CRITICAL_SECTION* crit) : m_Crit(crit)
    {
        EnterCriticalSection(m_Crit);
    }

    ~ScopeCritical()
    {
        LeaveCriticalSection(m_Crit);
    }

private:
    CRITICAL_SECTION* m_Crit;
};

// -------------------- Service---------------------------------

Service *Service::s_Instance;

Service::Service(const std::string &name, bool canStop, bool canShutdown, bool canPauseContinue)
  : m_Name(name),
    m_StatusHandle(nullptr),
    m_StatusLock(),
    m_StatusObject()
{
    InitializeCriticalSection(&m_StatusLock);

    // The service runs in its own process.
    m_StatusObject.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    // The service is starting.
    m_StatusObject.dwCurrentState = SERVICE_START_PENDING;

    // The accepted commands of the service.
    m_StatusObject.dwControlsAccepted = 0;
    if (canStop) 
        m_StatusObject.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    if (canShutdown) 
        m_StatusObject.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    if (canPauseContinue) 
        m_StatusObject.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;

    m_StatusObject.dwWin32ExitCode = NO_ERROR;
    m_StatusObject.dwServiceSpecificExitCode = 0;
    m_StatusObject.dwCheckPoint = 0;
    m_StatusObject.dwWaitHint = 0;
}

Service::~Service()
{
}

DWORD Service::Run()
{
    s_Instance = this;

    SERVICE_TABLE_ENTRY serviceTable[] = 
    {
        { (LPSTR)m_Name.c_str(), ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable))
    {
        return GetLastError();
    }

    return 0;
}

void Service::Start(DWORD argc, LPSTR* argv)
{
    SetStatus(Status::StartPending);
    OnStart(argc, argv);
    SetStatus(Status::Running);
}

void Service::Pause()
{
    SetStatus(Status::PausePending);
    OnPause();
    SetStatus(Status::Paused);
}

void Service::Continue()
{
    SetStatus(Status::ContinuePending);
    OnContinue();
    SetStatus(Status::Running);
}

void Service::Stop()
{
    SetStatus(Status::StopPending);
    OnStop();
    SetStatus(Status::Stopped);
}

void WINAPI Service::ServiceMain(DWORD argc, LPSTR *argv)
{
    assert(s_Instance != NULL);
    static volatile bool s_Wait = true;
    while (s_Wait)
    {
        Sleep(100);
    }

    // Register the handler function for the service
    s_Instance->m_StatusHandle = RegisterServiceCtrlHandler(s_Instance->m_Name.c_str(), ServiceCtrlHandler);
    if (s_Instance->m_StatusHandle == NULL)
    {
        s_Instance->Stop();
        return;
    }

    s_Instance->Start(argc, argv);
}

void WINAPI Service::ServiceCtrlHandler(DWORD ctrl)
{
    switch (ctrl)
    {
    case SERVICE_CONTROL_STOP:
        if (s_Instance->m_StatusObject.dwControlsAccepted & SERVICE_ACCEPT_STOP)
        {
            s_Instance->Stop();
        }
        break;
    case SERVICE_CONTROL_PAUSE:
        if (s_Instance->m_StatusObject.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
        {
            s_Instance->Pause();
        }
        break;
    case SERVICE_CONTROL_CONTINUE:
        if (s_Instance->m_StatusObject.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
        {
            s_Instance->Continue();
        }
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        if (s_Instance->m_StatusObject.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN)
        {
            s_Instance->Stop();
        }
        break;
    case SERVICE_CONTROL_INTERROGATE:
        SetServiceStatus(s_Instance->m_StatusHandle, &s_Instance->m_StatusObject);
        break;
    }
}

void Service::SetStatus(Status status)
{
    ScopeCritical _(&m_StatusLock);

    SetState_CS(status);
}

void Service::SetState_CS(Status status)
{
    m_Status = status;

    m_StatusObject.dwCurrentState = (DWORD)status;
    m_StatusObject.dwCheckPoint = 0;
    m_StatusObject.dwWaitHint = 0;
    
    ::SetServiceStatus(m_StatusHandle, &m_StatusObject);
}

void Service::Bump()
{
    ScopeCritical sc(&m_StatusLock);

    ++m_StatusObject.dwCheckPoint;
    ::SetServiceStatus(m_StatusHandle, &m_StatusObject);
}

void Service::HintTime(DWORD msec)
{
    ScopeCritical sc(&m_StatusLock);

    ++m_StatusObject.dwCheckPoint;
    m_StatusObject.dwWaitHint = msec;
    ::SetServiceStatus(m_StatusHandle, &m_StatusObject);
    m_StatusObject.dwWaitHint = 0; // won't apply after the next update
}
#endif // false