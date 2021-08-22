
// CsHexagonDbg.cpp : Debug info.
//

#include "StdAfx.h"
#include "CsTypes.h"
#include "GageWrapper.h"
#include "CsHexagon.h"
#include "CsHexagonCal.h"
#ifdef _WINDOWS_
#include "CsMsgLog.h"
#endif

static FILE* DbgFile = NULL;

/////////////////////////////////////////////////////////////////////////////
//	For Debugging purpose.
//  we use first 8 bits Address for option (4 bits for channel index, 4 bits for menu selected),
//  32 bits data as Params
//
////////////////////////////////////////////////////////////////////////////
int32 CsHexagon::DbgMain( uInt32 Options, uInt32 Params )
{
	int32 i32Ret = CS_SUCCESS;
	int32 Avg_x10 = 0;

	uInt16 u16Channel = ((Options >> 4) & 0xF) + 1; // convert to hw channel. index start from 1.
	uInt16 u16MenuSelected = (Options  & 0xF);

	if ( u16Channel > HEXAGON_CHANNELS)
		return CS_INVALID_PARAMETER;

	switch (u16MenuSelected)
	{
		case 0:
			DbgShowHelp();
			break;
		case 1:
			DbgShowCalibInfo();
			break;
		case 2:
			DbgCalibRegShow();
			break;
		case 3:
			DbgTriggerCfgShow();
			break;
		case 4:
			DbgDumpAddonReg();
			break;
		case 5:
			DbgDumpNiosReg();
			break;
		case 7:
			{	
				// Force calibration
				for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
				{
					uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
					m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
				}
				CalibrateAllChannels(true);
			}
			break;
		case 8:
			DbgAcquireAndAverage(u16Channel, Params );
			break;
		case 9:
			// Read calib ADC reference (ADS8681)
			ReadCalibADCRef(&Avg_x10);
			break;
		case 10:
			// Dump Calib Table: Params format.
			// First 8 bits
			//       -4 bits: ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
			//       -4 bits: Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
			// Second 8 bits: Gain code value (0-20h)
			// Ex: 0x0901	Gain=9, Source Ref=HEXAGON_CAL_SRC_0V, ecdId=ecdGain
			DumpCalibAvgTable(u16Channel, Params);
			break;
		case 11:
			// Dump Calib Table Zoom: Params format.
			// First 8 bits
			//       -4 bits: ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
			//       -4 bits: Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
			// Second 8 bits: Gain code value (0-20h)
			// Ex: 0x07f80901	Gain=9, Source Ref=HEXAGON_CAL_SRC_0V, ecdId=ecdPosition, ZoomAddress = 0x7f8 
			DbgAvgTableZoom(u16Channel, Params);
			break;
		case 12:
			// Dump Gain Table
			if (Params==0)
				DumpAverageTable(u16Channel, ecdGain, HEXAGON_CAL_SRC_0V, HEXAGON_NO_GAIN_SETTING, 1, 32);
			if (Params==1)
				DumpAverageTable(u16Channel, ecdGain, HEXAGON_CAL_SRC_POS, HEXAGON_NO_GAIN_SETTING, 1, 32);
			if (Params==2)
				DumpAverageTable(u16Channel, ecdGain, HEXAGON_CAL_SRC_NEG, HEXAGON_NO_GAIN_SETTING, 1, 32);
			break;
		case 13:
			{
				// Dump Coarse Table
				uInt16 u16DcGain = HEXAGON_DC_GAIN_CODE_1V;
				if (m_bLowRange_mV)
					u16DcGain = HEXAGON_DC_GAIN_CODE_LR;
				if (Params==0)
					DumpAverageTable(u16Channel, ecdPosition, HEXAGON_CAL_SRC_0V, u16DcGain, 1, 32);
				if (Params==1)
					DumpAverageTable(u16Channel, ecdPosition, HEXAGON_CAL_SRC_POS, u16DcGain, 1, 32);
				if (Params==2)
					DumpAverageTable(u16Channel, ecdPosition, HEXAGON_CAL_SRC_NEG, u16DcGain, 1, 32);
				break;
			}
		case 14:
			{
				// Dump fine Table
				uInt16 u16DcGain = HEXAGON_DC_GAIN_CODE_1V;
				if (m_bLowRange_mV)
					u16DcGain = HEXAGON_DC_GAIN_CODE_LR;
				if (Params==0)
					DumpAverageTable(u16Channel, ecdOffset, HEXAGON_CAL_SRC_0V, u16DcGain, 1, 32);
				if (Params==1)
					DumpAverageTable(u16Channel, ecdOffset, HEXAGON_CAL_SRC_POS, u16DcGain, 1, 32);
				if (Params==2)
					DumpAverageTable(u16Channel, ecdOffset, HEXAGON_CAL_SRC_NEG, u16DcGain, 1, 32);
				break;
			}
		case 15:
			DbgCalibPosition(u16Channel);
			break;
		default:
			break;
	}
	return i32Ret;

}

