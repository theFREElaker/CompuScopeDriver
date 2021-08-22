#include "ProcessList.h"

#ifdef _WIN32
#include <tchar.h>
#else
#include<dirent.h>
#include<stdlib.h>
#include<stdio.h>
#include <ctype.h>
#include "CsTypes.h"

#define BUF_LEN 100

#endif

#ifndef __linux__
CPsapiList::CPsapiList() : m_hModule(NULL),
						  m_fEnumProcesses(NULL),
						  m_fEnumProcessModules(NULL),
						  m_fGetModuleFileNameEx(NULL)
{
	// See if the PSAPI library is available. It should be there for
	// XP, 2K and NT. Win98 has to use the toolhelp library. If
	// PSAPI is not found, XP and 2K can still use toolhelp but
	// we can't get the full pathname to the executable. NT won't be 
	// able to get anything if PSAPI is not there.
	m_hModule = ::LoadLibrary(_TEXT("PSAPI"));
	if ( m_hModule )
	{
		m_fEnumProcesses = (pfEnumProcesses)::GetProcAddress(m_hModule, _TEXT("EnumProcesses"));
		m_fEnumProcessModules = (pfEnumProcessModules)::GetProcAddress(m_hModule, _TEXT("EnumProcessModules"));
#if defined(UNICODE) || defined(_UNICODE)
		m_fGetModuleFileNameEx = (pfGetModuleFileNameEx)::GetProcAddress(m_hModule, _TEXT("GetModuleFileNameExW"));
#else
		m_fGetModuleFileNameEx = (pfGetModuleFileNameEx)::GetProcAddress(m_hModule, _TEXT("GetModuleFileNameExA"));
#endif
		if ( (NULL == m_fEnumProcesses) || (NULL == m_fEnumProcessModules) || (NULL == m_fGetModuleFileNameEx) )
		{
			::FreeLibrary(m_hModule);
			m_hModule = NULL;
			m_fEnumProcesses = NULL;
			m_fEnumProcessModules = NULL;
			m_fGetModuleFileNameEx = NULL;
			throw(0);
		}
	}
	else
	{
		throw(0);
	}
}


CPsapiList::~CPsapiList(void)
{
	if ( m_hModule )
	{
		::FreeLibrary(m_hModule);
		m_hModule = NULL;
		m_fEnumProcesses = NULL;
		m_fEnumProcessModules = NULL;
		m_fGetModuleFileNameEx = NULL;
	}
}


int CPsapiList::GetName( unsigned long u32ProcessID, TCHAR *lpProcessName )
{
	int i32RetVal = 0; // if 0 is returned the process is not in the process list.

	// Get a handle to the process.
	HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION |
									PROCESS_VM_READ,
									FALSE, u32ProcessID );

    // Get the process name.
	if ( NULL != hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( m_fEnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded) )
		{
			if( m_fGetModuleFileNameEx( hProcess, hMod, lpProcessName, 256) )
			{
				i32RetVal = 1;
			}
		}
	}
	::CloseHandle(hProcess);
	return i32RetVal;
}
	

unsigned long CPsapiList::GetProcessFromList ( unsigned long u32ProcessID )
{
	// determines if the given process is in the process list	
	unsigned long u32RetVal = 0;	// if 0 is returned the process is not in the process list.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	if ( !m_fEnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded) )
	{
		return (unsigned long)-1;
	}

	cProcesses = cbNeeded / sizeof(DWORD);

	// Go thru the processes.  If we find our process return
	// the process number, else return 0 because we didn't find it
	for (i = 0; i < cProcesses; i++)
	{
		if (u32ProcessID == aProcesses[i])
		{
			u32RetVal = aProcesses[i];
			break;
		}
	}
	return u32RetVal;
}


CToolhelpList::CToolhelpList(void) : m_hModule(NULL),
									 m_fCreateToolhelp32Snapshot(NULL),
									 m_fProcess32First(NULL),
									 m_fProcess32Next(NULL)
{
	m_hModule = ::LoadLibrary(_TEXT("Kernel32"));
	// On the slim chance that an NT system didn't have PSAPI
	// the toolhelp library won't be there either.
	if (m_hModule)
	{
		m_fCreateToolhelp32Snapshot = (pfCreateToolhelp32Snapshot)::GetProcAddress(m_hModule, _TEXT("CreateToolhelp32SnapShot"));
		m_fProcess32First = (pfProcess32First)::GetProcAddress(m_hModule, _TEXT("Process32First"));
		m_fProcess32Next = (pfProcess32Next)::GetProcAddress(m_hModule, _TEXT("Process32Next"));

		if ( (NULL == m_fCreateToolhelp32Snapshot) || (NULL == m_fProcess32First) || (NULL == m_fProcess32Next) )
		{
			::FreeLibrary(m_hModule);
			m_hModule = NULL;
			m_fCreateToolhelp32Snapshot = NULL;
			m_fProcess32First = NULL;
			m_fProcess32Next = NULL;
			throw (0);
		}
	}
	else
	{
		throw(0);
	}
}

CToolhelpList::~CToolhelpList(void)
{
	if ( m_hModule )
	{
		::FreeLibrary(m_hModule);
		m_hModule = NULL;
		m_fCreateToolhelp32Snapshot = NULL;
		m_fProcess32First = NULL;
		m_fProcess32Next = NULL;
	}
}

