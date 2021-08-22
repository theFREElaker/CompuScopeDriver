#include "StdAfx.h"
#include "CsSsm.h"

#if defined(_WIN32)

#include <process.h> // for _beginthreadex
_MutexLock AsTxResultsList::_key("_AsTxResultsList");

#endif

CSystemData* CSystemData::_instance = NULL;
Critical_Section CSystemData::_key;




CSystemData::CSystemData()
{
}

void CSystemData::FreeInstance()
{
	if(NULL == _instance) // Lock-hint check (Race condition here)
	{	
		Lock_Guard<Critical_Section> gate(_key);

		if(NULL == _instance) // Double-check (Race condition resolved)
		{
			return;
		}
	}
	

	SYSTEMDATAMAP::iterator theIterator;

	while (0 != _instance->m_SystemMap.size())
	{
		theIterator = _instance->m_SystemMap.begin();

		// Free System will cleanup and erase the handle from the system map
		if( CS_FAILED(CsFreeSystem((*theIterator).first) ) )
		{
			_instance->m_SystemMap.erase(theIterator);
		}
	}

	delete _instance;
	_instance = NULL;
}

int32 CSystemData::ValidateSystemHandle(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) == m_SystemMap.end())
	{
		return (CS_INVALID_HANDLE);
	}

	return (CS_SUCCESS);
}


//!
//! \ingroup SSM_API
//! \fn	int32 AddSystem(CSHANDLE);
//!	\brief Add and initialize system for the process
//! 
//!	\param CSHANDLE : System to be initialized.
//! 
//! \return
//!		  - CS_SUCCESS: Operation was successful.
//!       - CS_FALSE: System was already present.
//!		  - CS_MEMORY_ERROR: Not enough memory.
//!		  - CS_INVALID_STRUCT_SIZE: Invalid struct size for configuration elements.
/*************************************************************/
int32 CSystemData::AddSystem(CSHANDLE hSystem)
{
	int32 i32RetVal = CS_SUCCESS;

	SYSTEMDATAMAP::iterator theIterator;

	//m_ChannelStructSize		= 0;
	//m_TriggerStructSize		= 0;

	// Check if handle has already been added.
	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		return (CS_FALSE);
	}

#if defined(_WIN32)
	PSYSTEM_DATA pSystemData = new SYSTEM_DATA;
#else
	PSYSTEM_DATA pSystemData = static_cast<PSYSTEM_DATA>(::VirtualAlloc(0, sizeof(SYSTEM_DATA), MEM_COMMIT, PAGE_READWRITE));
#endif

	if (NULL == pSystemData)
		return (CS_MEMORY_ERROR);

	::ZeroMemory(pSystemData, sizeof(SYSTEM_DATA));

	pSystemData->hLockedSystem = hSystem;
	pSystemData->bSMLock = false;
	pSystemData->bCommitedConfiguration = false; // Default: Commited and Acquired structure are the same
	
	// Create System State Machine
	pSystemData->pStateMachine = new CStateMachine(hSystem);
	if (NULL == pSystemData->pStateMachine)
		return (CS_MEMORY_ERROR);

	pSystemData->pStateMachine->PerformTransition(INIT_TRANSITION);
	i32RetVal = pSystemData->pStateMachine->RegisterDriverEvents(hSystem);

	// Create Worker Thread for events communication with driver
	if (CS_FAILED(i32RetVal))
	{
#if defined(_WIN32)
		pSystemData->hThread = NULL;
#endif
		pSystemData->bSMLock = true;
	}
	else
	{
#if defined(_WIN32)
		pSystemData->hThread = (HANDLE)::_beginthreadex(NULL, 0, ThreadFunc, pSystemData->pStateMachine, 0, (unsigned int *)&pSystemData->dwThreadID);
		::SetThreadPriority(pSystemData->hThread, THREAD_PRIORITY_HIGHEST);

		pSystemData->hThreadTxfer = (HANDLE)::_beginthreadex(NULL, 0, ThreadFuncTxfer, pSystemData->pStateMachine, 0, (unsigned int *)&pSystemData->dwThreadIDTxfer);
		::SetThreadPriority(pSystemData->hThreadTxfer, THREAD_PRIORITY_HIGHEST);
#else
		pSystemData->dwThreadID = ::pthread_create(&pSystemData->hThread, NULL, &ThreadFunc, pSystemData->pStateMachine);
		pSystemData->dwThreadIDTxfer = ::pthread_create(&pSystemData->hThreadTxfer, NULL, &ThreadFuncTxfer, pSystemData->pStateMachine);

#endif
	}

	// Create Worker Thread for Data Transfer
	pSystemData->pWorker = new Worker();

#if defined(_WIN32)
	// Initialize pDiskStream, even if we aren't going to use it
	pSystemData->pDiskStream = NULL;
#endif

	m_SystemMap.insert(SYSTEMDATAMAP::value_type(hSystem, pSystemData));

	// Retrieve System  Configuration
	return InitializeSystem( hSystem );

}


int32 CSystemData::InitializeSystem( CSHANDLE hSystem )
{
	int32	RetCode = -1;
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) == m_SystemMap.end())
	{
		return (CS_INVALID_HANDLE);
	}

	PSYSTEM_DATA pSystemData = (PSYSTEM_DATA)(*theIterator).second;
	
	pSystemData->wToken = 0;

	// Retrieve System  Configuration
	pSystemData->SystemInfo.u32Size = sizeof(CSSYSTEMINFO);
	RetCode = ::CsDrvGetAcquisitionSystemInfo(hSystem, &pSystemData->SystemInfo);
	if (CS_FAILED(RetCode))
		return (RetCode);

	// Retrieve default configuration from locked system (Default Config)
	ZeroMemory(&pSystemData->AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));
	pSystemData->AcquisitionConfig.u32Size = sizeof(CSACQUISITIONCONFIG);
	RetCode = ::CsDrvGetParams(hSystem, CS_ACQUISITION, &pSystemData->AcquisitionConfig);
	if (CS_FAILED(RetCode))
		return (RetCode);

	// Copy data to Commited configuration and Acquired Configuration
	memcpy(&pSystemData->Commit_AcquisitionConfig, &pSystemData->AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));
	memcpy(&pSystemData->Acquired_AcquisitionConfig, &pSystemData->AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));

	// Allocate memory for channel configurations
	pSystemData->ChannelStructSize = sizeof(ARRAY_CHANNELCONFIG) + ((pSystemData->SystemInfo.u32ChannelCount-1)* sizeof(CSCHANNELCONFIG));