void CsHexagon::DbgShowHelp()
{
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("=================================================================\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Use <address and data> as <FFxm Params> to manipulate a list of \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("debug calibration where channel+option input in address field, \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("params is data field\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Format:  FF=debug c=channel index m=option, \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("         Params depend on Function selected \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("List of debug options: \n--------------------------------------------------------\n\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF00>            \t\t\t\tShow Help\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF01>            \t\t\t\tShow Calib Info\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF02>            \t\t\t\tShow Calib Regs\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF03>            \t\t\t\tShow Trigger CFG\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF04>            \t\t\t\tShow CPLD Addon Reg\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF05>            \t\t\t\tShow Nios Reg\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF06>            \t\t\t\tShow NiosSPI Reg\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF07>            \t\t\t\tForce Calibrate All Channels\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FF08     F>      \t\t\t\tDbgAcquireAndAverage\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFx9>            \t\t\t\tReadCalibADCRef\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFxA    0000ggse>\t\t\t\tDumpCalibAvgTable\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFxB    zooogges>\t\t\t\tDbgAvgTableZoom\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFxC    s>       \t\t\t\tShow Gain Table\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFxD    s>       \t\t\t\tShow Coarse Table\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFxE    s>       \t\t\t\tShow Fine Table\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFcF>            \t\t\t\tCalibrate Position\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Parameters Reference:		\n---------------------------------------------------------\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           c:        Channel number (zero base) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("          gg:        Gain control (0-20h)\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("       ss, s:        Source Ref 0:0V, 1:VRef+, 2:Vref- \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           e:        ecdId: 0:Offset, 1: Position, 3:Gain \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("         ooo:        Offset (000-FFF), if ofset=0, use 0x800 \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           z:        Zoom scale (0=X1, 1=X2, 2=X4...) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("=================================================================\n")); 
	OutputDebugString(szDbgText);
}

// FExx Menu
int32 CsHexagon::DbgAdvMain( uInt32 Options, uInt32 Params )
{

	uInt16 u16Channel = ((Options >> 4) & 0xF) + 1; // convert to hw channel. index start from 1.
	uInt16 u16MenuSelected = (Options  & 0xF);

	if ( u16Channel > HEXAGON_CHANNELS)
		return CS_INVALID_PARAMETER;

	switch (u16MenuSelected)
	{
		case 0:
		default:
			DbgAdvShowHelp();
			break;
		case 1:
			DbgAdvAvgTable(u16Channel, Params);
			break;
		case 2:
			DbgAdvZeroOffset(u16Channel, Params);
			break;
		case 3:
			DbgAdvDCOffset(u16Channel, Params);
			break;
		case 4:
			DbgBoardSummary(Params);
			break;
		case 5:
			DbgGainBoundary();
			break;
		case 6:
			DbgRecalibrate(Params);			// ReCalibrate
			break;
		case 7:
			ZoomInMaxFromOffset(u16Channel, Params);			// Zoom In Max
			break;
		case 8:
			{
				CSBOARDINFO	CsBoardInfo = {0};
				DbgGetBoardsInfo( &CsBoardInfo );
				break;	
			}
	}

	return CS_SUCCESS;
}

void CsHexagon::DbgAdvShowHelp()
{
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("=================================================================\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Use <address and data> as <FFxm Params> to manipulate a list of \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("debug calibration where channel+option input in address field, \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("params is data field\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Format:  FF=debug c=channel index m=option, \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("         Params depend on Function selected \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("List of debug options: \n--------------------------------------------------------\n\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FE00>            \t\t\t\tShow Advance Menu\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc1    tsggse>  \t\t\t\tShow Advance Coarse Table\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc2    (0:1)>	 \t\t\t\tShow Advance Zero Table (0:Fine, 1: Coarse)\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc3   >		  \t\t\t\tShow Advance DC Offset\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc4   >		  \t\t\t\tBoard Calib Summary \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc5   >		  \t\t\t\tGain Boundary \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FEc6   >		  \t\t\t\tRecalibrate \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("<FFc7    zooogges>\t\t\t\tZoomInMaxFromOffset\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           c:        4bits Channel number (zero base) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("          gg:        8bits Gain control (0-20h). Default 0 = gain 8\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("       ss, s:        4bits Source Ref 0:0V, 1:VRef+, 2:Vref- \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           e:        ecdId: 0:Offset(fine), 1: Position(coarse), 2:Gain \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("          ts:        8bits Table size. Default = 32 \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("         ooo:        Offset (000-FFF), if ofset=0, use 0x800 \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("           z:        Zoom scale (0=X1, 1=X2, 2=X4...) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("=================================================================\n")); 
	OutputDebugString(szDbgText);
}

int32 CsHexagon::DbgShowCalibInfo()
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 i = 0;

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Coarse/Fine Max val: %d Gain Max Val %d \n"), c_u32CalDacCount, HEXAGON_DC_NB_GAIN_CONTROL); 
	DumpOutputMsg(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText),  "\n============= Calibration Setting =============== \n\n");
	DumpOutputMsg( szDbgText );

	for (i=1;i<=m_PlxData->CsBoard.NbAnalogChannel ;i++)
	{
		uInt32 u32RangeIndex, u32FeModIndex;
		uInt16 u16ChanZeroBased = i-1;
		uInt16 u16CoarseOffset = 0;
		uInt16 u16FineOffset = 0;
		uInt16 u16Gain = 0;
		uInt16 u16DeltaHaftIR = 0;
		int32 i32Position = 0;
		double fVolts = 0;
		//uInt32 u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
		i32Ret = FindFeCalIndices( i, u32RangeIndex, u32FeModIndex );
		if( CS_FAILED(i32Ret) )
			return i32Ret;

		u16CoarseOffset = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		u16FineOffset = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset;
		u16Gain = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16Gain;
		i32Position = m_pCardState->ChannelParams[u16ChanZeroBased].i32Position;
		u16DeltaHaftIR = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR;
		fVolts = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].fVoltOff;

		sprintf_s( szDbgText, sizeof(szDbgText),  " === Channel[%d]. ====\n",	i);
		DumpOutputMsg( szDbgText );

		sprintf_s( szDbgText, sizeof(szDbgText),  "       Coarse %5d (0x%03x)      Fine  %5d (0x%03x).\n",
				u16CoarseOffset,
				u16CoarseOffset,
				u16FineOffset,
				u16FineOffset);
		DumpOutputMsg( szDbgText );
		sprintf_s( szDbgText, sizeof(szDbgText),  "       Gain   %5d (0x%03x)   Position %5d (0x%03x).\n",
				u16Gain,
				u16Gain,
				u16DeltaHaftIR,
				u16DeltaHaftIR);
		DumpOutputMsg( szDbgText );

		sprintf_s( szDbgText, sizeof(szDbgText),  "       Real DC offset meas. ( %.3fmV) \n\n",	fVolts);
		DumpOutputMsg( szDbgText );
	}

	return i32Ret;
}

void CsHexagon::DbgCalibRegShow()
{
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("\n====== List of Calib Registers ========\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("0x0810201  (1,4,8)		Ref Voltage Seletc (1=0v,4=Vref+,8=Vref-)\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("0x0810301  (0, F)		 Calib Mode switching (0:DC, F:Normal) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("0x08102x2  (0-20h)		Gain Control - Channel x (Zero base channel)\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("0x0810x06  (0-FFF)		Offset Control - Channel x (zero base) \n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("                         		0:2:4:6 Coarse, 1:3:5:7 Fine\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("0x0800008				Read Ref - Channel x (Zero base channel)\n")); 
	OutputDebugString(szDbgText);
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------
int32 CsHexagon::DbgTriggerCfgShow()
{
	int32 i32Ret = CS_SUCCESS;
	uInt16	uNbTrigEng = 2*GetBoardNode()->NbAnalogChannel +1;
	uInt32 u32RegVal = 0;
	 
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("\n======== Trigger Engines CFG: =========\n\n")); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger Sensitivity: %d\n"), m_u16TrigSensitivity); 
	OutputDebugString(szDbgText);
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Ext Triggger Sensitivity: %d\n"), m_i32ExtTrigSensitivity); 
	OutputDebugString(szDbgText);

	for(int i=0; i<uNbTrigEng; i++)
	{
		PCS_TRIGGER_PARAMS pTrigParams = &m_CfgTrigParams[i]; 
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger[%d]  Source %d Condition %d Level %d \n"), 
			i,
			pTrigParams->i32Source, 
			pTrigParams->u32Condition, 
			pTrigParams->i32Level); 
		OutputDebugString(szDbgText);
	}

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger A1 Level %06d Sensitive %06d  B1 Level %06d Sensitive %06d \n"), 
			m_pCardState->BaseBoardRegs.i16TrigLevelA1,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveA1,
			m_pCardState->BaseBoardRegs.i16TrigLevelB1,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveB1); 
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger A2 Level %06d Sensitive %06d  B2 Level %06d Sensitive %06d \n"), 
			m_pCardState->BaseBoardRegs.i16TrigLevelA2,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveA2,
			m_pCardState->BaseBoardRegs.i16TrigLevelB2,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveB2); 
		OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger A3 Level %06d Sensitive %06d  B4 Level %06d Sensitive %06d \n"), 
			m_pCardState->BaseBoardRegs.i16TrigLevelA3,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveA3,
			m_pCardState->BaseBoardRegs.i16TrigLevelB3,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveB3); 
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Trigger A4 Level %06d Sensitive %06d  B4 Level %06d Sensitive %06d \n"), 
			m_pCardState->BaseBoardRegs.i16TrigLevelA4,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveA4,
			m_pCardState->BaseBoardRegs.i16TrigLevelB4,
			m_pCardState->BaseBoardRegs.u16TrigSensitiveB4); 
	OutputDebugString(szDbgText);

	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_TRIGGER_IO_CONTROL );
	sprintf_s( szDbgText, sizeof(szDbgText), "TRIGGER_IO_CONTROL: [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_TRIGGER_IO_CONTROL,
				u32RegVal);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), "DIGI TRIGGER CONTROL0: [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_DT_CONTROL,
				m_pCardState->BaseBoardRegs.TrigCtrl_Reg.u16RegVal);
	OutputDebugString(szDbgText);
	
	sprintf_s( szDbgText, sizeof(szDbgText), "DIGI TRIGGER CONTROL1: [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_DT_CONTROL +1,
				m_pCardState->BaseBoardRegs.TrigCtrl_Reg_2.u16RegVal);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), "EXT TRIGGER SETTING  : [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_EXT_TRIGGER_SETTING,
	m_pCardState->AddOnReg.u16ExtTrigEnable);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), "EXT TRIGGER LEVEL    : [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_EXTTRIG_DAC | HEXAGON_SPI_WRITE,
	m_pCardState->AddOnReg.u16ExtTrigLevel);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), "HW Trigger Timeout   : [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_BB_TRIGTIMEOUT_REG,
	m_pCardState->BaseBoardRegs.u32TrigTimeOut);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), "TRIGGER OUT DISABLE  : [Addr=0x%08x, data=0x%08x]\n", 
				HEXAGON_TRIGOUT_DIS,
	m_pCardState->bDisableTrigOut);
	OutputDebugString(szDbgText);

	return i32Ret;
}

//------------------------------------------------------------------------------
// Params format	0000 ggse
//       e (4bits) : ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
//       s (4bits) : Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
//       gg (8bits): Gain code value (0-20h)
// Ex: (hex)
//         12   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_NEG 
//         22   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_POS 
//       0901	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_0V,  Gain Control=9
//       0b11	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_NEG, Gain Control=0b
//       0800	ecdId=ecdOffset,   Source Ref=HEXAGON_CAL_SRC_POS, Gain Control=08
//------------------------------------------------------------------------------
int32 CsHexagon::DumpCalibAvgTable(uInt16 u16Channel, uInt32 Params)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16srcRef = (uInt16)( (Params>>4) & 0x0F);
	uInt16 u16Gain = (uInt16) ((Params >> 8) & 0xFF);
	uInt16 u16srcRefMap = HEXAGON_CAL_SRC_0V;

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DumpCalibAvgTable(%d)  u16ecdId %d u16srcRef %d u16Gain %d \n"), 
						u16Channel, u16ecdId, u16srcRef, u16Gain);
	OutputDebugString(szDbgText);

	if ( (u16Gain > 0x20) || (u16ecdId > ecdGain) || (u16srcRef > 2) )
	{
		return(i32Ret);
	}
	switch(u16srcRef)
	{
		case 0: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
		case 1: u16srcRefMap = HEXAGON_CAL_SRC_POS; break;
		case 2: u16srcRefMap = HEXAGON_CAL_SRC_NEG; break;
	}

	DumpAverageTable(u16Channel, (eCalDacId)(u16ecdId), u16srcRefMap, u16Gain, 1, 32);

	return i32Ret;
}

