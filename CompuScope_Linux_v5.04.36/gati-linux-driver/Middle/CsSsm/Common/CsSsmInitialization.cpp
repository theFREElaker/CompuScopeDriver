#include "StdAfx.h"
#include "CsSsm.h"

//! \mainpage  CompuScope API
//! This help file contains a complete description of the Gage CompuScope Applications Programming Interface (API), which controls
//! CompuScope hardware.  Key elements are: the C functions or methods that are called to control the hardware, the structures
//! in which settings are packed and passed to these methods and the defined constants that may be used to specify these settings.<BR> <BR>
//! Only precise API descriptions are given.  This help file should be used with the CompuScope C/C# SDK, which provides several
//! sample programs that illustrates the use of CompuScope hardware in different operating modes.
//! The CompuScope C/C# SDK User's Guide and the CompuScope Hardware Manual and Installation Guide should be consulted since they
//! provide functional descriptions of CompuScope operation.


//!
//! \defgroup SSM_API CompuScope API Methods
//! \{
//!

//! \fn int32 SSM_API CsInitialize(void);
//!	Initialize Gage Driver for further processing.
//!
//! This method initializes the CompuScope driver and establishes all necessary
//! links between various parts of the driver. This should be the first CompuScope API method
//! that is invoked in any CompuScope C or C# application.
//!
//! \sa  CsGetSystem
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//!	\warning This method must be called before any other CompuScope API methods are called
////////////////////////////////////////////////////////////////////
int32 SSM_API CsInitialize(void)
{
	int32 i32RetVal = ::CsRmInitialize();

	if (CS_SUCCEEDED(i32RetVal))
		g_RMInitialized = true;

	return (i32RetVal);
}



int32 SSM_API CsReInitialize(int16 i16Action)
{
	int32 i32RetVal = 0;

	i32RetVal = ::CsRmReInitialize(i16Action);

	if (CS_SUCCEEDED(i32RetVal))
		g_RMInitialized = true;

	return (i32RetVal);
}



int32 SSM_API CsGetAvailableSystems(void)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	return (::CsRmGetAvailableSystems());
}


//! \fn int32 SSM_API CsGetSystem(CSHANDLE* hSystem, uInt32 u32BoardType, uInt32 u32Channels, uInt32 u32SampleBits, int16 i16Index)
//!	Locks and initializes the CompuScope system specified by the parameters.
//!
//! This method finds a specified CompuScope system, if one exists. It then retrieves the CompuScope system handle and reserves
//! access to this handle uniquely for the process that invoked  the method.
//! The returned handle must be used in all subsequent calls to this system.
//! The method searches for CompuScope systems that satisfy the criteria specified by the method parameters.
//! Any of the specifying parameters may be zero in which case they are ignored as the search criteria.
//! The first CompuScope system that is not already associated with any processes and that satisfies the specified criteria is returned.
//! In the case where more than one retrieved CompuScope system satisfies the search criteria, the system with the specified i16Index
//! parameter is selected. i16Index values are assigned to CompuScope systems that match the criteria specified by the method parameters, if any.
//! The order of i16Index identification is preserved between sessions. However the order may change if CompuScope card(s)
//! are plugged into different PCI slot in different sessions.
//! Failure to find a CompuScope system may be result because the system is being used by another process or because of incorrect
//! selection criteria. CompuScope Manager may be run in order to view all available CompuScope systems.
//!
//! \param hSystem		: Handle of CompuScope system found by the method. A NULL value indicates that no system was found.
//! \param u32BoardType	: \link BOARD_TYPES CompuScope model \endlink
//! \param u32Channels	: Number of channels in the CompuScope system
//! \param u32SampleBits: Resolution of the CompuScope system
//! \param i16Index		: Index of CompuScope system to search for.
//!
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa  CsFreeSystem
//!
//! Example:<BR>
//!		To retrieve
//!			CsGetSystem(&hSystem, 0, 4) will retrieve hSystem as the handle of a CompuScope system with 4 input channels,
//! if available.
//!
////////////////////////////////////////////////////////////////////
int32 SSM_API CsGetSystem(CSHANDLE* phSystem, uInt32 u32BoardType, uInt32 u32Channels, uInt32 u32SampleBits, int16 i16Index)
{
	_ASSERT(g_RMInitialized);

	CSHANDLE	hLockSystem = 0;
	int32		i32RetVal = 0;

	if(NULL == phSystem)
		return CS_NULL_POINTER;

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	if ((0 != *phSystem) && (0xffffffff == u32BoardType) && (0xffffffff == u32Channels) && (0xffffffff == u32SampleBits))
	{
		i32RetVal = ::CsRmLockSystem(*phSystem);
		if (CS_SUCCEEDED(i32RetVal))
		{
			i32RetVal = ::CsDrvAcquisitionSystemInit(*phSystem, TRUE);
			if (CS_SUCCEEDED(i32RetVal))
				i32RetVal = CSystemData::instance().AddSystem(*phSystem);
		}

		return (i32RetVal);
	}

	*phSystem = 0;

	i32RetVal = ::CsRmGetSystem(u32BoardType, u32Channels, u32SampleBits, &hLockSystem, i16Index);
	if (CS_SUCCEEDED(i32RetVal))
	{
		i32RetVal = ::CsDrvAcquisitionSystemInit(hLockSystem, TRUE);
		if (CS_SUCCEEDED(i32RetVal))
		{
			i32RetVal = CSystemData::instance().AddSystem(hLockSystem);
			if (CS_SUCCEEDED(i32RetVal))
			{
				*phSystem = hLockSystem;
				return (CS_SUCCESS);
			}
		}
	}

	return (i32RetVal);
}


