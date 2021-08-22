
// CsFastBall.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CsAdvHwDLL.h"
#include "CurrentDriver.h"

#include "Cs8xxxCapsInfo.h"
#include "CsPeevStruct.h"
#include "CsSpiderOptions.h"
#include "CsNucleonBaseFlash.h"
#include "CsPrivatePrototypes.h"

#ifdef _WINDOWS_
#include "resource.h"
#endif

#define nUSE_SHARE_BOARDCAPS


// Shared memory for all processes

// Global inter process varabales
extern	SHAREDDLLMEM		Globals;

#ifdef _WINDOWS_
	// Global variables
	CTraceManager*		g_pTraceMngr= NULL;
#endif

CsBaseHwDLL*		g_pBaseHwDll = NULL;
CsAdvHwDLL*			g_pAdvanceHwDll = NULL;
GLOBAL_PERPROCESS	g_GlobalPerProcess = {0};

// Prototypes functions
int64  ValidateSampleRate (DRVHANDLE DrvHdl,  PCSSYSTEMINFO pSysInfo, PCSSYSTEMCONFIG pSysCfg );
BOOL   ValidateChannelIndex( PCSSYSTEMINFO pSysInfo, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u16ChannelIndex );
int32  CsDrvGetAvailableSampleRates(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig, PCSSAMPLERATETABLE pSRTable, uInt32 *BufferSize );
int32  CsDrvGetAvailableInputRanges(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig, PCSRANGETABLE pRangesTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableImpedances(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,  PCSIMPEDANCETABLE pImpedancesTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableCouplings(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,   PCSCOUPLINGTABLE pCouplingsTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableFilters ( DRVHANDLE DrvHdl, uInt32 u32SubCapsId, PCSSYSTEMCONFIG pSystemConfig, PCSFILTERTABLE pFilterTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableAcqModes(DRVHANDLE DrvHdl, uInt32 SubCapsId, PCSSYSTEMCONFIG pSystemConfig, uInt32 *u32AcqModes, uInt32 *BufferSize );
int32  CsDrvGetAvailableTerminations(DRVHANDLE DrvHdl, uInt32 SubCapsId, PCSSYSTEMCONFIG pSystemConfig, uInt32 *u32Terminations, uInt32 *BufferSize );
int32  CsDrvGetAvailableFlexibleTrigger( DRVHANDLE DrvHdl, uInt32 CapsId, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetBoardTriggerEngines( DRVHANDLE DrvHdl, uInt32 CapsId, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetChannelTriggerEngines( DRVHANDLE DrvHdl, uInt32 CapsId, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetAvailableTriggerSources(CSHANDLE DrvHdl, uInt32 SubCapsId, int32 *pATriggerSources, uInt32 *BufferSize );
int32  CsDrvGetAuxConfig(CSHANDLE DrvHdl, PCS_CAPS_AUX_CONFIG pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetAvailableClockOut( DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSystemConfig, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize );
int32  CsDrvGetAvailableTrigOut( DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSystemConfig, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize );
int32  CsDrvGetMaxSegmentPadding( DRVHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetTriggerResolution( DRVHANDLE , uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetMinExtRate( DRVHANDLE , uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetMaxExtRate( DRVHANDLE , uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetSkipCount( DRVHANDLE, uInt32, PCSSYSTEMCONFIG, uInt32 *pBuffer,  uInt32 *BufferSize );
int64  ValidateSampleRate (DRVHANDLE DrvHdl,  PCSSYSTEMINFO pSysInfo, PCSSYSTEMCONFIG pSysCfg );
BOOL   ValidateChannelIndex( PCSSYSTEMINFO pSysInfo, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u16ChannelIndex );
PCSSYSTEMINFO			CsDllGetSystemInfoPointer( DRVHANDLE DrvHdl, PCSSYSTEMINFOEX *pSysInfoEx = NULL );			
PCSACQUISITIONCONFIG	CsDllGetAcqSystemConfigPointer( DRVHANDLE DrvHdl );
PCS8XXXBOARDCAPS		CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl );

const VALID_AUX_CONFIG	ValidTrigOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Trigger Out",		CS_TRIGOUT,
};

const VALID_AUX_CONFIG	ValidClkOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Sample Clock",		CS_CLKOUT_SAMPLE,
	"Reference Clock",	CS_CLKOUT_REF,
};

const VALID_AUX_CONFIG	ValidPulseOut[] =
{
	"Data Valid",			CS_PULSEOUT_DATAVALID,
	"Not Data Valid",		CS_PULSEOUT_NOT_DATAVALID,
	"Data Valid Rising",	CS_PULSEOUT_START_DATAVALID,
	"Data Valid Falling",	CS_PULSEOUT_END_DATAVALID,
	"Sync. Out",			CS_PULSEOUT_SYNC
};

const uInt32	g_u32ValidMode =  CS_MODE_OCT |
                                  CS_MODE_QUAD |
								  CS_MODE_DUAL |
								  CS_MODE_SINGLE | 
								  CS_MODE_REFERENCE_CLK |
								  CS_MODE_SW_AVERAGING |
								  CS_MODE_USER1 | CS_MODE_USER2;

const uInt32	g_u32ValidExpertFw =  CS_BBOPTIONS_MULREC_AVG_TD | CS_BBOPTIONS_STREAM;


int CompareInputRange( const void* pIr1, const void*pIr2 )
{
	return int(PCSRANGETABLE(pIr2)->u32InputRange) - int(PCSRANGETABLE(pIr1)->u32InputRange);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
typedef SharedMem< CS8XXXBOARDCAPS > SHARED8xxxCAPS;
int32 CsDrvAcquisitionSystemInit( DRVHANDLE	DrvHdl, BOOL bSystemDefaultSetting )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;
	int32		i32Status = g_pBaseHwDll->CsDrvAcquisitionSystemInit( DrvHdl, bSystemDefaultSetting );

	if (CS_SUCCESS != i32Status)
		return i32Status;

	for (uInt32 i = 0; i< Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		if ( pDrvSystem[i].DrvHandle == DrvHdl )
		{
			// Save a copy of current AcquisitonConfig
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_ACQUISITION, &pDrvSystem[i].AcqConfig );

			TCHAR strSharedCapsName[_MAX_PATH];
			_stprintf_s(strSharedCapsName, _countof(strSharedCapsName), SHARED_CAPS_NAME _T("-%d"), i);

#ifdef USE_SHARE_BOARDCAPS
			SHARED8xxxCAPS* pCaps = new SHARED8xxxCAPS( strSharedCapsName, true );
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, pCaps->it() );
			qsort(pCaps->it()->RangeTable, CSPDR_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
			pDrvSystem[i].pCapsInfo = pCaps;
#else
			CS8XXXBOARDCAPS* pCaps = new CS8XXXBOARDCAPS;
			if ( pCaps != NULL )
			{
				g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, pCaps );
				qsort(&pCaps->RangeTable, CSPDR_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
				pDrvSystem[i].pCapsInfo = pCaps;
			}
			else
			{
				g_pBaseHwDll->CsDrvAcquisitionSystemCleanup( DrvHdl );
				i32Status = CS_MEMORY_ERROR;
			}
#endif			
			break;
		}
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvAcquisitionSystemCleanup(DRVHANDLE DrvHdl)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;
	for (uInt32 i = 0; i< Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		if ( pDrvSystem[i].DrvHandle == DrvHdl && pDrvSystem[i].pCapsInfo != NULL )
		{
#ifdef USE_SHARE_BOARDCAPS
			delete ((SHARED8xxxCAPS *)pDrvSystem[i].pCapsInfo);
#else
			delete ((CS8XXXBOARDCAPS *)pDrvSystem[i].pCapsInfo);
#endif
			pDrvSystem[i].pCapsInfo = 0;
		}
	}

	return g_pBaseHwDll->CsDrvAcquisitionSystemCleanup( DrvHdl );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableDcOffset (DRVHANDLE DrvHdl);
int32 CsDrvGetAcquisitionSystemCaps(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg,
									 void *pBuffer, uInt32 *BufferSize)
{	
	uInt32			MainCapId = CapsId & CAPS_ID_MASK;
    uInt16			SubCapId = (uInt16) (CapsId & 0xFFFF);
	PCSSYSTEMINFOEX	pSysInfoEx; 
	
	if ( NULL == CsDllGetSystemCapsInfo(DrvHdl) )
		return CS_SYSTEM_NOT_INITIALIZED;

	CsDllGetSystemInfoPointer( DrvHdl, &pSysInfoEx );

	switch( MainCapId )
	{
	case CAPS_SAMPLE_RATES:
		return CsDrvGetAvailableSampleRates( DrvHdl, SubCapId, pSysCfg, (PCSSAMPLERATETABLE)pBuffer, BufferSize );

	case CAPS_INPUT_RANGES:
		return CsDrvGetAvailableInputRanges( DrvHdl, SubCapId, pSysCfg, (PCSRANGETABLE)pBuffer, BufferSize );

	case CAPS_IMPEDANCES:
		return CsDrvGetAvailableImpedances( DrvHdl, SubCapId, pSysCfg, (PCSIMPEDANCETABLE) pBuffer, BufferSize );

	case CAPS_COUPLINGS:
		return CsDrvGetAvailableCouplings( DrvHdl, SubCapId, pSysCfg, (PCSCOUPLINGTABLE) pBuffer, BufferSize );

	case CAPS_ACQ_MODES:
		return CsDrvGetAvailableAcqModes( DrvHdl, SubCapId, pSysCfg, (uInt32 *) pBuffer, BufferSize );

	case CAPS_TERMINATIONS:
		return CsDrvGetAvailableTerminations( DrvHdl, SubCapId, pSysCfg, (uInt32 *) pBuffer, BufferSize );

	case CAPS_FLEXIBLE_TRIGGER:
		return CsDrvGetAvailableFlexibleTrigger( DrvHdl, SubCapId, (uInt32 *) pBuffer, BufferSize );

	case CAPS_BOARD_TRIGGER_ENGINES:
		return CsDrvGetBoardTriggerEngines( DrvHdl, SubCapId, (uInt32 *) pBuffer, BufferSize );

	case CAPS_TRIG_ENGINES_PER_CHAN:
		return CsDrvGetChannelTriggerEngines( DrvHdl, SubCapId, (uInt32 *) pBuffer, BufferSize );

	case CAPS_TRIGGER_SOURCES:
		return g_pBaseHwDll->CsDrvGetAvailableTriggerSources( DrvHdl, SubCapId, (int32 *) pBuffer, BufferSize );

	case CAPS_MAX_SEGMENT_PADDING:
		return CsDrvGetMaxSegmentPadding( DrvHdl, (uInt32 *) pBuffer, BufferSize );

	case CAPS_FILTERS:
		return CsDrvGetAvailableFilters( DrvHdl, SubCapId, pSysCfg, (PCSFILTERTABLE) pBuffer, BufferSize );

	case CAPS_DC_OFFSET_ADJUST:
		return CsDrvGetAvailableDcOffset(DrvHdl);

	case CAPS_AUX_CONFIG:
		return CsDrvGetAuxConfig( DrvHdl, (PCS_CAPS_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_CLOCK_OUT:
		return CsDrvGetAvailableClockOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_TRIG_OUT:
		return CsDrvGetAvailableTrigOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_CLK_IN:			// external clock
		if ( CS_DEVID_SPIDER_LP == pSysInfoEx->u16DeviceId )
			return CS_INVALID_REQUEST;
		else
		{
			PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
			if( 0 == pCaps->ExtClkTable[0].i64Max  )
				return CS_INVALID_REQUEST;
		}
		break;

	case CAPS_MULREC:
		break;

	case CAPS_STM_TRANSFER_SIZE_BOUNDARY:
	case CAPS_TRIGGER_RES:
		return CsDrvGetTriggerResolution( DrvHdl, (uInt32 *) pBuffer, BufferSize );
		break;

	case CAPS_MIN_EXT_RATE:
		return CsDrvGetMinExtRate( DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );
		break;

	case CAPS_MAX_EXT_RATE:
		return CsDrvGetMaxExtRate( DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );
		break;

	case CAPS_SKIP_COUNT:
		return CsDrvGetSkipCount( DrvHdl, SubCapId, pSysCfg, (uInt32 *) pBuffer, BufferSize );
		break;

	case CAPS_BOOTIMAGE0:
		if ( CS_DEVID_SPIDER_PCIE != pSysInfoEx->u16DeviceId && CS_DEVID_ZAP_PCIE != pSysInfoEx->u16DeviceId )
			return CS_INVALID_REQUEST;
		break;

	default:
		return CS_INVALID_REQUEST;
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetParams(DRVHANDLE DrvHdl, uInt32 ParamID, void *ParamBuffer)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	switch (ParamID)
	{
		case CS_SYSTEM:
		case CS_ACQUISITION:
		case CS_TRIGGER:
		case CS_CHANNEL:
		case CS_READ_FLASH:
		case CS_MEM_READ_THRU:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		case CS_FIR_CONFIG:
		case CS_TRIG_OUT:
		case CS_CLOCK_OUT:
		case CS_EXTENDED_BOARD_OPTIONS:
		case CS_SYSTEMINFO_EX:
		case CS_FFT_CONFIG:
		case CS_FFTWINDOW_CONFIG:
		case CS_MULREC_AVG_COUNT:
		case CS_TRIG_OUT_CFG:
		{
			int32 i32Status;
	
			// Validate Buffer for Write Access
			i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( ParamID, ParamBuffer, FALSE );
			if (i32Status != CS_SUCCESS)
				return i32Status;
		}
		break;

		case CS_READ_NVRAM:
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = (PCS_PLX_NVRAM_RW_STRUCT) ParamBuffer;
			
			if ( IsBadWritePtr( ParamBuffer, sizeof(CS_PLX_NVRAM_RW_STRUCT) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pNVRAM_Struct->u32Size != sizeof( CS_PLX_NVRAM_RW_STRUCT ) )
				return CS_INVALID_STRUCT_SIZE;

			return g_pBaseHwDll->CsDrvGetParams( DrvHdl, ParamID, &pNVRAM_Struct->PlxNvRam );
		}

		case CS_READ_VER_INFO:
		{
			// check buffer for Read access
			PCS_FW_VER_INFO pInfo_Struct = (PCS_FW_VER_INFO) ParamBuffer;
			
			if ( IsBadWritePtr( ParamBuffer, sizeof(CS_FW_VER_INFO) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pInfo_Struct->u32Size != sizeof( CS_FW_VER_INFO ) )
				return CS_INVALID_STRUCT_SIZE;

			return g_pBaseHwDll->CsDrvGetParams( DrvHdl, ParamID, ParamBuffer );
		}
		break;

		case CS_READ_EEPROM:
		{
			// check buffer for Write access
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) ParamBuffer;

			if ( IsBadReadPtr( pGenericRw, sizeof(CS_GENERIC_EEPROM_READWRITE) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pGenericRw->u32Size != sizeof( CS_GENERIC_EEPROM_READWRITE ) )
					return CS_INVALID_STRUCT_SIZE;

			if ( IsBadWritePtr( pGenericRw->pBuffer,  pGenericRw->u32BufferSize ) )
				return CS_INVALID_POINTER_BUFFER;

		}
		break;

		case CS_PEEVADAPTOR_CONFIG:
			{
			// check buffer for Write access
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) ParamBuffer;

			if ( IsBadWritePtr( pPvAdaptorConfig,  sizeof(PVADAPTORCONFIG) ) )
				return CS_INVALID_POINTER_BUFFER;
			}
			break;

		case CS_FLASHINFO:
			{		
				if ( IsBadWritePtr( ParamBuffer, sizeof(CS_FLASHLAYOUT_INFO) ) )
					return CS_INVALID_POINTER_BUFFER;

				PCSSYSTEMINFOEX	pSysInfoEx; 
				uInt32			u32Size = sizeof(CS_FLASHLAYOUT_INFO);
				CsDllGetSystemInfoPointer( DrvHdl, &pSysInfoEx );

				return _CsDrvGetBoardCaps( pSysInfoEx->u16DeviceId, CS_FLASHINFO, ParamBuffer, &u32Size );
			}
			break;

		case CS_READ_PCIeLINK_INFO:
			{
			if ( IsBadWritePtr( ParamBuffer,  sizeof(PCIeLINK_INFO) ) )
				return CS_INVALID_POINTER_BUFFER;
			}
			break;

		case CS_TRIGGERED_INFO:
			if ( IsBadReadPtr( ParamBuffer,  sizeof(TRIGGERED_INFO_STRUCT) ) )
				return CS_INVALID_POINTER_BUFFER;
			break;


		case CS_SEGMENTTAIL_SIZE_BYTES:
		case CS_READ_GAGE_REGISTER:
		case CS_READ_PCI_REGISTER:
			if ( IsBadWritePtr( ParamBuffer,  sizeof(uInt32) ) )
				return CS_INVALID_POINTER_BUFFER;
			break;

		case CS_TIMESTAMP_TICKFREQUENCY:
		case CS_STREAM_SEGMENTDATA_SIZE_SAMPLES:
		case CS_STREAM_TOTALDATA_SIZE_BYTES:
		{
			// check buffer for Write access
			int64 *pTickFrequency = (int64 *) ParamBuffer;

			if ( IsBadWritePtr( pTickFrequency, sizeof(int64) ) )
				return CS_INVALID_POINTER_BUFFER;

		}
        break;

		default:
			return CS_INVALID_PARAMS_ID;
	}

	return g_pBaseHwDll->CsDrvGetParams( DrvHdl, ParamID, ParamBuffer );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvSetParams(DRVHANDLE DrvHdl, uInt32 ParamID, void *ParamBuffer)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	// Validate configuration Params
	switch (ParamID)
	{
		case CS_ACQUISITION:
		case CS_TRIGGER:
		case CS_CHANNEL:
		case CS_SYSTEM:
		case CS_WRITE_FLASH: 
		case CS_SEND_DAC:
		case CS_SELF_TEST_MODE:
		case CS_TRIG_OUT:
		case CS_CLOCK_OUT:
		case CS_ADDON_RESET:
		case CS_TRIG_ENABLE:
		case CS_NIOS_DEBUG:
		case CS_MEM_WRITE_THRU:
		case CS_CALIBMODE_CONFIG:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		case CS_FIR_CONFIG:
		case CS_DELAY_LINE_MODE:
		case CS_FFT_CONFIG:
		case CS_FFTWINDOW_CONFIG:
		case CS_MULREC_AVG_COUNT:
		case CS_TRIG_OUT_CFG:
		case CS_SAVE_OUT_CFG:
		case CS_SEND_NIOS_CMD:
		{
			int32 i32Status;

			// Validate Buffer for Read/Write
			i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( ParamID, ParamBuffer, TRUE );
			if (i32Status != CS_SUCCESS)
				return i32Status;
		}
		break;

		case CS_WRITE_NVRAM:
		{
			// check buffer for Read access
			PCS_PLX_NVRAM_RW_STRUCT pNVRAM_Struct = (PCS_PLX_NVRAM_RW_STRUCT) ParamBuffer;
			
			if ( IsBadReadPtr( ParamBuffer, sizeof(CS_PLX_NVRAM_RW_STRUCT) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pNVRAM_Struct->u32Size != sizeof( CS_PLX_NVRAM_RW_STRUCT ) )
				return CS_INVALID_STRUCT_SIZE;

			return g_pBaseHwDll->CsDrvSetParams( DrvHdl, ParamID, &pNVRAM_Struct->PlxNvRam );
		}
		break;


		case CS_WRITE_EEPROM:
		{
			// check buffer for Write access
			PCS_GENERIC_EEPROM_READWRITE pGenericRw = (PCS_GENERIC_EEPROM_READWRITE) ParamBuffer;

			if ( IsBadReadPtr( pGenericRw, sizeof(CS_GENERIC_EEPROM_READWRITE) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pGenericRw->u32Size != sizeof( CS_GENERIC_EEPROM_READWRITE ) )
					return CS_INVALID_STRUCT_SIZE;

			if ( IsBadReadPtr( pGenericRw->pBuffer,  pGenericRw->u32BufferSize ) )
				return CS_INVALID_POINTER_BUFFER;
		}
		break;

		case CS_TRIM_CALIBRATOR:
			break;

		case CS_SAVE_CALIBTABLE_TO_EEPROM:
			break;

		case CS_PEEVADAPTOR_CONFIG:
			{
			// check buffer for Read access
			PPVADAPTORCONFIG pPvAdaptorConfig = (PPVADAPTORCONFIG) ParamBuffer;

			if ( IsBadReadPtr( pPvAdaptorConfig,  sizeof(PVADAPTORCONFIG) ) )
				return CS_INVALID_POINTER_BUFFER;
			}
			break;

		case CS_SPDR12_TRIM_TCAPS:
			{
				// check buffer for Read access
				PCS_SPDR12_TRIM_TCAP_STRUCT pTcapStruct = (PCS_SPDR12_TRIM_TCAP_STRUCT) ParamBuffer;

				if ( IsBadReadPtr( pTcapStruct,  sizeof(CS_SPDR12_TRIM_TCAP_STRUCT) ) )
					return CS_INVALID_POINTER_BUFFER;
			}
			break;
		case CS_WRITE_PCI_REGISTER:
			break;

		default:
			return CS_INVALID_PARAMS_ID;

	}


	return g_pBaseHwDll->CsDrvSetParams( DrvHdl, ParamID, ParamBuffer );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
PCS8XXXBOARDCAPS CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl)
{
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Search for SystemInfo of the current system based on DrvHdl
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
#ifdef USE_SHARE_BOARDCAPS
			return ((SHARED8xxxCAPS*)(pDrvSystem[i].pCapsInfo))->it();
#else
			return (PCS8XXXBOARDCAPS)pDrvSystem[i].pCapsInfo;
#endif
		}
	}
	_ASSERT(0);
	return NULL;
}


//#define	MAX_SPDR_TRIGGER_ADDRESS_OFFSET		24		// Should be multiple of 8
#define CSPDR_MIN_SEG_SIZE					8		// Should be multiple of 8
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvValidateAcquisitionConfig( DRVHANDLE DrvHdl,
									  PCSSYSTEMCONFIG pSysCfg,
									  PCSSYSTEMINFO pSysInfo,
									  uInt32 u32Coerce )

{	
	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	int32	i32Status = CS_SUCCESS;
	uInt32	u32AcqModeValid = g_u32ValidMode;
	uInt32	u32Size = sizeof( u32AcqModeValid );
	uInt32	u32ModeMaskChannel = CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL | CS_MODE_SINGLE;
	
	CsDrvGetAvailableAcqModes( DrvHdl, 0, pSysCfg, &u32AcqModeValid, &u32Size );

	// Validation of Mode
	if ( 0 != (pAcqCfg->u32Mode & ~u32AcqModeValid)  )
	{
		if ( u32Coerce )
		{
			pAcqCfg->u32Mode &= u32AcqModeValid;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_ACQ_MODE;
	}

	if ( !(pAcqCfg->u32Mode & u32AcqModeValid) ||
		 (( pAcqCfg->u32Mode & u32ModeMaskChannel ) == 0 ) ||
		 (( pAcqCfg->u32Mode & CS_MODE_USER1 ) && (pAcqCfg->u32Mode & CS_MODE_USER2) ) ||	// Only one expert can be set
		 ((( pAcqCfg->u32Mode & u32ModeMaskChannel ) != CS_MODE_SINGLE ) &&					// Only one channel mode can be set
		 (( pAcqCfg->u32Mode & u32ModeMaskChannel ) != CS_MODE_DUAL ) &&
		 (( pAcqCfg->u32Mode & u32ModeMaskChannel ) != CS_MODE_QUAD ) &&
		 (( pAcqCfg->u32Mode & u32ModeMaskChannel ) != CS_MODE_OCT ) ) 
		 )
	{
		if ( u32Coerce )
		{
			pAcqCfg->u32Mode = CS_MODE_DUAL;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_ACQ_MODE;
	}

	//----- Sample Rate
	int64 i64CoerceSampleRate = ValidateSampleRate(DrvHdl, pSysInfo, pSysCfg );
	if ( i64CoerceSampleRate != pSysCfg->pAcqCfg->i64SampleRate )
	{
		if ( u32Coerce )
		{
			pAcqCfg->i64SampleRate = i64CoerceSampleRate;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_SAMPLE_RATE;
	}

	if ( pAcqCfg->u32SegmentCount == 0 )
	{
		if ( u32Coerce )
		{
			pAcqCfg->u32SegmentCount = 1;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_COUNT;
	}

	// Depth counters are in 128 (memory bus width) devided by ( 16 (sample width) * # channels ) increments
	int64	i64MaxHwDepthCounter = ((int64)CSMV_LIMIT_HWDEPTH_COUNTER) * 128/(16*(pAcqCfg->u32Mode&CS_MASKED_MODE));

	// Validation of trigger holdoff
	if  ( ( pAcqCfg->i64TriggerHoldoff < 0 ) ||
		  ( pAcqCfg->i64TriggerHoldoff > i64MaxHwDepthCounter ) )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64TriggerHoldoff = 0;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGHOLDOFF;
	}
	
	// Added Sept 15, 2015 to report error if total data size is too big (overflows a uint64) but we're not in infinite mode
	if ( -1 != pAcqCfg->i32SegmentCountHigh && -1 != pAcqCfg->i64Depth )
	{
		const uint64 uMax64 = (uint64) ~0;
		ULONGLONG uSubTotal = pAcqCfg->i64Depth * pAcqCfg->u32SampleSize * (pAcqCfg->u32Mode & CS_MASKED_MODE);
		if ( 0 != uSubTotal  && pAcqCfg->u32SegmentCount > (uMax64 / uSubTotal) )
		{
			return CS_STM_TOTALDATA_SIZE_INVALID;
		}
	}
// end addition
	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvValidateChannelConfig( DRVHANDLE DrvHdl,
								  PCSSYSTEMCONFIG pSysCfg,
								  PCSSYSTEMINFO pSysInfo,
								  uInt32 Coerce )
{
	PCSCHANNELCONFIG	pChanCfg = (PCSCHANNELCONFIG) pSysCfg->pAChannels->aChannel;
	int32				i32Status = CS_SUCCESS;


	if ( pSysCfg->pAChannels->u32ChannelCount > pSysInfo->u32ChannelCount )
	{
		if ( Coerce )
		{
			pSysCfg->pAChannels->u32ChannelCount = pSysInfo->u32ChannelCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_CHANNEL_COUNT;
	}	

	for ( uInt32 k = 0; k < pSysCfg->pAChannels->u32ChannelCount; k ++, pChanCfg++ )
	{
		//----- Channel Index
		if ( ! ValidateChannelIndex( pSysInfo, pSysCfg->pAcqCfg, pChanCfg->u32ChannelIndex ))
		{
			if ( Coerce )
			{
				pChanCfg->u32ChannelIndex = 1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_CHANNEL;
		}
				
		//----- Term
		int		j;
		PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
		for( j = 0; j < CSPDR_COUPLING; j++ )
		{
			if ( pChanCfg->u32Term == pCaps->CouplingTable[j].u32Coupling )
				break;
		}
		if ( CSPDR_COUPLING == j )
		{
			if ( Coerce )
			{
				pChanCfg->u32Term = pCaps->CouplingTable[0].u32Coupling;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_COUPLING;
		}

		//----- Impedance
		uInt32	ImpedanceMask = CS_IMP_1M_OHM;
		for( j = 0; j < CSPDR_IMPED; j++ )
		{
			if ( pChanCfg->u32Impedance == pCaps->ImpedanceTable[j].u32Imdepdance )
			{
				if( CS_REAL_IMP_50_OHM == pChanCfg->u32Impedance )
					ImpedanceMask = CS_IMP_50_OHM;
                break;
			}
		}
		if ( CSPDR_IMPED == j )
		{
			if ( Coerce )
			{
				pChanCfg->u32Impedance = pCaps->ImpedanceTable[0].u32Imdepdance;
				j = 0;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_IMPEDANCE;
		}

		//----- Gain
		int		i;

		for ( i = 0; i < CSPDR_MAX_RANGES; i ++ )
		{
			if ( pCaps->RangeTable[i].u32Reserved & ImpedanceMask )
			{
				if ( pCaps->RangeTable[i].u32InputRange == pChanCfg->u32InputRange )
					break;
			}
		}

		if ( i >= CSPDR_MAX_RANGES )
		{
			if ( Coerce )
			{
				pChanCfg->u32InputRange = CS_GAIN_2_V;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_GAIN;
		}

		//----- Filter	
		if( pChanCfg->u32Filter != 0 )
		{
			if( CS_NO_FILTER == pCaps->FilterTable[1].u32LowCutoff )
			{
				if ( Coerce )
				{
					pChanCfg->u32Filter = 0;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
				{
					return CS_INVALID_FILTER;
				}
			}
		}
		//----- Position
		if( pChanCfg->i32DcOffset > int32(pChanCfg->u32InputRange/2)  )
		{
			if ( Coerce )
			{
				pChanCfg->i32DcOffset = int32(pChanCfg->u32InputRange);
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_INVALID_POSITION;
			}
		}
		else if( pChanCfg->i32DcOffset < -int32(pChanCfg->u32InputRange/2)  )
		{
			if ( Coerce )
			{
				pChanCfg->i32DcOffset = -int32(pChanCfg->u32InputRange);
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_INVALID_POSITION;
			}
		}
	}

	return i32Status;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvValidateTriggerConfig( PCSSYSTEMCONFIG pSysCfg,
								  PCSSYSTEMINFO pSysInfo,
								  uInt32 Coerce )
{
	PCSTRIGGERCONFIG	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	int32				i32Status = CS_SUCCESS;


	if ( pSysCfg->pATriggers->u32TriggerCount != pSysInfo->u32TriggerMachineCount )
	{
		if ( Coerce )
		{
			pSysCfg->pATriggers->u32TriggerCount = pSysInfo->u32TriggerMachineCount;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGGER_COUNT;
	}	

	// Looking for at least on trigger source enabled valid
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		//----- trigger count: must be 0 < count < max_count
		if ( pTrigCfg->u32TriggerIndex <= 0 ||
			pTrigCfg->u32TriggerIndex > pSysInfo->u32TriggerMachineCount )
		{
			if ( Coerce )
			{
				pTrigCfg->u32TriggerIndex = 1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIGGER;
		}
	}
				 
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		//----- condition: might be pos, neg or stg else TBD 
		if ( pTrigCfg->u32Condition != CS_TRIG_COND_POS_SLOPE &&
				pTrigCfg->u32Condition != CS_TRIG_COND_NEG_SLOPE &&
				pTrigCfg->u32Condition != CS_TRIG_COND_PULSE_WIDTH )
		{
			if ( Coerce )
			{
				pTrigCfg->u32Condition = CS_TRIG_COND_POS_SLOPE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_COND;
		}

		//----- level: must be -100<=level<=100
		if ( pTrigCfg->i32Level > 100 || pTrigCfg->i32Level < -100  )
		{
			if ( Coerce )
			{
				pTrigCfg->i32Level = 0;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_TRIG_LEVEL;
		}

		//----- coupling for external trigger: must be either AC or DC
		if ( pTrigCfg->i32Source == CS_TRIG_SOURCE_EXT )
		{
			if ( (pTrigCfg->u32ExtCoupling != CS_COUPLING_AC) &&
				(pTrigCfg->u32ExtCoupling != CS_COUPLING_DC ) )
			{
				if ( Coerce )
				{
					pTrigCfg->u32ExtCoupling = CS_COUPLING_DC;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_COUPLING;	
			}
			
			//----- gain for External Trig, 
			if ( (pTrigCfg->u32ExtTriggerRange != CS_GAIN_2_V) &&
				(pTrigCfg->u32ExtTriggerRange != CS_GAIN_10_V) )
			{
				if ( Coerce )
				{
					pTrigCfg->u32ExtTriggerRange = CS_GAIN_2_V;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_GAIN;
			}
		}
	}

	return i32Status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 ValidateNumberOfTriggerEnabled( PARRAY_TRIGGERCONFIG pATriggers,
									PCSSYSTEMINFO pSysInfo,
								    uInt32 Coerce )
{
	PCSTRIGGERCONFIG pTrigCfg = pATriggers->aTrigger;
	uInt32	u32NumOfTriggerEnabled = 0;

	for ( uInt32 i = 0; i < pATriggers->u32TriggerCount; i ++) //----- True if u32TriggerCount is the counter of the enabled triggers 
															   //----- meaning for Cs8xxx we can have up to 2 engines so 0, 1
															   //----- triggerIndex will tell us which brd should triggered
	{
		if ( CS_TRIG_SOURCE_DISABLE != pTrigCfg[i].i32Source )
			u32NumOfTriggerEnabled ++;
	}

	if ( u32NumOfTriggerEnabled > (pSysInfo->u32TriggerMachineCount / pSysInfo->u32BoardCount) )
	{
		if ( Coerce )
		{
			for ( uInt32 i = 0; i < pATriggers->u32TriggerCount; i ++) 
			{
				if ( CS_TRIG_SOURCE_DISABLE != pTrigCfg[i].i32Source )
				{
					if ( u32NumOfTriggerEnabled < 2 )
						u32NumOfTriggerEnabled ++;
					else
					{
						// Diable all the remaining trigge engines
						pTrigCfg[i].i32Source = CS_TRIG_SOURCE_DISABLE;
					}
				}
			}
			return CS_CONFIG_CHANGED;
		}
		else
		{
			return CS_INVALID_TRIGGER_ENABLED;
		}
	}
	else
		return CS_SUCCESS;

}


//-----------------------------------------------------------------------------
//	User channel skip depends on mode amd number of channels per card
//-----------------------------------------------------------------------------
uInt16	GetUserChannelSkip( uInt32 u32ChannelCount, uInt32 u32Mode )
{
	uInt16	u16ChannelSkip = 0;


	switch ( u32ChannelCount )
	{
		case 8:
			{
			if ( u32Mode & CS_MODE_OCT )
				u16ChannelSkip = 1;
			else if ( u32Mode & CS_MODE_QUAD )
				u16ChannelSkip = 2;
			else if ( u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 4;
			else if ( u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 8;
			}
			break;
		case 4:
			{
			if ( u32Mode & CS_MODE_QUAD )
				u16ChannelSkip = 1;
			else if ( u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 2;
			else if ( u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 4;
			}
			break;
		case 2:
			{
			if ( u32Mode & CS_MODE_DUAL )
				u16ChannelSkip = 1;
			else if ( u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 2;
			}
			break;
		case 1:
			{
			if ( u32Mode & CS_MODE_SINGLE )
				u16ChannelSkip = 1;
			}
			break;
	}

#ifdef  _WINDOWS_
	ASSERT( u16ChannelSkip != 0 );
#endif

	return u16ChannelSkip;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 ValidateTriggerSources ( PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 Coerce )
{
	PCSTRIGGERCONFIG	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	int32				i32Status = CS_SUCCESS;
	BOOL				bErrorTriggerSource = FALSE;

	uInt32	u16ChannelPerCard = pSysInfo->u32ChannelCount / pSysInfo->u32BoardCount;
	uInt32 u32ChannelSkip = GetUserChannelSkip(u16ChannelPerCard, pAcqCfg->u32Mode );

	// Validation of Trigger Source
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( ( pTrigCfg->i32Source > (int32) pSysInfo->u32ChannelCount ) || pTrigCfg->i32Source < CS_TRIG_SOURCE_EXT )
				bErrorTriggerSource = TRUE;
			else if ( pTrigCfg->i32Source > 0 )
			{
				uInt32	j = 0;

				for ( j = 1; j <= pSysInfo->u32ChannelCount; )
				{
					if ( j == (uInt32) pTrigCfg->i32Source )
						break;
					
					j += u32ChannelSkip; 
				}

				if ( j > pSysInfo->u32ChannelCount )
					bErrorTriggerSource = TRUE;
			}
	
			if ( bErrorTriggerSource )
			{
				if ( Coerce )
				{
					pTrigCfg->i32Source = CS_TRIG_SOURCE_DISABLE;
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_TRIG_SOURCE;
			}
		}
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvValidateParams(DRVHANDLE DrvHdl, uInt32 ParamID, uInt32 u32Coerce, void *ParamBuffer)
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	int32		i32Status = CS_SUCCESS;
	int32		i32CoerceStatus = CS_SUCCESS;

	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );
	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	// Validate Buffer for Read/Write
	i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( ParamID, ParamBuffer, TRUE );
	if (i32Status != CS_SUCCESS)
		return i32Status;
    
	//----- Validate configuration Params
	switch (ParamID)
	{
		case CS_SYSTEM:
		{
			PCSSYSTEMCONFIG pSysCfg = (PCSSYSTEMCONFIG) ParamBuffer;
			
			//----- ACQUISITION
			i32Status = CsDrvValidateAcquisitionConfig( DrvHdl, pSysCfg, pSysInfo, u32Coerce );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			if ((u32Coerce != 0) && (i32CoerceStatus != CS_CONFIG_CHANGED))
				i32CoerceStatus = i32Status;

			//----- TRIGGER
			i32Status = g_pBaseHwDll->CsDrvValidateTriggerConfig( pSysCfg, pSysInfo, u32Coerce );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			if ((u32Coerce != 0) && (i32CoerceStatus != CS_CONFIG_CHANGED))
				i32CoerceStatus = i32Status;

			i32Status = ValidateTriggerSources( pSysCfg, pSysInfo,  u32Coerce );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			if ((u32Coerce != 0) && (i32CoerceStatus != CS_CONFIG_CHANGED))
				i32CoerceStatus = i32Status;

			//----- CHANNEL
			i32Status = CsDrvValidateChannelConfig(DrvHdl, pSysCfg, pSysInfo, u32Coerce );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			if ((u32Coerce != 0) && (i32CoerceStatus != CS_CONFIG_CHANGED))
				i32CoerceStatus = i32Status;

			i32Status = g_pBaseHwDll->CsDrvValidateParams( DrvHdl, u32Coerce, pSysCfg );
			if ( CS_FAILED(i32Status) )
				return i32Status;

			if ((u32Coerce != 0) && (i32CoerceStatus != CS_CONFIG_CHANGED))
				i32CoerceStatus = i32Status;

		}
		break;

	default:
		return CS_INVALID_PARAMS_ID;
	
	}		// end of switch

	if (u32Coerce)
		return i32CoerceStatus;
	else
		return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvTransferDataEx( DRVHANDLE drvHandle, PIN_PARAMS_TRANSFERDATA_EX InParams, POUT_PARAMS_TRANSFERDATA_EX OutParams )
{
	UNREFERENCED_PARAMETER(drvHandle);
	UNREFERENCED_PARAMETER(InParams);
	UNREFERENCED_PARAMETER(OutParams);
	return CS_FUNCTION_NOT_SUPPORTED;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvTransferData(DRVHANDLE DrvHdl, IN_PARAMS_TRANSFERDATA InParams,
							POUT_PARAMS_TRANSFERDATA	OutParams )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	int64			LimitPreTrigger = (int64)-1*0xFFFFFFFF;
	const uInt32	cu32ValidTxMode	= ( TxMODE_SLAVE | TxMODE_DATA_16 | TxMODE_DATA_ONLYDIGITAL | 
										TxMODE_DATA_32 | TxMODE_DATA_INTERLEAVED);


	// Search for SystemInfo of the current system based on DrvHdl
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );
	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	uInt8 u8Alignment = 0;
	uInt16 u16Backup = 0;

	PCSACQUISITIONCONFIG	pAcqCfg = CsDllGetAcqSystemConfigPointer( DrvHdl );

	// Validation of Segment
	if( (InParams.u32Segment == 0) || (InParams.u32Segment > pAcqCfg->u32SegmentCount) )
		return CS_INVALID_SEGMENT;

	if ( InParams.u32Mode == 0 || ((InParams.u32Mode & cu32ValidTxMode ) != 0 ) )
	{
		//
		// Mode Data Transfer Normal ( Integer )
		//

		// Validation of ChannelIndex
		if ( (InParams.u16Channel == 0) || (InParams.u16Channel > pSysInfo->u32ChannelCount) )
			return CS_INVALID_CHANNEL;

		// Check buffer for DWORD alignment
		u8Alignment = uInt8(DWORD_PTR(InParams.pDataBuffer) & DWORD_PTR(0x3));
		if( 0 != u8Alignment )
		{
			if( (sizeof(u16Backup) != u8Alignment) ||  (0 != (TxMODE_DATA_32 & InParams.u32Mode)) )
				return CS_BUFFER_NOT_ALIGNED;

			InParams.pDataBuffer = (void*)( DWORD_PTR(InParams.pDataBuffer) & ~DWORD_PTR(0x3) );
			u16Backup = *(uInt16*)InParams.pDataBuffer;
			InParams.i64Length++;
			InParams.i64StartAddress--;
		}

		// Validation for the Length of transfer (4 bytes boundary)
		if ( InParams.i64Length < 4 ||  InParams.i64Length % 2 != 0)
			return	CS_INVALID_LENGTH;

		if ( 0 == (InParams.u32Mode & TxMODE_DATA_INTERLEAVED ) )
		{
			// Limit the transfer size around the value of Segment size.
			// The kernel driver will not return data exceeded the SegmentSize anyway.
			if ( InParams.i64Length > pAcqCfg->i64SegmentSize + CS_MAX_PADDING )
				InParams.i64Length = pAcqCfg->i64SegmentSize + CS_MAX_PADDING;
		}

		// Validation for the StartAddress
		if ( (InParams.i64StartAddress >  pAcqCfg->i64Depth + pAcqCfg->i64TriggerDelay) ||
			(InParams.i64StartAddress < LimitPreTrigger ))
			return	CS_INVALID_START;

		// Validation of pointers for returned values
		if ( IsBadWritePtr( OutParams, sizeof( OUT_PARAMS_TRANSFERDATA ) ) )
			return CS_INVALID_POINTER_BUFFER;

		if ( TxMODE_DATA_32 & InParams.u32Mode )
		{
			// Convert length from samples to bytes.
			// Expected 32bits/sample data buffer
			InParams.i64Length = InParams.i64Length * sizeof(uInt32);
		}
		else
		{
			// Convert length from samples to bytes
			InParams.i64Length = InParams.i64Length * pSysInfo->u32SampleSize;
		}
	}
	else if (InParams.u32Mode & TxMODE_TIMESTAMP)
	{
		//
		// Mode Data Transfer Timestamp
		//
		
		// InParams.i64Start will be ignored
		// InParams.u32Segment will be the starting segment 
		// InParams.Length is the number of timestamp to transfer

		if ( ! InParams.pDataBuffer )
			return CS_NULL_POINTER;
		
		// Validation of pointers for returned values
		if ( IsBadWritePtr( OutParams, sizeof( OUT_PARAMS_TRANSFERDATA ) ) )
			return CS_INVALID_POINTER_BUFFER;

		// Convert length from sizeof(int64) to bytes
		InParams.i64Length = InParams.i64Length * sizeof(int64);
	}
	else
		return CS_INVALID_TRANSFER_MODE;

	if ( InParams.pDataBuffer != NULL && ::IsBadWritePtr( InParams.pDataBuffer, (uInt32) InParams.i64Length ) )
		return CS_INVALID_POINTER_BUFFER;

	// IMPORTANT !!!
	// Length is now in bytes
	int32 i32Ret = g_pBaseHwDll->CsDrvTransferData( DrvHdl, InParams, OutParams );

	if( CS_SUCCEEDED(i32Ret)  && 0 != u8Alignment )
	{
		*(uInt16*)(InParams.pDataBuffer) = u16Backup;
		(OutParams->i64ActualLength)--;
		(OutParams->i64ActualStart)++;
	}
	return i32Ret;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDrvGetAvailableSampleRates(CSHANDLE DrvHdl, uInt32 , PCSSYSTEMCONFIG pSystemConfig, PCSSAMPLERATETABLE pSRTable,
								   uInt32 *BufferSize )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	if( NULL == pCaps)
		return (CS_INVALID_CAPS_ID);

	uInt32	u32NumOfSr = 0;
	uInt32 i;	
	if ( pSystemConfig == NULL )//----- return the complete table
	{
		// If pSystemConfig == NULL,
		// Application request for all sample rates available, regardless the current configuration
		//
		
		// Calculate number of element in SR table depending on card
		for (i = 0; i < CSPDR_MAX_SR_COUNT; i ++ )
		{
			if ( 0 != pCaps->SrTable[i].PublicPart.i64SampleRate  )
				u32NumOfSr ++;
		}

		if ( pSRTable == NULL )
		{
			// If pSRTable == NULL
			// Application request for size(in byte) of RS Table and number of elements inside
			// 
			*BufferSize = u32NumOfSr * sizeof(CSSAMPLERATETABLE);
			return u32NumOfSr;
		}
		else 
		{
			// If pSRTable != NULL
			// pSRTable is the pointer to application allocated memory
			// 
			if ( *BufferSize < u32NumOfSr * sizeof(CSSAMPLERATETABLE) )
				return CS_BUFFER_TOO_SMALL;
			else 
			{
				for (i = 0; i < u32NumOfSr; i ++ )
				{
					memcpy( &pSRTable[i], &(pCaps->SrTable[i].PublicPart),  sizeof(CSSAMPLERATETABLE) );
				}
				return u32NumOfSr;
			}
		}
	}
	else //----- pSystemConfig != NULL
		 //----- return the table according to the mode
	{
		int32	i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( CS_SYSTEM, pSystemConfig, TRUE );
		if ( i32Status != CS_SUCCESS )
			return i32Status;
		PCSACQUISITIONCONFIG pAcqCfg = pSystemConfig->pAcqCfg;

		//----- calculate the size of the table for that mode
		for (i = 0; i < CSPDR_MAX_SR_COUNT; i ++ )
		{
			if ( (0 != pCaps->SrTable[i].PublicPart.i64SampleRate) &&
				 (pCaps->SrTable[i].u32Mode & pAcqCfg->u32Mode ) )
				u32NumOfSr ++;
		}
		
		if ( pSRTable == NULL )//----- first call return the size
		{
			*BufferSize = u32NumOfSr * sizeof(CSSAMPLERATETABLE);
			return CS_SUCCESS;
		}
		else //----- 2nd call return the table
		{
			if ( *BufferSize < u32NumOfSr * sizeof(CSSAMPLERATETABLE))
				return CS_BUFFER_TOO_SMALL;
			else
			{
				for (i = 0; i < u32NumOfSr; i ++ )
				{
					if ( pCaps->SrTable[i].u32Mode & pAcqCfg->u32Mode )
					{
						memcpy( pSRTable, &(pCaps->SrTable[i].PublicPart), sizeof(CSSAMPLERATETABLE));
						pSRTable ++;
					}
				}
			}
			return CS_SUCCESS;
		}
	}

}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableInputRanges(DRVHANDLE DrvHdl, uInt32 SubCapsId, PCSSYSTEMCONFIG pSystemConfig,
								   PCSRANGETABLE pRangesTbl,
								   uInt32 *BufferSize )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	uInt32	u32NumOfElement = 0;
	uInt32	i, j;

	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	if ( pSystemConfig == NULL )
	{
		// If pSystemConfig == NULL,
		// Application request for all sample rates available, regardless the current configuration
		//

		for( i = 0; i < CSPDR_MAX_RANGES; i++ )
		{
			if ( 0 != pCaps->RangeTable[i].u32InputRange )
			{
				u32NumOfElement++;
			}
		}
		
		if ( pRangesTbl == NULL )
		{
			// If pSRTable == NULL
			// Application request for size(in byte) of IR Table and number of elements inside
			// 
			*BufferSize = sizeof(CSRANGETABLE) * u32NumOfElement;
			return u32NumOfElement;
		}
		else
		{
			if ( *BufferSize < sizeof(CSRANGETABLE) * u32NumOfElement )
				return CS_BUFFER_TOO_SMALL;
			else 
			{
				memcpy( pRangesTbl, pCaps->RangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
				return u32NumOfElement;
			}
		}
	}
	else
	{
		int32	i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( CS_SYSTEM, pSystemConfig, TRUE );
		if ( i32Status != CS_SUCCESS )
			return i32Status;

		uInt16 Input = (uInt16) SubCapsId;
		PCSCHANNELCONFIG pChanCfg = pSystemConfig->pAChannels->aChannel;
		uInt16		ImpedanceMask(0);

		if ( Input == 0 )
		{
			for (i = 0; i < CSPDR_EXT_TRIG_RANGES; i ++ )
			{
				if ( 0 != pCaps->ExtTrigRangeTable[i].u32InputRange  )
                    u32NumOfElement ++;
			}

			if ( pRangesTbl == NULL )
			{
				*BufferSize = sizeof(CSRANGETABLE) * u32NumOfElement;
				return u32NumOfElement;
			}
			else
			{
				if ( *BufferSize < sizeof(CSRANGETABLE) * u32NumOfElement )
					return CS_BUFFER_TOO_SMALL;
				else 
				{
					memcpy( pRangesTbl, pCaps->ExtTrigRangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
					return u32NumOfElement;
				}
			}
		}
		else
		{
			if ( Input  > pSystemConfig->pAChannels->u32ChannelCount )
				return CS_INVALID_CHANNEL;

			for ( i = 0; i < pSystemConfig->pAChannels->u32ChannelCount; i ++ )
			{
				if (Input == pChanCfg[i].u32ChannelIndex)
				{
					if ( pChanCfg[i].u32Impedance == CS_REAL_IMP_50_OHM )
						ImpedanceMask = CS_IMP_50_OHM;
					else
						ImpedanceMask = CS_IMP_1M_OHM;
					break;
				}
			}

			for (i = 0; i < CSPDR_MAX_RANGES; i ++ )
			{
				if ( (pCaps->RangeTable[i].u32Reserved  & ImpedanceMask) == ImpedanceMask )
					u32NumOfElement ++;
			}
			if ( pRangesTbl == NULL )
			{
				*BufferSize = u32NumOfElement * sizeof(CSRANGETABLE);
				return u32NumOfElement;
			}
			else
			{
				if ( *BufferSize < u32NumOfElement * sizeof(CSRANGETABLE))
					return CS_BUFFER_TOO_SMALL;
				
				for (i = 0, j= 0; i < CSPDR_MAX_RANGES && j < u32NumOfElement; i++ )
				{
					if ( (pCaps->RangeTable[i].u32Reserved  & ImpedanceMask) == ImpedanceMask )
					{
						pRangesTbl[j++] = pCaps->RangeTable[i];
					}
				}
			}
			return u32NumOfElement;
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableImpedances( DRVHANDLE DrvHdl, uInt32 u32ChannelIndex, PCSSYSTEMCONFIG ,
								   PCSIMPEDANCETABLE pImpedancesTbl,
								   uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32 u32NumOfElement = 0;


	if ( 0 != u32ChannelIndex )
	{
		for( uInt32 i = 0; i< CSPDR_IMPED; i++ )
		{
			if( 0 == pCaps->ImpedanceTable[i].u32Reserved && 
				0 != pCaps->ImpedanceTable[i].u32Imdepdance )
				u32NumOfElement++;
		}

		if ( NULL == pImpedancesTbl )//----- first call return the size
		{
			*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
			return u32NumOfElement;
		}
		else //----- 2nd call return the table
		{
			if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
				return CS_BUFFER_TOO_SMALL;
			else 
			{
				for( uInt32 i = 0; i< CSPDR_IMPED; i++ )
				{
					if( 0 == pCaps->ImpedanceTable[i].u32Reserved && 
						0 != pCaps->ImpedanceTable[i].u32Imdepdance )
						memcpy( &pImpedancesTbl[i], &pCaps->ImpedanceTable[i], sizeof(CSIMPEDANCETABLE ) );
				}

				return u32NumOfElement;
			}
		}	
	}
	else
	{
		u32NumOfElement = 1;		// Only 2K impedance supported

		// External trigger impedance
		if ( NULL == pImpedancesTbl )//----- first call return the size
		{
			*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
			return u32NumOfElement;
		}
		else //----- 2nd call return the table
		{
			if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
				return CS_BUFFER_TOO_SMALL;
			else 
			{
				for( uInt32 i = 0; i< CSPDR_IMPED; i++ )
				{
					if( CS_IMP_EXT_TRIG & pCaps->ImpedanceTable[i].u32Reserved )
						memcpy( pImpedancesTbl, &pCaps->ImpedanceTable[i],  sizeof(CSIMPEDANCETABLE ) );			
				}
				return u32NumOfElement;
			}
		}	
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableCouplings(DRVHANDLE DrvHdl, uInt32 , PCSSYSTEMCONFIG ,
								   PCSCOUPLINGTABLE pCouplingTbl,
								   uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32 u32NumOfElement = 0;

	for( uInt32 i = 0; i< CSPDR_COUPLING; i++ )
	{
		if( 0 != pCaps->CouplingTable[i].u32Coupling )
			u32NumOfElement++;
	}

	if ( NULL == pCouplingTbl )//----- first call return the size
	{
		*BufferSize = u32NumOfElement * sizeof( CSCOUPLINGTABLE );
		return u32NumOfElement;
	}
	else //----- 2nd call return the table
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSCOUPLINGTABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			memcpy( pCouplingTbl, pCaps->CouplingTable,  u32NumOfElement * sizeof(CSCOUPLINGTABLE ) );
			return u32NumOfElement;
		}
	}	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableDcOffset (DRVHANDLE DrvHdl)
{
	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	if( pCaps->bDcOffset )
		return CS_SUCCESS;
	else
		return CS_FUNCTION_NOT_SUPPORTED;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableFilters ( DRVHANDLE DrvHdl, uInt32 , PCSSYSTEMCONFIG ,
								 PCSFILTERTABLE pFilterTbl,	
								 uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32	i;
	uInt32	u32NumOfElement = 0;
	bool	bNoFilterFound = false;

	for( i = 0; i< CSPDR_FILTERS; i++ )
	{
		if( CS_NO_FILTER == pCaps->FilterTable[i].u32LowCutoff )
		{
			if( !bNoFilterFound )
				u32NumOfElement++;
			bNoFilterFound = true;
		}
		else
		{
			u32NumOfElement++;
		}
	}

	if ( NULL == pFilterTbl )//----- first call return the size
	{
		*BufferSize = u32NumOfElement * sizeof( CSFILTERTABLE );
		return u32NumOfElement;
	}
	else //----- 2nd call return the table
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSFILTERTABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{

			int j = 0;
			bNoFilterFound = false;
			for( i = 0; i< CSPDR_FILTERS; i++ )
			{
				if( CS_NO_FILTER == pCaps->FilterTable[i].u32LowCutoff )
				{
					if( !bNoFilterFound )
						pFilterTbl[j++] = pCaps->FilterTable[i];

					bNoFilterFound = true;
				}
				else
				{
					pFilterTbl[j++] = pCaps->FilterTable[i];
				}
			}
			
			return j;
		}
	}	
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableAcqModes(DRVHANDLE DrvHdl, uInt32 , PCSSYSTEMCONFIG ,
								   uInt32 *u32AcqModes,
								   uInt32 *BufferSize )
{
	PCSSYSTEMINFOEX	pSysInfoEx; 
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl, &pSysInfoEx );

	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( u32AcqModes == NULL )	//----- first call return the size
	{
		*BufferSize = sizeof(g_u32ValidMode);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if ( *BufferSize < sizeof(g_u32ValidMode) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			uInt32	u32ValidMode;
			uInt16	u16ChannelPerCard = (uInt16) (pSysInfo->u32ChannelCount / pSysInfo->u32BoardCount);	

			if ( 4 == u16ChannelPerCard ) 
				u32ValidMode = g_u32ValidMode & ~CS_MODE_OCT;
			else if ( 2 == u16ChannelPerCard ) 
				u32ValidMode = g_u32ValidMode & ~(CS_MODE_OCT | CS_MODE_QUAD);			
			else if ( 1 == u16ChannelPerCard ) 
				u32ValidMode = g_u32ValidMode & ~(CS_MODE_OCT | CS_MODE_QUAD | CS_MODE_DUAL);	
			else
				u32ValidMode = g_u32ValidMode;
						
			if ( CS_DEVID_SPIDER_LP == pSysInfoEx->u16DeviceId )
				u32ValidMode &= ~( CS_MODE_REFERENCE_CLK | CS_MODE_SW_AVERAGING );

			*u32AcqModes = u32ValidMode;
			return CS_SUCCESS;
		}
	}	
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableTerminations(DRVHANDLE DrvHdl, uInt32 , PCSSYSTEMCONFIG ,
								   uInt32 *u32Terminations,
								   uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( u32Terminations == NULL )	//----- first call return the size
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
	else						//----- 2nd call return the table
	{
		if ( *BufferSize < sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			*u32Terminations = 0;
			PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
			for( int j = 0; j < CSPDR_COUPLING; j++ )
			{
				*u32Terminations |= pCaps->CouplingTable[j].u32Coupling;
			}
			return CS_SUCCESS;
		}
	}	
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetAvailableFlexibleTrigger( DRVHANDLE , uInt32 , uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	*pBuffer = 2;			// Support Flexible trigger only on channels but not on external
	*BufferSize = sizeof(uInt32);
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetBoardTriggerEngines( DRVHANDLE DrvHdl, uInt32 CapsId, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	int32	i32Status = CsDrvGetChannelTriggerEngines( DrvHdl, CapsId, pBuffer, BufferSize);
	if ( CS_FAILED(i32Status) )
		return i32Status;

	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );

	// 2 * u32ChannelCount + 1 external
	*pBuffer = 1 + pSysInfo->u32ChannelCount * (*pBuffer);
	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetChannelTriggerEngines( DRVHANDLE , uInt32 , uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	*pBuffer =  2;									// 2 trigger engines per channel
	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetMaxSegmentPadding( DRVHANDLE , uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL != pBuffer )
	{
		*pBuffer = 	MAX_SPDR_TAIL_INVALID_SAMPLES + MAX_SPDR_TRIGGER_ADDRESS_OFFSET + (BYTESRESERVED_HWMULREC_TAIL/sizeof(uInt32)); // Max Segment Padding
	}

	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetTriggerResolution( DRVHANDLE, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL != pBuffer )
	{
		if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
			return CS_INVALID_POINTER_BUFFER;
		*pBuffer = CSPDR_TRIGGER_RES;
	}

	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetMinExtRate( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(int64);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( BufferSize, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;

	if ( NULL != pBuffer )
	{
		PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
		PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

		uInt32 MaxIndex = sizeof(pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]);
		for ( uInt32 i = 0; i < MaxIndex; i++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				*pBuffer = pCaps->ExtClkTable[i].i64Min;
				bFound = true;
			}
		}
	}

	*BufferSize = sizeof(int64);

	return bFound ? CS_SUCCESS : CS_INVALID_ACQ_MODE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetMaxExtRate( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(int64);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( BufferSize, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;

	if ( NULL != pBuffer )
	{
		PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
		PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

		uInt32 MaxIndex = sizeof(pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]);
		for ( uInt32 i = 0; i < MaxIndex; i ++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				*pBuffer = pCaps->ExtClkTable[i].i64Max;
				bFound = true;
			}
		}
	}

	*BufferSize = sizeof(int64);

	return bFound ? CS_SUCCESS : CS_INVALID_ACQ_MODE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetSkipCount( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG pSysCfg, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;

	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32 MaxIndex = sizeof(pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]);
	for ( uInt32 i = 0; i < MaxIndex; i ++ )
	{
		if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
		{
			*pBuffer = pCaps->ExtClkTable[i].u32SkipCount;
			bFound = true;
		}
	}
	*BufferSize = sizeof(uInt32);

	return bFound ? CS_SUCCESS : CS_INVALID_ACQ_MODE;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAuxConfig(DRVHANDLE, PCS_CAPS_AUX_CONFIG pAuxCfg,  uInt32 *u32BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if ( NULL == pAuxCfg )
		*u32BufferSize = sizeof(CS_CAPS_AUX_CONFIG);
	else if ( sizeof(CS_CAPS_AUX_CONFIG) != *u32BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
	{
		pAuxCfg->u8DlgType	= AUX_IO_SIMPLE_DLG;
		pAuxCfg->bClockOut	= TRUE;
		pAuxCfg->bTrigOut	= TRUE;
		pAuxCfg->bTrigOut_IN = FALSE;
		pAuxCfg->bAuxOUT	= FALSE;
		pAuxCfg->bAuxIN		= FALSE;
	}

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAvailableClockOut( DRVHANDLE, PCSSYSTEMCONFIG, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		i32Status = CS_INVALID_PARAMETER;
	else if (NULL == pBuffer)
		*BufferSize = sizeof(ValidClkOut);
	else if ( sizeof(ValidClkOut) != *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
		memcpy(pBuffer, ValidClkOut, sizeof(ValidClkOut));

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAvailableTrigOut( DRVHANDLE, PCSSYSTEMCONFIG, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		i32Status = CS_INVALID_PARAMETER;
	else if (NULL == pBuffer)
		*BufferSize = sizeof(ValidTrigOut);
	else if ( sizeof(ValidTrigOut) != *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
		memcpy(pBuffer, ValidTrigOut, sizeof(ValidTrigOut));

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 ValidateSampleRate ( DRVHANDLE DrvHdl, PCSSYSTEMINFO , PCSSYSTEMCONFIG pSysCfg )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	int64				i64CoerceSampleRate = pAcqCfg->i64SampleRate;
	PCS8XXXBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	
	if ( pAcqCfg->u32ExtClk )
	{
		// Validation of Exnternal clock frequency
		for ( uInt32 i = 0; i < CSPDR_MODE_COUNT; i ++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				if ( pAcqCfg->i64SampleRate > pCaps->ExtClkTable[i].i64Max )
					i64CoerceSampleRate = pCaps->ExtClkTable[i].i64Max;
				else if ( pAcqCfg->i64SampleRate < pCaps->ExtClkTable[i].i64Min )
					i64CoerceSampleRate = pCaps->ExtClkTable[i].i64Min;
				break;
			}
		}
	}
	else
	{
		i64CoerceSampleRate = g_pBaseHwDll->CoerceSampleRate( pAcqCfg->i64SampleRate, 
			                                                  pAcqCfg->u32Mode,
			                                                  0, 
															  pCaps->SrTable, 
															  CSPDR_MAX_SR_COUNT * sizeof( CSSAMPLERATETABLE_EX ) );
	}

	return  i64CoerceSampleRate;
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL	ValidateChannelIndex( PCSSYSTEMINFO pSysInfo, PCSACQUISITIONCONFIG , uInt32 u16ChannelIndex )
{
	if ( 0 == u16ChannelIndex || u16ChannelIndex > pSysInfo->u32ChannelCount )
		return FALSE;

	return TRUE;

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	_CsDrvGetHwOptionText( uInt32 u32BoardType, PCS_HWOPTIONS_CONVERT2TEXT pHwOptionText )
{
	UNREFERENCED_PARAMETER(u32BoardType);

	if ( IsBadReadPtr( pHwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pHwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	if ( pHwOptionText->bBaseBoard )
	{
		// Mask for options that require special BB firmware
		pHwOptionText->u32RevMask = CSPDR_BBHW_OPT_FWREQUIRED_MASK;

		// There is no BB HW options for now
		strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
		return CS_SUCCESS;
	}

	char szAddonOptions[][HWOPTIONS_TEXT_LENGTH] = 
								{	"Alt. Calibration",			// 0
									"Double Calibration Amp",	// 1
									"Aux Connector",			// 2
									"M/S Support",				// 3
									"??",						// 4
									"??",						// 5
									"??",						// 6
									"??",						// 7
									"??",						// 8
									"??",						// 9
									"??",						// A
									"??",						// B
									"??",						// C
									"??",						// D
									"??",						// E
									"??",						// F
									"??",						//10
									"??",						//11
									"??",						//12
									"??",						//13
									"??",						//14
									"??",						//15
									"??",						//16
									"??",						//17
									"??",						//18
									"??",						//19
									"??",						//1A
									"??",						//1B
									"??",						//1C
									"??",						//1D
									"??",						//1E
									"Emona FF"					//1F

								};

	// Mask for options that require special AO firmware
	pHwOptionText->u32RevMask = CSPDR_AOHW_OPT_FWREQUIRED_MASK;

	// Options text from base board
	uInt32	nSize = sizeof( szAddonOptions ) / HWOPTIONS_TEXT_LENGTH;
	for (uInt32 i = 0; i < nSize; i++)
	{
		if ( i == pHwOptionText->u8BitPosition )
		{
			strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), szAddonOptions[i] );
			return CS_SUCCESS;
		}
	}

	strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	_CsDrvGetFwOptionText( PCS_FWOPTIONS_CONVERT2TEXT pFwOptionText)
{
	if ( IsBadReadPtr( pFwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pFwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	if ( pFwOptionText->bBaseBoard )
	{
		if ( (1<<pFwOptionText->u8BitPosition) & g_u32ValidExpertFw )
		{
			// Public expert options
			return g_pBaseHwDll->CsDrvGetFwOptionText( pFwOptionText );
		}
		else
		{
			// Mask for Expert options
			pFwOptionText->u32RevMask = CS_BBOPTIONS_EXPERT_MASK;
			
			// No BB private options defined for now
			strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
		}
	}
	else
	{
		// Add on Fw options ...
		// Mask for Expert options
		pFwOptionText->u32RevMask = 0;
		strcpy_s(pFwOptionText->szOptionText, sizeof(pFwOptionText->szOptionText), "??" );
	}
	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	_CsDrvGetBoardType( uInt32 *u32MinBoardType, uInt32 *u32MaxBoardType )
{
	if ( IsBadReadPtr( u32MaxBoardType,  sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( IsBadReadPtr( u32MinBoardType,  sizeof(uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	*u32MinBoardType = CS8_BT_FIRST_BOARD;
	*u32MaxBoardType = CS8_BT_LAST_BOARD;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetBoardCaps( uInt32 u32DeviceId, uInt32 u32Param, void *pBuffer, uInt32 *u32BufferSize)
{
	switch( u32Param )
	{
	case CS_FLASHINFO:
		{
			if ( *u32BufferSize <  sizeof(CS_FLASHLAYOUT_INFO) )
				return CS_BUFFER_TOO_SMALL;

			PCS_FLASHLAYOUT_INFO	pCsFlashLayoutInfo = (PCS_FLASHLAYOUT_INFO) pBuffer;

			if ( IsBadWritePtr( pCsFlashLayoutInfo, sizeof(CS_FLASHLAYOUT_INFO) ) )
				return CS_INVALID_POINTER_BUFFER;

			if ( pCsFlashLayoutInfo->u32Addon )
			{
				pCsFlashLayoutInfo->u32FlashType			= 0;
				pCsFlashLayoutInfo->u32NumOfImages			= 3;
				pCsFlashLayoutInfo->u32ImageStructSize		= FIELD_OFFSET( AddOnFlashData, Image2 );
				pCsFlashLayoutInfo->u32WritableStartOffset	= 0;
				pCsFlashLayoutInfo->u32CalibInfoOffset		= FIELD_OFFSET( AddOnFlashData, Calib1 );
				pCsFlashLayoutInfo->u32FwInfoOffset			= (uInt32) -1;
				pCsFlashLayoutInfo->u32FwInfoOffset			= (uInt32) -1;
				pCsFlashLayoutInfo->u32FwBinarySize			= 577*1024;
				return CS_SUCCESS;
			}
			else if ( CS_DEVID_SPIDER_PCIE == u32DeviceId || CS_DEVID_ZAP_PCIE == u32DeviceId )
			{
				pCsFlashLayoutInfo->u32FlashType			= 1;
				pCsFlashLayoutInfo->u32NumOfImages			= NUCLEON_NUMBER_OF_FLASHIMAGES-1;
				pCsFlashLayoutInfo->u32ImageStructSize		= FIELD_OFFSET( NUCLEON_FLASH_LAYOUT, FlashImage1 );
				pCsFlashLayoutInfo->u32WritableStartOffset	= FIELD_OFFSET( NUCLEON_FLASH_LAYOUT, FlashImage1 );
				pCsFlashLayoutInfo->u32CalibInfoOffset		= FIELD_OFFSET( NUCLEON_FLASH_IMAGE_STRUCT, Calib );
				pCsFlashLayoutInfo->u32FwInfoOffset			= FIELD_OFFSET( NUCLEON_FLASH_IMAGE_STRUCT, FwInfo );
				pCsFlashLayoutInfo->u32FlashFooterOffset	= FIELD_OFFSET( NUCLEON_FLASH_LAYOUT, BoardData);
				pCsFlashLayoutInfo->u32FwBinarySize			= NUCLEON_FLASH_IMAGETRUCT_SIZE;
				return CS_SUCCESS;
			}
			else		// Combine base 
				return g_pBaseHwDll->CsDrvGetFlashLayoutInfo(pCsFlashLayoutInfo);
		}
		break;

	default:
		return CS_INVALID_REQUEST;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL _CsDrvIsMyBoardType( uInt32 u32BoardType)
{
	if( u32BoardType >= CS8_BT_FIRST_BOARD && u32BoardType <= CS8_BT_LAST_BOARD )
		return TRUE;
	else if( u32BoardType >= (CSNUCLEONBASE_BOARDTYPE | CS8_BT_FIRST_BOARD) && u32BoardType <= (CSNUCLEONBASE_BOARDTYPE | CS8_BT_LAST_BOARD) )
		return TRUE;
	else
		return FALSE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#ifdef _WINDOWS_
INT_PTR CALLBACK CfgAuxIoDlg( HWND hwndDlg,  UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// Assume that CompuscopeManager does open this Dialog for only one system at time then
	// it is safe using static variable to here
	static	CSHANDLE DrvHdl = 0;
	switch (uMsg)
	{
		case WM_INITDIALOG: 
		{
			DrvHdl = (CSHANDLE) lParam;

			CS_TRIG_OUT_STRUCT	TrigOut = {0};
			CS_CLOCK_OUT_STRUCT	ClockOut = {0};

			CsDrvGetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
			CsDrvGetParams(DrvHdl, CS_CLOCK_OUT, &ClockOut);

			int i;

			for(i = 0; i< sizeof(ValidTrigOut)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_TRIGOUT, CB_ADDSTRING, 0, (LPARAM)ValidTrigOut[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_TRIGOUT, CB_SETITEMDATA, i, (LPARAM)ValidTrigOut[i].u32Ctrl);

				if (TrigOut.u16Value == ValidTrigOut[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidTrigOut[i].Str);
			}

			for(i = 0; i< sizeof(ValidClkOut)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_CLOCKOUT, CB_ADDSTRING, 0, (LPARAM)ValidClkOut[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_CLOCKOUT, CB_SETITEMDATA, i, (LPARAM)ValidClkOut[i].u32Ctrl);

				if (ClockOut.u16Value == ValidClkOut[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_CLOCKOUT, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidClkOut[i].Str);
			}

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					CS_TRIG_OUT_STRUCT	TrigOut = {0};
					CS_CLOCK_OUT_STRUCT	ClockOut = {0};
					int32 nIndex;

					TrigOut.u16Mode = CS_TRIGOUT_MODE;
					nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_GETCURSEL, 0, 0);
					if( CB_ERR != nIndex )
					{
						TrigOut.u16Value =  (int16)(SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_GETITEMDATA, (WPARAM)nIndex, 0));
						if( CB_ERR != TrigOut.u16Value )
							CsDrvSetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
					}

					nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_CLOCKOUT, CB_GETCURSEL, 0, 0);
					if( CB_ERR != nIndex )
					{
						ClockOut.u16Value =  (int16)(SendDlgItemMessage (hwndDlg, IDC_CLOCKOUT, CB_GETITEMDATA, (WPARAM)nIndex, 0));
						if( CB_ERR != ClockOut.u16Value )
							CsDrvSetParams(DrvHdl, CS_CLOCK_OUT, &ClockOut);
					}

					if ( SendDlgItemMessage (hwndDlg, IDC_SAVE_CONFIG, BM_GETCHECK, 0, 0) )
					{
						uInt32	u32Save = 1;		// 1 = Save, 0 = Clear;
						CsDrvSetParams(DrvHdl, CS_SAVE_OUT_CFG, &u32Save);
					}

					::EndDialog(hwndDlg, IDOK);
				}
				break;

				case IDCANCEL: ::EndDialog(hwndDlg, IDCANCEL);
					           break;
			}
			break;
		}
		case WM_CLOSE: ::EndDialog(hwndDlg, IDCANCEL);
			           break;
	}
	return FALSE;
}
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvStmAllocateBuffer( DRVHANDLE DrvHdl, int32 nCardIndex, uInt32 u32BufferSize, PVOID *pVa )
{
	CsSpiderDev	*pDev = (CsSpiderDev *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

	if ( NULL == pDev )
		return CS_INVALID_HANDLE;

	// If it is a M/S system, search for Card object based on the CardIndex
	if ( 0 != pDev->m_Next )
	{
		pDev = pDev->GetCardPointerFromBoardIndex( (uInt16) nCardIndex );
		if ( NULL == pDev )
			return CS_INVALID_CARD;
	}

	CsDevIoCtl		*pDevIoCtl = dynamic_cast<CsDevIoCtl*>(pDev);
	return pDevIoCtl->IoctlStmAllocateDmaBuffer( u32BufferSize, pVa );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvStmFreeBuffer( DRVHANDLE DrvHdl, int32 nCardIndex, PVOID pVa )
{
	CsSpiderDev	*pDev = (CsSpiderDev *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

	if ( NULL == pDev )
		return CS_INVALID_HANDLE;

	// If it is a M/S system, search for Card object based on the CardIndex
	if ( 0 != pDev->m_Next )
	{
		pDev = pDev->GetCardPointerFromBoardIndex( (uInt16) nCardIndex );
		if ( NULL == pDev )
			return CS_INVALID_CARD;
	}

	CsDevIoCtl		*pDevIoCtl = dynamic_cast<CsDevIoCtl*>(pDev);
	return pDevIoCtl->IoctlStmFreeDmaBuffer( pVa );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvStmTransferToBuffer( DRVHANDLE DrvHdl, int32 nCardIndex, PVOID pVa, uInt32 u32TransferSize )
{
	CsSpiderDev	*pDev = (CsSpiderDev *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

	if ( NULL == pDev )
		return CS_INVALID_HANDLE;

	if ( 0 == u32TransferSize ||
		 0 != (u32TransferSize % 2) )			// u32TransferSize should be DWORD boundary
		return CS_STM_INVALID_TRANSFER_SIZE;

	// If it is a M/S system, search for Card object based on the CardIndex
	if ( 0 != pDev->m_Next )
	{
		pDev = pDev->GetCardPointerFromBoardIndex( (uInt16) nCardIndex );
		if ( NULL == pDev )
			return CS_INVALID_CARD;
	}

	return pDev->CsStmTransferToBuffer( pVa, u32TransferSize );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvStmGetTransferStatus( DRVHANDLE DrvHdl, int32 nCardIndex, uInt32 u32TimeoutMs, uInt32 *pu32ErrorFlag, uInt32 *pu32ActualSize, uInt8 *pu8EndOfData )
{
	CsSpiderDev	*pDev = (CsSpiderDev *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

	if ( NULL == pDev )
		return CS_INVALID_HANDLE;

	// If it is a M/S system, search for Card object based on the CardIndex
	if ( 0 != pDev->m_Next )
	{
		pDev = pDev->GetCardPointerFromBoardIndex( (uInt16) nCardIndex );
		if ( NULL == pDev )
			return CS_INVALID_CARD;
	}

	return pDev->CsStmGetTransferStatus( u32TimeoutMs, pu32ErrorFlag, pu32ActualSize, pu8EndOfData );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvExpertCall( DRVHANDLE DrvHdl, void *FuncParams )
{
	UNREFERENCED_PARAMETER(DrvHdl);
	UNREFERENCED_PARAMETER(FuncParams);
	return CS_FUNCTION_NOT_SUPPORTED;
}
