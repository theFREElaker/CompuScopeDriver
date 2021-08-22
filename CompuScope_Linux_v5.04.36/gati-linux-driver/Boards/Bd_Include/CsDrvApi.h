#ifndef _CS_DRV_API_H_
#define _CS_DRV_API_H_

#include "CsTypes.h"

#ifdef __cplusplus
	extern "C"
	{
#endif

	int32  CsDrvCloseSystemForRm( DRVHANDLE DrvHdl );
	int32  CsDrvOpenSystemForRm( DRVHANDLE DrvHdl );
	int32  CsDrvAutoFirmwareUpdate();
	int32  CsDrvGetAcquisitionSystemCount( uInt16 bForceIndependantMode, uInt16* pu16SystemFound );
	uInt32 CsDrvGetAcquisitionSystemHandles(PDRVHANDLE pDrvHdl, uInt32 u32Size);
	int32  CsDrvAcquisitionSystemInit( DRVHANDLE DrvHdl, BOOL bResetDefaultSetting );
	int32  CsDrvGetAcquisitionSystemInfo(DRVHANDLE DrvHdl, PCSSYSTEMINFO);
	int32  CsDrvGetAcquisitionSystemCaps(DRVHANDLE DrvHdl, uInt32 u32CapsId, PCSSYSTEMCONFIG pSysCfg, void* pBuffer, uInt32* pu32BufferSize);
	int32  CsDrvAcquisitionSystemCleanup(DRVHANDLE DrvHdl);
	int32  CsDrvGetBoardsInfo(DRVHANDLE DrvHdl, PARRAY_BOARDINFO pBoardInfo);
	int32  CsDrvGetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void* pParambuffer);
	int32  CsDrvSetParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, void* pParambuffer);
	int32  CsDrvValidateParams(DRVHANDLE DrvHdl, uInt32 u32ParamID, uInt32 u32Coerce, void* pParamBuffer);
	int32  CsDrvDo(DRVHANDLE DrvHdl, uInt32 u32ActionID, void* pActionBuffer );
	int32  CsDrvTransferData(DRVHANDLE DrvHdl, IN_PARAMS_TRANSFERDATA inParams, POUT_PARAMS_TRANSFERDATA pOutParams );
	int32  CsDrvTransferDataEx(DRVHANDLE DrvHdl, PIN_PARAMS_TRANSFERDATA_EX InParams, POUT_PARAMS_TRANSFERDATA_EX OutParams );
	int32  CsDrvRegisterEventHandle(DRVHANDLE DrvHdl, uInt32 u32EventYpe, HANDLE* phEventHandle);
	int32  CsDrvGetAsyncTransferDataResult(DRVHANDLE DrvHdl, uInt8 Channel, CSTRANSFERDATARESULT* pTxDataResult, BOOL bWait );
	int32  CsDrvGetAcquisitionStatus(DRVHANDLE DrvHdl, uInt32* pu32Status);
	int32  CsDrvRegisterStreamingBuffers(DRVHANDLE DrvHdl, uInt8 u8CardIndex, CSFOLIOARRAY* pStmBufferList, HANDLE* hSemaphore, PSTMSTATUS* pStmStatus);
	int32  CsDrvReleaseStreamingBuffers( DRVHANDLE DrvHdl, uInt8 u8CardIndex );
	int32  CsDrvExpertCall(DRVHANDLE CsHandle, void* pFuncParams);
	int32  CsDrvCfgAuxIo(CSHANDLE);
	int32  CsDrvStmAllocateBuffer(DRVHANDLE DrvHdl, int32 nCardIndex, uInt32 u32BufferSize, PVOID *pVa);
	int32  CsDrvStmFreeBuffer(DRVHANDLE DrvHdl, int32 nCardIndex, PVOID pVa);
	int32  CsDrvStmTransferToBuffer(DRVHANDLE DrvHdl, int32 nCardIndex, PVOID pVa, uInt32 u32TransferSize);
	int32  CsDrvStmGetTransferStatus(DRVHANDLE DrvHdl, int32 nCardIndex, uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData);
	int32  CsDrvSetRemoteSystemInUse(DRVHANDLE drvHandle, BOOL bSet);
	int32  CsDrvIsRemoteSystemInUse(DRVHANDLE drvHandle, BOOL* bSet);

	// Functions that do not use HANDLE
	int32  _CsDrvGetBoardType( uInt32* pu32MaxBoardType, uInt32* pu32MinBoardType );
	int32  _CsDrvGetHwOptionText( uInt32 u32BoardType, PCS_HWOPTIONS_CONVERT2TEXT pHwOptionText );
	int32  _CsDrvGetFwOptionText( PCS_HWOPTIONS_CONVERT2TEXT pHwOptionsText);
	int32  _CsDrvGetBoardCaps( uInt32 u32ProductId, uInt32 u32Param, void* pBuffer, uInt32* pu32BufferSize);
	BOOL   _CsDrvIsMyBoardType( uInt32 u32BoardType);

#ifdef __cplusplus
	}
#endif

#endif