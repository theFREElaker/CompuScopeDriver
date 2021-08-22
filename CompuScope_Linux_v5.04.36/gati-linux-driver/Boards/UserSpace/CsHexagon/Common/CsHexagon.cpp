
// CsHexagon.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CsBaseHwDLL.h"
#include "CsAdvHwDLL.h"
#include "CurrentDriver.h"

#include "CsHexagonCapsInfo.h"
#include "CsHexagonFlash.h"

#ifdef _WINDOWS_
#include "resource.h"
#endif


#define nUSE_SHARE_BOARDCAPS

// Global inter process varabales
extern SHAREDDLLMEM		Globals;

#ifdef _WINDOWS_
	// Global variables
	CTraceManager*		g_pTraceMngr= NULL;
#endif

CsBaseHwDLL*		g_pBaseHwDll = NULL;
CsAdvHwDLL*			g_pAdvanceHwDll = NULL;
GLOBAL_PERPROCESS	g_GlobalPerProcess = {0};

#if !defined(USE_SHARE_BOARDCAPS)
CSHEXAGON_BOARDCAPS		g_BoardCaps[4];				//max 4 hex boards/system
#endif

// Prototypes functions
int64  ValidateExtClockRate (DRVHANDLE DrvHdl,  PCSSYSTEMINFO pSysInfo, PCSSYSTEMCONFIG pSysCfg );
BOOL   ValidateChannelIndex( PCSSYSTEMINFO pSysInfo, PCSACQUISITIONCONFIG pAcqCfg, uInt32 u16ChannelIndex );
int32  CsDrvGetAvailableSampleRates(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig, PCSSAMPLERATETABLE pSRTable, uInt32 *BufferSize );
int32  CsDrvGetAvailableInputRanges(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig, PCSRANGETABLE pRangesTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableImpedances(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,  PCSIMPEDANCETABLE pImpedancesTbl, uInt32 *BufferSize );
int32  CsDrvGetAvailableCouplings(DRVHANDLE DrvHdl, uInt32 CapsId, PCSSYSTEMCONFIG pSystemConfig,   PCSCOUPLINGTABLE pCouplingsTbl, uInt32 *BufferSize );
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

int32  CsDrvGetMaxPreTrigger(CSHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetMaxStreamSegmentSize(CSHANDLE DrvHdl, int64 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetDepthIncrement(CSHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize );
int32  CsDrvGetTriggerDelayIncrement(CSHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize );

PCSSYSTEMINFO			CsDllGetSystemInfoPointer( DRVHANDLE DrvHdl, PCSSYSTEMINFOEX *pSysInfoEx = NULL );			
PCSACQUISITIONCONFIG	CsDllGetAcqSystemConfigPointer( DRVHANDLE DrvHdl );
PCSHEXAGON_BOARDCAPS		CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl );


const VALID_AUX_CONFIG	ValidTrigOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Trigger Out",		CS_TRIGOUT,
};

const VALID_AUX_CONFIG	ValidClkOut[] =
{
	"Disable",			CS_OUT_NONE,
	"Sample Clock",		CS_CLKOUT_SAMPLE,
	"Reference Clock",	CS_CLKOUT_REF
};


const uInt32	g_u32ValidMode =  CS_MODE_QUAD |
								  CS_MODE_DUAL |
								  CS_MODE_SINGLE |
								  CS_MODE_REFERENCE_CLK |
								  CS_MODE_SW_AVERAGING |
								  CS_MODE_USER1 | CS_MODE_USER2;


#ifdef _DEBUG
/// This function does nothing but make some compile time assertions
void _CompileTimeChecks()
{
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_CONFIG_REG1))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_CHAN_OFFSET))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_FS_RANG))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_CALIB_ADJ))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_TIMING_ADJ))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_DELAY_COARSE))
	GAGE_CT_ASSERT(2 == sizeof(ADC12D_DELAY_FINE))
}

#endif

int DumpBuffer(char *Label, uInt8 *pBuf, int32 size)
{
   int i = 0;
   if (pBuf && size)
   {
      if (Label)
         fprintf(stdout, "DisplayBuffer: %s [%d,%p] \n", Label, size, pBuf);  
      else
         fprintf(stdout, "DisplayBuffer: [%d,%p] \n", size, pBuf);  
      for (i=0; i<size; i++)
      {
        fprintf(stdout, " %02x", pBuf[i]);  
      }   
      fprintf(stdout, "\n");  
   }
   return(0);
}