//! \fn int32 SSM_API CsFreeSystem(CSHANDLE hSystem);
//!	Unlocks a CompuScope system and frees its resources.
//!
//! When called,  this method will dissociate a CompuScope system from the calling process. The system will then be made available
//! for other processes.  Failure to call this method before terminating a process may cause a resource leak that will only be
//! fixed by the next refresh operation from the CompuScope manager utility.
//! Such a failure may occur, for instance, by stopping a program during debugging.
//!
//! \param hSystem Handle of CompuScope system to be freed.
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//!
//! \sa CsGetSystem
////////////////////////////////////////////////////////////////////
int32 SSM_API CsFreeSystem(CSHANDLE hSystem)
{
	_ASSERT(g_RMInitialized);

	if (!g_RMInitialized)
		return CS_NOT_INITIALIZED;

	int32 nRetCode = 0;

	nRetCode = CSystemData::instance().ValidateSystemHandle(hSystem);
	if (CS_INVALID_HANDLE == nRetCode)
		return (nRetCode);

#if defined(_WIN32)
	if ( !(hSystem & RM_REMOTE_HANDLE) ) //RG - Causes an acess to fci-shared memory after the dll has freed it
	{
		if (CSystemData::instance().PerformTransition(hSystem, ABORT_TRANSITION))
			::CsDrvDo(hSystem, ACTION_ABORT, NULL);
	}
#else

	//TODO: we're commenting out this line to avoid a run-time error (bad
//		dynamic_cast in getState. we also changed the dll destructor to atexit.
//		we're masking a problem here and we need to look into it further.

//		It works because we're freeing the system anyways so the transition doesn't
//		matter
//	if (CSystemData::instance().PerformTransition(hSystem, ABORT_TRANSITION))
//	{
		::CsDrvDo(hSystem, ACTION_ABORT, NULL);
//	}

#endif

	// Release system from the ResourceManager table
	nRetCode = ::CsRmFreeSystem(hSystem);

	// EG - Validate Error code on ClearSystem
	if (CS_SUCCEEDED(nRetCode))
	{
		::CsDrvAcquisitionSystemCleanup(hSystem);
		CSystemData::instance().ClearSystem(hSystem);
	}

	return (nRetCode);
}

// \fn int32 SSM_API CsBorrowSystem(CSHANDLE* hSystem, uInt32 u32BoardType, uInt32 u32Channels, uInt32 u32SampleBits, int16 i16Index)
//	..........
//
// \param hSystem		: Handle of CompuScope system found by the method. A NULL value indicates that no system was found.
// \param u32BoardType	: \link BOARD_TYPES CompuScope model \endlink
// \param u32Channels	: Number of channels in the CompuScope system
// \param u32SampleBits: Resolution of the CompuScope system
// \param i16Index		: Index of CompuScope system to search for.
//
//
//	\return
//		- Positive value (>0) for success.
//		- Negative value (<0) for error.
//			Call CsGetErrorString with the error code to obtain a descriptive error string.
//
// \sa  CsGetSystem, CsReleaseSystem
//
////////////////////////////////////////////////////////////////////
int32 SSM_API CsBorrowSystem(CSHANDLE* phSystem, uInt32 u32BoardType, uInt32 u32Channels, uInt32 u32SampleBits, int16 i16Index)
{
	// Use of the variables to remove compile warnings
	
	u32BoardType = u32BoardType;
	u32Channels = u32Channels;
	u32SampleBits = u32SampleBits;
	i16Index = i16Index;

	*phSystem = 0;
	return (CS_FUNCTION_NOT_SUPPORTED);
}


// \fn int32 SSM_API CsReleaseSystem(CSHANDLE hSystem);
//
// \param hSystem Handle of CompuScope system to be released
//
//	\return
//			Call CsGetErrorString with the error code to obtain a descriptive error string.
//
// \sa CsBorrowSystem
////////////////////////////////////////////////////////////////////
int32 SSM_API CsReleaseSystem(CSHANDLE hSystem)
{
	// Use of the variables to remove compile warnings
	hSystem = hSystem;

	return (CS_FUNCTION_NOT_SUPPORTED);
}

//! \fn int32 SSM_API CsGetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo);
//!	Retrieves the static information  about the CompuScope system
//!
//! This method queries static or unchangeable information about the CompuScope system, such as sample resolution
//! and memory size. To query for dynamic parameters settings the CsGet method should be used.
//!
//! \param hSystem		: Handle of the CompuScope system to be addressed. Obtained from CsGetSystem
//! \param pSystemInfo	: Pointer to the structure that will be filled with the  CompuScope system information.
//!
//!	\return
//!		- Positive value (>0) for success.
//!		- Negative value (<0) for error.
//!			Call CsGetErrorString with the error code to obtain a descriptive error string.
//! \sa CSSYSTEMINFO, CsGet
//!
////////////////////////////////////////////////////////////////////
int32 SSM_API CsGetSystemInfo(CSHANDLE hSystem, PCSSYSTEMINFO pSystemInfo)
{
	_ASSERT(g_RMInitialized);

	if (g_RMInitialized)
	{
		if (FS_BASE_HANDLE & (ULONG_PTR) hSystem)
			return (CS_INVALID_HANDLE);

		if (pSystemInfo && (pSystemInfo->u32Size > sizeof(CSSYSTEMINFO) || 0 == pSystemInfo->u32Size))
			return (CS_INVALID_STRUCT_SIZE);

		return (CSystemData::instance().GetSystemInfo(hSystem, pSystemInfo));
	}
	else
		return CS_NOT_INITIALIZED;
}

//!
//! \}
//!
