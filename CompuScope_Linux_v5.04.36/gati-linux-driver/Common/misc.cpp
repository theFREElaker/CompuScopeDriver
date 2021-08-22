#if defined (__linux__) && !defined(__KERNEL__)
	#include <stdint.h>
	#include <CsLinuxPort.h>
#endif

#include "misc.h"

#if defined (_WIN32)

	#include <windows.h>
	#include <tchar.h>
	#include <psapi.h>
	#include <tlhelp32.h>
	#include <process.h>
	#include "Debug.h"

	// Functions loaded from PSAPI
	typedef BOOL (WINAPI *pfEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD (WINAPI *pfGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	typedef BOOL (WINAPI *pfEnumProcesses)(DWORD *lpidProcesses, DWORD cb, DWORD *cbNeeded);


	//Functions loaded from Kernel32 for the TOOLHELP API
	typedef HANDLE (WINAPI *pfCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL (WINAPI *pfProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL (WINAPI *pfProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);

	//Function to determine if we're running under a 64 bit OS
	typedef BOOL (WINAPI *LPFN_Is32on64PROCESS) (HANDLE, PBOOL);

#else
	#ifndef __KERNEL__
		#include <stdlib.h>
		#include <stdio.h>
		#include <string.h>
		#include <dirent.h>
		#include <errno.h>
		#include <sys/types.h>
		#include <sys/time.h>
		#include <signal.h>
		#include <unistd.h>
		#include <netdb.h> // for h_errno

		#include <CWinEventHandle.h>
	#endif  // __KERNEL__
#endif // _WIN32

#if defined(_WIN32)

uInt32 GetProcessFromProcessList (uInt32 u32ProcessID) 
{
    uInt32 u32RetVal = 0; // if 0 is returned the process is not in the process list.
    
   	HMODULE PSAPI = ::LoadLibrary(_TEXT("PSAPI"));
	if (PSAPI)
	{
		pfEnumProcesses fEnumProcesses = (pfEnumProcesses)::GetProcAddress(PSAPI, _TEXT("EnumProcesses"));
		if ( NULL == fEnumProcesses )
		{
			::FreeLibrary(PSAPI);
			return (uInt32)-1;
		}

		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;
		if (!fEnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
			return (uInt32)-1;

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
		::FreeLibrary(PSAPI);
		return u32RetVal;
	}
	else
	{
		HANDLE	hProcessSnap = NULL;
		PROCESSENTRY32 pe32      = {0}; 
 
		HMODULE TOOLHELP = ::LoadLibrary(_TEXT("Kernel32"));
		// On the slim chance that an NT system didn't have PSAPI
		// the toolhelp library won't be there either.
		if (TOOLHELP)
		{
			pfCreateToolhelp32Snapshot fCreateToolhelp32Snapshot = (pfCreateToolhelp32Snapshot)::GetProcAddress(TOOLHELP, _TEXT("CreateToolhelp32SnapShot"));
			pfProcess32First fProcess32First = (pfProcess32First)::GetProcAddress(TOOLHELP, _TEXT("Process32First"));
			pfProcess32Next fProcess32Next = (pfProcess32Next)::GetProcAddress(TOOLHELP, _TEXT("Process32Next"));

			if ((NULL == fCreateToolhelp32Snapshot) || (NULL == fProcess32First) || (NULL == fProcess32Next))
			{
				::FreeLibrary(TOOLHELP);
				return u32RetVal;
			}
    
			//  Take a snapshot of all processes in the system. 
	
			hProcessSnap = fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

		    if (hProcessSnap == INVALID_HANDLE_VALUE) 
			    return (uInt32)(-1); 
 
		   //  Fill in the size of the structure before using it. 

		    pe32.dwSize = sizeof(PROCESSENTRY32); 
		    //  Walk the snapshot of the processes.
	
		    if (fProcess32First(hProcessSnap, &pe32)) 
			{ 
				do 
				{ 
					if (u32ProcessID == pe32.th32ProcessID)
					{
						u32RetVal = pe32.th32ProcessID;
						break;
					}
				} 
				while (fProcess32Next(hProcessSnap, &pe32)); 
			} 
			else 
				u32RetVal = (uInt32)-1;    // could not walk the list of processes 
 
			  // Do not forget to clean up the snapshot object. 
			::CloseHandle (hProcessSnap);
			::FreeLibrary(TOOLHELP);
			return (u32RetVal); 
		}
	}
	return (uInt32)-1; // couldn't walk the list either way
}


int32 GetProcessNameFromToolhelp (uInt32 u32ProcessID, TCHAR *lpProcessName) 
{
	int32 i32RetVal = 0; // if 0 is returned the process is not in the process list.
	HANDLE         hProcessSnap = NULL;
	PROCESSENTRY32 pe32      = {0}; 

	HMODULE TOOLHELP = ::LoadLibrary(_TEXT("Kernel32"));
	// On the slim chance that an NT system didn't have PSAPI
	// the toolhelp library won't be there either.
	if (TOOLHELP)
	{
		pfCreateToolhelp32Snapshot fCreateToolhelp32Snapshot = (pfCreateToolhelp32Snapshot)::GetProcAddress(TOOLHELP, _TEXT("CreateToolhelp32SnapShot"));
		pfProcess32First fProcess32First = (pfProcess32First)::GetProcAddress(TOOLHELP, _TEXT("Process32First"));
		pfProcess32Next fProcess32Next = (pfProcess32Next)::GetProcAddress(TOOLHELP, _TEXT("Process32Next"));

		if ((NULL == fCreateToolhelp32Snapshot) || (NULL == fProcess32First) || (NULL == fProcess32Next))
		{
			::FreeLibrary(TOOLHELP);
			return i32RetVal;
		}
 
	   //  Take a snapshot of all processes in the system. 

	    hProcessSnap = fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

	    if (hProcessSnap == INVALID_HANDLE_VALUE) 
		    return i32RetVal; 
 
	   //  Fill in the size of the structure before using it. 

	    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
	   //  Walk the snapshot of the processes, and for each process, 
		//  display information. 

	    if (fProcess32First(hProcessSnap, &pe32)) 
		{ 
			do 
	        { 
				if (u32ProcessID == pe32.th32ProcessID)
				{
					i32RetVal = 1;
					break;
				}
			} 
	        while (fProcess32Next(hProcessSnap, &pe32)); 
		} 
 
		if (i32RetVal > 0)
			lstrcpy(lpProcessName, pe32.szExeFile);

		// Do not forget to clean up the snapshot object. 
		::CloseHandle (hProcessSnap);
		::FreeLibrary(TOOLHELP);
		return (i32RetVal); 
	}
	return (i32RetVal);
}

int32 GetProcessName(uInt32 u32ProcessID, TCHAR *lpProcessName)
{
	// See if the PSAPI library is available. It should be there for
	// XP, 2K and NT. Win98 has to use the toolhelp library. If
	// PSAPI is not found, XP and 2K can still use toolhelp but
	// we can't get the full pathname to the executable. NT won't be 
	// able to get anything if PSAPI is not there.
	int32 i32RetVal = 0; // if 0 is returned the process is not in the process list.
	HMODULE PSAPI = ::LoadLibrary(_TEXT("PSAPI"));
	if (PSAPI)
	{
		pfEnumProcessModules fEnumProcessModules = (pfEnumProcessModules)::GetProcAddress(PSAPI, _TEXT("EnumProcessModules"));
#if defined(UNICODE) || defined(_UNICODE)
		pfGetModuleFileNameEx fGetModuleFileNameEx = (pfGetModuleFileNameEx)::GetProcAddress(PSAPI, _TEXT("GetModuleFileNameExW"));
#else
		pfGetModuleFileNameEx fGetModuleFileNameEx = (pfGetModuleFileNameEx)::GetProcAddress(PSAPI, _TEXT("GetModuleFileNameExA"));
#endif
		if ((NULL == fEnumProcessModules) || (NULL == fGetModuleFileNameEx))
		{
			::FreeLibrary(PSAPI);
			return i32RetVal;
		}

		// Get a handle to the process.
		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION |
										PROCESS_VM_READ,
										FALSE, u32ProcessID );

	    // Get the process name.
		if (NULL != hProcess)
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if (fEnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
			{
				if(fGetModuleFileNameEx( hProcess, hMod, lpProcessName, 256))
					i32RetVal = 1;
			}
		}

		::CloseHandle(hProcess);
		
		::FreeLibrary(PSAPI);
		return i32RetVal;
	}
	else
		return ::GetProcessNameFromToolhelp(u32ProcessID, lpProcessName);
}

void DisplayErrorMessage(int nErrorCode, int nErrorType /* = 0*/)
{
#ifdef _DEBUG

	LPVOID lpMsgBuf;
	nErrorType = nErrorType; // Not used in Windows
	if (FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						nErrorCode,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL ))
	{
		// Display the string.
		TRACE(MISC_TRACE_LEVEL, (LPCTSTR)lpMsgBuf);
		// Free the buffer.
		LocalFree( lpMsgBuf );
	}
#else

	nErrorCode = nErrorCode; // for compiler warnings
	nErrorType = nErrorType; 
#endif
}

MUTEX_HANDLE MakeMutex(MUTEX_HANDLE hMutex, BOOL bInitialOwner, LPCTSTR strName)
{
	hMutex = hMutex;
	return ::CreateMutex(NULL, bInitialOwner, strName);
}

int GetMutex(MUTEX_HANDLE hMutex)
{
	DWORD dwRet = ::WaitForSingleObject(hMutex, INFINITE);
	if (dwRet == WAIT_OBJECT_0)
		return 0;
	else
		return -1; // RICKY - PROPER ERROR CODE
}


void DestroyMutex(MUTEX_HANDLE hRmMutex)
{
	if (NULL != hRmMutex)
	{
       	::CloseHandle(hRmMutex);
		hRmMutex = NULL;
	}                
}

#else // _WIN32

uInt32 GetProcessFromProcessList (uInt32 u32ProcessID)
{
    DIR *dp;
    char filename[24];
    uInt32 u32RetVal = 0; // if 0 is returned the process is not in the process list.

    snprintf(filename, sizeof(filename), "/proc/%d", (int)u32ProcessID);
    dp = opendir(filename);
    if (NULL == dp)
    {
    	int err = ::GetLastError();
        if (err == ENOENT || err == ENOTDIR)
        	u32RetVal = 0;
        else
        	u32RetVal = (uInt32)-1;
    }
    else
    	u32RetVal = u32ProcessID;

    return u32RetVal;
}

int32 GetProcessName(uInt32 u32ProcessID, TCHAR *lpProcessName)
{
	DIR *dp;
	char filename[24];
	uInt32 u32RetVal = 0; // if 0 is returned the process is not in the process list.

	snprintf(filename, sizeof(filename), "/proc/%d", (int)u32ProcessID);
	dp = opendir(filename);
	if (NULL == dp)
	{
		int err = ::GetLastError();
		if (err == ENOENT || err == ENOTDIR)
			u32RetVal = 0;
		else
			u32RetVal = (uInt32)-1;
	}
	else
	{
        strcat(filename, "/exe");
        int nChars = readlink(filename, lpProcessName, MAX_PATH);
        if (nChars <= 0)
			u32RetVal = 0;
		else
		{
			// add the end of string character
			lpProcessName[nChars] = '\0';
			u32RetVal = 1;
		}
	}

	return u32RetVal;
}

void DisplayErrorMessage(int nErrorCode, int nErrorType /* = 0*/)
{
#if _DEBUG
	if (0 == nErrorType)
		printf("%s\n", strerror(nErrorCode));
	else
		printf("%s\n", hstrerror(nErrorCode));
#endif
}

void InitializeCriticalSection(CRITICAL_SECTION *pCriticalSection)
{//TODO: RG - Oct 26
	pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

	::pthread_mutex_init(pCriticalSection, &mutexattr); //Should it be NULL (?)
	pthread_mutexattr_destroy(&mutexattr);
}

void DeleteCriticalSection(CRITICAL_SECTION *pCriticalSection)
{
	::pthread_mutex_destroy(pCriticalSection);
}

MUTEX_HANDLE MakeMutex(MUTEX_HANDLE hMutex, BOOL bInitialOwner, LPCTSTR strName)
{
	MUTEX_HANDLE pMutex = (MUTEX_HANDLE)malloc(sizeof(pthread_mutex_t));

	hMutex = pMutex;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutexattr_init(&mutexattr);
	(void)pthread_mutex_init(hMutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	if (FALSE != bInitialOwner)
	{
		pthread_mutex_lock(hMutex);
	}
	return hMutex;
}

int GetMutex(MUTEX_HANDLE hMutex)
{
	return(::pthread_mutex_lock(hMutex));
}

int ReleaseMutex(MUTEX_HANDLE hMutex)
{
	return(::pthread_mutex_unlock(hMutex));
}

void DestroyMutex(MUTEX_HANDLE hRmMutex)
{
	if (NULL != hRmMutex)
	{
 		pthread_mutex_destroy(hRmMutex);
// 		free(hRmMutex);//RG - memory leak, need to put this back
		hRmMutex = NULL;
	}
}

SEM_HANDLE CreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, LPCSTR lpName)
{
	lpSemaphoreAttributes = lpSemaphoreAttributes;
	lMaximumCount = lMaximumCount;
	lpName = lpName;

	SEM_HANDLE hSemaphore = (SEM_HANDLE)malloc(sizeof(sem_t));
	int ret = sem_init(hSemaphore, 0, (unsigned int)lInitialCount);
	if (0 != ret)
		return NULL;
	else
		return hSemaphore;
}
// note: these semaphores only work between threads. For
// semaphores to work between processes we'll need to use the System V semaphores
int32 DestroySemaphore(SEM_HANDLE hSemaphore)
{
	int ret = -1;
	if (NULL != hSemaphore)
	{
		ret = sem_destroy(hSemaphore);
		free(hSemaphore);
		hSemaphore = NULL;
	}
	return ret;
}

BOOL ReleaseSemaphore(SEM_HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount)
{
	lpPreviousCount = lpPreviousCount;
	lReleaseCount = lReleaseCount;
	if (0 == sem_post(hSemaphore))
	   return TRUE;
	else
		return FALSE;
}

int GetLastError(void)
{
	return errno;
}

int WSAGetLastError(void)
{
	return ( 0 < errno ) ? errno : h_errno;
}

void WSASetLastError(int nError)
{
	h_errno = nError;
}

HMODULE LoadLibrary(LPCTSTR lpFileName)
{
	HMODULE hModule = dlopen(lpFileName, /*RTLD_NOW*/RTLD_NOW/* | RTLD_GLOBAL*/);
	if (0 == hModule)
		printf("Error: %s\n", dlerror());
	return hModule;
}

BOOL FreeLibrary(HMODULE handle)
{
	if (0 == dlclose(handle))
		return TRUE;
	else
		return FALSE;
}

FARPROC GetProcAddress(HMODULE handle, LPCSTR lpSymbol)
{
	dlerror(); // clear out the error first
	FARPROC p = dlsym(handle, lpSymbol);
	const char *pError = dlerror();
	if (NULL == pError)
		return p;
	else
	{
#if _DEBUG
		fprintf(stdout, "%s\n", pError);
#endif
		return NULL;
	}
}

DWORD GetCurrentProcessId(void)
{
	return ::getpid();
}

pthread_t GetCurrentThread(void)
{
	return ::pthread_self();
}


DWORD GetTickCount()//RG - JUST FOR NOW (WE HAVE 2 OF THEM)
{
#define USE_RT_ROUTINE
#ifndef USE_RT_ROUTINE
	struct timeval _tstart;
	struct timezone tz;
	double dMilliseconds;

	gettimeofday(&_tstart, &tz);
	dMilliseconds = (double)(_tstart.tv_sec * 1000.0) + (double)(_tstart.tv_usec / 1000.0);
	return (DWORD)dMilliseconds;
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts); // or CLOCK_PROCESS_CPUTIME_ID
	DWORD ret(ts.tv_sec);
	return ret * 1000 + ts.tv_nsec / 1000000;
#endif
}


BOOL IsBadWritePtr(void *p, UINT_PTR ucb)
{
	return 0;
}

BOOL IsBadReadPtr(const void *p, UINT_PTR ucb)
{
	return 0;
}

int _ecvt_s(char *pBuffer, size_t SizeInBytes, double dValue, int nCount, int *nDec, int *nSign)
{
	// check if pBuffer is valid and not NULL
//	if (NULL == pBuffer)
	char *buf;
	buf = ecvt(dValue, nCount, nDec, nSign);
	if (NULL == buf)
	{
		return -1; //RG - return other error codes ?
	}
	strncpy(pBuffer, buf, strlen(buf));
	return 0; // return 0 for success
}

#endif //_WIN32


void CalculateTimeActive(DWORD dwTimeStarted, DWORD *dwHours, DWORD *dwMinutes, DWORD *dwSeconds, DWORD *dwMilliseconds)
{
	DWORD dwTime;
	DWORD dwElapsedTimeInMS = ::GetTickCount() - dwTimeStarted;
	*dwSeconds = dwElapsedTimeInMS / 1000;
	*dwMinutes = *dwSeconds / 60;
	*dwHours = *dwMinutes / 60;
	dwTime = dwElapsedTimeInMS - (*dwHours * 60 * 60 * 1000);
	*dwSeconds = dwTime / 1000;
	*dwMinutes = *dwSeconds / 60;
	dwTime = dwTime - (*dwMinutes * 60 * 1000);
	*dwSeconds = dwTime / 1000;
	*dwMilliseconds = dwTime - (*dwSeconds * 1000);
}
