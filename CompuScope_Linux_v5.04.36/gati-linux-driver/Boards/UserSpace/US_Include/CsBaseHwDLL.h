/////////////////////////////////////////////////////////////
// CsBaseHwDLL.h
/////////////////////////////////////////////////////////////


#ifndef _CSBASE_HWDLL_H_
#define _CSBASE_HWDLL_H_

#include "CsDefines.h"
#include "CsTypes.h"
#include "CsAdvStructs.h"
#include "CsDriverTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsErrors.h"
#include "CsExpert.h"
#include "CsSharedMemory.h"
#include "DllStruct.h"
#include "CsDrvApi.h"



#ifdef __linux__
//
// Macros used in Linux

#define  GAGE_DEVICE_IOCTL( hdrive, IoCtlCode, InOutPtr ) \
	 ((-1 == ioctl( hdrive, IoCtlCode, InOutPtr ) ) ? 0 : 1)

#else


#define IDD_AUX_IO_DLG                  200

#include <vector>

//
// Macros used in MS Windows
//
#define  GAGE_DEVICE_IOCTL( hdrive, IoCtlCode, InPtr, InSize, OutPtr, OutSize, ByteRet, Overlap, InOutPtr ) \
	DeviceIoControl( hdrive,		/* handle to the driver */ \
				  IoCtlCode,		/* control code of the operation to perform */ \
				  InPtr,			/* address of buffer for input data */ \
				  InSize,			/* size of input data */ \
				  OutPtr,			/* address of buffer for output data */ \
				  OutSize,			/* size of Output buffer */ \
				  ByteRet,			/* Bytes returned */ \
				  Overlap			/* pointer to overlapped structure */ );
//				  InOutPtr			/* For Linux only, not used by Windows */


#endif

#ifdef __linux__

#define	DEV_IOCTL_ERROR		-1

#else

#define	DEV_IOCTL_ERROR		0

#endif

#ifdef __cplusplus
	extern "C"
	{
#endif

// Private functions
int32 DLL_Initialize();
void DLL_CleanUp();

BOOL GetSharedSection(BOOL bUsingCsRm = TRUE);
void DeleteSharedSection();

#ifdef __cplusplus
}
#endif


typedef struct _ACQSYSTEM
{
	FILE_HANDLE			hAcqDriver;			// Driver file Handle per acquisition system
	OVERLAPPED			Overlapped;			// Overlapped structure per Cs system
											// Assume only one Asynchronous request per system.
											// This is TRUE because asynchronous requests are queued by
											// SSM
} ACQSYSTEM, *PACQSYSTEM;


class CsBaseHwDLL
{
private:
	int32			m_Status;		
	char			m_DriverName[DRIVER_MAX_NAME_LENGTH];	// Indicate which Dll the class belong.
	char			m_LinkName[DRIVER_MAX_NAME_LENGTH];		// Symbolic link into kernel.
	uInt32			m_BytesRead;	// Bytes read for the current IOCTL
	FILE_HANDLE		m_hDriver;		// Generic driver handle
	PHW_DLL_LOOKUPTABLE m_pHwLookupTable;
public:
	CsBaseHwDLL(char *SymbolicLink, PHW_DLL_LOOKUPTABLE pDrvSystem );
	~CsBaseHwDLL();
	
