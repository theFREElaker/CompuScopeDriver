#ifndef _CSADVANCE_HWDLL_H_
#define _CSADVANCE_HWDLL_H_

#include "CsTypes.h"
#include "CsDriverTypes.h"
#include "CsStruct.h"
#include "CsPrivateStruct.h"
#include "CsErrors.h"
#include "CsExpert.h"
#include "CsBaseHwDLL.h"


class CsAdvHwDLL
{
private:
	uInt32			m_BytesRead;	// Bytes read for the current IOCTL
	OVERLAPPED		m_Ovlp;			// Generic overlap structure for asynchronous IO
	PDRVSYSTEM		m_pDrvSystem;	// Pointer to Acq system params. Shared accross different process
	PACQSYSTEM		m_AcqSystem;	// Lookup table for Drivers File Handle. Cannot be shared.
	uInt8			*m_u8LinkName;	// Indicate which Dll the class belong.

	CsBaseHwDLL		*m_pBaseHwDll;

public:
	CsAdvHwDLL( CsBaseHwDLL *pBaseHwDll, PDRVSYSTEM pDrvSystem, PACQSYSTEM pAcqSystem, uInt8 *u8LinkName );
	~CsAdvHwDLL();

	int32 CsDrvExpertCall(CSHANDLE, void *FuncParams );

	int32 CsDrvCreateMinMaxInfoQueue( CSHANDLE DrvHdl, uInt32 u32QueueSize,
											   uInt16 u16DetectorResetMode,
                                               uInt16 u16TsResetMode,
											   EVENT_HANDLE *hQueueEvent,EVENT_HANDLE *hErrorEvent,
											   EVENT_HANDLE *hSwFifoFullEvent );
	int32 CsDrvDestroyMinMaxInfoQueue( CSHANDLE DrvHdl );
	int32 CsDrvGetMinMaxSegmentInfo( CSHANDLE DrvHdl, PMINMAXSEGMENT_INFO pBuffer, uInt32 u32BufferSize );
	int32 CsDrvClearErrorMinMaxQueue( CSHANDLE DrvHdl );

	int32 CsDrvSendRequestWithoutParams( CSHANDLE DrvHdl, uInt32 u32IoCtlCode );
	inline uInt16 NormalizedChannelIndex( uInt32 u32Mode, uInt16 u16ChannelIndex );
	int32 CsDrvAllChannelsDataRead( CSHANDLE DrvHdl );

	int32 CsDrvMulrecRawDataTransfer( CSHANDLE	DrvHdl,
										uInt32	u32StartSegment,
										uInt32	u32EndSegment,
										PVOID	pBuffer,
										int64	i64BufferSize,
										int64	*i64ActualBufferSize );

	int32 CsDrvGetRawMulrecHeader( CSHANDLE DrvHd, PMULRECRAWDATA_HEADER pHeader );

};


#endif	//_CSADVANCE_HWDLL_H_