#if defined(_WIN32)
	pSystemData->pChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(NULL, pSystemData->ChannelStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	pSystemData->pChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(0, pSystemData->ChannelStructSize, MEM_COMMIT, PAGE_READWRITE);
#endif

	pSystemData->pChannelConfig->u32ChannelCount = pSystemData->SystemInfo.u32ChannelCount;
	for (uInt16 i = 0; i < pSystemData->pChannelConfig->u32ChannelCount; i++)
	{
		ZeroMemory(&pSystemData->pChannelConfig->aChannel[i], sizeof(CSCHANNELCONFIG));
		pSystemData->pChannelConfig->aChannel[i].u32Size = sizeof(CSCHANNELCONFIG);
		pSystemData->pChannelConfig->aChannel[i].u32ChannelIndex = i+1; // Channel is 1-based
	}

	RetCode = ::CsDrvGetParams(hSystem, CS_CHANNEL, pSystemData->pChannelConfig);
	if (CS_FAILED(RetCode))
		return (RetCode);

	// Copy data to Commited configuration and Acquired Configuration
#if defined(_WIN32)
	pSystemData->pCommit_ChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(NULL, pSystemData->ChannelStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	pSystemData->pAcquired_ChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(NULL, pSystemData->ChannelStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	pSystemData->pCommit_ChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(0, pSystemData->ChannelStructSize, MEM_COMMIT, PAGE_READWRITE);
	pSystemData->pAcquired_ChannelConfig = (PARRAY_CHANNELCONFIG)VirtualAlloc(0, pSystemData->ChannelStructSize, MEM_COMMIT, PAGE_READWRITE);
#endif

	memcpy(pSystemData->pCommit_ChannelConfig, pSystemData->pChannelConfig, pSystemData->ChannelStructSize);
	memcpy(pSystemData->pAcquired_ChannelConfig, pSystemData->pChannelConfig, pSystemData->ChannelStructSize);

	// Default Trigger Configuration
	// Allocate memory for trigger configurations
	pSystemData->TriggerStructSize = sizeof(ARRAY_TRIGGERCONFIG)+((pSystemData->SystemInfo.u32TriggerMachineCount-1)* sizeof(CSTRIGGERCONFIG));
#if defined(_WIN32)
	pSystemData->pTriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(NULL, pSystemData->TriggerStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	pSystemData->pTriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(0, pSystemData->TriggerStructSize, MEM_COMMIT, PAGE_READWRITE);
#endif

	pSystemData->pTriggerConfig->u32TriggerCount = pSystemData->SystemInfo.u32TriggerMachineCount;

	for (uInt16 i = 0; i < pSystemData->pTriggerConfig->u32TriggerCount; i++)
	{
		ZeroMemory(&pSystemData->pTriggerConfig->aTrigger[i], sizeof(CSTRIGGERCONFIG));
		pSystemData->pTriggerConfig->aTrigger[i].u32Size = sizeof(CSTRIGGERCONFIG);
		pSystemData->pTriggerConfig->aTrigger[i].u32TriggerIndex = i+1; // Channel is 1-based
	}

	RetCode = ::CsDrvGetParams(hSystem, CS_TRIGGER, pSystemData->pTriggerConfig);
	if (CS_FAILED(RetCode))
		return (RetCode);

	// Copy data to Commited configuration and Acquired Configuration
#if defined(_WIN32)
	pSystemData->pCommit_TriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(NULL, pSystemData->TriggerStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	pSystemData->pAcquired_TriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(NULL, pSystemData->TriggerStructSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	pSystemData->pCommit_TriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(0, pSystemData->TriggerStructSize, MEM_COMMIT, PAGE_READWRITE);
	pSystemData->pAcquired_TriggerConfig = (PARRAY_TRIGGERCONFIG)VirtualAlloc(0, pSystemData->TriggerStructSize, MEM_COMMIT, PAGE_READWRITE);
#endif
	memcpy(pSystemData->pCommit_TriggerConfig, pSystemData->pTriggerConfig, pSystemData->TriggerStructSize);
	memcpy(pSystemData->pAcquired_TriggerConfig, pSystemData->pTriggerConfig, pSystemData->TriggerStructSize);

	pSystemData->SystemConfig.u32Size    = sizeof(CSSYSTEMCONFIG);
	pSystemData->SystemConfig.pAcqCfg    = &pSystemData->AcquisitionConfig;
	pSystemData->SystemConfig.pAChannels = pSystemData->pChannelConfig;
	pSystemData->SystemConfig.pATriggers = pSystemData->pTriggerConfig;

	pSystemData->Commit_SystemConfig.u32Size    = sizeof(CSSYSTEMCONFIG);
	pSystemData->Commit_SystemConfig.pAcqCfg    = &pSystemData->Commit_AcquisitionConfig;
	pSystemData->Commit_SystemConfig.pAChannels = pSystemData->pCommit_ChannelConfig;
	pSystemData->Commit_SystemConfig.pATriggers = pSystemData->pCommit_TriggerConfig;

	pSystemData->Acquired_SystemConfig.u32Size    = sizeof(CSSYSTEMCONFIG);
	pSystemData->Acquired_SystemConfig.pAcqCfg    = &pSystemData->Acquired_AcquisitionConfig;
	pSystemData->Acquired_SystemConfig.pAChannels = pSystemData->pAcquired_ChannelConfig;
	pSystemData->Acquired_SystemConfig.pATriggers = pSystemData->pAcquired_TriggerConfig;

	return (CS_SUCCESS);

}


//!
//! \ingroup SSM_API
//! \fn	int32 ClearSystem(CSHANDLE);
//!	\brief Remove and cleanup the system for the process
//! 
//!	\param hSystem : Handle of system to be released.
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful.
//!		  - CS_INVALID_HANDLE: Handle is invalid. 	
/*************************************************************/
int32 CSystemData::ClearSystem(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if ((*theIterator).second->hThread)
		{
			// Terminate thread and close handle
			::SetEvent((*theIterator).second->pStateMachine->GetSystemReleaseEvent());

			// Wait for the event that comes from the thread before it ends rather than
			// wait for the thread id.  Waiting for the thread id was not working 
			// under LabVIEW, probably because of the LabVIEW message loop
			::WaitForSingleObject((*theIterator).second->pStateMachine->GetThreadEndEvent(), INFINITE);
			::WaitForSingleObject((*theIterator).second->pStateMachine->GetThreadEndEventTxfer(), INFINITE);

#if defined(_WIN32)
			::CloseHandle((*theIterator).second->hThread);//????
			::CloseHandle((*theIterator).second->hThreadTxfer);//????
#endif

		}

		delete (*theIterator).second->pWorker;

#if defined(_WIN32)
		// Check if pDiskStream has been used and not destroyed. If so we can
		// still destroy it here
		if ( NULL != (*theIterator).second->pDiskStream )
		{
			delete (*theIterator).second->pDiskStream;
			(*theIterator).second->pDiskStream = NULL;
		}
#endif

		// Destroy system state machine
		delete (*theIterator).second->pStateMachine;

		// Clean Channel configuration
		if ((*theIterator).second->pChannelConfig)
		{
			// Free all the cache structure for the channel configuration
			::VirtualFree((*theIterator).second->pAcquired_ChannelConfig, 0, MEM_RELEASE);
			(*theIterator).second->pAcquired_ChannelConfig = NULL;

			::VirtualFree((*theIterator).second->pCommit_ChannelConfig, 0, MEM_RELEASE);
			(*theIterator).second->pCommit_ChannelConfig = NULL;

			::VirtualFree((*theIterator).second->pChannelConfig, 0, MEM_RELEASE);
			(*theIterator).second->pChannelConfig = NULL;
		}

		_ASSERT(NULL != (*theIterator).second->pTriggerConfig);
		if ((*theIterator).second->pTriggerConfig)
		{
			_ASSERT(NULL != (*theIterator).second->pCommit_TriggerConfig);

			// Free all the cache structure for the trigger configuration
			::VirtualFree((*theIterator).second->pAcquired_TriggerConfig, 0, MEM_RELEASE);
			(*theIterator).second->pAcquired_TriggerConfig = NULL;

			::VirtualFree((*theIterator).second->pCommit_TriggerConfig, 0, MEM_RELEASE);
			(*theIterator).second->pCommit_TriggerConfig = NULL;

			::VirtualFree((*theIterator).second->pTriggerConfig, 0, MEM_RELEASE);
			(*theIterator).second->pTriggerConfig = NULL;
		}

		(*theIterator).second->TokenResults.clear();
	
#if defined(_WIN32)
		//::VirtualFree((*theIterator).second, 0, MEM_RELEASE);
		delete (*theIterator).second;
#else
		::VirtualFree((*theIterator).second, 0, MEM_RELEASE);
#endif

		m_SystemMap.erase(theIterator);

		//m_ChannelStructSize		= 0;
		//m_TriggerStructSize		= 0;

		return (CS_SUCCESS);
	}

	return (CS_INVALID_HANDLE);
}


//!
//! \ingroup SSM_API
//! \fn	int32 GetChannelCount(CSHANDLE hSystem);
//!	\brief Return the channel count with respect of the mode
//! 
//!	\param hSystem : System to query
//! 
//! \return
//!		  - > 0 : Channel count of system 
//!       - ==0 : Error: invalid handle 
/*************************************************************/
uInt32 CSystemData::GetChannelCount(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if (1 == (*theIterator).second->AcquisitionConfig.u32Mode)
			return ((*theIterator).second->pChannelConfig->u32ChannelCount);
		else
			return ((*theIterator).second->pChannelConfig->u32ChannelCount);
	}
	
	return (0);
}


//!
//! \ingroup SSM_API
//! \fn	int32 GetTriggerCount(CSHANDLE hSystem);
//!	\brief Return the number of configured trigger
//! 
//!	\param hSystem : System to query
//! 
//! \return
//!		  - > 0 : Trigger count of system 
//!       - ==0 : Error: invalid handle
/*************************************************************/
uInt32 CSystemData::GetTriggerCount(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		return ((*theIterator).second->pTriggerConfig->u32TriggerCount);
	}
	
	return (0);
}

//!
//! \ingroup SSM_API
//! \fn	int32 GetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo)
//!	\brief Eric
//! 
//!	\param hSystem : System to query for configuration
//!	\param pSystemInfo : Pointer to system information structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!
/*************************************************************/
int32 CSystemData::GetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Keep the requested structure size for restoration in case size are different
		uInt32 u32RequestedSize = pSystemInfo->u32Size;

		memcpy(pSystemInfo, &((*theIterator).second->SystemInfo), pSystemInfo->u32Size);
		pSystemInfo->u32Size = u32RequestedSize;
		return (CS_SUCCESS);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 GetSystemConfig(CSHANDLE hSystem, PCSSYSTEMCONFIG pConfig)
//!	\brief Eric
//! 
//!	\param hSystem : System to query for configuration
//!	\param pConfig : Pointer to system configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetAcquisitionConfig, GetChannelConfig, GetTriggerConfig
/*************************************************************/
int32 CSystemData::GetSystemConfig(CSHANDLE hSystem, PCSSYSTEMCONFIG pConfig)
{
	SYSTEMDATAMAP::iterator theIterator;

	// Internal usage only.
	// This function only copy the pointers value to be passed internally to the driver for 
	// validation or systems capabilities...
	// Internal usage only.
	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		memcpy(pConfig, &((*theIterator).second->SystemConfig), pConfig->u32Size);
		return (CS_SUCCESS);
	}
	
	return (CS_INVALID_HANDLE);
}


//!
//! \ingroup SSM_API
//! \fn	int32 GetAcquisitionConfig(CSHANDLE hSystem, int32 nConfigType, PCSACQUISITIONCONFIG pConfig)
//!	\brief Retrieve the acquisition configuration for the system
//! 
//!	\param hSystem : System to query for configuration
//!	\param nConfigType : Type of configuration: CS_CURRENT_CONFIGURATION, CS_ACQUISITION_CONFIGURATION or CS_ACQUIRED_CONFIGURATION
//!	\param pConfig : Pointer to acquisition configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!		  - CS_INVALID_REQUEST: Requested configuration is invalid.
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetSystemConfig, GetChannelConfig, GetTriggerConfig
/*************************************************************/
int32 CSystemData::GetAcquisitionConfig(CSHANDLE hSystem, int32 nConfigType, PCSACQUISITIONCONFIG pConfig)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Keep the requested structure size for restoration in case size are different
		uInt32 u32RequestedSize = pConfig->u32Size;

		if (CS_CURRENT_CONFIGURATION == nConfigType)
		{

			memcpy(pConfig, &((*theIterator).second->AcquisitionConfig), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUISITION_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->Commit_AcquisitionConfig), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUIRED_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->Acquired_AcquisitionConfig), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}

		return (CS_INVALID_REQUEST);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 SetAcquisitionConfig(CSHANDLE hSystem, PCSACQUISITIONCONFIG pConfig)