int CToolhelpList::GetName( unsigned long u32ProcessID, TCHAR *lpProcessName )
{
	int i32RetVal = 0; // if 0 is returned the process is not in the process list.
	HANDLE         hProcessSnap = NULL;
	PROCESSENTRY32 pe32      = {0}; 

	//  Take a snapshot of all processes in the system. 
    hProcessSnap = m_fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

    if ( hProcessSnap == INVALID_HANDLE_VALUE ) 
	{
	    return i32RetVal; 
	}
	//  Fill in the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
	//  Walk the snapshot of the processes, and for each process, 
	//  display information. 

    if ( m_fProcess32First(hProcessSnap, &pe32) ) 
	{ 
		do 
        { 
			if ( u32ProcessID == pe32.th32ProcessID )
			{
				i32RetVal = 1;
				break;
			}
		} 
        while( m_fProcess32Next(hProcessSnap, &pe32) ); 
	} 
 
	if ( i32RetVal > 0 )
	{
		lstrcpy(lpProcessName, pe32.szExeFile);
	}
	// Do not forget to clean up the snapshot object. 
	::CloseHandle (hProcessSnap);

	return (i32RetVal); 
}


unsigned long CToolhelpList::GetProcessFromList ( unsigned long u32ProcessID )
{
	// determines if the given process is in the process list	
	HANDLE	hProcessSnap = NULL;
	PROCESSENTRY32 pe32      = {0}; 
	unsigned long u32RetVal = 0;	// if 0 is returned the process is not in the process list.
 
	//  Take a snapshot of all processes in the system. 
	
	hProcessSnap = m_fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    if ( hProcessSnap == INVALID_HANDLE_VALUE ) 
	{
	    return (unsigned long)(-1); 
	}
 
   //  Fill in the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
    //  Walk the snapshot of the processes.
	
    if ( m_fProcess32First(hProcessSnap, &pe32) ) 
	{ 
		do 
		{ 
			if ( u32ProcessID == pe32.th32ProcessID )
			{
				u32RetVal = pe32.th32ProcessID;
				break;
			}
		} 
		while ( m_fProcess32Next(hProcessSnap, &pe32) ); 
	} 
	else 
	{
		u32RetVal = (unsigned long)-1;    // could not walk the list of processes 
	}
	// Do not forget to clean up the snapshot object. 
	::CloseHandle (hProcessSnap);
	return (u32RetVal); 
}

#else // it is linux

CLinuxList::CLinuxList()
{
	// check if the /proc directory exists
	DIR* dir = opendir("/proc");
	
	if (dir)
	{
		closedir(dir);
	}
	else
	{
		throw(0); // directory does not exist or cannot be opened
	}
}

int CLinuxList::IsUint(char input[])
{
	// /proc stores process info is directories with the name of the process id
	// i.e. process 12 info would be in /proc/12
	int i;
	for (i=0;i<strlen(input);i++)
	{
		if(!isdigit(input[i]))
		{
			return 0;
		}
	}
	return 1;
}

int CLinuxList::GetName( unsigned long u32ProcessID, TCHAR *lpProcessName )
{
	int i32RetVal = 0; // if 0 is returned the process is not in the process list.
	int ret = 0; 
	DIR* directory;
	directory = opendir("/proc");
	/*
	   this is fix here, because /proc is always the proc directory.
	*/
	if(NULL == directory)
	{
		// note: here 0 does not mean success as it usually does
		// in linux
		
		// should somehow show that we didn't actuall find the directory
		// maybe put it in a log
		return (int)(-1); 
	}
	
	char* curr_path = (char*)malloc(sizeof(char) * BUF_LEN);	
	char* dirname_buf;
	struct dirent* dir;	
	char name[BUF_LEN];
	char null[BUF_LEN];
	
	while((dir = readdir(directory)) != NULL)
	{
		dirname_buf=dir->d_name;
		// if the name is not a number (specifically an unsigned number)
		// the it isn't a process id
		if(!IsUint(dirname_buf))
		{
			continue;
		}
		uInt32 u32ProcID = atol(dirname_buf);
		if (u32ProcID != u32ProcessID)
		{
			continue;
		}
		// else they're the same
		i32RetVal = 1;
		
		strcpy(curr_path,"/proc/");
		strcat(curr_path,dirname_buf);
		strcat(curr_path,"/");

		strcat(curr_path, "status");
		FILE * file=fopen(curr_path, "r");
		if(file)
		{
			ret = fscanf( file, "%s ", null);
			ret = fscanf( file, "%s ",name);
			lstrcpy(lpProcessName, name);
			fclose(file);		
		}
	}
	closedir(directory);
	free(curr_path);
	return (i32RetVal);
}


unsigned long CLinuxList::GetProcessFromList ( unsigned long u32ProcessID )
{
	// determines if the given process is in the process list
	unsigned long u32RetVal = 0;	// if 0 is returned the process is not in the process list.
	
	DIR* directory;
	directory = opendir("/proc");
	/*
	   this is fix here, because /proc is always the proc directory.
	*/
	if(NULL == directory)
	{
		// note: here 0 does not mean success as it usually does
		// in linux
		
		// should somehow show that we didn't actuall find the directory
		// maybe put it in a log
		return (unsigned long)(-1); 
	}
	
	char* curr_path = (char*)malloc(sizeof(char) * 10);	
	char* dirname_buf;
	struct dirent* dir;	
	
	while((dir = readdir(directory)) != NULL)
	{
		dirname_buf=dir->d_name;
		// if the name is not a number (specifically an unsigned number)
		// the it isn't a process id
		if(!IsUint(dirname_buf))
		{
			continue;
		}
		uInt32 u32ProcID = atol(dirname_buf);
		if (u32ProcID == u32ProcessID)
		{
			u32RetVal = u32ProcID;
			break;
		}	
	}
	closedir(directory);	
	free(curr_path);	
	return u32RetVal;
}


#endif
