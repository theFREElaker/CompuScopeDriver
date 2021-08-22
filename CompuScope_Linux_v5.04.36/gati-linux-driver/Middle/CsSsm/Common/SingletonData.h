
#pragma once

#include <map>
#include "CsThreadLock.h"

#if defined(_WIN32)
#include "DiskStream/CsDiskStream.h"
#else
#include <deque>
#include <assert.h>
#include <CsLinuxPort.h> // RG for define of HANDLE, need to move someplace else
#include <CsTypes.h> //RG for int64 for now
#endif

typedef LPCsEventCallback CALLBACKLIST;

using namespace std;


typedef struct _AS_TX_RESULT_
{
	int32			i32TokenValue;
	int32			i32TokenStatus;
	int64			i64TokenResult;
 
} AS_TX_RESULT, *PAS_TX_RESULT;

#if defined(_WIN32)

#define MAX_TOKEN_COUNT 128 // Maximum token count for the deque on AS transfer results

class AsTxResultsList
{
	AS_TX_RESULT  _ResultsList[MAX_TOKEN_COUNT+1];
	size_t        _nCurrentListSize;
	static _MutexLock _key;
public:
	AsTxResultsList()
	{
		_key.acquire();
		_nCurrentListSize = 0;
		::ZeroMemory(_ResultsList, sizeof(_ResultsList));
		_key.release();
	}
	~AsTxResultsList(){_key.acquire();_key.release();}
	void	UnLock(){_key.release();}
	size_t size(){_key.acquire();return _nCurrentListSize;}
	void push_front ( const AS_TX_RESULT& _Val)
	{
		_key.acquire();
		for(size_t i = 0; i < _nCurrentListSize; i++)
			_ResultsList[i+1] = _ResultsList[i];

		_ResultsList[0] = _Val;
		_nCurrentListSize++;
		if( _nCurrentListSize > MAX_TOKEN_COUNT )
			_nCurrentListSize=MAX_TOKEN_COUNT;
		_key.release();
	}
	void clear( void ){_key.acquire();_nCurrentListSize=0;_key.release();}
	AS_TX_RESULT& at(size_t _Pos)
	{
		_key.acquire();
		AS_TX_RESULT& ref = _ResultsList[0];
		if( _Pos < _nCurrentListSize )
			ref = _ResultsList[_Pos];
		else
			ASSERT(!"AsTxResultsList:;at out of bounce");
		_key.release();
		return ref;
	}

	void* operator new(size_t size ){return ::VirtualAlloc(NULL, sizeof(size), MEM_COMMIT, PAGE_READWRITE);	}
	void* operator new[]( size_t size ){return ::VirtualAlloc(NULL, sizeof(size), MEM_COMMIT, PAGE_READWRITE);}
	void operator delete(void* p ){::VirtualFree(p,0, MEM_RELEASE);	}
	void operator delete[]( void* p ){::VirtualFree(p,0, MEM_RELEASE);}
};
#else

typedef deque<AS_TX_RESULT> AS_TX_RESULTS_LIST;

#endif	// _WIN32


