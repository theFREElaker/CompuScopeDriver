#ifndef __CS_PROCESS_LIST_H__
#define __CS_PROCESS_LIST_H__

#if defined (_WIN32)
#pragma once

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#else

#include "CsLinuxPort.h"

#endif

#ifndef __linux__
// Functions loaded from PSAPI
typedef BOOL (WINAPI *pfEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef DWORD (WINAPI *pfGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
typedef BOOL (WINAPI *pfEnumProcesses)(DWORD *lpidProcesses, DWORD cb, DWORD *cbNeeded);


//Functions loaded from Kernel32 for the TOOLHELP API
typedef HANDLE (WINAPI *pfCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL (WINAPI *pfProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI *pfProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
#endif

class CProcessList
{
public:
	CProcessList(void) {};
	virtual ~CProcessList(void) {};
	virtual int GetName( unsigned long u32ProcessID, TCHAR *lpProcessName) = 0;
	virtual unsigned long GetProcessFromList ( unsigned long u32ProcessID ) = 0;

private:
	CProcessList* m_pList;

};

class CPsapiList : public CProcessList
{
public:
	CPsapiList(void);
	~CPsapiList(void);
//#ifndef __linux__
	int GetName( unsigned long u32ProcessID, TCHAR *lpProcessName );
	unsigned long GetProcessFromList ( unsigned long u32ProcessID );
#ifndef __linux__	
	HMODULE GetLibraryHandle() { return m_hModule; }

private:
	HMODULE m_hModule;

	pfEnumProcesses m_fEnumProcesses;
	pfEnumProcessModules m_fEnumProcessModules;
	pfGetModuleFileNameEx m_fGetModuleFileNameEx;
#endif
};


class CToolhelpList : public CProcessList
{
public:
	CToolhelpList(void);
	~CToolhelpList(void);

	int GetName( unsigned long u32ProcessID, TCHAR *lpProcessName );
	unsigned long GetProcessFromList ( unsigned long u32ProcessID );
#ifndef __linux__	
	HMODULE GetLibraryHandle() { return m_hModule; }

private:
	HMODULE m_hModule;

	pfCreateToolhelp32Snapshot m_fCreateToolhelp32Snapshot;
	pfProcess32First m_fProcess32First;
	pfProcess32Next m_fProcess32Next;
#endif
};

class CLinuxList : public CProcessList
{
public:	
	CLinuxList(void);
	~CLinuxList(void) {};
	
	int GetName( unsigned long u32ProcessID, TCHAR *lpProcessName );
	unsigned long GetProcessFromList ( unsigned long u32ProcessID );	
	
private:
	int IsUint(char input[]);
};
	


#endif // __CS_PROCESS_LIST_H__