//------------------------------------------------------------------------------
// Params format (32bits total) = zooo ggse
//       e (4bits)   : ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
//       s (4bits)   : Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
//       gg (8bits)  : Gain code value (0-20h)
// Ex: (hex)
//         12   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_NEG 
//         22   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_POS 
//       0901	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_0V,  Gain Control=9
//       0b11	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_NEG, Gain Control=0b
//       0800	ecdId=ecdOffset,   Source Ref=HEXAGON_CAL_SRC_POS, Gain Control=08
//------------------------------------------------------------------------------
int32 CsHexagon::DbgAvgTable(uInt16 u16Channel, uInt32 Params)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16srcRef = (uInt16)( (Params>>4) & 0x0F);
	uInt16 u16Gain = (uInt16) ((Params >> 8) & 0xFF);
	uInt16 u16srcRefMap = HEXAGON_CAL_SRC_0V;

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DbgAvgTable(%d)  u16ecdId %d u16srcRef %d u16Gain %d \n"), 
						u16Channel, u16ecdId, u16srcRef, u16Gain);
	OutputDebugString(szDbgText);

	if ( (u16Gain > 0x20) || (u16ecdId > ecdGain) || (u16srcRef > 2) )
	{
		return(i32Ret);
	}

	if ( (!u16Gain ) && (u16ecdId != ecdGain) )
		u16Gain = HEXAGON_DC_GAIN_CODE_1V;

	switch(u16srcRef)
	{
		case 0: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
		case 1: u16srcRefMap = HEXAGON_CAL_SRC_POS; break;
		case 2: u16srcRefMap = HEXAGON_CAL_SRC_NEG; break;
		default: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
	}

	DumpAverageTable(u16Channel, (eCalDacId)(u16ecdId), u16srcRefMap, u16Gain, 1, 32);

	return i32Ret;
}


//------------------------------------------------------------------------------
// Params format (32bits total) = zooo ggse
//       e (4bits)   : ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
//       s (4bits)   : Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
//       gg (8bits)  : Gain code value (0-20h)
//      ooo (12bits) : DAC Offset value setting
//        z (4bits)  : Zoom factor
// Offset (if Offset=0 then use default 800)
// Ex: (hex)
//         12   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_NEG 
//         22   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_POS 
//       0901	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_0V,  Gain Control=9
//       0b11	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_NEG, Gain Control=0b
//       0800	ecdId=ecdOffset,   Source Ref=HEXAGON_CAL_SRC_POS, Gain Control=08
//  07f4 0900   off=07f4 gain=09 ecdId=ecdOffset srcRef=HEXAGON_CAL_SRC_0V
//------------------------------------------------------------------------------
int32 CsHexagon::DbgAvgTableZoom(uInt16 u16Channel, uInt32 Params)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16srcRef = (uInt16)( (Params>>4) & 0x0F);
	uInt16 u16Gain = (uInt16) ((Params >> 8) & 0xFF);
	uInt16 u16Offset = (uInt16) ((Params >> 16) & 0xFFF);	// DAC 12bits
	uInt16 u16ZoomFactor = (uInt16) ((Params >> 28) & 0xF);	// DAC 12bits
	uInt16 u16srcRefMap = HEXAGON_CAL_SRC_0V;

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DbgAvgTableZoom(%d)  u16ecdId %d u16srcRef %d u16Gain %d u16Offset 0x%x \n"), 
						u16Channel, u16ecdId, u16srcRef, u16Gain, u16Offset);
	OutputDebugString(szDbgText);

	if ( (u16Gain > 0x20) || (u16ecdId > ecdGain) || (u16srcRef > 2) )
	{
		return(i32Ret);
	}
	switch(u16srcRef)
	{
		case 0: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
		case 1: u16srcRefMap = HEXAGON_CAL_SRC_POS; break;
		case 2: u16srcRefMap = HEXAGON_CAL_SRC_NEG; break;
	}

	DumpAverageTable(u16Channel, (eCalDacId)(u16ecdId), u16srcRefMap, u16Gain, 1, 32, true, u16Offset, u16ZoomFactor);

	return i32Ret;
}

//------------------------------------------------------------------------------
// channel input : not use
// Params : channel mask (0xF all channels)
//------------------------------------------------------------------------------
int32 CsHexagon::DbgAcquireAndAverage( uInt16 u16Channel, uInt32 Params )
{
	int32 i32Ret = CS_SUCCESS;
	int32	i32Avg_x10 = 0;
	int32	i32AvgEven_x10 = 0;
	double	fVolt =  0;

	if (!(Params & 0xF))
		return i32Ret;

	if (!m_pi16CalibBuffer)
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );


	for (uInt16 i=0; i< m_PlxData->CsBoard.NbAnalogChannel; i++)
	{
		if ( Params & (1<<i) )
		{
			u16Channel = i+1;
			i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10, &i32AvgEven_x10);
			fVolt =  (-1.0*i32Avg_x10/m_pCardState->AcqConfig.i32SampleRes/10)*1000;

			sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DbgAcquireAndAverage(%d)  i32Avg_x10 = [%d 0x%x] fVolt %lf \n"), 
						u16Channel, i32Avg_x10, i32Avg_x10, fVolt);
			OutputDebugString(szDbgText);
		}
	}

	if (m_pi16CalibBuffer)
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;
	}

	return i32Ret;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int32 CsHexagon::DbgCalibPosition(uInt16 u16Channel)
{
	int32 i32Ret = CS_SUCCESS;
	if( (0 == u16Channel) || (u16Channel > HEXAGON_CHANNELS)  )
		return CS_INVALID_CHANNEL;

	uInt16 u16ChanZeroBased = u16Channel-1;
	int32 i32PosCode = 0;
	uInt32 u32Code = 0;
	uInt32 u32NegPos = 0;

	uInt32 u32InpuRange = m_pCardState->ChannelParams[u16ChanZeroBased].u32InputRange;
	HEXAGON_CAL_LUT CalInfo;
	uInt32 u32CalRange = u32InpuRange;
	bool	bCalibBufAlloc = false;

	//DumpAverageTable(u16Channel, ecdPosition, 0, 0, 32);

	uInt32 u32RangeIndex, u32FeModIndex;
	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = 0;

	// Find the setup for the current input ranges
	i32Ret = FindCalLutItem(u32CalRange, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	// Positive side
	// Find the value of DAC so that we have DC offset at +50%
	int32 i32Target = labs(m_pCardState->AcqConfig.i32SampleRes)/2;		//1/2 gain = SampleRes * 10 /2
	int32 i32Avg_x10 = 0;
	uInt32 u32PosPos = 0, u32PosDelta = c_u32CalDacCount/4;

	// Switch to the Calibmode DC
	i32Ret = SendCalibModeSetting( ecmCalModeDc );
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset = HEXAGON_DC_OFFSET_DEFAULT_VALUE;

	if (!m_pi16CalibBuffer)
	{
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );
		bCalibBufAlloc = true;
	}

	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_0V, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode += u32PosPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt16(i32PosCode) : c_u32CalDacCount-1 ) : 0;
		
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, (uInt16)u32Code, false );

		if( CS_FAILED(i32Ret) )	goto EndDbgCalibPosition;
		BBtiming( 100 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	goto EndDbgCalibPosition;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10/10, i32Target*10);
		if( Abs(i32Avg_x10/10) < Abs(i32Target) )
			u32PosPos |= u32PosDelta;
		u32PosDelta >>= 1;

		sprintf_s( szDbgText, sizeof(szDbgText), "DbgCalibPos(%d) Pos side. u32Code 0x%x i32Avg [%d, 0x%x] i32PosCode 0x%x \n", 
				u16Channel,
				u32Code,
				i32Avg_x10/10,
				i32Avg_x10/10,
				i32PosCode);
		OutputDebugString(szDbgText);
	}
	
	if( 0 == u32PosPos || (c_u32CalDacCount - 1) == u32PosPos )
	{
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );
		sprintf_s( szDbgText, sizeof(szDbgText),  "CalibratePosition failed! Positive side. channel %d\n", u16Channel);
		OutputDebugString(szDbgText);
		i32Ret = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
		goto EndDbgCalibPosition;
		//return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
	}

	// Negative side
	// Find the value of DAC so that we have DC offset at -25%
	i32Target = m_pCardState->AcqConfig.i32SampleRes/2;		//1/2 gain = SampleRes * 10 /4
	u32PosDelta = c_u32CalDacCount/4;

	while( u32PosDelta > 0 )
	{
		i32PosCode = m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset;
		i32PosCode -= u32NegPos|u32PosDelta;
		u32Code = i32PosCode > 0 ? ( i32PosCode < int32(c_u32CalDacCount-1) ? uInt32(i32PosCode) : c_u32CalDacCount-1 ) : 0;

		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, (uInt16)u32Code, false );
		if( CS_FAILED(i32Ret) )	goto EndDbgCalibPosition;

		BBtiming( 100 );

		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )	goto EndDbgCalibPosition;

		TraceCalib( u16Channel, TRACE_CAL_ITER, u32Code, TRACE_CAL_POS, i32Avg_x10/10, i32Target*10);
		if( Abs(i32Avg_x10/10) < Abs(i32Target) )
			u32NegPos |= u32PosDelta;
		u32PosDelta >>= 1;

		sprintf_s( szDbgText, sizeof(szDbgText), "DbgCalibNeg(%d) Neg side. u32Code 0x%x i32Avg [%d, 0x%x] i32PosCode 0x%x \n", 
				u16Channel,
				u32Code,
				i32Avg_x10/10,
				i32Avg_x10/10,
				i32PosCode);
		OutputDebugString(szDbgText);
	}
	
	if( 0 == u32NegPos || (c_u32CalDacCount - 1) == u32NegPos )
	{
		// Write default value.
		i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );
		sprintf_s( szDbgText, sizeof(szDbgText),  "CalibratePosition failed! Negative side. channel %d\n", u16Channel);
		OutputDebugString(szDbgText);
		//return CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
		i32Ret = CS_POSITION_CAL_FAILED - CS_ERROR_FORMAT_THRESHOLD * ConvertToUserChannel(u16Channel);
		goto EndDbgCalibPosition;

	}
	
