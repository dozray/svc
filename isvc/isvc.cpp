#include <windows.h>


//Declare globales:
HINSTANCE							hDll;
SERVICE_STATUS_HANDLE		hServiceStatusHandle;
SERVICE_STATUS					serviceStatus;
HANDLE								hEvent,hFile;
char									buf[MAX_PATH];

char* 	svcName		=	"svcd";
char* 	svcGroup	=	"netsvcs";
LPCSTR	hostKey		=	"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";

int WINAPI InstallService(){
	HKEY hkService,hkSvchost;
	SC_HANDLE schScm , schService;
	char* pData;
	DWORD dwError=0,dwLen,dwSize=0;
	schScm=OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS);
	if(schScm){
		char szPath[255];
		char* szHost="%SystemRoot%\\system32\\svchost.exe -k ";
		strcpy(szPath,szHost);
		strcat(szPath,svcGroup);
		//LPCSTR	szPath =	"%SystemRoot%\\System32\\svchost.exe -k netsvcs";
		schService=CreateService(schScm,
			svcName,
			svcName,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_SHARE_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_SEVERE,
			szPath,
			0,
			0,
			0,
			0,
			0
			);
		SERVICE_DESCRIPTION sd;
		sd.lpDescription="Data transfer accelerator.";
		ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,&sd);
		char svcPara[255];
		strcpy(svcPara,"SYSTEM\\CurrentControlSet\\Services\\");
		strcat(svcPara,svcName);
		strcat(svcPara,"\\Parameters");
		//LPCSTR svcPara ="SYSTEM\\CurrentControlSet\\Services\\isvc\\Parameters";
		RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			svcPara,
			0,
			0,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			0,
			&hkService,
			0
			);
		GetModuleFileName(hDll,buf,sizeof(buf));
		RegSetValueEx(hkService,"ServiceDll",0,REG_EXPAND_SZ,(BYTE*)buf,lstrlen(buf)+1);
		RegCloseKey(hkService);
		RegOpenKeyEx(HKEY_LOCAL_MACHINE,hostKey,0,KEY_ALL_ACCESS,&hkSvchost);
		RegQueryValueEx(hkSvchost,svcGroup,0,0,0,&dwSize);
		lstrcpy(buf,svcName);
		dwLen=lstrlen(buf);
		pData =(char*) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwSize+dwLen+1);
		if(pData)
		{
			char* pNames;
			RegQueryValueEx(hkSvchost,svcGroup,0,0,(BYTE*)pData,&dwSize);
			for(pNames= pData;*pNames;pNames=strchr(pNames,0)+1)
			{
				if(!lstrcmpi(pNames,buf))
					break;
			}
			if(*pNames==0)
			{
				memcpy(pData+dwSize-1,buf,dwLen+1);
				RegSetValueEx(hkSvchost,svcGroup,0,REG_MULTI_SZ,(BYTE*)pData,dwSize+dwLen+1);
			}
			else
				dwError=ERROR_ALREADY_EXISTS;

			HeapFree(GetProcessHeap(),0,pData);
		}
		else
			dwError=ERROR_NOT_ENOUGH_MEMORY;
		RegCloseKey(hkSvchost);
		CloseServiceHandle(schScm);
	}
	return dwError?dwError:GetLastError();
}

/*
Function:		remove service
*/
DWORD WINAPI RemoveService(){
	HKEY			hkSvchost;
	SC_HANDLE		schScm,schService;
	char*		pData;
	DWORD		dwError=0,dwSize=0;
	schScm=OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS);
	if(schScm)
	{
		schService = OpenService(schScm,svcName,SERVICE_ALL_ACCESS);
		if(schService)
		{
			if(DeleteService(schService))
			{
				RegOpenKeyEx(HKEY_LOCAL_MACHINE,hostKey,0,KEY_ALL_ACCESS,&hkSvchost);
				RegQueryValueEx(hkSvchost,svcGroup,0,0,0,&dwSize);
				lstrcpy(buf,svcName);
				pData =(char*) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwSize);
				if(pData)
				{
					char *pNames;
					RegQueryValueEx(hkSvchost,svcGroup,0,0,(BYTE*)pData,&dwSize);
					for(pNames=pData;*pNames;pNames=strchr(pNames,0)+1)
					{
						if(!lstrcmpi(pNames,buf))
							break;
					}
						if(*pNames)
						{
							char* pNext;
							pNext = strchr(pNames,0)+1;
							memcpy(pNames,pNext,dwSize-(pNext-pData));
							RegSetValueEx(hkSvchost,svcGroup,0,REG_MULTI_SZ,(BYTE*)pData,dwSize-(lstrlen(buf)+1));
						}
						else
							dwError=ERROR_NO_SUCH_MEMBER;
					
						HeapFree(GetProcessHeap(),0,pData);
				}
				else
					dwError=ERROR_NOT_ENOUGH_MEMORY;
				RegCloseKey(hkSvchost);
			}
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schScm);
	}
	return dwError?dwError:GetLastError();
}

