#pragma region Includes
#include "SvcBase.h"
#include <assert.h>
#include <tchar.h>
#include <strsafe.h>
#pragma endregion

#pragma region Static Members

// Initalize the singleton service instance.
CSvcBase *CSvcBase::svc = NULL;

/*
	FUNCTION :CSvcBase::Run(CSvcBase &)

	PURPOSE : Register the execuable for a service with the Service
	Control Manager (SCM). After you call Run(SvcBase), the SCM 
	issues a Start command, which results a call to the OnStart 
	method in the service.
	This method blocks until the service has stopped.
*/
BOOL CSvcBase::Run(CSvcBase &service){
	svc = &service;
	SERVICE_TABLE_ENTRY serviceTable[]= 
	{
		{service.name,ServiceMain},
		{NULL,NULL}
	};

	// Connects the main thread of a service process to the service
	// control manager, which causes the thread to be the service
	// control dispatcher thread for the calling process. This call
	// returns when the service has stopped. The process should 
	// simply terminate when the call returns.
	return StartServiceCtrlDispatcher(serviceTable);
}

/*
	FUNCTION : CSvcBase::ServiceMain(DWORD,PWSTR *)
	PURPOSE : Entry point of the service. It registers the handler function
	for the service and starts the service.

	PARAMETERS:
	* dwArgc		-number of command line arguments
	* lpszArgv		-array of command line arguments
*/
void WINAPI CSvcBase::ServiceMain(DWORD dwArgc,PWSTR *pszArgv){
	assert(svc!=NULL);

	//Register the handler function for the service
	svc->statusHandle = RegisterServiceCtrlHandler(
		svc->name,ServiceCtrlHandler);
	if(svc->statusHandle == NULL)
	{
		throw GetLastError();
	}
	// Start the service
	svc->Start(dwArgc,pszArgv);
}