EndDbgCalibPosition:
	if (bCalibBufAlloc && m_pi16CalibBuffer)
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;
	}

	i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );

	m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR = (uInt16) (u32PosPos + u32NegPos)/2;
	
	sprintf_s( szDbgText, sizeof(szDbgText), "DbgCalibPos(%d) result. u16CodeDeltaForHalfIR [%d] u32PosPos 0x%x u32NegPos 0x%x \n", 
				u16Channel,
				m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CodeDeltaForHalfIR,
				u32PosPos,
				u32NegPos);
	OutputDebugString(szDbgText);

	SendCalibModeSetting();

	TraceCalib( u16Channel, TRACE_CAL_POS, u32PosPos, u32NegPos);

	return i32Ret;
}

int32 CsHexagon::DbgDumpAddonReg()
{
	int32 i32Ret = CS_SUCCESS;

	uInt32 u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_MISC_STATUS );
	sprintf_s( szDbgText, sizeof(szDbgText), "\n ---CPLD ADD ON REGISTERS----\n");
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " CPLD_MISC_STATUS  :	[0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_MISC_STATUS,
				u32RegVal);
	OutputDebugString(szDbgText);
	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_EXTCLOCK_REF );
	sprintf_s( szDbgText, sizeof(szDbgText), " CPLD_EXTCLOCK_REF : [0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_EXTCLOCK_REF,
				u32RegVal);
	OutputDebugString(szDbgText);

	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_CALSRC_REF );
	sprintf_s( szDbgText, sizeof(szDbgText), " CPLD_CALSRC_REF   : [0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_CALSRC_REF,
				u32RegVal);
	OutputDebugString(szDbgText);
	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_CALSEL_CHAN );
	sprintf_s( szDbgText, sizeof(szDbgText), " CPLD_CALSEL_CHAN  : [0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_CALSEL_CHAN,
				u32RegVal);
	OutputDebugString(szDbgText);
	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_TRIGGER_IO_CONTROL );
	sprintf_s( szDbgText, sizeof(szDbgText), " TRIGGER_IO_CONTROL: [0x%08x, 0x%08x]\n", 
				HEXAGON_TRIGGER_IO_CONTROL,
				u32RegVal);
	OutputDebugString(szDbgText);
	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_FW_DATE );
	sprintf_s( szDbgText, sizeof(szDbgText), " FW_DATE           : [0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_FW_DATE,
				u32RegVal);
	OutputDebugString(szDbgText);
	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_ADDON_CPLD_FW_VERSION );
	sprintf_s( szDbgText, sizeof(szDbgText), " FW_VERSION        : [0x%08x,	0x%08x]\n", 
				HEXAGON_ADDON_CPLD_FW_VERSION,
				u32RegVal);
	OutputDebugString(szDbgText);

	return i32Ret;
}

