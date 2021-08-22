
// CsDeere.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CsAdvHwDLL.h"
#include "CurrentDriver.h"
#include "CsDeereFlash.h"

#ifdef _WINDOWS_

#include "resource.h"
	// Global variables
	CTraceManager*		g_pTraceMngr= NULL;
#else
#define		min(a,b)	MIN(a,b)
#endif

// Global inter process varabales
extern	SHAREDDLLMEM		Globals;


CsBaseHwDLL*		g_pBaseHwDll = NULL;
CsAdvHwDLL*			g_pAdvanceHwDll = NULL;
GLOBAL_PERPROCESS	g_GlobalPerProcess = {0};

#if !defined(USE_SHARE_BOARDCAPS)
CSDEEREBOARDCAPS	g_BoardCaps;
#endif

// Prototypes functions
int64  ValidateSampleRate (DRVHANDLE DrvHdl,  PCSSYSTEMINFO pSysInfo, PCSSYSTEMCONFIG pSysCfg );
BOOL   ValidateChannelIndex( PCSSYSTEMINFO pSysInfo, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u16ChannelIndex );
int32  JohnDeereCommit(DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSysCfg );
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
int32  CsDrvGetAvailableExtTrigEnable( DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSystemConfig, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize );
int32  CsDrvGetAvailableAuxOut( DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSystemConfig, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize );
int32  CsDrvGetAvailableAuxInTimeStamp( DRVHANDLE DrvHdl, PCSSYSTEMCONFIG pSystemConfig, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize );
int32  CsDrvGetMaxSegmentPadding( DRVHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetTriggerResolution( DRVHANDLE , uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetMinExtRate( DRVHANDLE , uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetMaxExtRate( DRVHANDLE , uInt32, PCSSYSTEMCONFIG pSysCfg, int64 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetSkipCount( DRVHANDLE, uInt32, PCSSYSTEMCONFIG, uInt32 *pBuffer,  uInt32 *BufferSize );

PCSSYSTEMINFO			CsDllGetSystemInfoPointer( DRVHANDLE DrvHdl, PCSSYSTEMINFOEX *pSysInfoEx = NULL );			
PCSACQUISITIONCONFIG	CsDllGetAcqSystemConfigPointer( DRVHANDLE DrvHdl );
PCSDEEREBOARDCAPS		CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl );


const VALID_AUX_CONFIG	ValidTrigOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Enable",			CS_TRIGOUT
};

const VALID_AUX_CONFIG	ValidClkOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Reference Clock",	CS_CLKOUT_REF,
};

const VALID_AUX_CONFIG	ValidAuxOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Busy Out",			CS_PULSEOUT_DATAVALID,
};

const VALID_AUX_CONFIG	ValidExtTrigEn[] =
{
	"Always Enable",			0,
	"Gate (Live Pos. Level)",	CS_EXTTRIGEN_LIVE,
	"Pos.Edge Latch",			CS_EXTTRIGEN_POS_LATCH,
	"Neg.Edge Latch",			CS_EXTTRIGEN_NEG_LATCH,
	"Pos.Edge Latch Once",		CS_EXTTRIGEN_POS_LATCH_ONCE,
	"Neg.Edge Latch Once",		CS_EXTTRIGEN_NEG_LATCH_ONCE,
};

const VALID_AUX_CONFIG	ValidTsReset[] =
{
	"Disabled",					0,
	"Enabled (Live Pos. Level)",	CS_EXTTRIGEN_LIVE,
//	"Pos.Edge Latch",			CS_EXTTRIGEN_POS_LATCH,
//	"Neg.Edge Latch",			CS_EXTTRIGEN_NEG_LATCH,
//	"Pos.Edge Latch Once",		CS_EXTTRIGEN_POS_LATCH_ONCE,
//	"Neg.Edge Latch Once",		CS_EXTTRIGEN_NEG_LATCH_ONCE,
};


const uInt32	g_u32ValidMode = CS_MODE_SINGLE | CS_MODE_DUAL | CS_MODE_SW_AVERAGING | CS_MODE_REFERENCE_CLK | CS_MODE_USER1 | CS_MODE_USER2;
const uInt32	g_u32ValidExpertFw = CS_BBOPTIONS_MULREC_AVG_TD;

