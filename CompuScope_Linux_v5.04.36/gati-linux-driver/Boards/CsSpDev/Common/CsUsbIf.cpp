#include "stdafx.h"
#include "CsSpDevice.h"


CsSpDeviceManager* m_pSpDevCtl = NULL;

int32  CsDrvCfgAuxIo(CSHANDLE){return CS_FUNCTION_NOT_SUPPORTED;}
int32  CsDrvCloseSystemForRm( DRVHANDLE	 ){return CS_SUCCESS;}
int32  CsDrvOpenSystemForRm( DRVHANDLE	){return CS_SUCCESS;}

int32  CsDrvGetAsyncTransferDataResult(DRVHANDLE, uInt8 , CSTRANSFERDATARESULT* , BOOL ){return CS_FUNCTION_NOT_SUPPORTED;}
int32  CsDrvRegisterStreamingBuffers(DRVHANDLE , uInt8 , CSFOLIOARRAY*, HANDLE*, PSTMSTATUS*){return CS_FUNCTION_NOT_SUPPORTED;}
int32  CsDrvReleaseStreamingBuffers( DRVHANDLE , uInt8 ){return CS_FUNCTION_NOT_SUPPORTED;}
int32  CsDrvExpertCall(CSHANDLE, void*){return CS_FUNCTION_NOT_SUPPORTED;}

// Functions that do not use HANDLE
int32	_CsDrvGetBoardType( uInt32* pu32MinBoardType, uInt32* pu32MaxBoardType )
{
	if ( IsBadReadPtr( pu32MaxBoardType,  sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadReadPtr( pu32MinBoardType,  sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	*pu32MinBoardType = CSUSB_BT_FIRST_BOARD;
	*pu32MaxBoardType = CSUSB_BT_LAST_BOARD;

	return CS_SUCCESS;
}

BOOL _CsDrvIsMyBoardType( uInt32 u32BoardType )
{
	if( u32BoardType >= CSUSB_BT_LAST_BOARD &&
		u32BoardType <= CSUSB_BT_LAST_BOARD )
		return TRUE;
	else
		return FALSE;
}

int32  _CsDrvGetFwOptionText( PCS_FWOPTIONS_CONVERT2TEXT pFwOptionText)
{
	if ( IsBadReadPtr( pFwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pFwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	if ( pFwOptionText->bBaseBoard )
	{
		// base board options 
		if ( (1<<pFwOptionText->u8BitPosition) & CS_OPTIONS_USB_FX70T_MASK )
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "FX-70 Device" );
		else
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
	}
	else
	{
		// Add on csi options ...
		// Mask for USB firmware options
		if ( (1<<pFwOptionText->u8BitPosition) & CS_OPTIONS_USB_FIRMWARE_MASK )
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "USB Firmware" );
		else if ( (1<<pFwOptionText->u8BitPosition) & CS_OPTIONS_USB_SX50T_MASK )
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "SX-50 Device" );
		else
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
	}
	return CS_SUCCESS;
}

int32  _CsDrvGetHwOptionText( uInt32 u32BoardType, PCS_HWOPTIONS_CONVERT2TEXT pHwOptionText )
{
	UNREFERENCED_PARAMETER(u32BoardType);

	if ( IsBadReadPtr( pHwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pHwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	if ( !pHwOptionText->bBaseBoard )
	{
		// Add input options
		// Mask for USB firmware options
		if ( (1<<pHwOptionText->u8BitPosition) & CS_USB_AC_DC_INPUT )
			strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "DC/AC input" );
		else
			strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
		
	}
	return CS_SUCCESS;
}

int32  _CsDrvGetBoardCaps( uInt32 , uInt32 , void* , uInt32* ){return CS_INVALID_REQUEST;}