typedef struct _SYSTEM_DATA
{
public:
	CSHANDLE				hLockedSystem;	//!< Handle of the locked system
	DWORD					dwThreadID;		//!< Thread id for the Event thread
	DWORD					dwThreadIDTxfer;	//!< Thread id for the Event thread

#ifdef __linux__//TODO
	pthread_t				hThread;		//!< Handle of Event thread
	pthread_t				hThreadTxfer;	//!< Handle of Event thread
#else
	EVENT_HANDLE			hThread;		//!< Handle of Event thread
	EVENT_HANDLE			hThreadTxfer;	//!< Handle of Event thread
#endif

	bool					bSMLock;			//!< Lock status of state machine
	bool					bCommitedConfiguration;	//!< Flag for updates between Commited and Acquired configuration structure
	Worker*					pWorker;		//!< Class element that performs the queue for asynchrone calls
	CStateMachine*			pStateMachine;	//!< State machine object
	CSSYSTEMINFO			SystemInfo;		//!< System information (Static)

	size_t					ChannelStructSize;	//<! size of the channel structure stored later: Acquisition, Commit, ...
	size_t					TriggerStructSize; //<! size of the trigger structure stored later: Acquisition, Commit, ...

	// Requested Configuration
	CSACQUISITIONCONFIG		AcquisitionConfig;		//!< Requested Acquisition configuration
	PARRAY_CHANNELCONFIG	pChannelConfig;			//!< Requested configuration for all channels
	PARRAY_TRIGGERCONFIG	pTriggerConfig;			//!< Requested configuration for all triggers
	CSSYSTEMCONFIG			SystemConfig;			//!< Requested configuration for current system. Links all previous elements.

	// Commited Configuration
	CSACQUISITIONCONFIG		Commit_AcquisitionConfig;	//!< Committed Acquisition configuration
	PARRAY_CHANNELCONFIG	pCommit_ChannelConfig;		//!< Committed configuration for all channels
	PARRAY_TRIGGERCONFIG	pCommit_TriggerConfig;		//!< Committed configuration for all triggers
	CSSYSTEMCONFIG			Commit_SystemConfig;		//!< Committed configuration for current system. Links all previous elements.

	// Acquired Configuration
	CSACQUISITIONCONFIG		Acquired_AcquisitionConfig;	//!< Acquired Acquisition configuration
	PARRAY_CHANNELCONFIG	pAcquired_ChannelConfig;	//!< Acquired configuration for all channels
	PARRAY_TRIGGERCONFIG	pAcquired_TriggerConfig;	//!< Acquired configuration for all triggers
	CSSYSTEMCONFIG			Acquired_SystemConfig;		//!< Acquired configuration for current system. Links all previous elements.

	// Asynchronous related variables
	WORD			wToken;				//!< Current valuie to be used to generate token
	int32			i32CurrentToken;	//!< Serialized token currently in use
	int32			i32TokenStatus;		//!< Status (Success or Error codes) of the current token

	CALLBACKLIST	EndAcqCallbackList;
	CALLBACKLIST	EndTxCallbackList;

#if defined(_WIN32)
	AsTxResultsList TokenResults;       //!< Transfer results of the last tokens 

	// Instance of CsDiskStream for DiskStream functionality
	CsDiskStream*	pDiskStream;

	void* operator new(size_t size ){return ::VirtualAlloc(NULL, sizeof(size), MEM_COMMIT, PAGE_READWRITE);}
	void* operator new[]( size_t size ){return ::VirtualAlloc(NULL, sizeof(size), MEM_COMMIT, PAGE_READWRITE);}
	void operator delete(void* p ){::VirtualFree(p,0, MEM_RELEASE);}
	void operator delete[]( void* p ){::VirtualFree(p,0, MEM_RELEASE);}
#else
	AS_TX_RESULTS_LIST	TokenResults;		//!< Transfer results of the last tokens
#endif

} SYSTEM_DATA, *PSYSTEM_DATA;

typedef map<CSHANDLE, PSYSTEM_DATA> SYSTEMDATAMAP;

class CSystemData
{
public:
	static CSystemData& instance();

	static void	FreeInstance();

	void	Lock();
	void	UnLock();
	int32 ClearSystem(CSHANDLE hSystem);

	int32 AddSystem(CSHANDLE);


	bool PerformTransition(CSHANDLE hSystem, int16 nAction);


	uInt32 GetChannelCount(CSHANDLE hSystem);


	uInt32 GetTriggerCount(CSHANDLE hSystem);


#ifdef _WIN32
	EVENT_HANDLE GetDrvEvent(CSHANDLE hSystem, uInt32 u32EventType);
#else
	int GetDrvEvent(CSHANDLE hSystem, uInt32 u32EventType);
#endif