int CompareInputRange( const void* pIr1, const void*pIr2 )
{
	return int(PCSRANGETABLE(pIr2)->u32InputRange) - int(PCSRANGETABLE(pIr1)->u32InputRange);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
typedef SharedMem< CSHEXAGON_BOARDCAPS > SHAREDxyG8CAPS;
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

			SHAREDxyG8CAPS* pCaps = new SHAREDxyG8CAPS( strSharedCapsName, true );
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, pCaps->it() );
			qsort(pCaps->it()->RangeTable, HEXAGON_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
			pDrvSystem[i].pCapsInfo = pCaps;
#else
			g_pBaseHwDll->CsDrvGetParams( DrvHdl, CS_DRV_BOARD_CAPS, &g_BoardCaps[i] );
			qsort(&g_BoardCaps[i].RangeTable, HEXAGON_MAX_RANGES, sizeof(CSRANGETABLE),CompareInputRange );
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
			delete ((SHAREDxyG8CAPS *)pDrvSystem[i].pCapsInfo);
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
    uInt16  SubCapId = (uInt16) (CapsId & 0xFFFF);
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

	case CAPS_AUX_CONFIG:
		return CsDrvGetAuxConfig( DrvHdl, (PCS_CAPS_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_CLOCK_OUT:
		return CsDrvGetAvailableClockOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_TRIG_OUT:
		return CsDrvGetAvailableTrigOut( DrvHdl, pSysCfg, (PVALID_AUX_CONFIG) pBuffer, BufferSize );

	case CAPS_MAX_SEGMENT_PADDING:
		return CsDrvGetMaxSegmentPadding( DrvHdl, (uInt32 *) pBuffer, BufferSize );

	case CAPS_EXT_TRIGGER_UNIPOLAR:		// External trigger input is unipolar
	case CAPS_CLK_IN:					// external clock supported
	case CAPS_DC_OFFSET_ADJUST:
	case CAPS_MULREC:
		break;

	case CAPS_STM_TRANSFER_SIZE_BOUNDARY:
	case CAPS_TRIGGER_RES:
		return CsDrvGetTriggerResolution(DrvHdl, (uInt32 *) pBuffer, BufferSize );
		break;

	case CAPS_MIN_EXT_RATE:
		return CsDrvGetMinExtRate(DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );
		break;

	case CAPS_MAX_EXT_RATE:
		return CsDrvGetMaxExtRate(DrvHdl, SubCapId, pSysCfg, (int64 *) pBuffer, BufferSize );
		break;

	case CAPS_SKIP_COUNT:
		return CsDrvGetSkipCount( DrvHdl, SubCapId, pSysCfg, (uInt32 *) pBuffer, BufferSize );
		break;

	case CAPS_BOOTIMAGE0:
	case CAPS_FWCHANGE_REBOOT:
		break;

	case CAPS_MAX_PRE_TRIGGER:
		return CsDrvGetMaxPreTrigger(DrvHdl, (uInt32*) pBuffer, BufferSize );
		break;

	case CAPS_STM_MAX_SEGMENTSIZE:
		return CsDrvGetMaxStreamSegmentSize(DrvHdl, (int64*) pBuffer, BufferSize );
		break;

	case CAPS_DEPTH_INCREMENT:
		return CsDrvGetDepthIncrement(DrvHdl, (uInt32*) pBuffer, BufferSize );
		break;

	case CAPS_TRIGGER_DELAY_INCREMENT:
		return CsDrvGetTriggerDelayIncrement(DrvHdl, (uInt32*) pBuffer, BufferSize );
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

	return g_pBaseHwDll->CsDrvSetParams( DrvHdl, ParamID, ParamBuffer );
}



PCSHEXAGON_BOARDCAPS CsDllGetSystemCapsInfo( DRVHANDLE DrvHdl)
{
	PDRVSYSTEM	pDrvSystem = Globals.it()->g_StaticLookupTable.DrvSystem;

	// Search for SystemInfo of the current system based on DrvHdl
	for (uInt32 i = 0; i < Globals.it()->g_StaticLookupTable.u16SystemCount; i++)
	{
		if ( pDrvSystem[i].DrvHandle ==  DrvHdl )
		{
#ifdef USE_SHARE_BOARDCAPS
			return ((SHAREDxyG8CAPS*)(pDrvSystem[i].pCapsInfo))->it();
#else
			return &g_BoardCaps[i];
#endif
		}
	}
	_ASSERT(0);
	return NULL;
}


#define HEXAGON_MIN_SEG_SIZE					16		// Should be multiple of 8

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
	uInt32	u32ModeMaskChannel = CS_MODE_QUAD |CS_MODE_DUAL | CS_MODE_SINGLE;

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
		 (( pAcqCfg->u32Mode & CS_MODE_USER1 ) && (pAcqCfg->u32Mode & CS_MODE_USER2) ) )
	{
		if ( u32Coerce )
		{
			pAcqCfg->u32Mode = ( u32AcqModeValid & CS_MODE_QUAD ) ? CS_MODE_QUAD : CS_MODE_DUAL;
			i32Status = CS_CONFIG_CHANGED ;
		}
		else
			return CS_INVALID_ACQ_MODE;
	}

	//----- Sample Rate
	int64 i64CoerceSampleRate = ValidateExtClockRate(DrvHdl, pSysInfo, pSysCfg );
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

	// Added Sept 15, 2015 to report error if total data size is too big (overflows a uint64) but we're not in infinite mode
	if ( -1 != pAcqCfg->i32SegmentCountHigh && -1 != pAcqCfg->i64Depth )
	{
        const uint64 uMax64 = (uint64)~0;
		uint64 uSubTotal = pAcqCfg->i64Depth * pAcqCfg->u32SampleSize * (pAcqCfg->u32Mode & CS_MASKED_MODE);
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
		PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
		for( j = 0; j < HEXAGON_COUPLING; j++ )
		{
			if ( pChanCfg->u32Term == pCaps->CouplingTable[j].u32Coupling )
				break;
		}
		if ( HEXAGON_COUPLING == j )
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
		for( j = 0; j < HEXAGON_IMPED; j++ )
		{
			if ( pChanCfg->u32Impedance == pCaps->ImpedanceTable[j].u32Imdepdance )
			{
				if( CS_REAL_IMP_50_OHM == pChanCfg->u32Impedance )
					ImpedanceMask = CS_IMP_50_OHM;
                break;
			}
		}
		if ( HEXAGON_IMPED == j )
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

		for ( i = 0; i < HEXAGON_MAX_RANGES; i ++ )
		{
			if ( pCaps->RangeTable[i].u32Reserved & ImpedanceMask )
			{
				if ( pCaps->RangeTable[i].u32InputRange == pChanCfg->u32InputRange )
					break;
			}
		}

		if ( i >= HEXAGON_MAX_RANGES )
		{
			if ( Coerce )
			{
				// Detect board type by Board Name
				if ( strstr(pSysInfo->strBoardName, HEXAGON_LOWRANGE_EXT_NAME) )
					pChanCfg->u32InputRange = HEXAGON_GAIN_LR_MV;			
				else
					pChanCfg->u32InputRange = CS_GAIN_2_V;			
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_GAIN;
		}

		//----- Position
		int32	i32MaxDcOffset = pChanCfg->u32InputRange/2;

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
				pTrigCfg->i32Source = -1;
				i32Status = CS_CONFIG_CHANGED;
			}
			else
				return CS_INVALID_EXT_TRIG;
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

	uInt32				CardIndex1 = 0;
	uInt32				CardIndex2 = 0;
	uInt32				TrigEnabledIndex1 = 0;
	uInt32				TrigEnabledIndex2 = 0;

	// Looking for at least on trigger source enabled valid
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( pTrigCfg->i32Source > 0 &&  ( pTrigCfg->i32Source <= (int32) pSysInfo->u32ChannelCount ) )
			{
				if ( 0 != (pAcqCfg->u32Mode & CS_MODE_SINGLE) && (1 != pSysInfo->u32ChannelCount/pSysInfo->u32BoardCount) )
				{
					if ( pTrigCfg->i32Source == CS_CHAN_1)
						bAtLeastOneTriggerSourceEnabled = TRUE;
				}
				else
					 bAtLeastOneTriggerSourceEnabled = TRUE;
			}
			else if  ( pTrigCfg->i32Source >= (int32) (-1*pSysInfo->u32BoardCount) )
				 bAtLeastOneTriggerSourceEnabled = TRUE;
		}
	}

	// Validation of Trigger Source
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( ( pTrigCfg->i32Source > (int32) pSysInfo->u32ChannelCount ) ||
	 			 ( pTrigCfg->i32Source < (int32) (-1*pSysInfo->u32BoardCount) ) )
				bErrorTriggerSource = TRUE;
			else if ( pTrigCfg->i32Source > 0 &&  ( pTrigCfg->i32Source <= (int32) pSysInfo->u32ChannelCount ) )
			{
				if ( 0 != (pAcqCfg->u32Mode & CS_MODE_SINGLE) && (1 != pSysInfo->u32ChannelCount/pSysInfo->u32BoardCount) )
				{
					if ( pTrigCfg->i32Source != CS_CHAN_1)
						bErrorTriggerSource = TRUE;
				}
			}

			if ( bErrorTriggerSource )
			{
				if ( Coerce )
				{
					if ( bAtLeastOneTriggerSourceEnabled )
						pTrigCfg->i32Source = CS_TRIG_SOURCE_DISABLE;
					else
					{
						pTrigCfg->i32Source = CS_CHAN_1;
						bAtLeastOneTriggerSourceEnabled = TRUE;
					}
					i32Status = CS_CONFIG_CHANGED;
				}
				else
					return CS_INVALID_TRIG_SOURCE;
			}

			// 4 channels board support channel 1& 3 in Dual Mode.
			if( ( pAcqCfg->u32Mode & CS_MODE_DUAL ) && ( pSysInfo->u32ChannelCount > 2 ) )
			{
				if ( ( pTrigCfg->i32Source == 2) || ( pTrigCfg->i32Source == 4) )
					return CS_INVALID_TRIG_SOURCE;
			}
		}
	}

	// Get CardIndex from the ftrigger source
	pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
	for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
	{
		if ( pTrigCfg->i32Source != CS_TRIG_SOURCE_DISABLE  )
		{
			if ( 0 == CardIndex1 )
			{
				TrigEnabledIndex1 = i;
				if ( pTrigCfg->i32Source > 0 )
					CardIndex1 = (pTrigCfg->i32Source + pSysInfo->u32ChannelCount-1) / pSysInfo->u32ChannelCount;
				else
					CardIndex1 = abs(pTrigCfg->i32Source);
			}
			else if ( 0 == CardIndex2 )
			{
				TrigEnabledIndex2 = i;
				if ( pTrigCfg->i32Source > 0 )
					CardIndex2 = (pTrigCfg->i32Source + pSysInfo->u32ChannelCount-1) / pSysInfo->u32ChannelCount;
				else
					CardIndex2 = abs(pTrigCfg->i32Source);
			}
		}
	}

	// Make sure 2 enabled trigger engines come from the same card
	if ( 0 != CardIndex2 && 0 != CardIndex2  )
	{
		if ( CardIndex1 != CardIndex2 )
		{
			if ( Coerce )
			{
				pTrigCfg = (PCSTRIGGERCONFIG) pSysCfg->pATriggers->aTrigger;
				for (uInt32	i = 0; i < pSysCfg->pATriggers->u32TriggerCount; i ++, pTrigCfg++)
				{
					// Disable the second trigger engine
					if (i == TrigEnabledIndex2)
						pTrigCfg->i32Source = CS_TRIG_SOURCE_DISABLE;
				}
				return CS_CONFIG_CHANGED;
			}
			else
			{
				return CS_NOT_TRIGGER_FROM_SAME_CARD;
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
int32 CsDrvTransferData(DRVHANDLE DrvHdl, IN_PARAMS_TRANSFERDATA InParams, POUT_PARAMS_TRANSFERDATA	OutParams)
{
	if (NULL == g_pBaseHwDll)
		return CS_FALSE;

	const uInt32	cu32ValidTxMode = (TxMODE_SLAVE | TxMODE_DATA_16 | TxMODE_DATA_ONLYDIGITAL | TxMODE_TIMESTAMP |
										TxMODE_DATA_32 | TxMODE_DATA_FFT | TxMODE_MULTISEGMENTS | TxMODE_DEBUG_DISP);

	// Search for SystemInfo of the current system based on DrvHdl
	PCSSYSTEMINFO	pSysInfo = CsDllGetSystemInfoPointer(DrvHdl);
	if (!pSysInfo)
		return CS_INVALID_HANDLE;

	// Validation of pointers for returned values
	if (IsBadWritePtr(OutParams, sizeof(OUT_PARAMS_TRANSFERDATA)))
		return CS_INVALID_POINTER_BUFFER;

	// Check for valid transfer mode
	if (InParams.u32Mode != 0 && ((InParams.u32Mode & cu32ValidTxMode) == 0))
		return CS_INVALID_TRANSFER_MODE;

	// Check buffer for DWORD alignment
	uInt8 u8Alignment = uInt8(DWORD_PTR(InParams.pDataBuffer) & DWORD_PTR(0x3));
	if (0 != u8Alignment)
		return CS_BUFFER_NOT_ALIGNED;

	int32 i32Ret = g_pBaseHwDll->CsDrvTransferData(DrvHdl, InParams, OutParams);
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

	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
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
		for (i = 0; i < HEXAGON_MAX_SR_COUNT; i ++ )
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
		for (i = 0; i < HEXAGON_MAX_SR_COUNT; i ++ )
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
				for (i = 0; i < HEXAGON_MAX_SR_COUNT; i ++ )
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

	PCSHEXAGON_BOARDCAPS	pCaps = CsDllGetSystemCapsInfo(DrvHdl);
	uInt16				u16Input = (uInt16) SubCapsId;
	PCSRANGETABLE		pLocRangeTable;
	uInt32				u32MaxRange;

	if ( u16Input > 0 )
	{
		u32MaxRange =  HEXAGON_MAX_RANGES;
		pLocRangeTable = pCaps->RangeTable;
	}
	else
	{
		u32MaxRange =  HEXAGON_EXT_TRIG_RANGES;
		pLocRangeTable = pCaps->ExtTrigRangeTable;
	}


	if ( pSystemConfig == NULL )
	{
		// If pSystemConfig == NULL,
		// Application request for input ranges available, regardless the current configuration
		//

		for (i = 0; i < HEXAGON_EXT_TRIG_RANGES; i ++ )
		{
			if ( 0 != pLocRangeTable[i].u32InputRange  )
				u32NumOfElement ++;
		}

		if ( NULL != pRangesTbl )
		{
			if ( *BufferSize < sizeof(CSRANGETABLE) * u32NumOfElement )
				return CS_BUFFER_TOO_SMALL;
			else
				memcpy( pRangesTbl, pLocRangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
		}

		*BufferSize = sizeof(CSRANGETABLE) * u32NumOfElement;
		return CS_SUCCESS;
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

			if ( NULL != pRangesTbl )
			{
				if ( *BufferSize < sizeof(CSRANGETABLE) * u32NumOfElement )
					return CS_BUFFER_TOO_SMALL;
				else
					memcpy( pRangesTbl, pLocRangeTable,  sizeof(CSRANGETABLE) * u32NumOfElement );
			}

			*BufferSize = sizeof(CSRANGETABLE) * u32NumOfElement;
			return CS_SUCCESS;

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

			for (i = 0; i < HEXAGON_MAX_RANGES; i ++ )
			{
				if ( (pCaps->RangeTable[i].u32Reserved  & ImpedanceMask) == ImpedanceMask )
					u32NumOfElement ++;
			}
		
			if ( NULL != pRangesTbl )
			{
				if ( *BufferSize < u32NumOfElement * sizeof(CSRANGETABLE))
					return CS_BUFFER_TOO_SMALL;
				else
				{
					for (i = 0, j= 0; i < HEXAGON_MAX_RANGES && j < u32NumOfElement; i++ )
					{
						if ( (pCaps->RangeTable[i].u32Reserved  & ImpedanceMask) == ImpedanceMask )
						{
							pRangesTbl[j++] = pCaps->RangeTable[i];
						}
					}
				}
			}

			*BufferSize = u32NumOfElement * sizeof(CSRANGETABLE);
			return CS_SUCCESS;
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

	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

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

	if ( NULL != pImpedancesTbl )
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else
			memcpy( pImpedancesTbl, pLocImpedTable,  u32NumOfElement * sizeof(CSIMPEDANCETABLE ) );
	}

	*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
	return CS_SUCCESS;

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

	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

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

	if ( NULL != pCouplingTbl )		//----- first call return the size
	{
		if ( *BufferSize < u32NumOfElement * sizeof( CSIMPEDANCETABLE ) )
			return CS_BUFFER_TOO_SMALL;
		else
			memcpy( pCouplingTbl, pLocTable,  u32NumOfElement * sizeof(CSIMPEDANCETABLE ) );
	}

	// return the size of the table
	*BufferSize = u32NumOfElement * sizeof(CSIMPEDANCETABLE );
	return CS_SUCCESS;
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

	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	uInt32	i;
	uInt32	u32NumOfElement = 0;
	bool	bNoFilterFound = false;

	for( i = 0; i< HEXAGON_FILTERS; i++ )
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
			for( i = 0; i< HEXAGON_FILTERS; i++ )
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
			uInt32	u32ValidMode = g_u32ValidMode;
			uInt16	u16ChannelPerCard = (uInt16) (pSysInfo->u32ChannelCount / pSysInfo->u32BoardCount);

			if ( 2 == u16ChannelPerCard )
				u32ValidMode &= ~CS_MODE_QUAD;

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
			PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);
			for( int j = 0; j < HEXAGON_COUPLING; j++ )
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

	if ( NULL != pBuffer )
	{
		if ( IsBadWritePtr( pBuffer, sizeof( uInt32) ) )
			return CS_INVALID_POINTER_BUFFER;
		else
			*pBuffer = HEXAGON_DEPTH_INCREMENT;
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
		PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

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
		PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

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

	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

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
#if 0
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
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int64 ValidateExtClockRate ( DRVHANDLE DrvHdl, PCSSYSTEMINFO , PCSSYSTEMCONFIG pSysCfg )
{
	if(NULL == g_pBaseHwDll )
		return CS_FALSE;

	PCSACQUISITIONCONFIG pAcqCfg = (PCSACQUISITIONCONFIG) pSysCfg->pAcqCfg;
	int64				i64CoerceSampleRate = pAcqCfg->i64SampleRate;
	PCSHEXAGON_BOARDCAPS pCaps = CsDllGetSystemCapsInfo(DrvHdl);

	if ( pAcqCfg->u32ExtClk )
	{
		// Validation of Exnternal clock frequency
		for ( uInt32 i = 0; i < HEXAGON_MODE_COUNT; i ++ )
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
		// There is no BB HW options for now
		strcpy_s(pHwOptionText->szOptionText, sizeof(pHwOptionText->szOptionText), "??" );
	}
	else
	{
		char szAddonOptions[][HWOPTIONS_TEXT_LENGTH] = 
									{	
										"2 Channels",	    //bit  0
										"500 MS/s",			//bit  1
										"AC Coupling",		//bit  2
										"LR-240mV"	,		//bit  3
										"No OVP"	,		//bit  4
									};

		pHwOptionText->u32RevMask = 0;

		// Options text from add-on
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
	}

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	_CsDrvGetFwOptionText( PCS_FWOPTIONS_CONVERT2TEXT pFwOptionText )
{
	if ( IsBadReadPtr( pFwOptionText,  sizeof(CS_HWOPTIONS_CONVERT2TEXT) ) )
		return CS_INVALID_POINTER_BUFFER;

	if ( sizeof(CS_HWOPTIONS_CONVERT2TEXT) != pFwOptionText->u32Size )
		return CS_INVALID_STRUCT_SIZE;

	if ( pFwOptionText->bBaseBoard )
	{
		if ( (1<<pFwOptionText->u8BitPosition) & HEXAGON_VALID_EXPERT_FW )
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

	*u32MinBoardType = CSHEXAGON_FIRST_BOARD;
	*u32MaxBoardType = CSHEXAGON_LAST_BOARD;

	return CS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

int32 _CsDrvGetBoardCaps( uInt32 u32DeviceId, uInt32 u32Param, void *pBuffer, uInt32 *u32BufferSize)
{
	UNREFERENCED_PARAMETER(u32DeviceId);

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
				// There is no EEPROM on the Hexagon addon.
				// EEPROM is just a location reserved for Addon in the Base board Flash
				pCsFlashLayoutInfo->u32FlashType			= 1;
				pCsFlashLayoutInfo->u32NumOfImages			= 
				pCsFlashLayoutInfo->u32ImageStructSize		= 
				pCsFlashLayoutInfo->u32WritableStartOffset	= 
				pCsFlashLayoutInfo->u32CalibInfoOffset		= 
				pCsFlashLayoutInfo->u32FwInfoOffset			= 
				pCsFlashLayoutInfo->u32FwBinarySize			= 0;
				pCsFlashLayoutInfo->u32FlashFooterOffset	= FIELD_OFFSET( HEXAGON_FLASH_LAYOUT, BoardData) + FIELD_OFFSET( HEXAGON_MISC_DATA, AddonInfo );
			}
			else
			{
				pCsFlashLayoutInfo->u32FlashType			= 1;
				pCsFlashLayoutInfo->u32NumOfImages			= HEXAGON_NUMBER_OF_IMAGES-1;	// -1 to remove the SafeBoot image.
				pCsFlashLayoutInfo->u32ImageStructSize		= sizeof(HEXAGON_FLS_IMAGE_STRUCT);
				pCsFlashLayoutInfo->u32WritableStartOffset	= FIELD_OFFSET( HEXAGON_FLASH_LAYOUT, FlashImage0 );
				pCsFlashLayoutInfo->u32CalibInfoOffset		= FIELD_OFFSET( HEXAGON_FLS_IMAGE_STRUCT, Calib );
				pCsFlashLayoutInfo->u32FwInfoOffset			= FIELD_OFFSET( HEXAGON_FLS_IMAGE_STRUCT, FwInfo );
				pCsFlashLayoutInfo->u32FlashFooterOffset	= FIELD_OFFSET( HEXAGON_FLASH_LAYOUT, BoardData) + FIELD_OFFSET( HEXAGON_MISC_DATA, BaseboardInfo );
				pCsFlashLayoutInfo->u32FwBinarySize			= HEXAGON_FWIMAGESIZE;
			}
		}
		return CS_SUCCESS;

	default:
		return CS_INVALID_REQUEST;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvExpertCall( DRVHANDLE DrvHdl, void *FuncParams )
{
	if(NULL == g_pAdvanceHwDll )
		return CS_FALSE;

#ifdef XPERT_SUPPORTED
	PCSEXPERTPARAM   pCsInParam = (PCSEXPERTPARAM) FuncParams;

	if ( IsBadReadPtr ( FuncParams, sizeof( CSEXPERTPARAM ) ) )
		return CS_INVALID_POINTER_BUFFER;

	switch ( pCsInParam->u32ActionId )
	{
		case EXFN_CREATEMINMAXQUEUE:
		case EXFN_DESTROYMINMAXQUEUE:
		case EXFN_GETSEGMENTINFO:
		case EXFN_CLEARERRORMINMAXQUEUE:
			return g_pAdvanceHwDll->CsDrvExpertCall( DrvHdl, FuncParams );

		default:
			return CS_INVALID_REQUEST;
	}
#else
	UNREFERENCED_PARAMETER(DrvHdl);
	UNREFERENCED_PARAMETER(FuncParams);
	return CS_INVALID_REQUEST;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL _CsDrvIsMyBoardType( uInt32 u32BoardType )
{

	if( u32BoardType >= CSHEXAGON_FIRST_BOARD && u32BoardType <= CSHEXAGON_LAST_BOARD )
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
	// it is safe using static variable to here
	static	CSHANDLE DrvHdl = 0;
	switch (uMsg)
	{
		case WM_INITDIALOG: 
		{
			DrvHdl = (CSHANDLE) lParam;

			CS_TRIG_OUT_STRUCT	TrigOut;
			CS_CLOCK_OUT_STRUCT	ClockOut;

			CsDrvGetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
			CsDrvGetParams(DrvHdl, CS_CLOCK_OUT, &ClockOut);

			int i;

			for(i = 0; i< sizeof(ValidTrigOut)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_TRIGOUT, CB_ADDSTRING, 0, (LPARAM)ValidTrigOut[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_TRIGOUT, CB_SETITEMDATA, i, (LPARAM)ValidTrigOut[i].u32Ctrl);

				if ( (TrigOut.u16Mode & CS_TRIGOUT_MODE) != 0 )
				{
					if (TrigOut.u16Value == ValidTrigOut[i].u32Ctrl)
						::SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidTrigOut[i].Str);
				}
			}

			for(i = 0; i< sizeof(ValidClkOut)/sizeof(VALID_AUX_CONFIG); i++)
			{
				::SendDlgItemMessage(hwndDlg, IDC_CLOCKOUT, CB_ADDSTRING, 0, (LPARAM)ValidClkOut[i].Str);
				::SendDlgItemMessage(hwndDlg, IDC_CLOCKOUT, CB_SETITEMDATA, i, (LPARAM)ValidClkOut[i].u32Ctrl);

				if (ClockOut.u16Value == ValidClkOut[i].u32Ctrl)
					::SendDlgItemMessage (hwndDlg, IDC_CLOCKOUT, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)ValidClkOut[i].Str);
			}

			if ( TrigOut.u16Mode == CS_EXTTRIG_EN_MODE )
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRIGOUT), FALSE);
				::SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_SETCURSEL, 0, 0);
				::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN_SELECT, BM_SETCHECK, 1, 0);
			}
			else 
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), FALSE);
				::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_SETCURSEL, 0, 0);
				::SendDlgItemMessage (hwndDlg, IDC_TRIGOUT_SELECT, BM_SETCHECK, 1, 0);
			}

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_TRIGOUT_SELECT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIGOUT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), FALSE);
					::SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_SETCURSEL, 0, 0);
					break;

				case IDC_EXTTRIG_EN_SELECT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_EXTTRIG_EN), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIGOUT), FALSE);
					::SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_SETCURSEL, 0, 0);
					break;
				case IDOK:
				{
					CS_TRIG_OUT_STRUCT	TrigOut = {0};
					CS_CLOCK_OUT_STRUCT	ClockOut = {0};
					int32 nIndex;

					if ( SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN_SELECT, BM_GETCHECK, 0, 0) )
					{
						TrigOut.u16Mode = CS_EXTTRIG_EN_MODE;
						nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_GETCURSEL, 0, 0);
						if( CB_ERR != nIndex )
						{
							TrigOut.u16Value =  (int16)(SendDlgItemMessage (hwndDlg, IDC_EXTTRIG_EN, CB_GETITEMDATA, (WPARAM)nIndex, 0));
							if( CB_ERR != TrigOut.u16Value )
								CsDrvSetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
						}
					}
					else
					{
						TrigOut.u16Mode = CS_TRIGOUT_MODE;
						nIndex = (int32) SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_GETCURSEL, 0, 0);
						if( CB_ERR != nIndex )
						{
							TrigOut.u16Value =  (int16)(SendDlgItemMessage (hwndDlg, IDC_TRIGOUT, CB_GETITEMDATA, (WPARAM)nIndex, 0));
							if( CB_ERR != TrigOut.u16Value )
								CsDrvSetParams(DrvHdl, CS_TRIG_OUT, &TrigOut);
						}
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
	CsHexagon	*pDev = (CsHexagon *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

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
	CsHexagon	*pDev = (CsHexagon *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

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
	CsHexagon	*pDev = (CsHexagon *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

	if ( NULL == pDev )
		return CS_INVALID_HANDLE;

	if ( 0 == u32TransferSize )
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
	CsHexagon	*pDev = (CsHexagon *) g_pBaseHwDll->GetAcqSystemCardPointer( DrvHdl );

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
int32	CsDrvGetMaxPreTrigger( DRVHANDLE, uInt32* pBuffer,  uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if ( (NULL == BufferSize) || (NULL == pBuffer) )
		i32Status = CS_INVALID_PARAMETER;
	else
	{
		*BufferSize = sizeof(uInt32);
		*pBuffer = (uInt32)MAX_PRETRIGGERDEPTH_SINGLE ;			// Single channel
	}

	return i32Status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetMaxStreamSegmentSize( DRVHANDLE DrvHdl, int64* pBuffer,  uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if ( (NULL == BufferSize) || (NULL == pBuffer) )
		i32Status = CS_INVALID_PARAMETER;
	else
	{
		*BufferSize = sizeof(int64);
		CsDrvGetParams(DrvHdl, CAPS_STM_MAX_SEGMENTSIZE, pBuffer);
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetDepthIncrement( DRVHANDLE DrvHdl, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if ( (NULL == BufferSize) || (NULL == pBuffer) )
		i32Status = CS_INVALID_PARAMETER;
	else
	{
		*BufferSize = sizeof(uInt32);
		CsDrvGetParams(DrvHdl, CAPS_DEPTH_INCREMENT, pBuffer);
	}

	return i32Status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int32	CsDrvGetTriggerDelayIncrement( DRVHANDLE, uInt32 *pBuffer,  uInt32 *BufferSize )
{
	int32 i32Status = CS_SUCCESS;

	if ( (NULL == BufferSize) || (NULL == pBuffer) )
		i32Status = CS_INVALID_PARAMETER;
	else if ( sizeof(uInt32) > *BufferSize )
		i32Status = CS_INVALID_STRUCT_SIZE;
	else
	{
		*BufferSize = sizeof(uInt32);
		*pBuffer = (uInt32)(HEXAGON_DELAY_INCREMENT * 2);		// Single channel
	}

	return i32Status;
}