int CompareInputRange( const void* pIr1, const void*pIr2 )
{
	return int(PCSRANGETABLE(pIr2)->u32InputRange) - int(PCSRANGETABLE(pIr1)->u32InputRange);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
typedef SharedMem< CSDEEREBOARDCAPS > SHAREDCAPS;
int32 CsDrvAcquisitionSystemInit( DRVHANDLE	DrvHdl, BOOL bSystemDefaultSetting )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;
	int32		i32Status = g_pBaseHwDll->CsDrvAcquisitionSystemInit( DrvHdl, bSystemDefaultSetting );

	if (CS_SUCCESS != i32Status)
		return i32Status;

	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle == DrvHdl )
		{
			// Save a copy of current AcquisitonConfig
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_ACQUISITION, &pDrvSystem[i].AcqConfig );

			
#ifdef USE_SHARE_BOARDCAPS
			TCHAR strSharedCapsName[_MAX_PATH];
			_stprintf_s(strSharedCapsName, _countof(strSharedCapsName), SHARED_CAPS_NAME _T("-%d"), i);

			SHAREDCAPS* pCaps = new SHAREDCAPS( strSharedCapsName, true );
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, pCaps->it() );
			qsort(pCaps->it()->RangeTable, DEERE_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
			pDrvSystem[i].pCapsInfo = pCaps;
#else
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, &g_BoardCaps );
			qsort(&g_BoardCaps.RangeTable, DEERE_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
#endif
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

#ifdef USE_SHARE_BOARDCAPS
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;
	for (uInt32 i = 0; i< Globals.it()->g_StaticLookupTable.u16SystemCount; i ++ )
	{
		if ( pDrvSystem[i].DrvHandle == DrvHdl && pDrvSystem[i].pCapsInfo != NULL )
		{
			delete ((SHAREDCAPS *) pDrvSystem[i].pCapsInfo);
		}
	}
#endif

	return g_pBaseHwDll->CsDrvAcquisitionSystemCleanup( DrvHdl );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAcquisitionSystemCaps(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSysCfg,
									 void *pBuffer, uInt32 *BufferSize)
{	
	uInt32  MainCapId = CapsId & CAPS_ID_MASK;
	uInt16  SubCapId = (uInt16) (CapsId & SUBCAPS_ID_MASK);

	PCSSYSTEMINFOEX	pSysInfoEx; 
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

	case CAPS_FILTERS:
		return CsDrvGetAvailableFilters( DrvHdl, SubCapId, pSysCfg, (PCSFILTERTABLE) pBuffer, BufferSize );

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
		return CsDrvGetAvailableTriggerSources( DrvHdl, SubCapId, (int32 *) pBuffer, BufferSize );

	case CAPS_MAX_SEGMENT_PADDING:
		return CsDrvGetMaxSegmentPadding( DrvHdl, (uInt32 *) pBuffer, BufferSize );

	case CAPS_DC_OFFSET_ADJUST:
			return CS_SUCCESS;
		break;

	case CAPS_TRANSFER_EX:
	case CAPS_MULREC:
			return CS_SUCCESS;

	case CAPS_AUX_CONFIG:
		return CsDrvGetAuxConfig( DrvHdl, (PCS_CAPS_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_CLOCK_OUT:
		return CsDrvGetAvailableClockOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_TRIG_OUT:
		return CsDrvGetAvailableTrigOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_TRIG_ENABLE:
		return CsDrvGetAvailableExtTrigEnable( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_AUX_OUT:
		return CsDrvGetAvailableAuxOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_AUX_IN_TIMESTAMP:
		return CsDrvGetAvailableAuxInTimeStamp( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_CLK_IN:
		{
		PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
		for ( uInt32 i = 0; i < sizeof(pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]); i ++ )
			if ( 0 != pCaps->ExtClkTable[i].u32Mode )
				return CS_SUCCESS;
		return CS_NO_EXT_CLK;
		}

	case CAPS_TRIGGER_RES:
		return CsDrvGetTriggerResolution(DrvHdl, (uInt32 *) pBuffer, BufferSize );

	case CAPS_MIN_EXT_RATE:
		return CsDrvGetMinExtRate(DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );
		
	case CAPS_MAX_EXT_RATE:
		return CsDrvGetMaxExtRate(DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );

	case CAPS_SKIP_COUNT:
		return CsDrvGetSkipCount( DrvHdl, SubCapId, pSysCfg, (uInt32 *) pBuffer, BufferSize );

	default:
		return CS_INVALID_REQUEST;
	}
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
		case CS_MEM_READ_THRU:
		case CS_TRIGADDR_OFFSET_ADJUST:
		case CS_TRIGLEVEL_ADJUST:
		case CS_TRIGGAIN_ADJUST:
		case CS_FIR_CONFIG:
		case CS_CLOCK_OUT:
		case CS_EXTENDED_BOARD_OPTIONS:
		case CS_SYSTEMINFO_EX:
		case CS_READ_DAC:
		case CS_ADC_ALIGN:
		case CS_TRIG_OUT:
		case CS_READ_CALIB_A2D:
		case CS_SAVE_OUT_CFG:
		case CS_MULREC_AVG_COUNT:
		case CS_AUX_IN:
		case CS_AUX_OUT:
		{
			int32 i32Status;
	
			// Validate Buffer for Write Access
			i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( ParamID, ParamBuffer, FALSE );
			if (i32Status != CS_SUCCESS)
				return i32Status;
		}
		break;

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

		case CS_READ_FLASH:
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

		case CS_TIMESTAMP_TICKFREQUENCY:
		{
			// check buffer for Write access
			int64 *pTickFrequency = (int64 *) ParamBuffer;

			if ( IsBadWritePtr( pTickFrequency, sizeof(int64) ) )
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

		case CS_MAX_SEGMENT_PADDING:
		case CS_USE_DACCALIBTABLE:
		case CS_DEFAULTUSE_DACCALIBTABLE:
		case CS_CHECK_ADC_DATA_ALIGN:
			if ( IsBadWritePtr( ParamBuffer, sizeof(uInt32) ) )
				return CS_INVALID_POINTER_BUFFER;
			break;

		case CS_READ_CALIBTABLE_FROM_EEPROM:
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
		case CS_SEND_DAC:
		case CS_SELF_TEST_MODE:
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
		case CS_SEND_CALIB_MODE:
		case CS_SEND_CALIB_DC:
		case CS_TRIG_OUT:
		case CS_SAVE_OUT_CFG:
		case CS_MULREC_AVG_COUNT:
		case CS_AUX_IN:
		case CS_AUX_OUT:
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

		case CS_WRITE_FLASH:
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

		case CS_DEBUG_MS_PULSE:
		case CS_DEBUG_MS_OFFSET:
		case CS_DEBUG_EXTTRIGCALIB:
			break;

		case CS_SAVE_CALIBTABLE_TO_EEPROM:
		case CS_DEFAULTUSE_DACCALIBTABLE:
		case CS_USE_DACCALIBTABLE:
		case CS_RESET_ADC:
		case CS_CHECK_ADC_DATA_ALIGN:
			{
				uInt32	*u32Valid = (uInt32 *) ParamBuffer;

				if ( IsBadReadPtr( u32Valid, sizeof(uInt32) ) )
					return CS_INVALID_POINTER_BUFFER;
			}
			break;

		case CS_SEND_NIOS_CMD:
			{
				if ( IsBadWritePtr( ParamBuffer, sizeof(CS_SEND_NIOS_CMD_STRUCT) ) )
					return CS_INVALID_POINTER_BUFFER;
			}
			break;

		default:
			return CS_INVALID_PARAMS_ID;

	}


	return g_pBaseHwDll->CsDrvSetParams( DrvHdl, ParamID, ParamBuffer );
}

#if 0
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 CsDrvDo(DRVHANDLE DrvHdl, uInt32 ActionID, void *ActionBuffer )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );
	
	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	switch( ActionID )
	{
	case ACTION_START:
	case ACTION_FORCE:
	case ACTION_ABORT:
	case ACTION_CALIB:
	case ACTION_RESET:
	case ACTION_TIMESTAMP_RESET:
		return g_pBaseHwDll->CsDrvDo( DrvHdl, ActionID, ActionBuffer );

	case ACTION_COMMIT:
	{
		int32					i32Status = CS_SUCCESS;
		PCSSYSTEMCONFIG			pSysCfg = (PCSSYSTEMCONFIG) ActionBuffer;
		PCSACQUISITIONCONFIG	pAcqCfg = CsDllGetAcqSystemConfigPointer( DrvHdl );

		i32Status =  JohnDeereCommit( DrvHdl,  pSysCfg );
		if ( CS_SUCCEEDED(i32Status) )
		{
			// Keep a latest copy of Valid AcquisitionConfig.
			int32 i32St = g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_ACQUISITION, pAcqCfg );
			if ( CS_FAILED(i32St) )	i32Status = i32St;
		}
		return i32Status;
	}

	default:
		return CS_INVALID_REQUEST;
	}
}

#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

PCSDEEREBOARDCAPS CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl)
{
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Search for SystemInfo of the current system based on DrvHdl
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
#ifdef USE_SHARE_BOARDCAPS
			return ((SHAREDCAPS*)(pDrvSystem[i].pCapsInfo))->it();
#else
			return &g_BoardCaps;
#endif
		}
	}
	_ASSERT(0);
	return NULL;
}