int32 CsHexagon::DbgDumpNiosReg()
{
	int32 i32Ret = CS_SUCCESS;
	uInt32 u32RegVal = 0;

	sprintf_s( szDbgText, sizeof(szDbgText), "\n ========== NIOS REGISTERS ============ \n\n"); 
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " MULREC_AVG_COUNT   : [0x%08x,	0x%08x]\n", 
				HEXAGON_SET_MULREC_AVG_COUNT,
				m_pCardState->BaseBoardRegs.u32AverageCount);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG TIMEOUT       : [0x%08x,	0x%08x]\n", 
				HEXAGON_BB_TRIGTIMEOUT_REG,
				m_pCardState->BaseBoardRegs.u32TrigTimeOut);
	OutputDebugString(szDbgText);

	u32RegVal = ReadRegisterFromNios( 0, HEXAGON_BB_TRIGTIMEOUT_REG );

	sprintf_s( szDbgText, sizeof(szDbgText), " FPGA TEMPERATURE    : [0x%08x,	0x%08x]\n", 
				HEXAGON_BB_FPGA_TEMPERATURE,
				u32RegVal);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIGGER OUT DISABLE: [0x%08x,	0x%08x]\n", 
				HEXAGON_TRIGOUT_DIS,
				m_pCardState->bDisableTrigOut);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " EXT TRIG SETTING   : [0x%08x,	0x%08x]\n", 
				HEXAGON_EXT_TRIGGER_SETTING,
				m_pCardState->AddOnReg.u16ExtTrigEnable);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " CLOCK DECIMATION   : [0x%08x,	0x%08x]\n", 
				HEXAGON_DECIMATION_SETUP,
				m_pCardState->AddOnReg.u32Decimation);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " DIGI TRIG CONTROL0 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DT_CONTROL,
				m_pCardState->BaseBoardRegs.TrigCtrl_Reg.u16RegVal);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " DIGI TRIG CONTROL1 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DT_CONTROL + 1,
				m_pCardState->BaseBoardRegs.TrigCtrl_Reg.u16RegVal);
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 LEVEL A1 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DLA1,
				(m_pCardState->BaseBoardRegs.i16TrigLevelA1 & 0xFFFF) | (m_pCardState->BaseBoardRegs.i16TrigLevelA3 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 SENSIT A1: [0x%08x,	0x%08x]\n", 
				HEXAGON_DSA1,
				(m_pCardState->BaseBoardRegs.u16TrigSensitiveA1 & 0xFFFF) | (m_pCardState->BaseBoardRegs.u16TrigSensitiveA3 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 LEVEL B1 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DLB1,
				(m_pCardState->BaseBoardRegs.i16TrigLevelB1 & 0xFFFF) | (m_pCardState->BaseBoardRegs.i16TrigLevelB3 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 SENSIT B1: [0x%08x,	0x%08x]\n", 
				HEXAGON_DSB1,
				(m_pCardState->BaseBoardRegs.u16TrigSensitiveB1 & 0xFFFF) | (m_pCardState->BaseBoardRegs.u16TrigSensitiveB3 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 LEVEL A2 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DLA2,
				(m_pCardState->BaseBoardRegs.i16TrigLevelA2 & 0xFFFF) | (m_pCardState->BaseBoardRegs.i16TrigLevelA2 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 SENSIT A2: [0x%08x,	0x%08x]\n", 
				HEXAGON_DSA2,
				(m_pCardState->BaseBoardRegs.u16TrigSensitiveA2 & 0xFFFF) | (m_pCardState->BaseBoardRegs.u16TrigSensitiveA4 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 LEVEL B2 : [0x%08x,	0x%08x]\n", 
				HEXAGON_DLB2,
				(m_pCardState->BaseBoardRegs.i16TrigLevelB2 & 0xFFFF) | (m_pCardState->BaseBoardRegs.i16TrigLevelB4 << 16) );
	OutputDebugString(szDbgText);

	sprintf_s( szDbgText, sizeof(szDbgText), " TRIG ENG1 SENSIT B2: [0x%08x,	0x%08x]\n\n", 
				HEXAGON_DSB2,
				(m_pCardState->BaseBoardRegs.u16TrigSensitiveB2 & 0xFFFF) | (m_pCardState->BaseBoardRegs.u16TrigSensitiveB4 << 16) );
	OutputDebugString(szDbgText);

	return i32Ret;

}

//------------------------------------------------------------------------------
// Params format (32bits total) = tsggse
//       e (4bits)   : ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
//       s (4bits)   : Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
//       gg (8bits)  : Gain code value (0-20h)
//       ts (8bits)  : Table size 16-256
// Ex: (hex)
//         12   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_NEG 
//         22   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_POS 
//       0901	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_0V,  Gain Control=9
//       0b11	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_NEG, Gain Control=0b
//       0800	ecdId=ecdOffset,   Source Ref=HEXAGON_CAL_SRC_POS, Gain Control=08
//------------------------------------------------------------------------------
int32 CsHexagon::DbgAdvAvgTable(uInt16 u16Channel, uInt32 Params)
{
	int32 i32Ret = CS_SUCCESS;
	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16srcRef = (uInt16)( (Params>>4) & 0x0F);
	uInt16 u16Gain = (uInt16) ((Params >> 8) & 0xFF);
	uInt16 u16Ts = (uInt16) ((Params >> 16) & 0xFF);
	uInt16 u16srcRefMap = HEXAGON_CAL_SRC_0V;

	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("DbgAdvAvgTable(%d)  u16ecdId %d u16srcRef %d u16Gain %d u16Ts %d \n"), 
						u16Channel, u16ecdId, u16srcRef, u16Gain, u16Ts);
	OutputDebugString(szDbgText);

	// no parameter, use default. Dump DC Coarse table 64 values, Vref = 0v;
	if (Params==0)
	{
		u16ecdId = ecdPosition;
		u16Ts = 64;
		u16srcRef = 0;
		if (IsBoardLowRange())
			u16Gain = HEXAGON_DC_GAIN_CODE_LR;
		else
			u16Gain = HEXAGON_DC_GAIN_CODE_1V;
	}

	// Validate parameters
	if ( (u16Gain > 0x20) || (u16ecdId > ecdGain) || (u16srcRef > 2) )
	{
		return(i32Ret);
	}

//	if ( (!u16Gain ) && (u16ecdId != ecdGain) )
//		u16Gain = HEXAGON_DC_GAIN_CODE_1V;

	if ( (u16Ts  <=8 ) || ( u16Ts > (HEXAGON_DB_MAX_TABLE_SIZE_DMP -1)) )
		u16Ts = 32;

	switch(u16srcRef)
	{
		case 0: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
		case 1: u16srcRefMap = HEXAGON_CAL_SRC_POS; break;
		case 2: u16srcRefMap = HEXAGON_CAL_SRC_NEG; break;
		default: u16srcRefMap = HEXAGON_CAL_SRC_0V; break;
	}

	DumpAverageTable(u16Channel, (eCalDacId)(u16ecdId), u16srcRefMap, u16Gain, 1, u16Ts);

	return i32Ret;
}

// Dump DC Offset -100%, -75%, -50%, -25%, 0%, 25%, 50%, 75%, 100%
int32 CsHexagon::DbgAdvDCOffset(uInt16 u16Channel, uInt32 Params)
{
	int32	i32Ret = CS_SUCCESS;
	int16	VoltagePos[]= {-1000,-750,-500,-250,0,250,500,750,1000};
	int16	VoltagePosLR[]= {-HEXAGON_GAIN_LR_MV/2,-HEXAGON_GAIN_LR_MV*3/8,-HEXAGON_GAIN_LR_MV/4,-HEXAGON_GAIN_LR_MV/8,0,
		HEXAGON_GAIN_LR_MV/8, HEXAGON_GAIN_LR_MV/4 ,HEXAGON_GAIN_LR_MV*3/8,HEXAGON_GAIN_LR_MV/2};
	int16	i = 0;
	int32	i32Avg_x10 = 0;
	double	fVoltCalib[16]={0};
	double	fVolt[16]={0};
	int16	*pVoltage = VoltagePos;
	int32	inputRange = 1000;

	UNREFERENCED_PARAMETER(Params);

	if (!m_pi16CalibBuffer)
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );

	SendCalibModeSetting( ecmCalModeDc );
	BBtiming( 10 );

	if (IsBoardLowRange())
	{
		pVoltage = VoltagePosLR;
		inputRange = HEXAGON_GAIN_LR_MV/2;
	}

	for ( i=0; i< (sizeof(VoltagePos)/sizeof(int16)); i++ )
	{
		i32Ret = SendPosition(u16Channel, pVoltage[i]);
		if( CS_FAILED(i32Ret) )
			break;
		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			break;

		fVoltCalib[i] =  (-1.0*i32Avg_x10/m_pCardState->AcqConfig.i32SampleRes/10)*inputRange;
	}

	SendCalibModeSetting();
	BBtiming( 10 );

	for ( i=0; i< (sizeof(VoltagePos)/sizeof(int16)); i++ )
	{
		i32Ret = SendPosition(u16Channel, pVoltage[i]);
		if( CS_FAILED(i32Ret) )
			break;
		i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
		if( CS_FAILED(i32Ret) )
			break;

		fVolt[i] =  (-1.0*i32Avg_x10/m_pCardState->AcqConfig.i32SampleRes/10)*inputRange;
	}

	DbgRestoreCalibSetting(u16Channel);
	BBtiming( 10 );

	if (m_pi16CalibBuffer)
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;
	}
	sprintf_s( szDbgText, sizeof(szDbgText), TEXT("\n============ [%d] DC Offset Mesured ================================ \n"),
						u16Channel);
	DumpOutputMsg(szDbgText);
	sprintf_s(szDbgText, sizeof(szDbgText), TEXT("\nOffset(mv)     DC Calib(mv)     DC (mv)          Delta(mv) \n"));
	DumpOutputMsg(szDbgText);
	sprintf_s(szDbgText, sizeof(szDbgText), TEXT("=============================================================== \n"));
	DumpOutputMsg(szDbgText);

	for ( i=0; i< (sizeof(VoltagePos)/sizeof(int16)); i++ )
	{
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("%05d        %05.05lf         %05.05lf         %lf \n"), 
						pVoltage[i], fVoltCalib[i], fVolt[i], fVolt[i] - fVoltCalib[i]);
		DumpOutputMsg(szDbgText);
	}
	return i32Ret;
}