//!	\brief Replace the current acquisition configuration with the new parameters
//! 
//!	\param hSystem : Handle of system to modify
//!	\param pConfig : Pointer to the newly acquisition configuration
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetChannelConfig, SetTriggerConfig, UpdateCommittedConfig
/*************************************************************/
int32 CSystemData::SetAcquisitionConfig(CSHANDLE hSystem, PCSACQUISITIONCONFIG pConfig, bool bCompare)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		int32 RetVal = CS_SUCCESS;

		if (bCompare)
		{
			if (0 != memcmp(&((*theIterator).second->AcquisitionConfig), pConfig, pConfig->u32Size))
				RetVal = CS_CONFIG_CHANGED;
		}

		memcpy(&((*theIterator).second->AcquisitionConfig), pConfig, pConfig->u32Size);
		(*theIterator).second->AcquisitionConfig.u32Size = sizeof(CSACQUISITIONCONFIG);			

		return (RetVal);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 CSystemData::GetArrayChannelConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_CHANNELCONFIG pConfig)
//!	\brief Retrieve the requested array channel configuration for the system
//! 
//!	\param hSystem : System to query for configuration
//!	\param nConfigType : Type of configuration: CS_CURRENT_CONFIGURATION, CS_ACQUISITION_CONFIGURATION or CS_ACQUIRED_CONFIGURATION
//!	\param pConfig : Pointer to the array of channels configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!		  - CS_INVALID_REQUEST: Requested configuration is invalid.
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetSystemConfig, GetAcquisitionConfig, GetTriggerConfig
/*************************************************************/
int32 CSystemData::GetArrayChannelConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_CHANNELCONFIG pConfig)
{
	int32				i32RetVal = CS_SUCCESS;
	PCSCHANNELCONFIG	pChannel = NULL;
	size_t				sizeStruct = 0;
	
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		for (uInt32 i = 0; i < pConfig->u32ChannelCount; i++)
		{
			pChannel = (PCSCHANNELCONFIG)&pConfig->aChannel[i];

			sizeStruct = pChannel->u32Size;
			if (sizeStruct > sizeof(CSCHANNELCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pChannel, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (pChannel->u32ChannelIndex && CSystemData::instance().GetChannelCount(hSystem) < pChannel->u32ChannelIndex)
				return (CS_INVALID_CHANNEL);

			i32RetVal = GetChannelConfig(hSystem, nConfigType, pChannel);
			if (CS_SUCCESS != i32RetVal)
				return (i32RetVal);
		}
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 GetChannelConfig(CSHANDLE hSystem, int32 nConfigType, PCSCHANNELCONFIG pConfig)
//!	\brief Retrieve channel configuration for the system
//! 
//!	\param hSystem : System to query for configuration
//!	\param nConfigType : Type of configuration: CS_CURRENT_CONFIGURATION, CS_ACQUISITION_CONFIGURATION or CS_ACQUIRED_CONFIGURATION
//!	\param pConfig : Pointer to channels configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!		  - CS_INVALID_REQUEST: Requested configuration is invalid.
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetSystemConfig, GetAcquisitionConfig, GetTriggerConfig
/*************************************************************/
int32 CSystemData::GetChannelConfig(CSHANDLE hSystem, int32 nConfigType, PCSCHANNELCONFIG pConfig)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Keep the requested structure size for restoration in case size are different
		uInt32 u32RequestedSize = pConfig->u32Size;

		if (CS_CURRENT_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pChannelConfig->aChannel[pConfig->u32ChannelIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUISITION_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pCommit_ChannelConfig->aChannel[pConfig->u32ChannelIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUIRED_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pAcquired_ChannelConfig->aChannel[pConfig->u32ChannelIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}

		return (CS_INVALID_REQUEST);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn		int32 SetArrayChannelConfig(CSHANDLE hSystem, PARRAY_CHANNELCONFIG pConfig, bool bCompare);
//!	\brief Replace the current array of channel configuration with the new parameters
//! 
//!	\param hSystem : Handle of system to modify
//!	\param pConfig : Pointer to the array of new channels configuration
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
/*************************************************************/
int32 CSystemData::SetArrayChannelConfig(CSHANDLE hSystem, PARRAY_CHANNELCONFIG pConfig, bool bCompare)
{
	int32				i32RetVal = CS_SUCCESS;
	PCSCHANNELCONFIG	pChannel = NULL;
	size_t				sizeStruct = 0;
	
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		for (uInt32 i = 0; i < pConfig->u32ChannelCount; i++)
		{
			pChannel = (PCSCHANNELCONFIG)&pConfig->aChannel[i];

			sizeStruct = pChannel->u32Size;
			if (sizeStruct > sizeof(CSCHANNELCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadReadPtr(pChannel, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (pChannel->u32ChannelIndex && CSystemData::instance().GetChannelCount(hSystem) < pChannel->u32ChannelIndex)
				return (CS_INVALID_CHANNEL);

			i32RetVal = SetChannelConfig(hSystem, pChannel, bCompare);
			if (CS_SUCCESS != i32RetVal && CS_CONFIG_CHANGED != i32RetVal)
				return (i32RetVal);
		}
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn		int32 SetChannelConfig(CSHANDLE hSystem, PCSCHANNELCONFIG pConfig);
//!	\brief Replace the current channel configuration with the new parameters
//! 
//!	\param hSystem : Handle of system to modify
//!	\param pConfig : Pointer to the newly channels configuration
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetAcquisitionConfig, SetTriggerConfig, UpdateCommittedConfig
/*************************************************************/
int32 CSystemData::SetChannelConfig(CSHANDLE hSystem, PCSCHANNELCONFIG pConfig, bool bCompare)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		int32 RetVal = CS_SUCCESS;

		// channel index cannot be greater than the channel count
		if (pConfig->u32ChannelIndex > (*theIterator).second->SystemInfo.u32ChannelCount)
			return (CS_INVALID_CHANNEL);

		if (bCompare)
		{
			if (0 != memcmp(&((*theIterator).second->pChannelConfig->aChannel[pConfig->u32ChannelIndex-1]), pConfig, pConfig->u32Size))
				RetVal = CS_CONFIG_CHANGED;
		}

		memcpy(&((*theIterator).second->pChannelConfig->aChannel[pConfig->u32ChannelIndex-1]), pConfig, pConfig->u32Size);
		(*theIterator).second->pChannelConfig->aChannel[pConfig->u32ChannelIndex-1].u32Size = sizeof(CSCHANNELCONFIG); 

		return (RetVal);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 CSystemData::GetArrayTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_TRIGGERCONFIG pConfig)
//!	\brief Retrieve the requested array of trigger configuration for the system
//! 
//!	\param hSystem : System to query for configuration
//!	\param nConfigType : Type of configuration: CS_CURRENT_CONFIGURATION, CS_ACQUISITION_CONFIGURATION or CS_ACQUIRED_CONFIGURATION
//!	\param pConfig : Pointer to the array of triggers configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!		  - CS_INVALID_REQUEST: Requested configuration is invalid.
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetSystemConfig, GetAcquisitionConfig, GetTriggerConfig
/*************************************************************/
int32 CSystemData::GetArrayTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PARRAY_TRIGGERCONFIG pConfig)
{
	int32				i32RetVal = CS_SUCCESS;
	PCSTRIGGERCONFIG	pTrigger = NULL;
	size_t				sizeStruct = 0;
	
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		for (uInt32 i = 0; i < pConfig->u32TriggerCount; i++)
		{
			pTrigger = (PCSTRIGGERCONFIG)&pConfig->aTrigger[i];

			sizeStruct = pTrigger->u32Size;
			if (sizeStruct > sizeof(CSTRIGGERCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pTrigger, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (!pTrigger->u32TriggerIndex || CSystemData::instance().GetTriggerCount(hSystem) < pTrigger->u32TriggerIndex)
				return (CS_INVALID_TRIGGER);

			i32RetVal = GetTriggerConfig(hSystem, nConfigType, pTrigger);
			if (CS_SUCCESS != i32RetVal)
				return (i32RetVal);
		}
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 GetTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PCSTRIGGERCONFIG pConfig)
//!	\brief Retrieve trigger configuration for the system
//! 
//!	\param hSystem : System to query for configuration
//!	\param nConfigType : Type of configuration: CS_CURRENT_CONFIGURATION, CS_ACQUISITION_CONFIGURATION or CS_ACQUIRED_CONFIGURATION
//!	\param pConfig : Pointer to channels configuration structure
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!		  - CS_INVALID_REQUEST: Requested configuration is invalid.
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa GetSystemConfig, GetAcquisitionConfig, GetChannelConfig
/*************************************************************/
int32 CSystemData::GetTriggerConfig(CSHANDLE hSystem, int32 nConfigType, PCSTRIGGERCONFIG pConfig)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Keep the requested structure size for restoration in case size are different
		uInt32 u32RequestedSize = pConfig->u32Size;

		if (CS_CURRENT_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pTriggerConfig->aTrigger[pConfig->u32TriggerIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUISITION_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pCommit_TriggerConfig->aTrigger[pConfig->u32TriggerIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}
		else if (CS_ACQUIRED_CONFIGURATION == nConfigType)
		{
			memcpy(pConfig, &((*theIterator).second->pAcquired_TriggerConfig->aTrigger[pConfig->u32TriggerIndex-1]), pConfig->u32Size);
			pConfig->u32Size = u32RequestedSize;
			return (CS_SUCCESS);
		}


		return (CS_INVALID_REQUEST);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 SetArrayTriggerConfig(CSHANDLE hSystem, PCSTRIGGERCONFIG pConfig, bool bCompare)
//!	\brief Replace the current trigger configuration with the new parameters
//! 
//!	\param hSystem : Handle of system to modify
//!	\param pConfig : Pointer to the newly triggers configuration
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetAcquisitionConfig, SetChannelConfig, UpdateCommittedConfig
/*************************************************************/
int32 CSystemData::SetArrayTriggerConfig(CSHANDLE hSystem, PARRAY_TRIGGERCONFIG pConfig, bool bCompare)
{
	int32				i32RetVal = CS_SUCCESS;
	PCSTRIGGERCONFIG	pTrigger = NULL;
	size_t				sizeStruct = 0;
	
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		for (uInt32 i = 0; i < pConfig->u32TriggerCount; i++)
		{
			pTrigger = (PCSTRIGGERCONFIG)&pConfig->aTrigger[i];

			sizeStruct = pTrigger->u32Size;
			if (sizeStruct > sizeof(CSTRIGGERCONFIG) || 0 == sizeStruct)
				return (CS_INVALID_STRUCT_SIZE);

			if (::IsBadWritePtr(pTrigger, sizeStruct))
				return (CS_INVALID_POINTER_BUFFER);

			if (!pTrigger->u32TriggerIndex || CSystemData::instance().GetTriggerCount(hSystem) < pTrigger->u32TriggerIndex)
				return (CS_INVALID_TRIGGER);

			i32RetVal = SetTriggerConfig(hSystem, pTrigger, bCompare);
			if (CS_SUCCESS != i32RetVal && CS_CONFIG_CHANGED != i32RetVal)
				return (i32RetVal);
		}
	}
	
	return (CS_INVALID_HANDLE);
}


//!
//! \ingroup SSM_API
//! \fn	int32 SetTriggerConfig(CSHANDLE hSystem, PCSTRIGGERCONFIG pConfig)
//!	\brief Replace the current trigger configuration with the new parameters
//! 
//!	\param hSystem : Handle of system to modify
//!	\param pConfig : Pointer to the newly triggers configuration
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetAcquisitionConfig, SetChannelConfig, UpdateCommittedConfig
/*************************************************************/
int32 CSystemData::SetTriggerConfig(CSHANDLE hSystem, PCSTRIGGERCONFIG pConfig, bool bCompare)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// channel index cannot be greater than the channel count
		if (pConfig->u32TriggerIndex > (*theIterator).second->SystemInfo.u32TriggerMachineCount)
			return (CS_INVALID_TRIGGER);

		int32 RetVal = CS_SUCCESS;
		if (bCompare)
		{
			if (0 != memcmp(&((*theIterator).second->pTriggerConfig->aTrigger[pConfig->u32TriggerIndex-1]), pConfig, pConfig->u32Size))
				RetVal = CS_CONFIG_CHANGED;
		}

		memcpy(&((*theIterator).second->pTriggerConfig->aTrigger[pConfig->u32TriggerIndex-1]), pConfig, pConfig->u32Size);
		(*theIterator).second->pTriggerConfig->aTrigger[pConfig->u32TriggerIndex-1].u32Size = sizeof(CSTRIGGERCONFIG);
		return (RetVal);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 UpdateCommittedConfig(CSHANDLE hSystem);
//!	\brief Update the structure to reflect the changes in the Acquisition structure
//! 
//!	\param hSystem : Handle of system to update
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetAcquisitionConfig, SetChannelConfig, SetTriggerConfig
/*************************************************************/
int32 CSystemData::UpdateCommittedConfig(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		PSYSTEM_DATA pSystemData = static_cast<PSYSTEM_DATA>((*theIterator).second);
		_ASSERT(pSystemData);

		if (pSystemData)
		{
			// Acquisition Config
			memcpy(&pSystemData->Commit_AcquisitionConfig, &pSystemData->AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));

			// Channel Config
			memcpy(pSystemData->pCommit_ChannelConfig, pSystemData->pChannelConfig, pSystemData->ChannelStructSize);

			// Trigger Config
			memcpy(pSystemData->pCommit_TriggerConfig, pSystemData->pTriggerConfig, pSystemData->TriggerStructSize);

			return (CS_SUCCESS);
		}

		return (CS_FALSE);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 UpdateCommittedConfig(CSHANDLE hSystem);
//!	\brief Update the structure to reflect the changes in the Acquisition structure
//! 
//!	\param hSystem : Handle of system to update
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
//!
//! \sa SetAcquisitionConfig, SetChannelConfig, SetTriggerConfig
/*************************************************************/
int32 CSystemData::UpdateAcquiredConfig(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		PSYSTEM_DATA pSystemData = static_cast<PSYSTEM_DATA>((*theIterator).second);
		_ASSERT(pSystemData);

		if (pSystemData)
		{
			// Acquisition Config
			memcpy(&pSystemData->Acquired_AcquisitionConfig, &pSystemData->Commit_AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));

			// Channel Config
			memcpy(pSystemData->pAcquired_ChannelConfig, pSystemData->pCommit_ChannelConfig, pSystemData->ChannelStructSize);

			// Trigger Config
			memcpy(pSystemData->pAcquired_TriggerConfig, pSystemData->pCommit_TriggerConfig, pSystemData->TriggerStructSize);

			return (CS_SUCCESS);
		}

		return (CS_FALSE);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	int32 CSystemData::RevertConfiguration(CSHANDLE hSystem)
//!	\brief Reset the configuration with the board commited configuration
//! 
//!	\param hSystem : Handle of system to update
//! 
//! \return
//!       - CS_SUCCESS: Operation was successful
//!       - CS_INVALID_HANDLE: System handle is invalid
/*************************************************************/
int32 CSystemData::RevertConfiguration(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		PSYSTEM_DATA pSystemData = static_cast<PSYSTEM_DATA>((*theIterator).second);
		_ASSERT(pSystemData);

		if (pSystemData)
		{
			// Acquisition Config
			memcpy(&pSystemData->AcquisitionConfig, &pSystemData->Commit_AcquisitionConfig, sizeof(CSACQUISITIONCONFIG));

			// Channel Config
			memcpy(pSystemData->pChannelConfig, pSystemData->pCommit_ChannelConfig, pSystemData->ChannelStructSize);

			// Trigger Config
			memcpy(pSystemData->pTriggerConfig, pSystemData->pCommit_TriggerConfig, pSystemData->TriggerStructSize);

			return (CS_SUCCESS);
		}

		return (CS_FALSE);
	}
	
	return (CS_INVALID_HANDLE);
}

//!
//! \ingroup SSM_API
//! \fn	bool GetCommittedFlag(CSHANDLE hSystem)
//!	\brief Return the Commit action occured flag
//! 
//!	\param hSystem : Handle of system to retrieve info
//! 
//! \return
//!       - No return value.
/*************************************************************/
bool CSystemData::GetCommittedFlag(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		return ((*theIterator).second->bCommitedConfiguration);
	}

	return (false);
}

//!
//! \ingroup SSM_API
//! \fn	void SetCommittedFlag(CSHANDLE hSystem, bool bFlag)
//!	\brief Set the Commit flag for sync between Acquired config and Commited config
//! 
//!	\param hSystem : Handle of system to update
//!	\param bFlag : true Commit occured. false. No Commit
//! 
//! \return
//!       - No return value.
/*************************************************************/
void CSystemData::SetCommittedFlag(CSHANDLE hSystem, bool bFlag)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		(*theIterator).second->bCommitedConfiguration = bFlag;
	}
}

//!
//! \ingroup SSM_API
//! \fn	int32 LockSM(CSHANDLE hSystem, bool bEnable);
//!	\brief Lock/Unkock State machine transition for performance improvement
//! 
//!	\param hSystem : Handle of system to update
//!	\param bEnable : true to lock SM. false to unlock.
//! 
//! \return
//!       - No return value.
/*************************************************************/
void CSystemData::LockSM(CSHANDLE hSystem, bool bEnable)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		(*theIterator).second->bSMLock = bEnable;
		
		// Propagate to the CStateMachine
		(*theIterator).second->pStateMachine->LockSM(bEnable);
	}
}

void CSystemData::Abort(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Propagate to the CStateMachine
		(*theIterator).second->pStateMachine->Abort(true);
	}
}

//!
//! \ingroup SSM_API
//! \fn	bool IsSMLock(CSHANDLE hSystem);
//!	\brief Return State Machine lock status
//! 
//!	\param hSystem : Handle of system to update
//! 
//! \return
//!       - Lock status: true is locked. false is unlocked.
/*************************************************************/
bool CSystemData::IsSMLock(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		return ((*theIterator).second->bSMLock);
	}

	return (false);
}

//!
//! \ingroup SSM_API
//! \fn	bool PerformTransition(CSHANDLE hSystem, int16 nAction, LPTSTR szStateName, int16 nSize)
//!	\brief Perform the transition on the State Machine of the system
//! 
//!	\param hSystem : Handle of system for the transition
//!	\param nAction : State machine transition
//!	\param szStateName : Output buffer for the result state name. (Can be NULL)
//!	\param nSize : Size of buffer szStateName
//! 
//! \return
//!       - true (!= 0): Operation was successful
//!		  - false (==0): Invalid handle or invalid action
/*************************************************************/
bool CSystemData::PerformTransition(CSHANDLE hSystem, int16 nAction)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		return ((*theIterator).second->pStateMachine->PerformTransition(nAction));
	}

	return (false);
}


//!
//! \ingroup SSM_API
//! \fn	HANDLE GetDrvEvent(CSHANDLE hSystem, int16 EventType);
//!	\brief Return the handle of the requested event: Busy, Trigger
//! 
//!	\param hSystem : System to query for information
//!	\param u32EventType : Event requested: ACQ_EVENT_END_BUSY or ACQ_EVENT_TRIGGERED
//! 
//! \return
//!		  - > 0 : Handle of requested event
//!       - CS_INVALID_EVENT:  Event requested is unknown.
//!       - CS_INVALID_HANDLE: Handle is invalid.
/*************************************************************/
#ifdef _WIN32
	EVENT_HANDLE CSystemData::GetDrvEvent(CSHANDLE hSystem, uInt32 u32EventType)
#else
	int CSystemData::GetDrvEvent(CSHANDLE hSystem, uInt32 u32EventType)
#endif
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		switch (u32EventType)
		{
			case ACQ_EVENT_END_BUSY:
				return ((*theIterator).second->pStateMachine->GetAppAcquisitionEndEvent());

			case ACQ_EVENT_TRIGGERED:
				return ((*theIterator).second->pStateMachine->GetAppTriggerEvent());

			default:
#ifdef _WIN32
				return ((EVENT_HANDLE)CS_INVALID_EVENT);
#else
				return ((int)CS_INVALID_EVENT);
#endif   
		}
	}