#define DEERE_MIN_SEG_SIZE					16		// Should be multiple of 8

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
	int64	i64MaxMemPerChan = pSysInfo->i64MaxMemory;
	uInt32	u32AcqModeValid = g_u32ValidMode;
	uInt32	u32Size = sizeof( u32AcqModeValid );
	uInt32	u32ModeMaskChannel = CS_MODE_DUAL | CS_MODE_SINGLE;
	PCSSYSTEMINFOEX	pSysInfoEx; 
	
	
	CsDllGetSystemInfoPointer( DrvHdl, &pSysInfoEx );

	if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
		i64MaxMemPerChan /= 2;
	
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
		 (( pAcqCfg->u32Mode & u32ModeMaskChannel ) == (CS_MODE_DUAL | CS_MODE_SINGLE)  ) ||
		 (( pAcqCfg->u32Mode & CS_MODE_USER1 ) && (pAcqCfg->u32Mode & CS_MODE_USER2) )  )
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
	int64	i64MaxHwDepthCounter = CSMV_LIMIT_HWDEPTH_COUNTER;

	i64MaxHwDepthCounter *= (128/(16*(pAcqCfg->u32Mode&CS_MASKED_MODE)));

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

	uInt32	u32DepthRes = 0;

	if ( pAcqCfg->u32Mode & CS_MODE_DUAL )
		u32DepthRes = 8;
	else 
		u32DepthRes = 16;

	// Validation of Segment Size
	if ( pAcqCfg->i64SegmentSize % u32DepthRes )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64SegmentSize -= (pAcqCfg->i64SegmentSize % u32DepthRes);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_SIZE;
	}

	if ( pAcqCfg->i64SegmentSize < DEERE_MIN_SEG_SIZE ) 
	{
		if (u32Coerce)
		{
			if ( pAcqCfg->i64SegmentSize < DEERE_MIN_SEG_SIZE) 
				pAcqCfg->i64SegmentSize = DEERE_MIN_SEG_SIZE;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_SEGMENT_SIZE;
	}

	// Validate triggger depth
	if ( pAcqCfg->i64Depth % u32DepthRes )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64Depth -= (pAcqCfg->i64Depth % u32DepthRes);
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIG_DEPTH;
	}

	if ( (pAcqCfg->i64Depth < DEERE_MIN_SEG_SIZE)  ||
			(pAcqCfg->i64Depth > pAcqCfg->i64SegmentSize) )
	{
		if (u32Coerce)
		{
			if ( pAcqCfg->i64Depth < DEERE_MIN_SEG_SIZE) 
				pAcqCfg->i64Depth = DEERE_MIN_SEG_SIZE;
			else
				pAcqCfg->i64Depth = pAcqCfg->i64SegmentSize;

			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIG_DEPTH;
	}

	// Validation of Trigger Delay
	if  ( ( pAcqCfg->i64TriggerDelay < 0) ||
		  (( pAcqCfg->i64Depth + pAcqCfg->i64TriggerDelay ) > i64MaxHwDepthCounter ) )
	{
		if (u32Coerce)
		{
			pAcqCfg->i64TriggerDelay = 0;
			i32Status = CS_CONFIG_CHANGED;
		}
		else
			return CS_INVALID_TRIGDELAY;
	}

#ifdef _WINDOWS_
	// While Nios Interface does not support 64 bit address,
	_ASSERT( pAcqCfg->i64SegmentSize  < 0x100000000 );
	_ASSERT( pAcqCfg->i64Depth < 0x100000000 );
#endif

	//------ Trigger 
	if ( ( pAcqCfg->u32TrigEnginesEn != CS_TRIG_ENGINES_DIS) && ( pAcqCfg->u32TrigEnginesEn != CS_TRIG_ENGINES_EN) )
		return CS_MISC_ERROR;

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
		PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
		for( j = 0; j < DEERE_COUPLING; j++ )
		{
			if ( pChanCfg->u32Term == pCaps->CouplingTable[j].u32Coupling )
				break;
		}
		if ( DEERE_COUPLING == j )
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
		for( j = 0; j < DEERE_IMPED; j++ )
		{
			if ( pChanCfg->u32Impedance == pCaps->ImpedanceTable[j].u32Imdepdance )
			{
				if( CS_REAL_IMP_50_OHM == pChanCfg->u32Impedance )
					ImpedanceMask = CS_IMP_50_OHM;
                break;
			}
		}
		if ( DEERE_IMPED == j )
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

		for ( i = 0; i < DEERE_MAX_RANGES; i ++ )
		{
			if ( pCaps->RangeTable[i].u32Reserved & ImpedanceMask )
			{
				if ( pCaps->RangeTable[i].u32InputRange == pChanCfg->u32InputRange )
					break;
			}
		}

		if ( i >= DEERE_MAX_RANGES )
		{
			if ( Coerce )
			{
				pChanCfg->u32InputRange = CS_GAIN_2_V;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_GAIN;
		}

		//----- Position

		int32	i32MaxDcOffset = DEERE_USER_OFFSET_LIMIT;
		if(pChanCfg->u32InputRange >= CS_GAIN_2_V)
			i32MaxDcOffset *= 10;

		if( pChanCfg->i32DcOffset > i32MaxDcOffset )
		{
			if ( Coerce )
			{
				pChanCfg->i32DcOffset = i32MaxDcOffset;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_INVALID_POSITION;
			}
		}
		else if( pChanCfg->i32DcOffset < -i32MaxDcOffset  )
		{
			if ( Coerce )
			{
				pChanCfg->i32DcOffset = -i32MaxDcOffset;
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
				pTrigCfg->u32Condition != CS_TRIG_COND_NEG_SLOPE )
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

		//----- Validation for Ext Trigger
		if ( pTrigCfg->i32Source < CS_TRIG_SOURCE_EXT )
		{
			if ( Coerce )
			{
				pTrigCfg->i32Source = 0;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_EXT_TRIG;
		}
		else  if ( pTrigCfg->i32Source == CS_TRIG_SOURCE_EXT )
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
//
//-----------------------------------------------------------------------------
int32 ValidateTriggerSources ( PCSSYSTEMCONFIG pSysCfg, PCSSYSTEMINFO pSysInfo, uInt32 Coerce )
{
	PCSTRIGGERCONFIG	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	int32				i32Status = CS_SUCCESS;
	BOOL			    bAtLeastOneTriggerSourceEnabled = FALSE;
	BOOL				bErrorTriggerSource = FALSE;
	BOOL				bSingleModeOnly = (1 == pSysInfo->u32ChannelCount/pSysInfo->u32BoardCount);


#ifdef FLEXIBLE_TRIGGER_ONLY
	uInt32	CardIndex1 = 0;
	uInt32	CardIndex2 = 0;
	uInt32	TrigEnabledIndex1 = 0;
	uInt32	TrigEnabledIndex2 = 0;
#endif

	// Looking for at least on trigger source enabled valid
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++)
	{
		if ( pTrigCfg[i].i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( pTrigCfg[i].i32Source > 0 &&  ( pTrigCfg[i].i32Source <= (int32) pSysInfo->u32ChannelCount ) )
			{
				if ( 0 != (pAcqCfg->u32Mode & CS_MODE_SINGLE) && ! bSingleModeOnly )
				{
					if ( pTrigCfg[i].i32Source % 2 )
						 bAtLeastOneTriggerSourceEnabled = TRUE;
				}
				else
					 bAtLeastOneTriggerSourceEnabled = TRUE;
			}
			else if  ( pTrigCfg[i].i32Source >= (int32) (-1*pSysInfo->u32BoardCount) )
				 bAtLeastOneTriggerSourceEnabled = TRUE;
		}


		if ( bAtLeastOneTriggerSourceEnabled )
			break;
	}

	// Validation of Trigger Source
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++)
	{
		if ( pTrigCfg[i].i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( ( pTrigCfg[i].i32Source > (int32) pSysInfo->u32ChannelCount ) ||
	 			 ( pTrigCfg[i].i32Source < (int32) (-1*pSysInfo->u32BoardCount) ) )
				bErrorTriggerSource = TRUE;
			else if ( pTrigCfg[i].i32Source > 0 &&  ( pTrigCfg[i].i32Source <= (int32) pSysInfo->u32ChannelCount ) )
			{
				if ( 0 != (pAcqCfg->u32Mode & CS_MODE_SINGLE) && ! bSingleModeOnly )
				{
					// Trigger source should be an odd number in Single mode
					if ( (pTrigCfg[i].i32Source % 2)  == 0 )
						bErrorTriggerSource = TRUE;
				}
			}

			if ( bErrorTriggerSource )
			{
				if ( Coerce )
				{
					if ( bAtLeastOneTriggerSourceEnabled )
						pTrigCfg[i].i32Source = CS_TRIG_SOURCE_DISABLE;
					else
					{
						pTrigCfg[i].i32Source = 1;
						bAtLeastOneTriggerSourceEnabled = TRUE;
					}
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_TRIG_SOURCE;
			}
		}
	}

#ifdef FLEXIBLE_TRIGGER_ONLY
	// VALIDATION FOR FLEXIBLE TRIGGER
	// Get CardIndex from the ftrigger source
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	uInt8 u8ChannelPerCard = (uInt8) (pSysInfo->u32ChannelCount/pSysInfo->u32BoardCount);


	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			TrigEnabledIndex1 = i;
			if ( pTrigCfg->i32Source > 0 )
				CardIndex1 = 1 + (pTrigCfg->i32Source - 1) / u8ChannelPerCard;
			else
				CardIndex1 = abs(pTrigCfg->i32Source);

			break;
		}
	}

	// Make sure 2 enabled trigger engines come from the same card
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			TrigEnabledIndex2 = i;
			if ( pTrigCfg->i32Source > 0 )
				CardIndex2 = 1 + (pTrigCfg->i32Source - 1) / u8ChannelPerCard;
			else
				CardIndex2 = abs(pTrigCfg->i32Source);
		}

		if ( CardIndex1 != CardIndex2 )
		{
			if ( Coerce )
			{
				// Disable the trigger engine
				pTrigCfg->i32Source = CS_TRIG_SOURCE_DISABLE;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_NOT_TRIGGER_FROM_SAME_CARD;
			}
		}
	}