int32 CsHexagon::DbgRestoreCalibSetting(uInt16 u16Channel)
{
	uInt32 u32RangeIndex, u32FeModIndex;
	int32 i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	if( CS_FAILED(i32Ret) )	return i32Ret;
	uInt16 u16ChanZeroBased = u16Channel - 1;

	// Restore 0V Vref
	i32Ret = WriteRegisterThruNios( HEXAGON_CAL_SRC_0V, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
	BBtiming( 10 );

	i32Ret = UpdateCalibDac(u16Channel, ecdPosition, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16CoarseOffset);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = UpdateCalibDac(u16Channel, ecdOffset, m_pCardState->DacInfo[u16ChanZeroBased][u32FeModIndex][u32RangeIndex].u16FineOffset);
	if( CS_FAILED(i32Ret) )	return i32Ret;

	i32Ret = SendPosition( u16Channel, m_pCardState->ChannelParams[u16ChanZeroBased].i32Position );
	if( CS_FAILED(i32Ret) )	return i32Ret;

	return CS_SUCCESS;

}

int32 CsHexagon::DumpOutputMsg(char *szMsg)
{
	int32	i32Ret = CS_SUCCESS;
	if (DbgFile)
		fwrite( szMsg, 1, strlen(szMsg), DbgFile);
	else
		OutputDebugString(szMsg);

	return i32Ret;
}

int32 CsHexagon::DbgGainBoundary()
{
	int32	i32Ret = CS_SUCCESS;
	uInt32 u32RangeIndex[4], u32FeModIndex[4];
	uInt16 u16UserChan = 0;
	uInt16 u16GainMax = 0;
	uInt16 u16GainMin = 0x20;

	for( u16UserChan = 1; u16UserChan <= m_PlxData->CsBoard.NbAnalogChannel; u16UserChan += GetUserChannelSkip() )
	{
		FindFeCalIndices( u16UserChan, u32RangeIndex[u16UserChan-1], u32FeModIndex[u16UserChan-1] );
		uInt16 u16GainTmp = m_pCardState->DacInfo[u16UserChan-1][u32FeModIndex[u16UserChan-1]][u32RangeIndex[u16UserChan-1]].u16Gain;
		if ( u16GainMax < u16GainTmp )
			u16GainMax = u16GainTmp;
		if ( u16GainMin > u16GainTmp )
			u16GainMin = u16GainTmp;
	}

	sprintf_s( szDbgText, sizeof(szDbgText), "\n====Gain Boundary Max=%d, Min=%d Delta=%d ===\n", u16GainMax, u16GainMin, u16GainMax - u16GainMin);
	DumpOutputMsg(szDbgText);

	if( m_PlxData->CsBoard.NbAnalogChannel == 4)
		sprintf_s( szErrorStr, sizeof(szErrorStr), "Gain code list [ %02d, %02d, %02d, %02d ] \n", 
						m_pCardState->DacInfo[0][u32FeModIndex[0]][u32RangeIndex[0]].u16Gain,
						m_pCardState->DacInfo[1][u32FeModIndex[1]][u32RangeIndex[1]].u16Gain,
						m_pCardState->DacInfo[2][u32FeModIndex[2]][u32RangeIndex[2]].u16Gain,
						m_pCardState->DacInfo[3][u32FeModIndex[3]][u32RangeIndex[3]].u16Gain);
	else
		sprintf_s( szErrorStr, sizeof(szErrorStr), "Gain code list  [ %02d, %02d ] \n", 
						m_pCardState->DacInfo[0][u32FeModIndex[0]][u32RangeIndex[0]].u16Gain,
						m_pCardState->DacInfo[1][u32FeModIndex[1]][u32RangeIndex[1]].u16Gain);

	DumpOutputMsg(szErrorStr);
                
	return i32Ret;
}
int32	CsHexagon::DbgGetBoardsInfo( PCSBOARDINFO	pCsBoardInfo )
{
	CsHexagon	*pDevice;
	PCSBOARDINFO	pBoardInfo = pCsBoardInfo;
	PCS_BOARD_NODE	pDrvBdInfo;


	if (pBoardInfo)
	{
		pBoardInfo->u32Size = sizeof(CSBOARDINFO);
		pBoardInfo->u32BoardIndex = 1;

		if ( NULL != (pDevice = GetCardPointerFromBoardIndex( (uInt16) pBoardInfo->u32BoardIndex ) ) )
		{
			uInt32	u32FwVersion;

			pBoardInfo->u32BoardType = m_pSystem->SysInfo.u32BoardType;
			pDrvBdInfo = pDevice->GetBoardNode();

			sprintf_s(pBoardInfo->strSerialNumber, sizeof(pBoardInfo->strSerialNumber),
								TEXT("%08x/%08x"), pDrvBdInfo->u32SerNum, pDrvBdInfo->u32SerNumEx);
			pBoardInfo->u32BaseBoardVersion = pDrvBdInfo->u32BaseBoardVersion;

			// Conversion of the firmware version to the standard  xx.xx.xx format
			pBoardInfo->u32BaseBoardFirmwareVersion = ((pDrvBdInfo->u32RawFwVersionB[0]  & 0xFF00) << 8) |
														((pDrvBdInfo->u32RawFwVersionB[0] & 0xF0) << 4) |
														(pDrvBdInfo->u32RawFwVersionB[0] & 0xF);

			pBoardInfo->u32AddonBoardVersion = pDrvBdInfo->u32AddonBoardVersion;

			// Conversion of the firmware version to to the standard xx.xx.xx format
			u32FwVersion = pDrvBdInfo->u32RawFwVersionB[0] >> 16;
			pBoardInfo->u32AddonBoardFirmwareVersion = ((u32FwVersion  & 0x7f00) << 8) |
														((u32FwVersion & 0xF0) << 4) |
														(u32FwVersion & 0xF);

			pBoardInfo->u32AddonFwOptions		= pDrvBdInfo->u32ActiveAddonOptions;
			pBoardInfo->u32BaseBoardFwOptions	= pDrvBdInfo->u32BaseBoardOptions[m_pCardState->i32ConfigBootLocation];
			pBoardInfo->u32AddonHwOptions		= pDrvBdInfo->u32AddonHardwareOptions;
			pBoardInfo->u32BaseBoardHwOptions	= pDrvBdInfo->u32BaseBoardHardwareOptions;

			return CS_SUCCESS;
		}
	}

	return CS_FALSE;
}

int32 CsHexagon::DbgBoardSummary(uInt32 Params)
{
	int32	i32Ret = CS_SUCCESS;
	int16 i = 0;
	char* tf = (char *)"./BoardCalibInfos.txt";
	CSBOARDINFO	BoardInfo = {0};

	if (Params)
	{
		DbgFile = fopen( tf, "wt+" );  
		if  (!DbgFile)
		{
			fprintf(stdout,"\nfailed to open %s \n", tf);	
			return(-1);
		}
	}

	// Get the first board in the system
	if ( DbgGetBoardsInfo(&BoardInfo) == CS_SUCCESS)
	{
		sprintf_s(szDbgText, sizeof(szDbgText), TEXT("\n============ HW Board Infos =====================\n\n"));
		DumpOutputMsg(szDbgText);

		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("BoardType: 0x%x    Serial Number: %s \n"), 
						BoardInfo.u32BoardType, BoardInfo.strSerialNumber );
		DumpOutputMsg(szDbgText);
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("BaseBoard: Version: 0x%x FWVersion: 0x%x \n"), 
			BoardInfo.u32BaseBoardVersion, BoardInfo.u32BaseBoardFirmwareVersion );
		DumpOutputMsg(szDbgText);
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Addon    : Version: 0x%x FWVersion: 0x%x \n"), 
			BoardInfo.u32AddonBoardVersion, BoardInfo.u32AddonBoardFirmwareVersion );
		DumpOutputMsg(szDbgText);
	}

	// Calib info
	DbgShowCalibInfo();

	// Gain boundary
	DbgGainBoundary();

	// DC Offset
	for( i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
		DbgAdvDCOffset(i, 0);

	// Gain
	for( i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
	{
		DumpAverageTable(i, ecdGain, HEXAGON_CAL_SRC_POS, HEXAGON_NO_GAIN_SETTING, 1, 32);
		DbgRestoreCalibSetting(i);
		BBtiming( 5 );
		DumpAverageTable(i, ecdGain, HEXAGON_CAL_SRC_NEG, HEXAGON_NO_GAIN_SETTING, 1, 32);
		DbgRestoreCalibSetting(i);
		BBtiming( 5 );
	}

	// Coarse
	for( i = 1; i <= GetBoardNode()->NbAnalogChannel; i ++ )
	{
		DumpAverageTable(i, ecdPosition, HEXAGON_CAL_SRC_0V, HEXAGON_NO_GAIN_SETTING, 1, 64);
		DbgRestoreCalibSetting(i);
	}

	if (DbgFile)
	{
		fclose(DbgFile);
		DbgFile = NULL;
	}
	return i32Ret;
}

int32 CsHexagon::DbgRecalibrate(uInt32 Params)
{
	int32	i32Ret = CS_SUCCESS;
	char* tf = (char *)"./BoardCalibTraces.txt";
	CSBOARDINFO	BoardInfo = {0};
	uInt32	u32DbgCalibTraceFlags = 0;

	if (Params)
	{
		DbgFile = fopen( tf, "wt+" );  
		if  (!DbgFile)
		{
			fprintf(stdout,"\nfailed to open %s \n", tf);	
			return(-1);
		}
	}

	// Get the first board in the system
	if ( DbgGetBoardsInfo(&BoardInfo) == CS_SUCCESS)
	{
		sprintf_s(szDbgText, sizeof(szDbgText), TEXT("\n============ HW Board Infos =====================\n\n"));
		DumpOutputMsg(szDbgText);

		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("BoardType: 0x%x    Serial Number: %s \n"), 
						BoardInfo.u32BoardType, BoardInfo.strSerialNumber );
		DumpOutputMsg(szDbgText);
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("BaseBoard: Version: 0x%x FWVersion: 0x%x \n"), 
			BoardInfo.u32BaseBoardVersion, BoardInfo.u32BaseBoardFirmwareVersion );
		DumpOutputMsg(szDbgText);
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Addon    : Version: 0x%x FWVersion: 0x%x \n"), 
			BoardInfo.u32AddonBoardVersion, BoardInfo.u32AddonBoardFirmwareVersion );
		DumpOutputMsg(szDbgText);
		sprintf_s( szDbgText, sizeof(szDbgText), TEXT("Number of Channels =  %d \n\n"), 
			m_PlxData->CsBoard.NbAnalogChannel );
		DumpOutputMsg(szDbgText);
	}


	// Force calibration
	for( uInt16 i = 1; i <= m_PlxData->CsBoard.NbAnalogChannel; i += GetUserChannelSkip() )
	{
		uInt16 u16ChanZeroBased = ConvertToHwChannel(i) - 1;
		m_pCardState->bCalNeeded[u16ChanZeroBased] = true;
	}
	
	u32DbgCalibTraceFlags = m_u32DebugCalibTrace;	// keep trace of previous flags setting
	m_u32DebugCalibTrace = 0xffff;					//Enable Calib Trace
	CalibrateAllChannels(true);
	m_u32DebugCalibTrace = u32DbgCalibTraceFlags; 	// Restore previous flags setting

	if (DbgFile)
	{
		fclose(DbgFile);
		DbgFile = NULL;
	}
	return i32Ret;
}