#ifdef _WIN32
	return ((EVENT_HANDLE)CS_INVALID_HANDLE);
#else
	return ((int)CS_INVALID_HANDLE);
#endif   
}

CStateMachine* CSystemData::GetStateMachine(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
		return ((*theIterator).second->pStateMachine);
	
	return (NULL);	
}

int32	CSystemData::GetToken(CSHANDLE hSystem, uInt16 u16Channel)
{
	int32 i32Token = 0;

	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// Token:	Low Word is Token value
		//			High Word is the channel value and flags
		(*theIterator).second->wToken++;
		i32Token = MAKELONG((*theIterator).second->wToken, u16Channel);
	
		// Keep a copy of newly constructed Token
		(*theIterator).second->i32CurrentToken = i32Token;

		return (i32Token);
	}

	return (CS_INVALID_HANDLE);
}

int32	CSystemData::ValidateToken(CSHANDLE hSystem, int32 i32Token, int32* pi32Status, uInt8 u8Action)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if (-1 == i32Token)
		{
			i32Token = (*theIterator).second->i32CurrentToken & MASK_TOKEN_VALUE;
		}

		if (((*theIterator).second->i32CurrentToken & MASK_TOKEN_VALUE) == i32Token)
		{
			if (ACTIVATE_TOKEN == u8Action)
			{
				// Add active bit to Token
				(*theIterator).second->i32CurrentToken |= MASK_TX_IN_PROGRESS;
				(*theIterator).second->i32TokenStatus = CS_SUCCESS;
				if (pi32Status)
					*pi32Status = NO_TX_STARTED;
				return (CS_SUCCESS);
			}
			else if (COMPLETE_TOKEN == u8Action)
			{
				AS_TX_RESULT		 AsTxResult = {0};
				CSTRANSFERDATARESULT TxResults = {0};

				(*theIterator).second->i32CurrentToken &= ~MASK_TX_IN_PROGRESS;
				(*theIterator).second->i32CurrentToken |= MASK_TX_COMPLETED;

				/*i32RetCode =*/ ::CsDrvGetAsyncTransferDataResult(hSystem, (uInt8)(HIWORD(i32Token)), &TxResults, FALSE);

				AsTxResult.i32TokenValue = i32Token;
				AsTxResult.i32TokenStatus = (*theIterator).second->i32TokenStatus;
				AsTxResult.i64TokenResult = TxResults.i64BytesTransfered;

				(*theIterator).second->TokenResults.push_front(AsTxResult);

				if (pi32Status)
					*pi32Status = TX_COMPLETED;

				return (CS_SUCCESS);
			}

			// Check presence of active bit
			if ((*theIterator).second->i32CurrentToken & MASK_TX_IN_PROGRESS)
			{
				if (pi32Status)
					*pi32Status = TX_PROGRESS;

				return (CS_SUCCESS);
			}

			// Check presence for Completion bit
			if ((*theIterator).second->i32CurrentToken & MASK_TX_COMPLETED)
			{
				if (pi32Status)
					*pi32Status = TX_COMPLETED;

				return (CS_SUCCESS);
			}

			return (CS_INVALID_STATE);
		}
		else
			return (CS_INVALID_TOKEN);
	}

	return (CS_INVALID_HANDLE);
}