/*
	FUNCTION: CSvcBase::ServiceCtrlHandler(DWORD)

	PURPOSE: The function is called by the SCM whenever a control
	code is send to the service.


	PARAMETERS:
	* dwCtrlCode	- the control code. This parameter can be one of 
	the following values.

	SERVICE_CONTROL_CONTINUE
	SERVICE_CONTROL_INTERROGATE
	SERVICE_CONTROL_NETBINDADD
	SERVICE_CONTROL_NETBINDDISABLE
	SERVICE_CONTROL_NETBINDMOCE
	SERVICE_CONTROL_PARAMCHANGE
	SERVICE_CONTROL_PAUSE
	SERVICE_CONTROL_SHUTDOWN
	SERVICE_CONTROL_STOP

	This parameter can also be a user-defined control code ranges 
	from 128 to 255
*/
void WINAPI CSvcBase::ServiceCtrlHandler(DWORD dwCtrl){
	switch(dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		svc->Stop();
		break;
	case SERVICE_CONTROL_PAUSE:
		svc->Pause();
		break;
	case SERVICE_CONTROL_CONTINUE:
		svc->Continue();
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		svc->Shutdown();
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

#pragma endregion Static Members

#pragma region Service Constructor and Destructor

/*
	FUNCTION: CSvcBase::CSvcBase(PWSTR,BOOL,BOOL,BOOL)

	PURPOSE: The constructor of CSvcBase. It initalizes a new 
	instance of the CSvcBase class. The optional parameters( fCanStrop,
	fCanShutdown,fCanPauseContinue) allow you to specify whether
	the service can be stopped,paused and continued, or be notified 
	when the system shutdown occurs.

	PARAMETERS:
		* pszServiceName			- the name of the service.
		* fCanStop					- the service can be stopped.
		* fCanShutdown			- the service is notified when the system shutdown occurs.
		* fCanPauseContiune	- the service can be paused and continued.
*/
CSvcBase::CSvcBase(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
{
	// Service name must be a valide string and cannot be NULL.
	name = pszServiceName==NULL?L"dozbray@gmail.com":pszServiceName;
	statusHandle = NULL;
	// The service runs in its own process
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	// The service is starting 
	status.dwCurrentState = SERVICE_START_PENDING;	

	// The accepted commands of the service.
	DWORD dwControlsAccepted = 0;
	if(fCanStop)
		dwControlsAccepted |=SERVICE_ACCEPT_STOP;
	if(fCanShutdown)
		dwControlsAccepted |=SERVICE_ACCEPT_SHUTDOWN;
	if(fCanPauseContinue)
		dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;

	status.dwControlsAccepted = dwControlsAccepted;
	status.dwWin32ExitCode = NO_ERROR;
	status.dwServiceSpecificExitCode=0;
	status.dwCheckPoint=0;
	status.dwWaitHint=0;
}

/*
	FUNCTION: CSvcBase::~CSvcBase()

	PURPOSE: The virtual destructor of CSvcBase
*/
CSvcBase::~CSvcBase(void){
}
#pragma endregion

#pragma region  Service Start, Stop, Pause, Continue, and Shutdown

/*
	FUNCTION: CSvcBase::Start(DWORD,PWSTR *)
	PURPOSE: The function starts the service. It calls the OnStart virtual 
	function in which you can specify the action to take when the 
	service starts. If an error occurs during the startup, the error will be 
	logged in the Application event log, and the service will be stopped.

	PARAMETERS:
		* dwArgc		- number of command line arguments.
		* lpszArgv		- array of command line arguments.

*/
void CSvcBase::Start(DWORD dwArgc,PWSTR *pszArgv){
	try{	
		// Tell SCM that the service is starting 
		SetServiceStatus(SERVICE_START_PENDING);
		// Perform service-specific initalization
		OnStart(dwArgc,pszArgv);
		// Tell SCM that the service is started
		SetServiceStatus(SERVICE_RUNNING);
	}catch(DWORD dwError){
		// Log the error 
		WriteErrorLogEntry(L"Service Start",dwError);
		// Set the service status to be stopped.
		SetServiceStatus(SERVICE_STOPPED,dwError);
	}catch(...){
		// Log the error 
		WriteEventLogEntry(L"Service failed to start.",EVENTLOG_ERROR_TYPE);
		// Set the service status to be stopped.
		SetServiceStatus(SERVICE_STOPPED);
	}
}

/*
	FUNCTION: CSvcBase::OnStart(DWORD, PWSTR *)

	PURPOSE: When implmented in a derived class, executes when a Start
	command is sent to the service by the SCM or when the operating system
	starts(for a service that starts automatically). Specifies actions to take 
	when the service starts. Be sure to periodically call 
	CSvcBase::SetServiceStatus() with SERVICE_START_PENDING if the procedure
	is going to take long time. You may also consider spawing a new thread
	in OnStart to perform time-consuming initalization tasks.

	PARAMETER:
		* dwArgc		- number of command line arguments.
		* lpszArgv		- array of command line arguments.

*/
void CSvcBase::OnStart(DWORD dwArgc,PWSTR *lpszArgv){

}

/*
	FUNCTION: CSvcBase::Strop()

	PURPOSE: The function stops the service. It calls the OnStop virtaul
	function in which you can specify the actions to the take when 
	the service stops. If an error occurs, the error will be logged in the
	Application event log, and the service will be restroed to the 
	original state.
*/
void CSvcBase::Stop(){
	DWORD dwOriginalState = status.dwCurrentState;
	try{
		// Tell the SCM that the service is stopping.
		SetServiceStatus(SERVICE_STOP_PENDING);
		
		// Perform service-specific stop operations
		OnStop();

		// Tell the SCM that the service is stopped.
		SetServiceStatus(SERVICE_STOPPED);
	}catch(DWORD dwError){
		// Log the error
		WriteErrorLogEntry(L"Service Stop",dwError);
		// Set the original service status.
		SetServiceStatus(dwOriginalState);
	}catch(...){
		// Log the error 
		WriteEventLogEntry(L"Service failed to stop.",EVENTLOG_ERROR_TYPE);
	}
}

/*
	FUNCTION: CSvcBase::OnStop()

	PURPOSE: When implemented in a derived class, executes when a Stop
	command is sent to the service by the SCM, Specifies actions to take
	when the service stops running. Be sure to periodically call
	CSvcBase::SetServiceStatus() with SERVICE_STOP_PENDING if the 
	procedure is going to take long time.
*/
void CSvcBase::OnStop(){
}

/*
	FUNCTION: CSvcBase::Pause()

	PURPOSE: The function pauses the service is the service supports
	pause and continue. It calls OnPause virtual function in which
	you can specify the action to take when the service pauses.
	If and error occurs, the error will be logged in the Application
	event log, and the service will become running.
*/
void CSvcBase::Pause(){
	try{
		// Tell SCM the service is pausing.
		SetServiceStatus(SERVICE_PAUSE_PENDING);

		// Perform service-specific pause operations.
		OnPause();

		// Tell SCM that the service is paused.
		SetServiceStatus(SERVICE_PAUSED);
	}catch(DWORD dwError){
		// Log the error 
		WriteErrorLogEntry(L"Service Pause",dwError);
		// Tell SCM that the service is still running
		SetServiceStatus(SERVICE_RUNNING);
	}catch(...){
		// Log the error 
		WriteEventLogEntry(L"Service failed to pause.",	EVENTLOG_ERROR_TYPE);
		// Tell the SCM that the service is still running
		SetServiceStatus(SERVICE_RUNNING);
	}

}

/*
	FUNCTION: CSvcBase::OnPause()

	PURPOSE: When implemented in a derived class, executes when a Pause
	command is sent to the service by the SCM. Specify actions to take 
	when a service pauses.

*/
void CSvcBase::OnPause(){

}

/*
	FUNCTION: CSvcBase::Continue()

	PURPOSE: The function resumes normal functioning after being paused 
	if the service supports pause and continue. It calls the OnContinue virtual 
	function in which you can specify the action to take when the service
	continues. If an error occurs, the error will be logged in the Application
	event log, and the service will be still paused.
*/
void CSvcBase::Continue(){
	try{
		// Tell the SCM that the service is resuming.
		SetServiceStatus(SERVICE_CONTINUE_PENDING);

		// Performed service-specific continue opertions.
		OnContinue();

		// Tell the SCM that the service is running.
		SetServiceStatus(SERVICE_RUNNING);
	}catch(DWORD dwError){
		// Log the error 
		WriteErrorLogEntry(L"Service Continue",dwError);

		// Tell the SCM that the service is still paused.
		SetServiceStatus(SERVICE_PAUSED);
	}catch(...){
		// Log the error 
		WriteEventLogEntry(L"Service failed to resume.",EVENTLOG_ERROR_TYPE);
		// Tell the SCM that the service is still paused.
		SetServiceStatus(SERVICE_PAUSED);
	}
}

/*
	FUNCTION:CSvcBase::OnContinue()

	PURPOSE: When implemented in a derived class, OnContinue runs when
	a Continue command is sent to the service by SCM. Specifies actions to
	take when a service resumes normal functioning after being paused.
*/
void CSvcBase::OnContinue(){
}

/*
	FUNCTION: CSvcBase::Shutdown()

	PURPOSE: The function executes when the system is shutting
	down. It calls the OnShutdown virtual function in which you 
	can specify what should occur immediately prior to the system
	shutting down. If an error occurs, the error will be logged in 
	the Application event log.
*/
void CSvcBase::Shutdown(){
	try{
		// Performed service-specific shutdown operations.
		OnShutdown();
		// Tell the SCM that the service is stopped.
		SetServiceStatus(SERVICE_STOPPED);
	}catch(DWORD dwError){
		// Log the error.
		WriteErrorLogEntry(L"Service Shutdown.",dwError);
	}catch(...){
		// Log the error.
		WriteEventLogEntry(L"Service failed to shut shut down.",EVENTLOG_ERROR_TYPE);
	}
}

/*
	FUNCTION: CSvcBase::OnShutdown()

	PURPOSE: When implemented in a derived class, executes when the system
	is shutting down. Specifies what should occur immediately prior to the 
	system shutting down.
*/
void CSvcBase::OnShutdown(){
}

#pragma endregion

#pragma region Helper Functions

/*
	FUNCTION: CSvcBase::SetServiceStatus(DWORD,DWORD,DWORD)

	PURPOSE: The function sets the service status and reports the status 
	to the SCM.

	PARAMETERS:
		* dwCurrentState		- the state of the service
		* dwWin32ExitCode	- error code to report
		* dwWaitHint			- estimated time for pending operation, in milliseconds.

*/
void CSvcBase::SetServiceStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint){
		static DWORD dwCheckPoint = 1;

		// Fill in the SERVICE_STATUS structure of the service
		status.dwCurrentState = dwCurrentState;
		status.dwWin32ExitCode = dwWin32ExitCode;
		status.dwWaitHint = dwWaitHint;
		status.dwCheckPoint = ((dwCurrentState ==SERVICE_RUNNING)||(dwCurrentState == SERVICE_STOPPED))?0:dwCheckPoint++;

		//Report the status of the service to the SCM.
		::SetServiceStatus(statusHandle,&status);
}

/*
	FUNCTION: CSvcBase::WriteEventLogEntry(PWSTR,WORD)

	PURPOSE: Log a message to the Application event log.

	PARAMETERS:
		* pszMessage		- string message to be logged.
		* wType				- the type of event to be logged. The parameter can be one of the following values.

		EVENTLOG_SUCCESS
		EVENTLOG_AUDIT_FAILURE
		EVENTLOG_AUDIT_SUCCESS
		EVENTLOG_ERROR_TYPE
		EVENTLOG_INFORMATION_TYPE
		EVENTLOG_WARNING_TYPE
 */
void CSvcBase::WriteEventLogEntry(PWSTR pszMessage,WORD wType){
	HANDLE hEventSource = NULL;
	LPCWSTR lpszStrings[2]={NULL,NULL};
	hEventSource = RegisterEventSource(NULL,name);
	if(hEventSource){
		lpszStrings[0] = name;
		lpszStrings[1] = pszMessage;

		ReportEvent(hEventSource,	// Event log hadle
			wType,								// Event type
			0,										// Event category
			0,										// Event indentifier
			NULL,								// No security indentifier
			2,										// Size of lpszStrings array
			0,										// No binary data
			lpszStrings,						// Array of strings.
			NULL								// No binary data
			);

		DeregisterEventSource (hEventSource);
	}
}

/*
	FUNCTION: CSvcBase::WriteErrorLogEntry(PWSTR,DWORD)

	PURPOSE:	Log an error message to the Application event log.
	PARAMETERS:
		* pszFunction		- the function that gives the error 
		* dwError				- the error code
*/
void CSvcBase::WriteErrorLogEntry(PWSTR pszFunction,DWORD dwError){
	wchar_t szMessage[260];
	StringCchPrintf(szMessage,ARRAYSIZE(szMessage),L"%s failed w/err 0x%08lx",pszFunction,dwError);
	//WriteEventLogEntry(szMessage,EVENTLOG_ERROR_TYPE);
	WriteEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}


void CSvcBase::logLastErrorEx()
{
	DWORD dwMessageID = ::GetLastError();
	TCHAR *szMessage = NULL;
	TCHAR szTitle[32] = {0};
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwMessageID, 0, (LPWSTR) &szMessage, 0, NULL);
	_stprintf(szTitle, _T("::GetLastError() = %d"), dwMessageID);
	WriteEventLogEntry(szMessage,EVENTLOG_INFORMATION_TYPE);	
	//::MessageBox(NULL, szMessage, szTitle, MB_OK);
}
#pragma endregion