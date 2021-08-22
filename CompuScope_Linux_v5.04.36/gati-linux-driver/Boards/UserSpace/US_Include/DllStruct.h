#ifndef _CS_DLLSTRUCT_H_
#define _CS_DLLSTRUCT_H_

#include "CsTypes.h"
#include "CsAdvStructs.h"
#include "CsSharedMemory.h"



#define		MAX_CARDS_MS		10	// Max cards in M/S system
#define		CS_MAX_PADDING		512

#define		FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))

typedef struct _CHANNEL_DATA_TRANSFER
{
	uInt16			u16ChannelTx;				// Current Channel
	BOOL			bUseSlaveTransfer;			// Bus master not supported
	uInt8			*pTempDataBuffer;			// Temporary data buffer used for Dmux
												// (Single channel data transfer slave only)
	int32			i32Status;					// Current transfer status

	uInt32			u32AsynThreadId;
	EVENT_HANDLE	hAsynThread;
	EVENT_HANDLE	hAsynEvent;
	EVENT_HANDLE	hUserAsynEvent;
	volatile long	BusyTransferCount;			// Transfer is in progress

} CHANNEL_DATA_TRANSFER, *PCHANNEL_DATA_TRANSFER;


#if defined(_WIN32)
typedef struct _CSTxPARAMS
{
	DRVHANDLE				DrvHdl;
	IN_PARAMS_TRANSFERDATA	InParams;
	PVOID					pBaseHwDLL;
	HANDLE					hBlockTxEvent;		// Block transfer
	HANDLE					*hUserNotifyEvent;
	BOOLEAN					bAbort;
	BOOLEAN					bMulrecRawDataTx;
} CSTxPARAMS, *PCSTxPARAMS;
#endif

typedef struct _DRVSYSTEM
{
	DRVHANDLE			DrvHandle;			// CS Driver habdle
	CSSYSTEMINFO		SysInfo;
	CSSYSTEMINFOEX		SysInfoEx;			// Private systeminfo
	CSACQUISITIONCONFIG AcqConfig;

	EVENT_HANDLE		hSerialAsyncTransfer;	// Handle of the thread when a big Async transfer is divided by seires of
												// smaller transfers by DLL.

	char				SymbolicLink[DRIVER_MAX_NAME_LENGTH];	// CS system SymbolicLink

	PVOID               pCapsInfo;				// Pointer to board specific caps info structure

	uInt8				u8CardIndexList[MAX_CARDS_MS];	// M/S system is build with Rm process. Once M/S system is built, this array keep track of 
														// Master, Slave1, Slave2 ... card index. The application process will base on this array to build M/S
														// system without going through M/S detection again

	uInt8				u8FirstCardIndex;		// Index of the fitst card in M/S system

	uInt16				u16AcqStatus;		// Acquisition System Status
	BOOL				bInitialized;		// The system has been initialized.
	BOOL				bReset;				// Abort current operation and
											// reset the system to the default state

	BOOL				bEndAcquisitionNotification;	// protect race condition for EndOfBusy notification
	uInt32				u32TriggerNotification;			// number of trigger notification to application

	uInt32				u32TriggeredCount;				// number of triggerred events received by interrupts

	EVENT_HANDLE		hAcqSystemCleanup;
	EVENT_HANDLE		hThreadIntr;				// thread waiting for interrupts
	uInt32				u32ThreadIntrId;			// thread waiting for interrupts
	EVENT_HANDLE		hEventTriggered;
	EVENT_HANDLE		hEventEndOfBusy;
	EVENT_HANDLE		hEventAlarm;

	EVENT_HANDLE		hUserEventTriggered;
	EVENT_HANDLE		hUserEventEndOfBusy;
	uInt16				u16ActiveChannelTx;				// Active Channel Data Transfer
	volatile long		BusyTransferCount;
	CRITICAL_SECTION	CsProtectAccess;		// Serialize Gage API access on the same system
	BOOL				bCrashRecovery;			// The system is recovering from a crash (currently only used by Linux)

}DRVSYSTEM, *PDRVSYSTEM;