DWORD WINAPI HandlerEx(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext)
{
	switch(dwControl)
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			serviceStatus.dwWaitHint = 1000;
			serviceStatus.dwCurrentState =SERVICE_STOP_PENDING;
			SetServiceStatus(hServiceStatusHandle,&serviceStatus);
			SetEvent(hEvent);
			return NO_ERROR;
		default:
			serviceStatus.dwWaitHint=0;
			break;
	}
	SetServiceStatus(hServiceStatusHandle,&serviceStatus);
	return NO_ERROR;
}


VOID CALLBACK TimerProc(LPVOID lpParameter,BOOLEAN TimerOrWaitFired){
	SYSTEMTIME st;
	DWORD dwLen;
	SetFilePointer(hFile,0,0,FILE_END);
	lstrcpy(buf,"test svc.");
	GetLocalTime(&st);
	GetDateFormat(0,0,&st,"dd/MM/yyyy",buf+lstrlen(buf),12);
	lstrcpy(buf,"  ");
	GetTimeFormat(0,0,&st,"HH:mm:ss",buf+lstrlen(buf),12);
	lstrcat(buf,"\r\n");
	WriteFile(hFile,buf,lstrlen(buf),&dwLen,0);
	return;
}

VOID WINAPI ServiceMain(DWORD dwArgc,LPTSTR *lpszArgv)
{
	hServiceStatusHandle = RegisterServiceCtrlHandlerEx(svcName,HandlerEx,NULL);
	if(hServiceStatusHandle)
	{
		serviceStatus.dwServiceType					= SERVICE_WIN32;
		serviceStatus.dwControlsAccepted			= 0;
		serviceStatus.dwWin32ExitCode				= 0;
		serviceStatus.dwServiceSpecificExitCode	= 0;
		serviceStatus.dwCheckPoint					= 0;
		serviceStatus.dwWaitHint						= 1000;
		serviceStatus.dwCurrentState				= SERVICE_START_PENDING;
		SetServiceStatus(hServiceStatusHandle,&serviceStatus);
		hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		if(hEvent)
		{
			hFile = CreateFile("C:\\isvc.log",GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,0,0);
			if(hFile!=INVALID_HANDLE_VALUE)
			{
				HANDLE hTimerQueue,hTimer;
				hTimerQueue = CreateTimerQueue();
				CreateTimerQueueTimer(&hTimer,hTimerQueue,TimerProc,0,0,60000,WT_EXECUTEINTIMERTHREAD);
				serviceStatus.dwWaitHint = 0;
				serviceStatus.dwCurrentState = SERVICE_RUNNING;
				serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
				SetServiceStatus(hServiceStatusHandle,&serviceStatus);
				WaitForSingleObject(hEvent,INFINITE);
				DeleteTimerQueueTimer(hTimerQueue,hTimer,0);
				DeleteTimerQueueEx(hTimerQueue,0);
				CloseHandle(hFile);

			}
			CloseHandle(hEvent);
		}
		serviceStatus.dwWaitHint = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hServiceStatusHandle,&serviceStatus);
	}
	return ;
}

// DllMain
BOOL APIENTRY DllMain(HINSTANCE hInst,DWORD dwReason,LPVOID lpReserved)
{
	hDll = hInst;
	return TRUE;
}