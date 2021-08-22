

#ifndef _HEXAGON_REGGISTERS_h_
#define	_HEXAGON_REGGISTERS_h_

#include "DrvCommonType.h"

#pragma pack (8)

typedef union _NIOS_CMD_3800000
{                           
	uInt32	u32RegVal;
	struct
	{
		uInt32	ClkOutCtrl		: 2;		
		uInt32	ClkInDisbaled	: 1;		
	} bits;

} HEXAGON_CLK_CTRL_REG, *PHEXAGON_CLK_CTRL_REG;

typedef union _ADDON_CPLD_REG_0
{                           
	uInt32	u32RegVal;
	struct
	{
		uInt32	ProtectInRange	: 1;		// 1: OK, 0: Over voltage is detected
		uInt32	OverInt			: 1;		
		uInt32	PLL_StatusLD1	: 1;		
		uInt32	PLL_StatusLD2	: 1;		
		uInt32	JESD_Sync		: 1;		
	} bits;

} HEXAGON_ADDON_MISC_STATUS_REG, *PHEXAGON_ADDON_MISC_STATUS_REG;

typedef union _ADDON_CPLD_REG_1
{                           
	uInt32	u32RegVal;
	struct
	{
		uInt32	ExtRef_IN_Sel	: 1;		// Should be 0
		uInt32	Clk_OUT_Sel		: 1;		// 0: REF Clk, 1: Sample Clock
	} bits;

} HEXAGON_CLOCK_REG, *PHEXAGON_CLOCK_REG;


typedef union _ADDON_CPLD_REG_16
{                           
	uInt32	u32RegVal;
	struct
	{
		uInt32	TrigInCal			: 1;
		uInt32	TrigOutXpEn			: 1;
		uInt32	TrigOutXpSel		: 1;
		uInt32	LdvsTrigRxEn		: 1;
		uInt32	LdvsTrigRxSel		: 1;
	} bits;

} HEXAGON_TRIG_IO_REG, *PHEXAGON_TRIG_IO_REG;

typedef union _ADC12D_CONFIG_REG1
{                           
	uInt16	u16RegVal;
	struct
	{
		uInt16	Cal		: 1;
		uInt16	Dps		: 1;
		uInt16	Ovs		: 1;
		uInt16	Tpm		: 1;
		uInt16	Pdi		: 1;
		uInt16	Pdq		: 1;
		uInt16	Res9	: 1;
		uInt16	Lfs		: 1;
		uInt16	Des		: 1;
		uInt16	Deq		: 1;
		uInt16	Diq		: 1;
		uInt16	TwoSc	: 1;
		uInt16	Tse		: 1;
		uInt16	Res0	: 3;
	} bits;

} ADC12D_CONFIG_REG1, *PADC12D_CONFIG_REG1;

typedef union _ADC12D_CHAN_OFFSET
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Res		: 3;
		uInt16	Pdq		: 1;
		uInt16	Os		: 1;
		uInt16	Om		: 11;
	} bits;

} ADC12D_CHAN_OFFSET, *PADC12D_CHAN_OFFSET;

typedef union _ADC12D_FS_RANG
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Res		: 2;
		uInt16	Fm		: 14;
	} bits;

} ADC12D_FS_RANG, *PADC12D_FS_RANG;


typedef union _ADC12D_CALIB_ADJ
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Res15	: 1;
		uInt16	Css		: 1;
		uInt16	Res8_13	: 6;
		uInt16	Ssc		: 1;
		uInt16	Res0_6	: 7;
	} bits;

} ADC12D_CALIB_ADJ, *PADC12D_CALIB_ADJ;


typedef union _ADC12D_TIMING_ADJ
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Dta		: 7;
		uInt16	Res		: 9;
	} bits;

} ADC12D_TIMING_ADJ, *PADC12D_TIMING_ADJ;

typedef union _ADC12D_DELAY_COARSE
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Cam		: 12;
		uInt16	Sta		: 1;
		uInt16	Dcc		: 1;
		uInt16	Res		: 2;
	} bits;

} ADC12D_DELAY_COARSE, *PADC12D_DELAY_COARSE;

typedef union _ADC12D_DELAY_FINE
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Fam		: 6;
		uInt16	Res		: 10;
	} bits;

} ADC12D_DELAY_FINE, *PADC12D_DELAY_FINE;