typedef struct _HW_DLL_LOOKUPTABLE
{
	uInt16		u16SystemCount;
	DRVSYSTEM	DrvSystem[MAX_ACQ_SYSTEMS];
} HW_DLL_LOOKUPTABLE, *PHW_DLL_LOOKUPTABLE;


typedef struct _SHARED_DLL_GLOBALS
{
	uInt32				ProcessCount;
	HW_DLL_LOOKUPTABLE	g_StaticLookupTable;
	volatile long		g_Critical;

#ifdef __linux__
	uInt16				g_RefCount;
#endif

} SHARED_DLL_GLOBALS, *PSHARED_DLL_GLOBALS;

typedef SharedMem< SHARED_DLL_GLOBALS >  SHAREDDLLMEM;
typedef SHAREDDLLMEM*  PSHAREDDLLMEM;


typedef struct _SHARED_CRITICAL_64_32
{
	uInt32				RefCount;
	volatile long		Critical;

#ifdef __linux__
	uInt16				g_RefCount;
#endif

} SHARED_DLL_CRITICAL_64_32, *PSHARED_DLL_CRITICAL_64_32;

#define	SHAREDDLL_CRITICALSECTION_NAME		"GlobalCriticalSection-64-32"
typedef SharedMem< SHARED_DLL_CRITICAL_64_32 >  SHAREDDLL_CRITICALSECTION;
typedef SHAREDDLL_CRITICALSECTION*  PSHAREDDLL_CRITICALSECTION;


typedef struct _GLOBAL_PERPROCESS
{
	PVOID			pFirstUnCfg;			// Before organized master/slave operation, all card detected are
											// considered as unconfig cards.
											// This pointer points to the first unconfig card
	PVOID			pLastUnCfg;				// Pointer to the last unconfig card
	PVOID			pInvalidCfg;			// Pointer to list of card that have invalidate configuration
											// (Ex: Slave cards but missing the Master card)
	uInt32			u32CardCount;			// Total of cards detected by kernel driver
	uInt16			NbSystem;				// Number of System DllStruct detected.
	uInt16			u16IndMode;				// 0: Normal, 1: force independant

	PVOID			hNativeFirstCard;		// First card delected

} GLOBAL_PERPROCESS, *PGLOBAL_PERPROCESS;



typedef struct _STREAMSTRUCT
{
	volatile LONG	u32BusyTransferCount;	// Number of DMA request
	volatile LONG	u32WaitTransferCount;	// Number of Wait for DMA request complete

	ULONGLONG	u64TotalDataSize;			// The total data aize for streaming in Bytes
	ULONGLONG	u64DataRemain;				// The remaining data to be transfer to application 
	int64	i64DataInFifo;					// When Fifo is full, this is the amount of valid data remaining in Fifo avallble for DMA to appliaction
	uInt32	u32TransferSize;				// The current DMA size (in Samples)

	bool	bFifoFull;						// Indicate that Fifo full has beem occured at the board level
	bool	bEnabled;						// Stream enabled
	bool	bRunForever;					// Stream forever
	bool	bInfiniteDepth;					// Stream with infinite depth
	bool	bInfiniteSegmentCount;			// Stream with infinite segment count
} STREAMSTRUCT, *PSTREAMSTRUCT;

// Structures and definitions for electron (remote faceless instrument)

const int	g_MaxSystemCount = 50;
#define		CS_FCI_SHARED_MEM	_T("csfci-shared-remote")

typedef struct _socketInfo
{
	TCHAR		strIPAddress[20];
	DWORD		dwPort;
	CSHANDLE	csHandle;
} SocketInfo;

typedef struct _remoteSystemInfo
{
	int	count;
	SocketInfo System[g_MaxSystemCount];
}RemoteSystemInfo;

typedef SharedMem< RemoteSystemInfo >  REMOTESYSTEM;
typedef REMOTESYSTEM*  PREMOTESYSTEM;

#endif   // _CS_DLLSTRUCT_H_
