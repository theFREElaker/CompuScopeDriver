// File: ADQAPI.h
// Imports the functions from ADQAPI.dll
// latest revision: 2009-10-08
// by Peter Johansson
#ifndef ADQAPI
#define ADQAPI

#ifndef OS_SETTING_NOWINDOWS
#include "windows.h"
#endif

#define ADQ214_DATA_FORMAT_PACKED_14BIT   0
#define ADQ214_DATA_FORMAT_UNPACKED_14BIT 1
#define ADQ214_DATA_FORMAT_UNPACKED_16BIT 2
#define ADQ214_DATA_FORMAT_UNPACKED_32BIT 3

#define ADQ214_STREAM_DISABLED     0
#define ADQ214_STREAM_ENABLED_BOTH 7
#define ADQ214_STREAM_ENABLED_A    3
#define ADQ214_STREAM_ENABLED_B    5

#define ADQ112_DATA_FORMAT_PACKED_12BIT   0
#define ADQ112_DATA_FORMAT_UNPACKED_12BIT 1
#define ADQ112_DATA_FORMAT_UNPACKED_16BIT 2
#define ADQ112_DATA_FORMAT_UNPACKED_32BIT 3

#define ADQ112_STREAM_DISABLED     0
#define ADQ112_STREAM_ENABLED      7

#define ADQ114_DATA_FORMAT_PACKED_14BIT   0
#define ADQ114_DATA_FORMAT_UNPACKED_14BIT 1
#define ADQ114_DATA_FORMAT_UNPACKED_16BIT 2
#define ADQ114_DATA_FORMAT_UNPACKED_32BIT 3

#define ADQ114_STREAM_DISABLED     0
#define ADQ114_STREAM_ENABLED      7

#define ADQ_CLOCK_INT_INTREF		0
#define ADQ_CLOCK_INT_EXTREF		1
#define ADQ_CLOCK_EXT				2

#define ADQ_TRANSFER_MODE_NORMAL        0x00
#define ADQ_TRANSFER_MODE_HEADER_ONLY   0x01

extern "C" __declspec(dllimport) void*			CreateADQControlUnit();
#ifndef OS_SETTING_NOWINDOWS
extern "C" __declspec(dllimport) void*			CreateADQControlUnitWN(HANDLE ReceiverWnd);
#endif
extern "C" __declspec(dllimport) void			DeleteADQControlUnit(void* adq_cu_ptr);
extern "C" __declspec(dllimport) int			ADQAPI_GetRevision(void);
extern "C" __declspec(dllimport) int			ADQControlUnit_FindDevices(void* adq_cu_ptr);
extern "C" __declspec(dllimport) int			ADQControlUnit_GetFailedDeviceCount(void* adq_cu_ptr);
extern "C" __declspec(dllexport) int            ADQControlUnit_NofADQ(void* adq_cu_ptr);
extern "C" __declspec(dllimport) int			ADQControlUnit_NofADQ112(void* adq_cu_ptr);
extern "C" __declspec(dllimport) int			ADQControlUnit_NofADQ114(void* adq_cu_ptr);
extern "C" __declspec(dllimport) int			ADQControlUnit_NofADQ214(void* adq_cu_ptr);
extern "C" __declspec(dllimport) void			ADQControlUnit_DeleteADQ112(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) void			ADQControlUnit_DeleteADQ114(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) void			ADQControlUnit_DeleteADQ214(void* adq_cu_ptr, int adq214_num);