#endif

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
			i32Status = CsDrvValidateTriggerConfig( pSysCfg, pSysInfo, u32Coerce );
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
int32 CsDrvValidateTransferParamsEx(DRVHANDLE DrvHdl, PIN_PARAMS_TRANSFERDATA_EX InParams)

{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	const uInt32	cu32ValidTxMode	= ( TxMODE_SEGMENT_TAIL | TxMODE_DATA_INTERLEAVED );

	// Search for SystemInfo of the current system based on DrvHdl
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );
	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	uInt8 u8Alignment = 0;
	uInt16 u16Backup = 0;

	PCSACQUISITIONCONFIG	pAcqCfg = CsDllGetAcqSystemConfigPointer( DrvHdl );


	// Validation of Segment
	if( (InParams->u32StartSegment == 0) || (InParams->u32StartSegment > pAcqCfg->u32SegmentCount) )
		return CS_INVALID_SEGMENT;

	if ( InParams->u32StartSegment + InParams->u32SegmentCount > pAcqCfg->u32SegmentCount + 1 )
		return CS_INVALID_SEGMENT_COUNT;

	if ( ((InParams->u32Mode & ~cu32ValidTxMode ) != 0 ) )
		return CS_INVALID_TRANSFER_MODE;

	if ( ((InParams->u32Mode & cu32ValidTxMode ) == (TxMODE_SEGMENT_TAIL | TxMODE_DATA_INTERLEAVED) ) )
		return CS_INVALID_TRANSFER_MODE;

	// Length should be multple of 4
	if ( (InParams->i64Length  & 0x3)  != 0 )
		return	CS_INVALID_LENGTH;

	if ( InParams->u32Mode == 0 || ((InParams->u32Mode & cu32ValidTxMode ) != 0 ) )
	{
		//
		// Mode Data Transfer Normal ( Integer )
		//
		// Validation of ChannelIndex
		if ( (InParams->u16Channel == 0) || (InParams->u16Channel > pSysInfo->u32ChannelCount) )
			return CS_INVALID_CHANNEL;

		// Check buffer for DWORD alignment
		u8Alignment = uInt8(DWORD_PTR(InParams->pDataBuffer) & DWORD_PTR(0x3));
		if( 0 != u8Alignment )
		{
			if( (sizeof(u16Backup) != u8Alignment) ||  (0 != (TxMODE_DATA_32 & InParams->u32Mode)) )
				return CS_BUFFER_NOT_ALIGNED;
		}

		int64		LimitPreTrigger = pAcqCfg->i64Depth - pAcqCfg->i64SegmentSize;
		
		// Validation for the StartAddress
		if ( ((InParams->i64StartAddress & 0x1F) != 0)   ||
			 (InParams->i64StartAddress < LimitPreTrigger ) ||
			 (InParams->i64StartAddress >  pAcqCfg->i64Depth + pAcqCfg->i64TriggerDelay) )
			return	CS_INVALID_START;

		// Validation for the Length of transfer
		// Length should be multiple of 4 and at least 16 samples
		if ( InParams->i64Length <= 0 ||
			 ((InParams->i64Length & 0x3) != 0) || ( InParams->i64Length < 16 ) ||
			 ( InParams->i64Length > pAcqCfg->i64SegmentSize ) ||
			 ( InParams->i64Length > pAcqCfg->i64Depth + pAcqCfg->i64TriggerDelay - InParams->i64StartAddress ) )
			return	CS_INVALID_LENGTH;		

		// Calculate the size of valid data in bytes
		int64 i64DataSize = 0;
		if ( 0 != (InParams->u32Mode & TxMODE_SEGMENT_TAIL) )
			i64DataSize = InParams->i64Length * InParams->u32SegmentCount;
		else
			i64DataSize = InParams->i64Length * InParams->u32SegmentCount * pAcqCfg->u32SampleSize;

		if ( (0 != (pAcqCfg->u32Mode & CS_MODE_DUAL)) && (0 != (InParams->u32Mode & TxMODE_DATA_INTERLEAVED)) )
			i64DataSize *= 2;

		if ( InParams->i64BufferLength < i64DataSize )
			return	CS_BUFFER_TOO_SMALL;

		if ( InParams->i64BufferLength > i64DataSize )
			InParams->i64BufferLength = i64DataSize;
	}
	else
		return CS_INVALID_TRANSFER_MODE;

	// Convert to number of bytes
	if ( 0 == (InParams->u32Mode & TxMODE_SEGMENT_TAIL) )
		InParams->i64Length *= pAcqCfg->u32SampleSize;

	if ( InParams->pDataBuffer != NULL )
		if ( IsBadWritePtr( InParams->pDataBuffer, (uInt32) InParams->i64BufferLength ) )
			return CS_INVALID_POINTER_BUFFER;


	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvTransferDataEx(DRVHANDLE DrvHdl, PIN_PARAMS_TRANSFERDATA_EX pInParamsEx, POUT_PARAMS_TRANSFERDATA_EX	pOutParamsEx )
{
	int32	i32Status = CS_SUCCESS;

	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	// Keep original copy of IN_PARAMS_TRANSFERDATA intact
	IN_PARAMS_TRANSFERDATA_EX InParamsEx;
	memcpy(&InParamsEx, pInParamsEx, sizeof(IN_PARAMS_TRANSFERDATA_EX));

	i32Status = CsDrvValidateTransferParamsEx( DrvHdl, &InParamsEx );
	if ( CS_FAILED(i32Status) )
		return i32Status;

	// Validation of pointers for returned values
	if ( IsBadWritePtr( pOutParamsEx, sizeof( OUT_PARAMS_TRANSFERDATA_EX ) ) )
		return CS_INVALID_POINTER_BUFFER;

	i32Status = g_pBaseHwDll->CsDrvTransferDataEx( DrvHdl, &InParamsEx, pOutParamsEx );

	return i32Status;
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
	const uInt32	cu32ValidTxMode	= ( TxMODE_SLAVE | TxMODE_DATA_16 | TxMODE_DATA_ONLYDIGITAL | TxMODE_DATA_32 );

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


		// Validation for the Length of transfer
		if ( InParams.i64Length <= 0 )
			return	CS_INVALID_LENGTH;

		// Limit the transfer size around the value of Segment size.
		// The kernel driver will not return data exceeded the SegmentSize anyway.
		if ( InParams.i64Length > pAcqCfg->i64SegmentSize + CS_MAX_PADDING )
			InParams.i64Length = pAcqCfg->i64SegmentSize + CS_MAX_PADDING;
				
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


	if ( InParams.pDataBuffer != NULL )
		if ( IsBadWritePtr( InParams.pDataBuffer, (uInt32) InParams.i64Length ) )
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

	PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
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
		for (i = 0; i < DEERE_MAX_SR_COUNT; i ++ )
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
		for (i = 0; i < DEERE_MAX_SR_COUNT; i ++ )
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
				for (i = 0; i < DEERE_MAX_SR_COUNT; i ++ )
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

	PCSDEEREBOARDCAPS	pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	uInt16				u16Input = (uInt16) SubCapsId;
	PCSRANGETABLE		pLocRangeTable;
	uInt32				u32MaxRange;

	if ( u16Input > 0 )
	{
		u32MaxRange =  DEERE_MAX_RANGES;
		pLocRangeTable = pCaps->RangeTable;
	}
	else
	{
		u32MaxRange =  DEERE_EXT_TRIG_RANGES;
		pLocRangeTable = pCaps->ExtTrigRangeTable;
	}


	if ( pSystemConfig == NULL )
	{
		// If pSystemConfig == NULL,
		// Application request for input ranges available, regardless the current configuration
		//

		for (i = 0; i < DEERE_EXT_TRIG_RANGES; i ++ )
		{
			if ( 0 != pLocRangeTable[i].u32InputRange  )
				u32NumOfElement ++;
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
				memcpy( pRangesTbl, pLocRangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
				return u32NumOfElement;
			}
		}
	}
	else
	{
		int32	i32Status = g_pBaseHwDll->CsValidateBufferForReadWrite( CS_SYSTEM, pSystemConfig, TRUE );
		if ( i32Status != CS_SUCCESS )
			return i32Status;

		PCSCHANNELCONFIG pChanCfg = pSystemConfig->pAChannels->aChannel;
		uInt16		ImpedanceMask(0);

		if ( u16Input == 0 )
		{
			// External trigger input
			for (i = 0; i < u32MaxRange; i ++ )
			{
				if ( 0 != pLocRangeTable[i].u32InputRange  )
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
					memcpy( pRangesTbl, pLocRangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
					*BufferSize = sizeof(CSRANGETABLE) * u32NumOfElement;
					return u32NumOfElement;
				}
			}
		}
		else
		{
			if ( u16Input  > pSystemConfig->pAChannels->u32ChannelCount )
				return CS_INVALID_CHANNEL;

			for ( i = 0; i < pSystemConfig->pAChannels->u32ChannelCount; i ++ )
			{
				if (u16Input == pChanCfg[i].u32ChannelIndex)
				{
					if ( pChanCfg[i].u32Impedance == CS_REAL_IMP_50_OHM )
						ImpedanceMask = CS_IMP_50_OHM;
					else
						ImpedanceMask = CS_IMP_1M_OHM;
					break;
				}
			}

			for (i = 0; i < DEERE_MAX_RANGES; i ++ )
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
				
				for (i = 0, j= 0; i < DEERE_MAX_RANGES && j < u32NumOfElement; i++ )
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
int32 CsDrvGetAvailableImpedances( DRVHANDLE DrvHdl, uInt32 u32SubCapsId, PCSSYSTEMCONFIG ,
								   PCSIMPEDANCETABLE pImpedancesTbl,
								   uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32 u32NumOfElement = 0;
	PCSIMPEDANCETABLE	pLocImpedTable;
	uInt32 u32TableSize;

	if( 0 == u32SubCapsId )
	{
		pLocImpedTable = pCaps->ExtTrigImpedanceTable;
		u32TableSize = _countof(pCaps->ExtTrigImpedanceTable);
	}
	else
	{
		pLocImpedTable = pCaps->ImpedanceTable;
		u32TableSize = _countof(pCaps->ImpedanceTable);
	}

	for( uInt32 i = 0; i< u32TableSize; i++ )
	{
		if( 0 != pLocImpedTable[i].u32Imdepdance )
			u32NumOfElement++;
	}

	if ( NULL == pImpedancesTbl )//----- first call return the size
	{
		*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
		return CS_SUCCESS;
	}
	else //----- 2nd call return the table
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			memcpy( pImpedancesTbl, pLocImpedTable,  u32NumOfElement * sizeof(CSIMPEDANCETABLE ) );
			return CS_SUCCESS;
		}
	}	
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableCouplings(DRVHANDLE DrvHdl, uInt32 u32SubCapsId, PCSSYSTEMCONFIG ,
								   PCSCOUPLINGTABLE pCouplingTbl,
								   uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32 u32NumOfElement = 0;
	PCSCOUPLINGTABLE	pLocTable;
	uInt32 u32TableSize;


	if ( 0 == u32SubCapsId )
	{
		pLocTable = pCaps->ExtTrigCouplingTable;
		u32TableSize = _countof(pCaps->ExtTrigCouplingTable);
	}
	else
	{
		pLocTable = pCaps->CouplingTable;
		u32TableSize = _countof(pCaps->CouplingTable);
	}

	for( uInt32 i = 0; i< u32TableSize; i++ )
	{
		if( 0 != pLocTable[i].u32Coupling)
			u32NumOfElement++;
	}

	if ( NULL == pCouplingTbl )//----- first call return the size
	{
		*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
		return CS_SUCCESS;
	}
	else //----- 2nd call return the table
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			memcpy( pCouplingTbl, pLocTable,  u32NumOfElement * sizeof(CSIMPEDANCETABLE ) );
			return CS_SUCCESS;
		}
	}
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

	PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32	i;
	uInt32	u32NumOfElement = 0;
	bool	bNoFilterFound = false;

	for( i = 0; i< DEERE_FILTERS; i++ )
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
			bNoFilterFound = true;
		}
	}

	if ( NULL == pFilterTbl )//----- first call return the size
	{
		*BufferSize = u32NumOfElement * sizeof( CSFILTERTABLE );
		return CS_SUCCESS;
	}
	else //----- 2nd call return the table
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSFILTERTABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			int j = 0;
			bNoFilterFound = false;
			for( i = 0; i< DEERE_FILTERS; i++ )
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
					bNoFilterFound = true;
				}
			}
			*BufferSize = u32NumOfElement * sizeof( CSFILTERTABLE );
			return CS_SUCCESS;
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

			if ( 1 == u16ChannelPerCard ) 
				u32ValidMode = g_u32ValidMode & ~CS_MODE_DUAL;
			else 
				u32ValidMode = g_u32ValidMode;

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
			PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
			for( int j = 0; j < DEERE_COUPLING; j++ )
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

	*pBuffer = 1;			// Support Flexible trigger
	*BufferSize = sizeof(uInt32);
	
	return CS_SUCCESS;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetBoardTriggerEngines( DRVHANDLE DrvHdl, uInt32 CapsId, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );

	CapsId;
	*pBuffer = pSysInfo->u32TriggerMachineCount/pSysInfo->u32BoardCount;
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
int32	CsDrvGetMaxSegmentPadding( DRVHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	i32Status = CsDrvGetParams(DrvHdl, CS_MAX_SEGMENT_PADDING, pBuffer );
	if ( CS_FAILED(i32Status) )
		return i32Status;
	else
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetTriggerResolution( DRVHANDLE , uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL != pBuffer )
	{
		*pBuffer = DEERE_TRIGGER_RES;
	}

	*BufferSize = sizeof(uInt32);

	return CS_SUCCESS;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetMinExtRate( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG pSysCfg, int64* pBuffer,  uInt32* BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(int64);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( pBuffer, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;

	if ( NULL != pBuffer )
	{
		PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
		PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

		uInt32 MaxIndex = sizeof (pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]);
		for ( uInt32 i = 0; i < MaxIndex; i ++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				*pBuffer = pCaps->ExtClkTable[i].i64Min;
				bFound = true;
			}
		}
		if (!bFound)
		{
			*pBuffer = 0;
		}
	}

	*BufferSize = sizeof(int64);

	return bFound ? CS_SUCCESS : CS_NO_EXT_CLK;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetMaxExtRate( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG pSysCfg, int64* pBuffer,  uInt32* BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	if ( IsBadWritePtr( BufferSize, sizeof( uInt32 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(int64);
		return CS_SUCCESS;
	}	

	
	if ( IsBadWritePtr( pBuffer, sizeof( int64 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;

	if ( NULL != pBuffer )
	{
		PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
		PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

		uInt32 MaxIndex = sizeof (pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]);
		for ( uInt32 i = 0; i < MaxIndex; i ++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				*pBuffer = pCaps->ExtClkTable[i].i64Max;
				bFound = true;
			}
		}
		if (!bFound)
		{
			*pBuffer = 0;
		}
	}

	*BufferSize = sizeof(int64);
	return bFound ? CS_SUCCESS : CS_NO_EXT_CLK;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetSkipCount( DRVHANDLE DrvHdl, uInt32, PCSSYSTEMCONFIG, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);
	if ( IsBadWritePtr( BufferSize, sizeof( uInt32) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( NULL == pBuffer )
	{
		*BufferSize = sizeof(uInt32);
		return CS_SUCCESS;
	}	

	if ( IsBadWritePtr( pBuffer, sizeof( uInt32 ) ) )
		return CS_INVALID_POINTER_BUFFER;

	bool bFound = false;
	if ( NULL != pBuffer )
	{
		PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
		for ( uInt32 i = 0; i < sizeof(pCaps->ExtClkTable)/sizeof(pCaps->ExtClkTable[0]); i ++ )
		{
			if ( 0 != pCaps->ExtClkTable[i].u32Mode )
			{
				bFound = true;
				break;
			}
		}
	}
	*BufferSize = sizeof(uInt32);
	if (bFound)
	{
		*pBuffer = 1000;
		return CS_SUCCESS;
	}
	else
	{
		*pBuffer = 0;
		return CS_NO_EXT_CLK;
	}
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
		pAuxCfg->u8DlgType	= AUX_IO_JD_DLG;
		pAuxCfg->bClockOut	= TRUE;
		pAuxCfg->bTrigOut	= TRUE;
		pAuxCfg->bTrigOut_IN = FALSE;
		pAuxCfg->bAuxOUT	= TRUE;
		pAuxCfg->bAuxIN		= TRUE;
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
int32  CsDrvGetAvailableExtTrigEnable( DRVHANDLE, PCSSYSTEMCONFIG, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		i32Status = CS_INVALID_PARAMETER;
	else if (NULL == pBuffer)
		*BufferSize = sizeof(ValidExtTrigEn);
	else if ( sizeof(ValidExtTrigEn) != *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
		memcpy(pBuffer, ValidExtTrigEn, sizeof(ValidExtTrigEn));

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAvailableAuxOut( DRVHANDLE, PCSSYSTEMCONFIG, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		i32Status = CS_INVALID_PARAMETER;
	else if (NULL == pBuffer)
		*BufferSize = sizeof(ValidAuxOut);
	else if ( sizeof(ValidAuxOut) != *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
		memcpy(pBuffer, ValidAuxOut, sizeof(ValidAuxOut));

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32  CsDrvGetAvailableAuxInTimeStamp( DRVHANDLE, PCSSYSTEMCONFIG, PVALID_AUX_CONFIG pBuffer, uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if (NULL == BufferSize)
		i32Status = CS_INVALID_PARAMETER;
	else if (NULL == pBuffer)
		*BufferSize = sizeof(ValidTsReset);
	else if ( sizeof(ValidTsReset) != *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
		memcpy(pBuffer, ValidTsReset, sizeof(ValidTsReset));

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
	PCSDEEREBOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	
	if ( pAcqCfg->u32ExtClk )
	{
		// Validation of Exnternal clock frequency
		bool bFound = false;
		for ( uInt32 i = 0; i < DEERE_MODE_COUNT; i ++ )
		{
			if ( pAcqCfg->u32Mode & pCaps->ExtClkTable[i].u32Mode )
			{
				bFound = true;
				if ( pAcqCfg->i64SampleRate > pCaps->ExtClkTable[i].i64Max )
					i64CoerceSampleRate = pCaps->ExtClkTable[i].i64Max;
				else if ( pAcqCfg->i64SampleRate < pCaps->ExtClkTable[i].i64Min )
					i64CoerceSampleRate = pCaps->ExtClkTable[i].i64Min;
				break;
			}
		}
		if( !bFound )
			return CS_NO_EXT_CLK;
	}
	else
	{
		i64CoerceSampleRate = g_pBaseHwDll->CoerceSampleRate( pAcqCfg->i64SampleRate, 
			                                                  pAcqCfg->u32Mode,
			                                                  0, 
															  pCaps->SrTable, 
															  DEERE_MAX_SR_COUNT * sizeof( CSSAMPLERATETABLE_EX ) );
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
		char szBaseOptions[][HWOPTIONS_TEXT_LENGTH] = {
										"No Memory",		// 0x1
										};

		// Mask for options that require special BB firmware
		pHwOptionText->u32RevMask = DEERE_BBHW_OPT_FWREQUIRED_MASK;

		// Options text from base board
		uInt32	nSize = sizeof( szBaseOptions ) / HWOPTIONS_TEXT_LENGTH;

		for (uInt32 i = 0; i < nSize; i++)
		{
			if ( i == pHwOptionText->u8BitPosition )
			{
				strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), szBaseOptions[i] );
				return CS_SUCCESS;
			}
		}

		// There is no BB HW options for now
		strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
		return CS_SUCCESS;
	}


	char szAddonOptions[][HWOPTIONS_TEXT_LENGTH] = {
									"Dual Channel",		// 0x1
									"14-bit ADC",		// 0x2
									"2 ADCs/Chan",		// 0x4
									"4 ADCs/Chan",		// 0x8
									"Aux I/O",			// 0x10
									"Low Cal Freq"		// 0x20
									};

	// Mask for options that require special AO firmware
	pHwOptionText->u32RevMask = 0;

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

	*u32MinBoardType = CSJD_FIRST_BOARD;
	*u32MaxBoardType = CSJD_LAST_BOARD;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetBoardCaps( uInt32 u32ProductId, uInt32 u32Param, void *pBuffer, uInt32 *u32BufferSize)
{
	u32ProductId;
	switch( u32Param )
	{
	case CS_FLASHINFO:
		{
			if ( *u32BufferSize <  sizeof(CS_FLASHLAYOUT_INFO) )
				return CS_BUFFER_TOO_SMALL;

			PCS_FLASHLAYOUT_INFO	pCsFlashLayoutInfo = (PCS_FLASHLAYOUT_INFO) pBuffer;

			if ( IsBadWritePtr( pCsFlashLayoutInfo, sizeof(CS_FLASHLAYOUT_INFO) ) )
				return CS_INVALID_POINTER_BUFFER;

			pCsFlashLayoutInfo->u32FlashType			= 1;
			pCsFlashLayoutInfo->u32NumOfImages			= DEERE_NUMBER_OF_FLASHIMAGES;
			pCsFlashLayoutInfo->u32ImageStructSize		= FIELD_OFFSET( DEERE_FLASH_LAYOUT, FlashImage2 );
			pCsFlashLayoutInfo->u32WritableStartOffset	= 0;
			pCsFlashLayoutInfo->u32CalibInfoOffset		= FIELD_OFFSET( DEERE_FLASH_IMAGE_STRUCT, Calib );
			pCsFlashLayoutInfo->u32FwInfoOffset			= FIELD_OFFSET( DEERE_FLASH_IMAGE_STRUCT, FwInfo );
			pCsFlashLayoutInfo->u32FlashFooterOffset	= FIELD_OFFSET( DEERE_FLASH_LAYOUT, Footer);
			pCsFlashLayoutInfo->u32FwBinarySize			= DEERE_FLASH_FWIMAGESIZE;
		}
		return CS_SUCCESS;

	default:
		return CS_INVALID_REQUEST;
	}
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32 CsDrvGetAvailableTriggerSources(CSHANDLE DrvHdl, uInt32 SubCapsId, int32 *i32ArrayTriggerSources,
													uInt32 *BufferSize )
{
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer( DrvHdl );
	if ( ! pSysInfo )
		return CS_INVALID_HANDLE;

	uInt32	i;
	uInt32	k;
	uInt32	u32ChannelSkip = 0;

	if (NULL == BufferSize)
		return (CS_INVALID_PARAMETER);

	PCSDEEREBOARDCAPS	pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	// Calculate number of trigger source available
	uInt32			u32NumberTriggerSources = 0;
	
	u32NumberTriggerSources = 1;			 		// Trigger Disbaled

	if ( CS_NO_EXTTRIG != pCaps->ExtTrigRangeTable[0].u32Reserved )
		u32NumberTriggerSources += 1; 				// External trigger

	uInt32	u32ChannelsPerCard = pSysInfo->u32ChannelCount / pSysInfo->u32BoardCount;
	if ( u32ChannelsPerCard == 1 )
	{
		u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
		u32ChannelSkip = 0;
	}
	else if ( u32ChannelsPerCard == 2 )
	{
		if ( SubCapsId == 0 || ( SubCapsId & CS_MODE_DUAL ) )
		{
			u32NumberTriggerSources += pSysInfo->u32ChannelCount;   // Channels Trigger
			u32ChannelSkip = 0;
		}
		else
		{
			u32NumberTriggerSources += pSysInfo->u32ChannelCount / 2;   // Channels Trigger
			u32ChannelSkip = 1;
		}
	}

	if ( i32ArrayTriggerSources == NULL )
	{
		*BufferSize = u32NumberTriggerSources * sizeof(uInt32);
		return CS_SUCCESS;
	}
	else 
	{
		if ( *BufferSize < u32NumberTriggerSources * sizeof(uInt32) )
			return CS_BUFFER_TOO_SMALL;
		else 
		{
			k = 0;
			//  Trigger source : External
			if ( CS_NO_EXTTRIG != pCaps->ExtTrigRangeTable[0].u32Reserved )
			{
				// External triggger from master card only
				i32ArrayTriggerSources[k++] =  -1;
			}

			// Trigger source disable
			i32ArrayTriggerSources[k++] = 0;
	
			// Trigger source : Channel
			for( i = 0; i < pSysInfo->u32ChannelCount; i ++ )
			{
				i32ArrayTriggerSources[k++] =  (int32) (i+1);
				i += u32ChannelSkip;
			}

			*BufferSize = u32NumberTriggerSources * sizeof(uInt32);
			return CS_SUCCESS;
		}
	}

}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL _CsDrvIsMyBoardType( uInt32 u32BoardType)
{
	if( u32BoardType >= CSJD_FIRST_BOARD &&
		u32BoardType <= CSJD_LAST_BOARD )
		return TRUE;
	else
		return FALSE;
}

#ifdef _WINDOWS_

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

INT_PTR CALLBACK CfgAuxIoDlg( HWND hwndDlg,  UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// Assume that CompuscopeManager does open this Dialog for only one system at time then
	// it is safe using static variable here
	static	CSHANDLE DrvHdl = 0;

	switch (uMsg)
	{
		case WM_INITDIALOG: 
		{
			DrvHdl = (CSHANDLE) lParam;

			CS_TRIG_OUT_STRUCT	TrigOut = {0};
			CS_CLOCK_OUT_STRUCT	ClockOut = {0};
			CS_AUX_IN_STRUCT	AuxIn = {0};
			CS_AUX_OUT_STRUCT	AuxOut = {0};

			int32 i32Status = CsDrvGetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
			i32Status = CsDrvGetParams(DrvHdl, CS_CLOCK_OUT, &ClockOut);
			i32Status = CsDrvGetParams(DrvHdl, CS_AUX_OUT, &AuxOut);
			if ( CS_FAILED(i32Status) )
			{
				// Diabled Aux I/O if it is not enabled by HW
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUX_OUT), FALSE);
				AuxOut.u16ModeVal = CS_OUT_NONE;
			}
			
			i32Status = CsDrvGetParams(DrvHdl, CS_AUX_IN, &AuxIn);
			if ( CS_FAILED(i32Status) )
			{
				// Diabled Aux I/O if it is not enabled by HW
				EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_RST_SELECT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TS_RESET), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN_SELECT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), FALSE);
			}

			int i;

			for(i = 0; i< sizeof(ValidAuxOut)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_AUX_OUT, CB_ADDSTRING, 0, (LPARAM)ValidAuxOut[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_AUX_OUT, CB_SETITEMDATA, i, (LPARAM)ValidAuxOut[i].u32Ctrl);

				if (AuxOut.u16ModeVal == ValidAuxOut[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_AUX_OUT, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidAuxOut[i].Str);
			}

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

			for(i = 0; i< sizeof(ValidExtTrigEn)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_EXTTRIG_EN, CB_ADDSTRING, 0, (LPARAM)ValidExtTrigEn[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_EXTTRIG_EN, CB_SETITEMDATA, i, (LPARAM)ValidExtTrigEn[i].u32Ctrl);
				if (AuxIn.u16ExtTrigEnVal == ValidExtTrigEn[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidExtTrigEn[i].Str);
			}

			for(i = 0; i< sizeof(ValidTsReset)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_TS_RESET, CB_ADDSTRING, 0, (LPARAM)ValidTsReset[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_TS_RESET, CB_SETITEMDATA, i, (LPARAM)ValidTsReset[i].u32Ctrl);
				if (AuxIn.u16ExtTrigEnVal == ValidExtTrigEn[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_TS_RESET, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidTsReset[i].Str);
			}

			if ( AuxIn.u16ModeVal == CS_EXTTRIG_EN_MODE )
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_TS_RESET), FALSE);
				::SendDlgItemMessage (hwndDlg, IDC_TS_RESET, CB_SETCURSEL, 0, 0);
				::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN_SELECT, BM_SETCHECK, 1, 0);
			}
			else 
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), FALSE);
				::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_SETCURSEL, 0, 0);
				::SendDlgItemMessage (hwndDlg, IDC_TIMESTAMP_RST_SELECT, BM_SETCHECK, 1, 0);
			}

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_TIMESTAMP_RST_SELECT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TS_RESET), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), FALSE);
					::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_SETCURSEL, 0, 0);
					break;

				case IDC_EXTTRIG_EN_SELECT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TS_RESET), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), TRUE);
					::SendDlgItemMessage (hwndDlg, IDC_TS_RESET, CB_SETCURSEL, 0, 0);
					break;

				case IDOK:
				{
					CS_TRIG_OUT_STRUCT	TrigOut = {0};
					CS_CLOCK_OUT_STRUCT	ClockOut = {0};
					CS_AUX_IN_STRUCT	AuxIn = {0};
					CS_AUX_OUT_STRUCT	AuxOut = {0};

					int32 nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_GETCURSEL, 0, 0);
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
	
					uInt32	u32SelectDlgId;
					if ( SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN_SELECT, BM_GETCHECK, 0, 0) )
					{
						AuxIn.u16ModeVal = CS_EXTTRIG_EN_MODE;
						u32SelectDlgId = IDC_EXTTRIG_EN;
					}
					else
					{
						AuxIn.u16ModeVal = CS_TS_RESET_MODE;
						u32SelectDlgId = IDC_TS_RESET;
					}

					nIndex = (int32) SendDlgItemMessage(hwndDlg, u32SelectDlgId, CB_GETCURSEL, 0, 0);
					if( CB_ERR != nIndex )
					{
						AuxIn.u16ExtTrigEnVal =  (int16)(SendDlgItemMessage (hwndDlg, u32SelectDlgId, CB_GETITEMDATA, (WPARAM)nIndex, 0));
						if( CB_ERR != AuxIn.u16ExtTrigEnVal )
							CsDrvSetParams(DrvHdl, CS_AUX_IN, &AuxIn);
					}

					nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_AUX_OUT, CB_GETCURSEL, 0, 0);
					if( CB_ERR != nIndex )
					{
						AuxOut.u16ModeVal =  (int16)(SendDlgItemMessage (hwndDlg, IDC_AUX_OUT, CB_GETITEMDATA, (WPARAM)nIndex, 0));
						if( CB_ERR != AuxOut.u16ModeVal )
							CsDrvSetParams(DrvHdl, CS_AUX_OUT, &AuxOut);
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