	int32 GetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo);

	int32 GetSystemConfig(CSHANDLE hSystem, PCSSYSTEMCONFIG pConfig);


	int32 GetAcquisitionConfig(CSHANDLE hSystem, int32 nConfigType, PCSACQUISITIONCONFIG pConfig);

	int32 GetChannelConfig(CSHANDLE hSystem, int32 nConfigType, PCSCHANNELCONFIG pConfig);
	int32 GetArrayChannelConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_CHANNELCONFIG pConfig);

	int32 GetTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PCSTRIGGERCONFIG pConfig);
	int32 GetArrayTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_TRIGGERCONFIG pConfig);


	int32 SetAcquisitionConfig(CSHANDLE hSystem, PCSACQUISITIONCONFIG pConfig, bool bCompare = false);

	int32 SetChannelConfig(CSHANDLE hSystem, PCSCHANNELCONFIG pConfig, bool bCompare = false);
	int32 SetArrayChannelConfig(CSHANDLE hSystem, PARRAY_CHANNELCONFIG pConfig, bool bCompare = false);

	int32 SetTriggerConfig(CSHANDLE hSystem, PCSTRIGGERCONFIG pConfig, bool bCompare = false);
	int32 SetArrayTriggerConfig(CSHANDLE hSystem, PARRAY_TRIGGERCONFIG pConfig, bool bCompare = false);

	int32 UpdateCommittedConfig(CSHANDLE hSystem);
	int32 UpdateAcquiredConfig(CSHANDLE hSystem);
	
	// Reset current configuration from the committed configuration
	int32 RevertConfiguration(CSHANDLE hSystem);

	bool GetCommittedFlag(CSHANDLE hSystem);
	void SetCommittedFlag(CSHANDLE hSystem, bool bFlag);

	void LockSM(CSHANDLE hSystem, bool bEnable);


	bool IsSMLock(CSHANDLE hSystem);


	int32	ValidateSystemHandle( CSHANDLE hSystem );
	int32	InitializeSystem( CSHANDLE hSystem );

	// Asynchronous Transfer Token functions
	int32	GetToken(CSHANDLE hSystem, uInt16 u16Channel);
	int32	ValidateToken(CSHANDLE hSystem, int32 i32Cookie, int32* pi32Status = NULL, uInt8 u8Action = 0);
	int32	SetTokenStatus(CSHANDLE hSystem, int32 i32Token, int32 i32Status);

#if defined(_WIN32)
	int64	GetTokenResult(CSHANDLE hSystem, const int32 ci32Cookie);
	int32	GetTokenStatus(CSHANDLE hSystem, const int32 ci32Token);
#else
	int64	GetTokenResult(CSHANDLE hSystem, int32 i32Cookie);
	int32	GetTokenStatus(CSHANDLE hSystem, int32 i32Token);
#endif

	// Callbacks Function Support	
	int32	IsEventSupported(CSHANDLE hSystem);
	int32   HasTransferCallback(CSHANDLE hSystem);
	int32	SetEndAcqCallback(CSHANDLE hSystem, LPCsEventCallback pCallBack);
	int32	SetEndTxCallback(CSHANDLE hSystem, LPCsEventCallback pCallBack);
	
	void	EndAcqCallback(CSHANDLE hSystem);
	void	EndTxCallback(CSHANDLE hSystem);


	Worker* GetWorkerThread(CSHANDLE);


#if defined(_WIN32)
	CsDiskStream* GetDiskStreamPtr(CSHANDLE);
	void SetDiskStreamPtr(CSHANDLE, CsDiskStream* ptr);
#endif

	void Abort(CSHANDLE hSystem);

	// Debug purpose
	CStateMachine* GetStateMachine(CSHANDLE);
	
protected:
	CSystemData();

	CSystemData(const CSystemData&) {assert(FALSE);};
	CSystemData& operator=(const CSystemData&) {assert(FALSE);};

private:
	static CSystemData* _instance;
	static Critical_Section _key;

	SYSTEMDATAMAP m_SystemMap;

	// Current system cache data
	// CSHANDLE	m_hSystem; 
	// SYSTEM_DATA	m_SystemData;

	//size_t	m_ChannelStructSize;
	//size_t	m_TriggerStructSize;

};

inline void	CSystemData::Lock()
{
	_key.acquire();
}

inline void	CSystemData::UnLock()
{
	_key.release();
}

inline CSystemData& CSystemData::instance()
{
	if(NULL == _instance) // Lock-hint check (Race condition here)
	{	
		Lock_Guard<Critical_Section> gate(_key);

		if(NULL == _instance) // Double-check (Race condition resolved)
			_instance = new CSystemData;
	}
	return *_instance;
}
