// svc.cpp : 定义控制台应用程序的入口点。
//

#pragma region Includes
#include <stdio.h>
#include <windows.h>
#include "SvcInstaller.h"
#include "SvcBase.h"
#include "TaskSvc.h"
#pragma endregion

// Internal name of the service.
#define SERVICE_NAME						L"SVC"
// Displayed name of the service.
#define SERVICE_DISPLAY_NAME		L"TaskSvc"
// Description of the service
#define SERVICE_DESCRIPTION			L"Auto shutdown system at 20:00 every day"
// Service start option
#define SERVICE_START_TYPE			SERVICE_AUTO_START
// List of service dependencies - "dep0\0dep1\0\0"
#define SERVICE_DEPENDENCIES		L""
// The name of the account under which the service should run.
/* Security Note: In this code sample, the service is configured to run as LocalService, 
instead of LocalSystem. 
The LocalSystem account has broad permissions. 
Use the LocalSystem account with caution, because it might increase your risk of attacks 
from malicious software. For tasks that do not need broad permissions, consider using 
the LocalService account, which acts as a non-privileged user on the local computer and 
presents anonymous credentials to any remote server. use NULL means LocalSystem
*/
#define SERVICE_ACCOUNT				NULL//L"NT AUTHORITY\\LocalSystem"//L"NT AUTHORITY\\LocalService"
//The password of the service account name.
#define SERVICE_PASSWORD			NULL

int wmain(int argc, wchar_t *argv[])
{
	if((argc>1)&&(*argv[1]==L'-' || *argv[1]==L'/')){
		if(_wcsicmp(L"install",argv[1]+1)==0){
			//install the service when the command is 
			// "-install" or "/install"
			InstallService(
				SERVICE_NAME,
				SERVICE_DISPLAY_NAME,
				SERVICE_START_TYPE,
				SERVICE_DEPENDENCIES,
				SERVICE_ACCOUNT,
				SERVICE_PASSWORD 
				);
			ReconfService(SERVICE_NAME,SERVICE_DESCRIPTION);
		}
		else if(_wcsicmp(L"remove",argv[1]+1)==0){
			//Uninstall the service when the command is 
			//"-remove" or "/remove"
			UninstallService(SERVICE_NAME);
		}
	}
	else{
		wprintf(L"This program is create by dozbray@gmail.com\n\n");
		wprintf(L"Parameter:\n");
		wprintf(L"-install		to install the servcie.\n");
		wprintf(L"-remove		to remove the service.\n\n");
		wprintf(L"Now starting the service SVC.\n");
		TaskSvc service(SERVICE_NAME);
		if(!TaskSvc::Run(service)){
			wprintf(L"Service failed to run w/err 0x%081x\n",GetLastError());
		}
	}
	return 0;
}