extern "C" __declspec(dllimport) unsigned int   ADQ112_SetCacheSize(void* adq_cu_ptr, int adq112_num, unsigned int newCacheSize);
extern "C" __declspec(dllimport) unsigned int   ADQ112_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq112_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ112_SetLvlTrigLevel(void* adq_cu_ptr, int adq112_num, int level);
extern "C" __declspec(dllimport) int			ADQ112_SetLvlTrigFlank(void* adq_cu_ptr, int adq112_num, int flank);
extern "C" __declspec(dllimport) int			ADQ112_SetLvlTrigEdge(void* adq_cu_ptr, int adq112_num, int edge);
extern "C" __declspec(dllimport) int			ADQ112_SetClockSource(void* adq_cu_ptr, int adq112_num, int source);
extern "C" __declspec(dllimport) int			ADQ112_SetClockFrequencyMode(void* adq_cu_ptr, int adq112_num, int clockmode);
extern "C" __declspec(dllimport) int			ADQ112_SetPllFreqDivider(void* adq_cu_ptr, int adq112_num, int divider);
extern "C" __declspec(dllimport) int			ADQ112_SetPll(void* adq_cu_ptr, int adq112_num, int n_divider, int r_divider, int vco_divider, int channel_divider);
extern "C" __declspec(dllimport) int			ADQ112_SetTriggerMode(void* adq_cu_ptr, int adq112_num, int trig_mode);
extern "C" __declspec(dllimport) int            ADQ112_SetSampleWidth(void* adq_cu_ptr, int adq112_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ112_SetNofBits(void* adq_cu_ptr, int adq112_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ112_SetPreTrigSamples(void* adq_cu_ptr, int adq112_num, unsigned int PreTrigSamples);
extern "C" __declspec(dllimport) int            ADQ112_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq112_num, unsigned int TriggerHoldOffSamples);
extern "C" __declspec(dllimport) int			ADQ112_SetBufferSizePages(void* adq_cu_ptr, int adq112_num, unsigned int pages);
extern "C" __declspec(dllimport) int            ADQ112_SetBufferSize(void* adq_cu_ptr, int adq112_num, unsigned int samples);
extern "C" __declspec(dllimport) int			ADQ112_ArmTrigger(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_DisarmTrigger(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_USBTrig(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_SWTrig(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_CollectDataNextPage(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_CollectRecord(void* adq_cu_ptr, int adq112_num, unsigned int record_num);
extern "C" __declspec(dllimport) int			ADQ112_GetErrorVector(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetStreamStatus(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_SetStreamStatus(void* adq_cu_ptr, int adq112_num, unsigned int status);
extern "C" __declspec(dllimport) int			ADQ112_GetStreamOverflow(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int*			ADQ112_GetPtrData(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) void*			ADQ112_GetPtrStream(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetWaitingForTrigger(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetTrigged(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetTriggedAll(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetPageCount(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetLvlTrigLevel(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetLvlTrigFlank(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetPllFreqDivider(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetClockSource(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetTriggerMode(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetBufferSizePages(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_GetBufferSize(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_GetMaxBufferSize(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_GetMaxBufferSizePages(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_GetSamplesPerPage(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetUSBAddress(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetPCIeAddress(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetBcdDevice(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) char*			ADQ112_GetBoardSerialNumber(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int*			ADQ112_GetRevision(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetTriggerInformation(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetTrigPoint(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetTrigType(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_GetOverflow(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetRecordSize(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetNofRecords(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_ResetDevice(void* adq_cu_ptr, int adq112_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ112_IsUSBDevice(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_IsPCIeDevice(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_IsAlive(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_SendProcessorCommand(void* adq_cu_ptr, int adq112_num, int command, int argument);
extern "C" __declspec(dllimport) unsigned int	ADQ112_SendLongProcessorCommand(void* adq_cu_ptr, int adq112_num, int command, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ112_WriteRegister(void* adq_cu_ptr, int ADQ112_num, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ112_ReadRegister(void* adq_cu_ptr, int ADQ112_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ112_WriteAlgoRegister(void* adq_cu_ptr, int adq112_num, int addr, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ112_ReadAlgoRegister(void* adq_cu_ptr, int adq112_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetTemperature(void* adq_cu_ptr, int adq112_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ112_WriteEEPROM(void* adq_cu_ptr, int adq112_num, int addr, int data, int accesscode);
extern "C" __declspec(dllimport) unsigned int	ADQ112_ReadEEPROM(void* adq_cu_ptr, int adq112_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ112_SetDataFormat(void* adq_cu_ptr, int adq112_num, unsigned int format);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetDataFormat(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_MultiRecordSetup(void* adq_cu_ptr, int adq114_num,int NumberOfRecords, int SamplesPerRecord);
extern "C" __declspec(dllimport) unsigned int   ADQ112_MultiRecordClose(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ112_MemoryDump(void* adq_cu_ptr, int adq114_num,	unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr);
extern "C" __declspec(dllimport) int			ADQ112_WriteTrig(void* adq_cu_ptr, int adq112_num, int data);
extern "C" __declspec(dllimport) unsigned int   ADQ112_SetTestPatternMode(void* adq_cu_ptr, int adq112_num, int mode);
extern "C" __declspec(dllimport) unsigned int   ADQ112_SetTestPatternConstant(void* adq_cu_ptr, int adq112_num, int value);
extern "C" __declspec(dllimport) int			ADQ112_SetDirectionTrig(void* adq_cu_ptr, int adq112_num, int direction);
extern "C" __declspec(dllimport) unsigned int	ADQ112_ReadGPIO(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_WriteGPIO(void* adq_cu_ptr, int adq112_num, unsigned int data, unsigned int mask);
extern "C" __declspec(dllimport) int			ADQ112_SetDirectionGPIO(void* adq_cu_ptr, int adq112_num, unsigned int direction, unsigned int mask);
extern "C" __declspec(dllimport) unsigned int   ADQ112_SetAlgoStatus(void* adq_cu_ptr, int adq112_num, int status);
extern "C" __declspec(dllimport) unsigned int   ADQ112_SetAlgoNyquistBand(void* adq_cu_ptr, int adq112_num, unsigned int band);
extern "C" __declspec(dllimport) unsigned int*	ADQ112_GetMultiRecordHeader(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned long long	ADQ112_GetTrigTimeCycles(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned long long ADQ112_GetTrigTime(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetTrigTimeSyncs(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) unsigned int	ADQ112_GetTrigTimeStart(void* adq_cu_ptr, int adq112_num);
extern "C" __declspec(dllimport) int			ADQ112_SetTrigTimeMode(void* adq_cu_ptr, int adq112_num, int TrigTimeMode);
extern "C" __declspec(dllimport) int			ADQ112_ResetTrigTimer(void* adq_cu_ptr, int adq112_num, int TrigTimeRestart);

extern "C" __declspec(dllimport) unsigned int   ADQ114_SetCacheSize(void* adq_cu_ptr, int adq114_num, unsigned int newCacheSize);
extern "C" __declspec(dllimport) unsigned int   ADQ114_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq114_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ114_SetLvlTrigLevel(void* adq_cu_ptr, int adq114_num, int level);
extern "C" __declspec(dllimport) int			ADQ114_SetLvlTrigFlank(void* adq_cu_ptr, int adq114_num, int flank);
extern "C" __declspec(dllimport) int			ADQ114_SetLvlTrigEdge(void* adq_cu_ptr, int adq114_num, int edge);
extern "C" __declspec(dllimport) int			ADQ114_SetClockSource(void* adq_cu_ptr, int adq114_num, int source);
extern "C" __declspec(dllimport) int			ADQ114_SetClockFrequencyMode(void* adq_cu_ptr, int adq114_num, int clockmode);
extern "C" __declspec(dllimport) int			ADQ114_SetPllFreqDivider(void* adq_cu_ptr, int adq114_num, int divider);
extern "C" __declspec(dllimport) int			ADQ114_SetPll(void* adq_cu_ptr, int adq114_num, int n_divider, int r_divider, int vco_divider, int channel_divider);
extern "C" __declspec(dllimport) int			ADQ114_SetTriggerMode(void* adq_cu_ptr, int adq114_num, int trig_mode);
extern "C" __declspec(dllimport) int            ADQ114_SetSampleWidth(void* adq_cu_ptr, int adq114_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ114_SetNofBits(void* adq_cu_ptr, int adq114_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ114_SetPreTrigSamples(void* adq_cu_ptr, int adq114_num, unsigned int PreTrigSamples);
extern "C" __declspec(dllimport) int            ADQ114_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq114_num, unsigned int TriggerHoldOffSamples);
extern "C" __declspec(dllimport) int			ADQ114_SetBufferSizePages(void* adq_cu_ptr, int adq114_num, unsigned int pages);
extern "C" __declspec(dllimport) int            ADQ114_SetBufferSize(void* adq_cu_ptr, int adq114_num, unsigned int samples);
extern "C" __declspec(dllimport) int			ADQ114_ArmTrigger(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_DisarmTrigger(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_USBTrig(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_SWTrig(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_CollectDataNextPage(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_CollectRecord(void* adq_cu_ptr, int adq114_num, unsigned int record_num);
extern "C" __declspec(dllimport) int			ADQ114_GetErrorVector(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetStreamStatus(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_SetStreamStatus(void* adq_cu_ptr, int adq114_num, unsigned int status);
extern "C" __declspec(dllimport) int			ADQ114_GetStreamOverflow(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int*			ADQ114_GetPtrData(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) void*			ADQ114_GetPtrStream(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetWaitingForTrigger(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetTrigged(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetTriggedAll(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetPageCount(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetLvlTrigLevel(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetLvlTrigFlank(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetPllFreqDivider(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetClockSource(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetTriggerMode(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetBufferSizePages(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_GetBufferSize(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_GetMaxBufferSize(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_GetMaxBufferSizePages(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_GetSamplesPerPage(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetUSBAddress(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetPCIeAddress(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetBcdDevice(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) char*			ADQ114_GetBoardSerialNumber(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int*			ADQ114_GetRevision(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetTriggerInformation(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetTrigPoint(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetTrigType(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_GetOverflow(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetRecordSize(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetNofRecords(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_ResetDevice(void* adq_cu_ptr, int adq114_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ114_IsUSBDevice(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_IsPCIeDevice(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_IsAlive(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_SendProcessorCommand(void* adq_cu_ptr, int adq114_num, int command, int argument);
extern "C" __declspec(dllimport) unsigned int	ADQ114_SendLongProcessorCommand(void* adq_cu_ptr, int adq114_num, int command, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ114_WriteRegister(void* adq_cu_ptr, int ADQ114_num, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ114_ReadRegister(void* adq_cu_ptr, int ADQ114_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ114_WriteAlgoRegister(void* adq_cu_ptr, int adq114_num, int addr, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ114_ReadAlgoRegister(void* adq_cu_ptr, int adq114_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetTemperature(void* adq_cu_ptr, int adq114_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ114_WriteEEPROM(void* adq_cu_ptr, int adq114_num, int addr, int data, int accesscode);
extern "C" __declspec(dllimport) unsigned int	ADQ114_ReadEEPROM(void* adq_cu_ptr, int adq114_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ114_SetDataFormat(void* adq_cu_ptr, int adq114_num, unsigned int format);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetDataFormat(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_MultiRecordSetup(void* adq_cu_ptr, int adq114_num,int NumberOfRecords, int SamplesPerRecord);
extern "C" __declspec(dllimport) unsigned int   ADQ114_MultiRecordClose(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int   ADQ114_MemoryDump(void* adq_cu_ptr, int adq114_num,	unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr);
extern "C" __declspec(dllimport) int			ADQ114_WriteTrig(void* adq_cu_ptr, int adq114_num, int data);
extern "C" __declspec(dllimport) unsigned int   ADQ114_SetTestPatternMode(void* adq_cu_ptr, int adq114_num, int mode);
extern "C" __declspec(dllimport) unsigned int   ADQ114_SetTestPatternConstant(void* adq_cu_ptr, int adq114_num, int value);
extern "C" __declspec(dllimport) int			ADQ114_SetDirectionTrig(void* adq_cu_ptr, int adq114_num, int direction);
extern "C" __declspec(dllimport) unsigned int	ADQ114_ReadGPIO(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_WriteGPIO(void* adq_cu_ptr, int adq114_num, unsigned int data, unsigned int mask);
extern "C" __declspec(dllimport) int			ADQ114_SetDirectionGPIO(void* adq_cu_ptr, int adq114_num, unsigned int direction, unsigned int mask);
extern "C" __declspec(dllimport) unsigned int   ADQ114_SetAlgoStatus(void* adq_cu_ptr, int adq114_num, int status);
extern "C" __declspec(dllimport) unsigned int   ADQ114_SetAlgoNyquistBand(void* adq_cu_ptr, int adq114_num, unsigned int band);
extern "C" __declspec(dllimport) unsigned int*	ADQ114_GetMultiRecordHeader(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned long long ADQ114_GetTrigTime(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned long long	ADQ114_GetTrigTimeCycles(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetTrigTimeSyncs(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) unsigned int	ADQ114_GetTrigTimeStart(void* adq_cu_ptr, int adq114_num);
extern "C" __declspec(dllimport) int			ADQ114_SetTrigTimeMode(void* adq_cu_ptr, int adq114_num, int TrigTimeMode);
extern "C" __declspec(dllimport) int			ADQ114_ResetTrigTimer(void* adq_cu_ptr, int adq114_num,int TrigTimeRestart);

extern "C" __declspec(dllimport) unsigned int   ADQ214_SetCacheSize(void* adq_cu_ptr, int adq214_num, unsigned int newCacheSize);
extern "C" __declspec(dllimport) unsigned int   ADQ214_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq214_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ214_SetLvlTrigLevel(void* adq_cu_ptr, int adq214_num, int level);
extern "C" __declspec(dllimport) int			ADQ214_SetLvlTrigFlank(void* adq_cu_ptr, int adq214_num, int flank);
extern "C" __declspec(dllimport) int			ADQ214_SetLvlTrigEdge(void* adq_cu_ptr, int adq214_num, int edge);
extern "C" __declspec(dllimport) int			ADQ214_SetLvlTrigChannel(void* adq_cu_ptr, int adq214_num,int channel);
extern "C" __declspec(dllimport) int			ADQ214_SetClockSource(void* adq_cu_ptr, int adq214_num, int source);
extern "C" __declspec(dllimport) int			ADQ214_SetClockFrequencyMode(void* adq_cu_ptr, int adq214_num, int clockmode);
extern "C" __declspec(dllimport) int			ADQ214_SetPllFreqDivider(void* adq_cu_ptr, int adq214_num, int divider);
extern "C" __declspec(dllimport) int			ADQ214_SetPll(void* adq_cu_ptr, int adq214_num, int n_divider, int r_divider, int vco_divider, int channel_divider);
extern "C" __declspec(dllimport) int			ADQ214_SetTriggerMode(void* adq_cu_ptr, int adq214_num, int trig_mode);
extern "C" __declspec(dllimport) int            ADQ214_SetSampleWidth(void* adq_cu_ptr, int adq214_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ214_SetNofBits(void* adq_cu_ptr, int adq214_num, unsigned int NofBits);
extern "C" __declspec(dllimport) int            ADQ214_SetPreTrigSamples(void* adq_cu_ptr, int adq214_num, unsigned int PreTrigSamples);
extern "C" __declspec(dllimport) int            ADQ214_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq214_num, unsigned int TriggerHoldOffSamples);
extern "C" __declspec(dllimport) int			ADQ214_SetBufferSizePages(void* adq_cu_ptr, int adq214_num, unsigned int pages);
extern "C" __declspec(dllimport) int            ADQ214_SetBufferSize(void* adq_cu_ptr, int adq214_num, unsigned int samples);
extern "C" __declspec(dllimport) int			ADQ214_ArmTrigger(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_DisarmTrigger(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_USBTrig(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_SWTrig(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_CollectDataNextPage(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_CollectRecord(void* adq_cu_ptr, int adq214_num, unsigned int record_num);
extern "C" __declspec(dllimport) int			ADQ214_GetErrorVector(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetStreamStatus(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_SetStreamStatus(void* adq_cu_ptr, int adq214_num, unsigned int status);
extern "C" __declspec(dllimport) int			ADQ214_GetStreamOverflow(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int*			ADQ214_GetPtrDataChA(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int*			ADQ214_GetPtrDataChB(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) void*			ADQ214_GetPtrStream(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetWaitingForTrigger(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTrigged(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTriggedAll(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetPageCount(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetLvlTrigLevel(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetLvlTrigFlank(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetLvlTrigChannel(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetPllFreqDivider(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetClockSource(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTriggerMode(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetBufferSizePages(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_GetBufferSize(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_GetMaxBufferSize(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_GetMaxBufferSizePages(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_GetSamplesPerPage(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetUSBAddress(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetPCIeAddress(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetBcdDevice(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) char*			ADQ214_GetBoardSerialNumber(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int*			ADQ214_GetRevision(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTriggerInformation(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTrigPoint(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetTrigType(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetTriggedCh(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_GetOverflow(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetRecordSize(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetNofRecords(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_ResetDevice(void* adq_cu_ptr, int adq214_num, int resetlevel);
extern "C" __declspec(dllimport) int			ADQ214_IsUSBDevice(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_IsPCIeDevice(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_IsAlive(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_SendProcessorCommand(void* adq_cu_ptr, int adq214_num, int command, int argument);
extern "C" __declspec(dllimport) unsigned int	ADQ214_SendLongProcessorCommand(void* adq_cu_ptr, int adq214_num, int command, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ214_WriteRegister(void* adq_cu_ptr, int ADQ214_num, int addr, int mask, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ214_ReadRegister(void* adq_cu_ptr, int ADQ214_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ214_WriteAlgoRegister(void* adq_cu_ptr, int adq214_num, int addr, int data);
extern "C" __declspec(dllimport) unsigned int	ADQ214_ReadAlgoRegister(void* adq_cu_ptr, int adq214_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetTemperature(void* adq_cu_ptr, int adq214_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ214_WriteEEPROM(void* adq_cu_ptr, int adq214_num, int addr, int data, int accesscode);
extern "C" __declspec(dllimport) unsigned int	ADQ214_ReadEEPROM(void* adq_cu_ptr, int adq214_num, int addr);
extern "C" __declspec(dllimport) unsigned int	ADQ214_SetDataFormat(void* adq_cu_ptr, int adq214_num, unsigned int format);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetDataFormat(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int   ADQ214_MemoryDump(void* adq_cu_ptr, int adq214_num,	unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr);
extern "C" __declspec(dllimport) unsigned int   ADQ214_MultiRecordSetup(void* adq_cu_ptr, int adq214_num,int NumberOfRecords, int SamplesPerRecord);
extern "C" __declspec(dllimport) unsigned int   ADQ214_MultiRecordClose(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_WriteTrig(void* adq_cu_ptr, int adq214_num, int data);
extern "C" __declspec(dllimport) unsigned int   ADQ214_SetTestPatternMode(void* adq_cu_ptr, int adq214_num, int mode);
extern "C" __declspec(dllimport) unsigned int   ADQ214_SetTestPatternConstant(void* adq_cu_ptr, int adq214_num, int value);
extern "C" __declspec(dllimport) int			ADQ214_SetDirectionTrig(void* adq_cu_ptr, int adq214_num, int direction);
extern "C" __declspec(dllimport) unsigned int	ADQ214_ReadGPIO(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_WriteGPIO(void* adq_cu_ptr, int adq214_num, unsigned int data, unsigned int mask);
extern "C" __declspec(dllimport) int			ADQ214_SetDirectionGPIO(void* adq_cu_ptr, int adq214_num, unsigned int direction, unsigned int mask);
extern "C" __declspec(dllimport) unsigned int*	ADQ214_GetMultiRecordHeader(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned long long ADQ214_GetTrigTime(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned long long	ADQ214_GetTrigTimeCycles(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetTrigTimeSyncs(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) unsigned int	ADQ214_GetTrigTimeStart(void* adq_cu_ptr, int adq214_num);
extern "C" __declspec(dllimport) int			ADQ214_SetTrigTimeMode(void* adq_cu_ptr, int adq214_num, int TrigTimeMode);
extern "C" __declspec(dllimport) int			ADQ214_ResetTrigTimer(void* adq_cu_ptr, int adq214_num,int TrigTimeRestart);

#ifdef USE_CPP_API
#include "AdqInterface.h"
extern "C" __declspec(dllimport) ADQInterface*	ADQControlUnit_GetADQ(void* adq_cu_ptr, int adq_num);
#endif 
#endif //ADQAPI