////
//------------------------------------------------------------------------------
// Params format (32bits total) = zooo ggse
//       e (4bits)   : ecdId (ecdOffset=0, ecdPosition=1, ecdGain=2)
//       s (4bits)   : Source Ref (HEXAGON_CAL_SRC_0V=0, HEXAGON_CAL_SRC_NEG=1, HEXAGON_CAL_SRC_POS=2)
//       gg (8bits)  : Gain code value (0-20h)
//      ooo (12bits) : DAC Offset value setting
//        z (4bits)  : zoom factor (0=8,1=16,2=32)
// Offset (if Offset=0 then use default 800)
// Ex: (hex)
//         12   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_NEG 
//         22   ecdId=ecdGain,     Source Ref=HEXAGON_CAL_SRC_POS 
//       0901	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_0V,  Gain Control=9
//       0b11	ecdId=ecdPosition, Source Ref=HEXAGON_CAL_SRC_NEG, Gain Control=0b
//       0800	ecdId=ecdOffset,   Source Ref=HEXAGON_CAL_SRC_POS, Gain Control=08
//  07f4 0800   z=0 nbstep=16 off=07f4 gain=08 srcRef=HEXAGON_CAL_SRC_0V ecdId=1
//------------------------------------------------------------------------------
int32 CsHexagon::ZoomInMaxFromOffset(uInt16 u16Channel, uInt32 Params)
{
	int32	i32Ret = CS_SUCCESS;
	int32	i32Avg_x10, i32AvgRef_x10;	
	uInt16	u16maxStep = 16;	

	uInt16 	u16CurrentPos = 0;
	uInt16 	u16Step = 1;
	int32	i32Avg_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	int32	i32AvgRef_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	HEXAGON_CAL_LUT CalInfo;
	bool	bCalibBufAlloc = false;
	bool    bCalibRefBufAlloc = false;
	uInt16  u16StartCode = 0;
	uInt32	u32FeModIndex = 0;
	uInt32	u32RangeIndex = 0;
	uInt16	u16OldCode =0;

	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16Vref = (uInt16)( (Params>>4) & 0x0F);
	uInt16 u16Gain = (uInt16) ((Params >> 8) & 0xFF);
	uInt16 u16Offset = (uInt16) ((Params >> 16) & 0xFFF);	// DAC 12bits
	uInt16 u16ZoomFactor = (uInt16) ((Params >> 28) & 0xF);	// DAC 12bits
	uInt16  u16Data = u16Gain;
	uInt16	u16NbSteps = 0;
	u16NbSteps = ( 2 <<  u16ZoomFactor) * 8;

	sprintf_s( szDbgText, sizeof(szDbgText),  "ZoomInMaxFromOffset Ch %d. params = 0x%x (ecdId=%d) VRef %d Gain code %d NbSteps %d u16Offset %d \n",
				u16Channel, Params,  u16ecdId, u16Vref, u16Gain, u16NbSteps, u16Offset);
	OutputDebugString(szDbgText);	

	if( u16Vref==0)
		u16Vref = HEXAGON_CAL_SRC_0V;

	if ((u16NbSteps>0) & (u16NbSteps<=HEXAGON_DB_MAX_TABLE_SIZE_DMP))
		u16maxStep = u16NbSteps;

	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( (u16ecdId!=ecdPosition) && (u16ecdId!=ecdOffset) )
		return i32Ret;

	FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );

	// Make sure to switch to mode DC
	SendCalibModeSetting( ecmCalModeDc );
	BBtiming( 30 );


	// prepare resource required
	if( NULL == m_pi16CalibBuffer)
	{
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		bCalibBufAlloc = true;
	}

	if( NULL == m_pi32CalibA2DBuffer)
	{
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( HEXAGON_CALIB_BLOCK_SIZE*sizeof(int16) );
		if( NULL == m_pi32CalibA2DBuffer) 
			return CS_INSUFFICIENT_RESOURCES;

		bCalibRefBufAlloc = true;
	}

	if ( u16Vref==HEXAGON_CAL_SRC_NEG || u16Vref==HEXAGON_CAL_SRC_0V || u16Vref==HEXAGON_CAL_SRC_POS) 
	{
		i32Ret = WriteRegisterThruNios( u16Vref, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
		sprintf_s( szDbgText, sizeof(szDbgText),  "WriteRegisterThruNios VRef=%d 0x%x \n",
			   u16Vref, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE);
		DumpOutputMsg(szDbgText);	
	}

	if (u16Gain > 32)		// gain control max value
		u16Data = HEXAGON_DC_GAIN_CODE_1V;			// default gain value 

	if (u16Gain != HEXAGON_NO_GAIN_SETTING)
//		i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)u16ecdId, u16Data, false );
		i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)ecdGain, u16Data, false );

	u16maxStep = u16NbSteps;
	u16StartCode = u16Offset - ( u16maxStep/2 );
	u16CurrentPos = u16StartCode;

	if ( (u16ecdId==ecdPosition) || (u16ecdId==ecdOffset) )
	{
		for (uInt16 i=0; i<=u16maxStep; i++)
		{
			u16CurrentPos = u16StartCode + u16Step*i;

			i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)u16ecdId, u16CurrentPos, false );
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			BBtiming( 20 );

			i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
			if( CS_FAILED(i32Ret) )
				break;
			
			ReadCalibADCRef(&i32AvgRef_x10);

			i32Avg_x10Table[i]=i32Avg_x10;
			i32AvgRef_x10Table[i]=i32AvgRef_x10;

			BBtiming( 20 );
		}
	}

	if (bCalibBufAlloc && m_pi16CalibBuffer)
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;
	}

	if (bCalibRefBufAlloc && m_pi32CalibA2DBuffer)
	{
		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
	}

	SendCalibModeSetting();
	BBtiming( 30 );

	// Restore old setting
	DbgRestoreCalibSetting(u16Channel);

	char szText[256]={0}, szText2[256]={0};
	double	fVoltTmp = 0.0;

    sprintf_s( szText, sizeof(szText),  "==================================================== \n");
    DumpOutputMsg(szText);	

	if (u16ecdId == ecdPosition)
	{
		sprintf_s( szText, sizeof(szText),  "[ch=%d] Coarse Table. VRef=%d GainCode=%d Coarse0V=0x%x \n",
			   u16Channel, u16Vref, u16Gain, u16OldCode);
		DumpOutputMsg(szText);	
	}
	else
	{
		sprintf_s( szText, sizeof(szText),  "[ch=%d] Fine Table. VRef=%d Gaincode= %d Fine0V=0x%x \n",
				   u16Channel, u16Vref, u16Gain, u16OldCode);
		DumpOutputMsg(szText);	
	 }
     sprintf_s( szText, sizeof(szText),  "==================================================== \n");
     DumpOutputMsg(szText);	
     sprintf_s( szText, sizeof(szText),  "code	 Avg(dec,hex)         Volt(mv)          Delta(mv) \n");
     DumpOutputMsg(szText);	
     sprintf_s( szText, sizeof(szText),  "------------------------------------------------------- \n");
     DumpOutputMsg(szText);	
      
	for(uInt16 i=0; i<u16maxStep; i++)
	{
		double	fVoltRef = 0.0;
		double	fVolt =  (-1.0*i32Avg_x10Table[i]/m_pCardState->AcqConfig.i32SampleRes/10)*1000;
		int16 i16Avg = (int16)(i32Avg_x10Table[i]/10 & 0xFFFF);
		u16CurrentPos = u16StartCode + u16Step*i;

      if (u16ecdId == ecdGain)
      {
         if (i32AvgRef_x10Table[i] <0)
            fVoltRef =  (-1.0)*double(Abs(i32AvgRef_x10Table[i]/10)-0x8000);
         else
            fVoltRef =  double((i32AvgRef_x10Table[i]/10)-0x8000);
         fVoltRef = fVoltRef*12*1000/m_pCardState->AcqConfig.i32SampleRes;
         int16 i16AvgRef = (int16)(i32AvgRef_x10Table[i]/10 & 0xFFFF);
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05d, 0x%08x]  %lf mv [%05d, 0x%08x] %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  i16AvgRef, 
                  i16AvgRef, 
                  fVoltRef); 
         DumpOutputMsg(szText2);	
      }
      else
      {
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05d, 0x%08x]  %lf mv  %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  fVolt - fVoltTmp); 
         DumpOutputMsg(szText2);	         
      }
      
      fVoltTmp = fVolt;
		//u16CurrentPos = u16StartCode + u16Step*i;
	}
	
	return i32Ret;

}