	int32 GetStatus();
	int32 CsDrvOpenSystemForRm( DRVHANDLE	DrvHdl );
	int32 CsDrvCloseSystemForRm( DRVHANDLE	DrvHdl );
	int32 CsDrvAcquisitionSystemInit( DRVHANDLE	DrvHdl, BOOL bResetDefaultSetting );
	int32 CsValidateBufferForReadWrite(uInt32 ParamID, void *ParamBuffer, BOOL bRead);
	int32 GetIndexInLookupTable( DRVHANDLE	DrvHdl );
	int32 CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16 *u16SystemFound );
	uInt32 CsDrvGetAcquisitionSystemHandles(PDRVHANDLE, uInt32);
	int32 CsDrvGetAcquisitionSystemInfo(DRVHANDLE, PCSSYSTEMINFO);
	int32 CsDrvGetAcquisitionSystemCaps(DRVHANDLE, PCSSYSTEMCAPS);
	int32 CsDrvAcquisitionSystemCleanup(DRVHANDLE);
	int32 CsDrvGetBoardsInfo(DRVHANDLE, PARRAY_BOARDINFO);
	int32 CsDrvUpdateLookupTable(DRVHANDLE, RMHANDLE);
	int32 CsDrvStartAcquisition(DRVHANDLE DrvHdl);
	int32 CsDrvAbort(DRVHANDLE DrvHdl);
	int32 CsDrvReset(DRVHANDLE DrvHdl, BOOL bHardReset = FALSE);
	int32 CsDrvForceTrigger(DRVHANDLE DrvHdl);
	int32 CsDrvForceCalibration(DRVHANDLE DrvHdl);
	
	int32 CsDrvGetParams(DRVHANDLE, uInt32 ParamID, void *Parambuffer);
	int32 CsDrvSetParams(DRVHANDLE, uInt32 ParamID, void *Parambuffer);
	int32 CsDrvDo(DRVHANDLE, uInt32 ActionID, void * ActionBuffer);

	int32 CsDrvTransferData( DRVHANDLE DrvHdl,
							 IN_PARAMS_TRANSFERDATA inParams,
							 POUT_PARAMS_TRANSFERDATA OutParams,
							 BOOL bMulRecRawDataTx = FALSE );

	int32 CsDrvTransferDataEx( DRVHANDLE DrvHdl, PIN_PARAMS_TRANSFERDATA_EX inParams, POUT_PARAMS_TRANSFERDATA_EX OutParams );

	int32 CsDrvGetAsyncTransferDataResult(DRVHANDLE, uInt16 u16Channel, CSTRANSFERDATARESULT *pTxDataResult,
	  									BOOL bWait );

	int32 CsDrvRegisterEventHandle(DRVHANDLE, uInt32 EventYpe, HANDLE *EventHandle);
	int32 CsDrvGetAvailableImpedances(DRVHANDLE , uInt32 pImpedance, uInt32 SizeOf);
	int32 CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32 *Status);

	int32 CsDrvValidateParams( DRVHANDLE DrvHdl, uInt32 u32Coerce, PCSSYSTEMCONFIG pSysCfg );
	int64 CoerceSampleRate( int64 i64AnySampleRate, uInt32 u32Mode, uInt32 CardId,
						  CSSAMPLERATETABLE_EX *pSrTable, uInt32 u32SrSize );

	int32 CsDrvGetAvailableTriggerSources(CSHANDLE DrvHdl, uInt32 SubCapsId, int32 *i32ArrayTriggerSources,
													uInt32 *BufferSize );


	int32 CsDrvValidateTriggerConfig( PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 Coerce );
	int32 CsDrvAutoFirmwareUpdate();

	// Functions that do not use HANDLE
	int32 CsDrvGetHwOptionText( PCS_HWOPTIONS_CONVERT2TEXT pHwOptionsText);
	int32 CsDrvGetFwOptionText( PCS_FWOPTIONS_CONVERT2TEXT pFwOptionsText);
	int32 CsDrvGetFlashLayoutInfo(PCS_FLASHLAYOUT_INFO	pCsFlashLayoutInfo );	


	int32	CsDrvAllocateCardObjectForProcess();
	uInt32	CsDrvGetDeviceInfo(PCSDEVICE_INFO pDevInfo, uInt32 u32Size);
	uInt32	CsDrvGetNumberOfDevices(CS_CARD_COUNT *CardCount);
	PDRVSYSTEM	GetAcqSystemPointer(DRVHANDLE	DrvHdl);
	PVOID	GetAcqSystemCardPointer(DRVHANDLE	DrvHdl);
	BOOL	SearchForActiveDriver();
	FILE_HANDLE  OpenDrvGenericLink();
	void	CloseDrvGenericLink();


#ifdef  DEBUG_PRF
	char	m_pDbgPrfBuffer[0x200000];

	int32	DebugPrf(DRVHANDLE	DrvHdl, uInt32 u32DebugOption);
#endif
};




#endif   // _CSBASE_HWDLL_H_

