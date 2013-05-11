#pragma once
# include "SvcBase.h"
class TaskSvc : public CSvcBase
{
public :
	TaskSvc(PWSTR pszServiceName,
		BOOL fCanStop = TRUE,
		BOOL fCanShutdown  = TRUE,
		BOOL fCanPauseContinue = FALSE);
	virtual ~TaskSvc(void);

protected:
	
	virtual void OnStart(DWORD dwArgc,PWSTR *pszArgv);
	virtual void OnStop();
	virtual void doTask();

	virtual BOOL ShutdownSystem();
	void ServiceWorkerThread(void);

private:
	BOOL fStopping;
	HANDLE hStoppedEvent;
};