typedef union _ADC12D_AUTOSYNC
{                           
	uInt16	u16RegVal;
	struct
	{	
		uInt16	Drc		: 9;
		uInt16	Res		: 2;
		uInt16	Sp		: 2;
		uInt16	Es		: 1;
		uInt16	Doc		: 1;
		uInt16	Dr		: 1;
	} bits;

} ADC12D_AUTOSYNC, *PADC12D_AUTOSYNC;

// BaseBoard register 0xC6
typedef union _DEC_FACTOR_REG
{ 
	uInt32	u32RegVal;
	struct {
		uInt32 DeciFactor: 29;		
	} bits;

} DEC_FACTOR_REG, *PDEC_FACTOR_REG;

// BaseBoard register 0xD6
typedef union _DIGI_TRIG_CTRL_REG
{ 
	uInt16	u16RegVal;
	struct
	{	
		uInt16 TrigEng1_En_A	: 1;		// 01h
		uInt16 TrigEng1_En_B	: 1;		// 02h
		uInt16 TrigEng1_Slope_A	: 1;		// 04h
		uInt16 TrigEng1_Slope_B	: 1;		// 08h
		uInt16 DataSelect1_A	: 1;		// 10h
		uInt16 DataSelect1_B	: 1;		// 20h
		uInt16 TrigEng2_En_A	: 1;		// 40h
		uInt16 TrigEng2_En_B	: 1;		// 80h
		uInt16 TrigEng2_Slope_A	: 1;		// 100h
		uInt16 TrigEng2_Slope_B	: 1;		// 200h
		uInt16 DataSelect2_A	: 1;		// 400h
		uInt16 DataSelect2_B	: 1;		// 800h
	} bits;

} DIGI_TRIG_CTRL_REG, *PDIGI_TRIG_CTRL_REG;

//  Baseboard register 0xA5
typedef union _EXTRTRIG_REG
{ 
	uInt16	u16RegVal;
	struct
	{
		uInt16 Unused			: 2;		// 03h
		uInt16 ExtTrigSlope		: 1;		// 04h
		uInt16 ExtTrigEnable	: 1;		// 08h

	} bits;

} EXTRTRIG_REG, *PEXTRTRIG_REG;

typedef struct _ADC12D_RESERVE
{                           
	uInt16	u16RegVal;

} ADC12D_RESERVE, *PADC12D_RESERVE,
  ADC12D_CALIBVALUE, *PADC12D_CALIBVALUE;


typedef struct _ADC12D1600_REGS
{  
	ADC12D_CONFIG_REG1	Config;
	ADC12D_RESERVE		Reserved_01h;
	ADC12D_CHAN_OFFSET	IChannelOffset;
	ADC12D_FS_RANG		IchannelFullScale;
	ADC12D_CALIB_ADJ	CalibAdjust;
	ADC12D_CALIBVALUE	CalibValues;
	ADC12D_RESERVE		Reserved_06h;
	ADC12D_TIMING_ADJ	DES_TimingAdj;
	ADC12D_RESERVE		Reserved_08h;
	ADC12D_RESERVE		Reserved_09h;
	ADC12D_CHAN_OFFSET	QchannelOffset;
	ADC12D_FS_RANG		QchannelFullScale;
	ADC12D_DELAY_COARSE	DelayCoarse;
	ADC12D_DELAY_FINE	DelayFine;
	ADC12D_AUTOSYNC		AutoSync;
	ADC12D_RESERVE		Reserved_0Fh;

} ADC12D1600_REGS, *PADC12D1600_REGS;


typedef union _ADDON_CPLD_VERSION
{                           
	uInt16	u16RegVal;
	struct
	{
		uInt16	Minor	: 4;
		uInt16	Major	: 4;
		uInt16	Version	: 7;
	} bits;

} HEXAGON_ADDON_CPLD_VERSION, *PHEXAGON_ADDON_CPLD_VERSION;

typedef union _FFTCONFIG_REG
{
	uInt32	u32RegVal;
	struct
	{
		uInt32		Enabled: 1;
		uInt32		Window: 1;
		uInt32		Inverted: 1;
	} bits;
	
} FFTCONFIG_REG, *PFFTCONFIG_REG;


typedef union _ADC_IQ_CALIB_REG
{                           
	uInt32	u32RegVal;
	struct
	{
		uInt32	GainEncoder: 4;				// Encoder Gain
		uInt32	ChannelSelect: 4;			// Channel selection
		uInt32	reserve_1: 8;
		uInt32	cal_relay_disable: 1;		// CAL relay toggle disable
	} bits;

} HEXAGON_ADC_IQ_CALIB_REG, *PHEXAGON_ADC_IQ_CALIB_REG;


#endif	//_HEXAGON_REGGISTERS_h_
