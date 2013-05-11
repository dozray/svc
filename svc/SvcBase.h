#pragma once
#include <windows.h>
class CSvcBase
{
public:

	// Register the executable for a service with the SCM 
	// After you can Run(SvcBase),the SCM issues a Start 
	// command, which results in a call to the OnStart method
	// in the service. This method blocks until the service has stopped.
	static BOOL Run(CSvcBase &svc);

	// Service object constructor. The optional parameters (fCanStop,
	// fCanShutdown and fCanPauseContinue) allow you to specifies
	// whether the service can be stopped, paused and continued, or 
	// be notified when system shutdown occurs.
	CSvcBase (PWSTR pszServiceName,
		BOOL fCanStop = TRUE,
		BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = TRUE);

	// Service object destructor
	virtual ~CSvcBase(void);
	
	// Stop the service
	void Stop();

protected:

	// When implements in a derived class,executes when a Start command
	// is send to the servide by the SCM or when the opertaing system starts
	// (for a service that starts automatically). Specifies actions to take when
	// the services starts.
	virtual void OnStart(DWORD dwArgc,PWSTR *pszArgv);

	// When implements in a derived class, executes when a Stop command
	// is send to the service by the SCM. Specifies to take when a service stops running.
	virtual void OnStop();

	// When implements in a derived class, executes when a Pause command
	// is send to the service by the SCM. Specifies to take when a service pauses.
	virtual void OnPause();
	
	// When implements in a derived class, OnContinue runs when a Continue 
	// command is send to the service by the SCM. Specifies actions to take 
	// when a service resumes normal functioning after being paused.
	virtual void OnContinue();

	// When implements in a derived class, executes when the system is 
	// shutting down. Specifies what should occur immediately prior to
	// the system shutting down.
	virtual void OnShutdown();
	// Set the service status and report the status to the SCM
	void SetServiceStatus(DWORD dwCurrentState, 
		DWORD dwWin32ExitCode = NO_ERROR,
		DWORD dwWaitHint=0);
	// Log an message to the Application event log.
	void WriteEventLogEntry(PWSTR pszMessage,WORD wType);
	// Log an error message to the Application event log.
	void WriteErrorLogEntry(PWSTR pszFunction,DWORD dwError=GetLastError());
	// Log an error message to the Application event log.
    virtual void logLastErrorEx();
private:
	// Entry point of the service. It registers the handler function for the service
	// and starts the service.
	static void WINAPI ServiceMain(DWORD dwArgc,LPWSTR *lpszArgv);
	// The function is called by the SCM whenever a control code is send to the service.
	static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
	// Start the service
	void Start (DWORD dwArgc ,PWSTR *pszArgv);
	// Pause the service.
	void Pause ();
	// Resume the service after being paused.
	void Continue();
	// Execute when the system is shutting down.
	void Shutdown();

	// The singleton service instance.
	static CSvcBase *svc;
	// The name of the service 
	PWSTR name;
	// The status of the service
	SERVICE_STATUS status;
	// The service status handle.
	SERVICE_STATUS_HANDLE statusHandle;
};