//------------------------------------------------------------------------------
// Params format = 0: Fine offset, 1:position
//
//------------------------------------------------------------------------------
int32 CsHexagon::DbgAdvZeroOffset(uInt16 u16Channel, uInt32 Params)
{
	int32	i32Ret = CS_SUCCESS;
//	uInt32	u32RangeIndex[4], u32FeModIndex[4];
	int32	i32Avg_x10, i32AvgRef_x10;	
	uInt16	u16maxStep = 16;	

	uInt16 	u16CurrentPos = 0;
	uInt16 	u16Step = 1;
	int32	i32Avg_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	int32	i32AvgRef_x10Table[HEXAGON_DB_MAX_TABLE_SIZE_DMP]={0};	
	HEXAGON_CAL_LUT CalInfo;
	bool	bCalibBufAlloc = false;
	bool    bCalibRefBufAlloc = false;
	uInt16  u16StartCode = 0;
	uInt32	u32FeModIndex = 0;
	uInt32	u32RangeIndex = 0;
	uInt16	u16OldCode =0;

	uInt16 u16ecdId = (uInt16)(Params & 0x0F);
	uInt16 u16Vref = HEXAGON_CAL_SRC_0V;
	uInt16 u16Gain = 0;
	uInt16 u16Offset = 0;	// DAC 12bits
	uInt16 u16ZoomFactor = (uInt16) ((Params >> 28) & 0xF);	// DAC 12bits
	uInt16  u16Data = u16Gain;
	uInt16	u16NbSteps = 0;
	u16NbSteps = ( 2 <<  u16ZoomFactor) * 8;

	sprintf_s( szDbgText, sizeof(szDbgText),  "ZoomZeroOffset Ch %d. params = 0x%x (ecdId=%d) VRef %d Gain code %d NbSteps %d u16Offset %d \n",
				u16Channel, Params,  u16ecdId, u16Vref, u16Gain, u16NbSteps, u16Offset);
	OutputDebugString(szDbgText);	

//	i32Ret = FindFeCalIndices( u16Channel, u32RangeIndex[u16Channel], u32FeModIndex[u16Channel] );

	if (u16Channel > m_PlxData->CsBoard.NbAnalogChannel)
		return(-1);

	if( u16Vref==0)
		u16Vref = HEXAGON_CAL_SRC_0V;

	if ((u16NbSteps>0) & (u16NbSteps<=HEXAGON_DB_MAX_TABLE_SIZE_DMP))
		u16maxStep = u16NbSteps;

	i32Ret = FindCalLutItem(0, 50, CalInfo);
	if( CS_FAILED(i32Ret) )
		return i32Ret;

	if ( (u16ecdId!=ecdPosition) && (u16ecdId!=ecdOffset) )
		return i32Ret;

	FindFeCalIndices( u16Channel, u32RangeIndex, u32FeModIndex );
	u16Gain = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16Gain;
	u16Offset = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16FineOffset;	// DAC 12bits
	if (Params==1)
			u16Offset = m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset;	// DAC 12bits
	// Make sure to switch to mode DC
	SendCalibModeSetting( ecmCalModeDc );
	BBtiming( 30 );


	// prepare resource required
	if( NULL == m_pi16CalibBuffer)
	{
		m_pi16CalibBuffer = (int16 *)::GageAllocateMemoryX( HEXAGON_CALIB_BUFFER_SIZE*sizeof(int16) );
		if( NULL == m_pi16CalibBuffer)
			return CS_INSUFFICIENT_RESOURCES;

		bCalibBufAlloc = true;
	}

	if( NULL == m_pi32CalibA2DBuffer)
	{
		m_pi32CalibA2DBuffer = (int32 *)::GageAllocateMemoryX( HEXAGON_CALIB_BLOCK_SIZE*sizeof(int16) );
		if( NULL == m_pi32CalibA2DBuffer) 
			return CS_INSUFFICIENT_RESOURCES;

		bCalibRefBufAlloc = true;
	}

	if ( u16Vref==HEXAGON_CAL_SRC_NEG || u16Vref==HEXAGON_CAL_SRC_0V || u16Vref==HEXAGON_CAL_SRC_POS) 
	{
		i32Ret = WriteRegisterThruNios( u16Vref, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE );
		sprintf_s( szDbgText, sizeof(szDbgText),  "WriteRegisterThruNios VRef=%d 0x%x \n",
			   u16Vref, HEXAGON_ADDON_CPLD_CALSRC_REF | HEXAGON_SPI_WRITE);
		DumpOutputMsg(szDbgText);	
	}

	if (u16Gain > 32)		// gain control max value
		u16Data = HEXAGON_DC_GAIN_CODE_1V;			// default gain value 

	// update Gain first
	if (u16Gain != HEXAGON_NO_GAIN_SETTING)
//		i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)u16ecdId, u16Data, false );
		i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)ecdGain, u16Data, false );

	if( u16ecdId==ecdOffset)
		UpdateCalibDac(u16Channel, (eCalDacId)ecdPosition, m_pCardState->DacInfo[u16Channel-1][u32FeModIndex][u32RangeIndex].u16CoarseOffset, false );

	u16maxStep = u16NbSteps;
	u16StartCode = u16Offset - ( u16maxStep/2 );
	u16CurrentPos = u16StartCode;

	if ( (u16ecdId==ecdPosition) || (u16ecdId==ecdOffset) )
	{
		for (uInt16 i=0; i<=u16maxStep; i++)
		{
			u16CurrentPos = u16StartCode + u16Step*i;

			i32Ret = UpdateCalibDac(u16Channel, (eCalDacId)u16ecdId, u16CurrentPos, false );
			if( CS_FAILED(i32Ret) )
				return i32Ret;

			BBtiming( 20 );

			i32Ret = AcquireAndAverage(u16Channel, &i32Avg_x10);
			if( CS_FAILED(i32Ret) )
				break;
			
			ReadCalibADCRef(&i32AvgRef_x10);

			i32Avg_x10Table[i]=i32Avg_x10;
			i32AvgRef_x10Table[i]=i32AvgRef_x10;

			BBtiming( 20 );
		}
	}

	if (bCalibBufAlloc && m_pi16CalibBuffer)
	{
		::GageFreeMemoryX(m_pi16CalibBuffer);
		m_pi16CalibBuffer = NULL;
	}

	if (bCalibRefBufAlloc && m_pi32CalibA2DBuffer)
	{
		::GageFreeMemoryX(m_pi32CalibA2DBuffer);
		m_pi32CalibA2DBuffer = NULL;
	}

	SendCalibModeSetting();
	BBtiming( 30 );

	// Restore old setting
	DbgRestoreCalibSetting(u16Channel);

#ifdef _WINDOWS_
	char szText[256]={0}, szText2[256]={0};
	double	fVoltTmp = 0.0;

    sprintf_s( szText, sizeof(szText),  "==================================================== \n");
    DumpOutputMsg(szText);	

	if (u16ecdId == ecdPosition)
	{
		sprintf_s( szText, sizeof(szText),  "[ch=%d] Coarse Table. VRef=%d GainCode=%d Coarse0V=0x%x \n",
			   u16Channel, u16Vref, u16Gain, u16OldCode);
		DumpOutputMsg(szText);	
	}
	else
	{
		sprintf_s( szText, sizeof(szText),  "[ch=%d] Fine Table. VRef=%d Gaincode= %d Fine0V=0x%x \n",
				   u16Channel, u16Vref, u16Gain, u16OldCode);
		DumpOutputMsg(szText);	
	 }
     sprintf_s( szText, sizeof(szText),  "==================================================== \n");
     DumpOutputMsg(szText);	
     sprintf_s( szText, sizeof(szText),  "code	 Avg(dec,hex)         Volt(mv)          Delta(mv) \n");
     DumpOutputMsg(szText);	
     sprintf_s( szText, sizeof(szText),  "------------------------------------------------------- \n");
     DumpOutputMsg(szText);	
      
	for(uInt16 i=0; i<u16maxStep; i++)
	{
		double	fVoltRef = 0.0;
		double	fVolt =  (-1.0*i32Avg_x10Table[i]/m_pCardState->AcqConfig.i32SampleRes/10)*1000;
		int16 i16Avg = (int16)(i32Avg_x10Table[i]/10 & 0xFFFF);
		u16CurrentPos = u16StartCode + u16Step*i;

      if (u16ecdId == ecdGain)
      {
         if (i32AvgRef_x10Table[i] <0)
            fVoltRef =  (-1.0)*double(Abs(i32AvgRef_x10Table[i]/10)-0x8000);
         else
            fVoltRef =  double((i32AvgRef_x10Table[i]/10)-0x8000);
         fVoltRef = fVoltRef*12*1000/m_pCardState->AcqConfig.i32SampleRes;
         int16 i16AvgRef = (int16)(i32AvgRef_x10Table[i]/10 & 0xFFFF);
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05ld, 0x%08x]  %lf mv [%05ld, 0x%08x] %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  i16AvgRef, 
                  i16AvgRef, 
                  fVoltRef); 
         DumpOutputMsg(szText2);	
      }
      else
      {
         sprintf_s( szText2, sizeof(szText2),  "[%03x]	[%05ld, 0x%08x]  %lf mv  %lf mv \n",
                  u16CurrentPos, 
                  i16Avg,
                  i16Avg, 
                  fVolt, 
                  fVolt - fVoltTmp); 
         DumpOutputMsg(szText2);	         
      }
      
      fVoltTmp = fVolt;
		//u16CurrentPos = u16StartCode + u16Step*i;
	}
#else
	fprintf( stdout,  "Hexagon %d Ch. (ecdId=%d) i32Avg_x10Table[0-7] %d %d %d %d %d %d %d %d\n",
				u16Channel, u16ecdId, 
				i32Avg_x10Table[0], i32Avg_x10Table[1], i32Avg_x10Table[2], i32Avg_x10Table[3], i32Avg_x10Table[4], i32Avg_x10Table[5], i32Avg_x10Table[6], i32Avg_x10Table[7]); 
	fprintf( stdout,  "Hexagon %d Ch. (ecdId=%d) i32Avg_x10Table[8-15] %d %d %d %d %d %d %d %d\n",
				u16Channel, u16ecdId, 
				i32Avg_x10Table[8], i32Avg_x10Table[9], i32Avg_x10Table[10], i32Avg_x10Table[11], i32Avg_x10Table[12], i32Avg_x10Table[13], i32Avg_x10Table[14], i32Avg_x10Table[15]); 
#endif	
	return i32Ret;

}
