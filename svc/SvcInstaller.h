#pragma once 

/*
	Function: InstallService

	Purpose: Install the current application as a service to the local 
	service control manager database.

	Parameters:
		* pszServiceName		- the name of the service to be installed
		* pszDisplayName		- the display name of the service
		* dwStartType			- the service start option. This parameter
										can be one of the following values:
											SERVICE_AUTO_START
											SERVICE_BOOT_START
											SERVICE_DEMAND_START
											SERVICE_DISABLED
											SERVICE_SYSTEM_START
		* pszDependencies	- a pointer to a double null-terminated array
										of null-separated names of services or load
										ordering groups that the system must start before this service.
		* pszAccount			- the name of the account under which the service runs.
		* pszPassword			- the password to the account name.

		NOTE: If the function fails to install the service, it print the error 
		in the standard output stream for users to diagnose the problem.

*/
void InstallService(PWSTR pszServiceName,
					PWSTR pszDisplayName,
					DWORD dwStartType,
					PWSTR pszDependencies,
					PWSTR pszAccount,
					PWSTR pszPassword);

/*
	Function: ReconfService(LPSTR lpServiceName,LPSTR lpDescription)

	Purpose: Change the description of the given service

	Parameters:
		* lpServiceName		- the name of the service to be change description.
		* lpDescription			- the description about the service.
*/
void ReconfService(PWSTR lpServiceName,PWSTR lpDescription);


/*
	Function: UnInstall Service

	Purpose: Stop and remove the service from the local service control 
	manager database.

	Parameters:
		* pszServiceName		- the name of the service to be removed.

	Note: If the function fails to uninstall the service, it prints the error 
	in the standard output stream for users to diagnose the problem.
*/
void UninstallService(PWSTR pszServiceName);