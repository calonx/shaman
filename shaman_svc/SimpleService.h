/* ---------------------- SimpleService.hpp ---------------------- */
/*++
Copyright (c) 2016 Microsoft Corporation
--*/

#pragma once

#if false

#include <string>

#include <windows.h>
#include <winsvc.h>

// -------------------- Service---------------------------------
// The general wrapper for running as a service.
// The subclasses need to define their virtual methods.

enum class Status : DWORD
{
    Stopped         = SERVICE_STOPPED,
    StartPending    = SERVICE_START_PENDING,
    StopPending     = SERVICE_STOP_PENDING,
    Running         = SERVICE_RUNNING,
    ContinuePending = SERVICE_CONTINUE_PENDING,
    PausePending    = SERVICE_PAUSE_PENDING,
    Paused          = SERVICE_PAUSED,
};

class Service
{
public:

    // The way the services work, there can be only one Service object
    // in the process. 
    Service(const std::string &name, bool canStop, bool canShutdown, bool canPauseContinue);

    Service() = delete;
    Service(const Service&) = delete;
    void operator=(const Service&) = delete;

    virtual ~Service();

    // Run the service. Returns after the service gets stopped.
    // When the Service object gets started, it will remember the instance pointer in the s_Instance static
    // member, and use it in the callbacks.
    DWORD Run();

    void Start(DWORD argc, LPSTR* argv);
    void Pause();
    void Continue();
    void Stop();

    virtual void OnStart(DWORD, LPSTR*) {}
    virtual void OnStop()               {}
    virtual void OnPause()              {}
    virtual void OnContinue()           {}
    virtual void OnShutdown()           {}

    // On the lengthy operations, periodically call this to tell the
    // controller that the service is not dead.
    // Can be called only while Run() is running.
    void Bump();

    // Can be used to set the expected length of long operations.
    // Also does the bump.
    // Can be called only while Run() is running.
    void HintTime(DWORD msec);

private:
    // The callback for the service start.
    static void WINAPI ServiceMain(DWORD argc, LPSTR* argv);
    // The callback for the requests.
    static void WINAPI ServiceCtrlHandler(DWORD ctrl);

    void SetStatus(Status status);

    // the internal version that expects the caller to already hold m_StatusLock
    void SetState_CS(Status status);

protected:
    static Service*       s_Instance;

    std::string           m_Name; // service name

    Status                m_Status;

    CRITICAL_SECTION      m_StatusLock;
    SERVICE_STATUS_HANDLE m_StatusHandle;
    SERVICE_STATUS        m_StatusObject;
};

#endif // false