int32	CSystemData::SetTokenStatus(CSHANDLE hSystem, int32 i32Token, int32 i32Status)
{
	i32Token = i32Token;

	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		(*theIterator).second->i32TokenStatus = i32Status;;
		return (CS_SUCCESS);
	}

	return (CS_INVALID_HANDLE);
}
#if defined(_WIN32)
int64	CSystemData::GetTokenResult(CSHANDLE hSystem, const int32 ci32Token)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		AS_TX_RESULT	AsTxResult = {0};
		size_t	sizeList = (*theIterator).second->TokenResults.size();
		for (size_t i = 1; i <= sizeList; i++)
		{
			AsTxResult = (*theIterator).second->TokenResults.at(i-1);
			if ( ci32Token == AsTxResult.i32TokenValue )
			{
				(*theIterator).second->TokenResults.UnLock();
				return (AsTxResult.i64TokenResult);
			}
		}
		(*theIterator).second->TokenResults.UnLock();

		return (CS_INVALID_TOKEN);
	}
	return (CS_INVALID_HANDLE);
}

int32	CSystemData::GetTokenStatus(CSHANDLE hSystem, const int32 ci32Token)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if (ci32Token == (*theIterator).second->i32CurrentToken )
			return ((*theIterator).second->i32TokenStatus);
		
		AS_TX_RESULT	AsTxResult = {0};
		size_t	sizeList = (*theIterator).second->TokenResults.size();
		for (size_t i = 1; i <= sizeList; i++)
		{
			AsTxResult = (*theIterator).second->TokenResults.at(i-1);
			if (AsTxResult.i32TokenValue == ci32Token)
			{
				(*theIterator).second->TokenResults.UnLock();
				return (AsTxResult.i32TokenStatus);
			}
		}
		(*theIterator).second->TokenResults.UnLock();
		return (CS_INVALID_TOKEN);
	}
	return (CS_INVALID_HANDLE);
}


#else

int64	CSystemData::GetTokenResult(CSHANDLE hSystem, int32 i32Token)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		AS_TX_RESULTS_LIST::size_type	i;
		AS_TX_RESULTS_LIST::size_type	sizeList;

		AS_TX_RESULT	AsTxResult = {0};

		sizeList = (*theIterator).second->TokenResults.size();
		for (i = 0; i < sizeList; i++)
		{
			AsTxResult = (*theIterator).second->TokenResults.at(i);
			if (AsTxResult.i32TokenValue == i32Token)
				return (AsTxResult.i64TokenResult);
		}

		return (CS_INVALID_TOKEN);
	}

	return (CS_INVALID_HANDLE);
}

int32	CSystemData::GetTokenStatus(CSHANDLE hSystem, int32 i32Token)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if ((*theIterator).second->i32CurrentToken == i32Token)
			return ((*theIterator).second->i32TokenStatus);

		AS_TX_RESULTS_LIST::size_type	i;
		AS_TX_RESULTS_LIST::size_type	sizeList;

		AS_TX_RESULT	AsTxResult = {0};

		sizeList = (*theIterator).second->TokenResults.size();
		for (i = 0; i < sizeList; i++)
		{
			AsTxResult = (*theIterator).second->TokenResults.at(i);
			if (AsTxResult.i32TokenValue == i32Token)


				return (AsTxResult.i32TokenStatus);

		}

		return (CS_INVALID_TOKEN);
	}

	return (CS_INVALID_HANDLE);
}


#endif


//!
//! \ingroup SSM_API
//! \fn	int32 IsEventSupported(CSHANDLE hSystem);
//!	\brief Return if the system supports events, callbacks
//! 
//!	\param hSystem : System to query
//! 
//! \return
//!		  - 1 : (TRUE) Support of events 
//!       - 0 : (FALSE) Events are not supported 
//!       - < 0 : Error: invalid handle 
/*************************************************************/
int32 CSystemData::IsEventSupported(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if ((*theIterator).second->hThread != 0)
			return ((int32)TRUE);
		else
			return ((int32)FALSE);
	}
	
	return (CS_INVALID_HANDLE);
}


//!
//! \ingroup SSM_API
//! \fn	int32 HasTransferCallback(CSHANDLE hSystem);
//!	\brief Return if the system has callbacks for end of transfer
//! 
//!	\param hSystem : System to query
//! 
//! \return
//!		  - 1 : (TRUE) Has callback set 
//!       - 0 : (FALSE) Has no callback 
//!       - < 0 : Error: invalid handle 
/*************************************************************/
int32 CSystemData::HasTransferCallback(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if (NULL != (*theIterator).second->EndTxCallbackList)
			return ((int32)TRUE);
		else
			return ((int32)FALSE);
	}
	
	return (CS_INVALID_HANDLE);
}

int32 CSystemData::SetEndAcqCallback(CSHANDLE hSystem, LPCsEventCallback pCallBack)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// (*theIterator).second->EndAcqCallbackList.push(pCallBack);
		(*theIterator).second->EndAcqCallbackList = pCallBack;
		return (CS_SUCCESS);
	}
	
	return (CS_INVALID_HANDLE);
}

int32 CSystemData::SetEndTxCallback(CSHANDLE hSystem, LPCsEventCallback pCallBack)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		// (*theIterator).second->EndTxCallbackList.push(pCallBack);
		(*theIterator).second->EndTxCallbackList = pCallBack;
		return (CS_SUCCESS);
	}
	
	return (CS_INVALID_HANDLE);
}

void CSystemData::EndAcqCallback(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if ((*theIterator).second->EndAcqCallbackList)
		{
			CALLBACK_DATA CbXfer;
			CbXfer.u32Size = sizeof(CALLBACK_DATA);
			CbXfer.hSystem = hSystem;
			CbXfer.i32Token  = 0;
			CbXfer.u32ChannelIndex  = 0;
			(*theIterator).second->EndAcqCallbackList(&CbXfer);
		}
	}
}


void CSystemData::EndTxCallback(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
	{
		if ((*theIterator).second->EndTxCallbackList)
		{
			AS_TX_RESULT	AsTxResult = {0};
			CALLBACK_DATA	CbXfer;
			
			CbXfer.u32Size = sizeof(CALLBACK_DATA);
			CbXfer.hSystem = hSystem;
			CbXfer.i32Token  = 0;

			// The first element in the array is the last transfer that was completed.
#if defined(_WIN32)
			size_t	sizeList = (*theIterator).second->TokenResults.size();
			if( sizeList > 0 )
				AsTxResult = (*theIterator).second->TokenResults.at(0);
			(*theIterator).second->TokenResults.UnLock();
#else
			AsTxResult = (*theIterator).second->TokenResults[0];
#endif
			CbXfer.u32ChannelIndex  = (AsTxResult.i32TokenValue & 0xffff0000) >> 16;

			(*theIterator).second->EndTxCallbackList(&CbXfer);
		}
	}
}

Worker* CSystemData::GetWorkerThread(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
		return ((*theIterator).second->pWorker);
	
	return (NULL);	
}

#if defined(_WIN32)
CsDiskStream* CSystemData::GetDiskStreamPtr(CSHANDLE hSystem)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
		return ((*theIterator).second->pDiskStream);
	
	return (NULL);	
}

void CSystemData::SetDiskStreamPtr(CSHANDLE hSystem, CsDiskStream* ptr)
{
	SYSTEMDATAMAP::iterator theIterator;

	if ((theIterator = m_SystemMap.find(hSystem)) != m_SystemMap.end())
		(*theIterator).second->pDiskStream = ptr;

}
#endif
