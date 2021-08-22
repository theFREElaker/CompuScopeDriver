// Project: ADQ-API
// File: AdqInterface.h
//

#ifndef __cplusplus
	#pragma message("AdqInterface.h is suitable only for C++ compilations")
#else
#ifndef ADQINTERFACE_H
#define ADQINTERFACE_H

class HWIF;
class ADQInterface
    {
    public:
	
        // Must overrides
        virtual int Open(HWIF* device) = 0; 
        virtual void* GetPtrData(unsigned int channel) = 0;
        virtual int SetDataFormat(unsigned int format) = 0;
        virtual int GetADQType(void) = 0;

        // Selected overrides (default returns NULL)
        virtual int* GetPtrData() = 0;
        virtual int* GetPtrDataChA() = 0;
        virtual int* GetPtrDataChB() = 0;
		virtual int SetLvlTrigChannel(int channel) = 0;
		virtual int GetLvlTrigChannel() = 0;
		virtual int GetTriggedCh() = 0;
        virtual int SetAlgoNyquistBand(unsigned int band) = 0;
        virtual int SetAlgoStatus(int status) = 0;
        virtual unsigned int SetSampleSkip(unsigned int) = 0;
		virtual unsigned int GetSampleSkip() = 0;
        virtual unsigned int SetSampleDecimation(unsigned int) = 0;
		virtual unsigned int GetSampleDecimation() = 0;
		virtual int SetAfeSwitch(unsigned int afe) = 0;

        // Non-overrides (generic implementation for all ADQs)
        virtual int GetData(void** target_buffers, 
                            unsigned int target_buffer_size,
                            unsigned char target_bytes_per_sample,
                            unsigned int StartRecordNumber,
                            unsigned int NumberOfRecords,
                            unsigned char ChannelsMask,
                            unsigned int StartSample,
                            unsigned int nSamples,
                            unsigned char TransferMode) = 0;
        virtual unsigned int GetLastError(void) = 0;
        virtual void* GetPtrStream(void) = 0;
		virtual unsigned int GetErrorVector(void) = 0;
		virtual unsigned int IsAlive(void) = 0;
		virtual unsigned int ReadEEPROM(unsigned int addr) = 0;
        virtual unsigned int ResetDevice(int) = 0;
        virtual int SetCacheSize(unsigned int) = 0;
		virtual int SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize) = 0;
		virtual unsigned int WriteEEPROM(unsigned int addr, unsigned int data, unsigned int accesscode) = 0;
		virtual int SetLvlTrigLevel(int level) = 0;
		virtual int SetLvlTrigEdge(int edge) = 0;
		virtual int SetClockSource(int source) = 0;
		virtual int SetClockFrequencyMode(unsigned int clockmode) = 0;
		virtual int SetPllFreqDivider(int divider) = 0;
		virtual int SetPll(int n_divider, int r_divider, int vco_divider, int channel_divider) = 0;
		virtual int SetTriggerMode(int trig_mode) = 0;
		virtual int SetPreTrigSamples(unsigned int PreTrigSamples) = 0;
		virtual int SetTriggerHoldOffSamples(unsigned int TriggerHoldOffSamples) = 0;
		virtual int SetBufferSizePages(unsigned int pages) = 0;
		virtual int SetBufferSizeWords(unsigned int words) = 0;
		virtual int SetBufferSize(unsigned int samples) = 0;
		virtual int SetNofBits(unsigned int NofBits) = 0;
		virtual int SetSampleWidth(unsigned int SampleWidth) = 0;
		virtual int SetWordsPerPage(unsigned int WordsPerPage) = 0;
		virtual int SetPreTrigWords(unsigned int PreTrigWords) = 0;
		virtual int SetWordsAfterTrig(unsigned int WordsAfterTrig) = 0;
		virtual int SetTrigLevelResetValue(int OffsetValue) = 0;
		virtual int SetTrigMask1(unsigned int TrigMask) = 0;
		virtual int SetTrigLevel1(int TrigLevel) = 0;
        virtual int SetTrigPreLevel1(int TrigLevel) = 0;
		virtual int SetTrigCompareMask1(unsigned int TrigCompareMask) = 0;
		virtual int SetTrigMask2(unsigned int TrigMask) = 0;
		virtual int SetTrigLevel2(int TrigLevel) = 0;
        virtual int SetTrigPreLevel2(int TrigLevel) = 0;
		virtual int SetTrigCompareMask2(unsigned int TrigCompareMask) = 0;
		virtual int ArmTrigger() = 0;
		virtual int DisarmTrigger() = 0;
		virtual int SWTrig() = 0;
		virtual int CollectDataNextPage() = 0;
		virtual int GetWaitingForTrigger() = 0;
        virtual int GetAcquired() = 0;
		virtual unsigned int GetPageCount() = 0;
		virtual int GetLvlTrigLevel() = 0;
		virtual int GetLvlTrigEdge() = 0;
		virtual int GetPllFreqDivider() = 0;
		virtual int GetClockSource() = 0;
		virtual int GetTriggerMode() = 0;
		virtual unsigned int GetBufferSizePages() = 0;
		virtual unsigned int GetBufferSize() = 0;
		virtual unsigned int GetMaxBufferSize() = 0;
		virtual unsigned int GetMaxBufferSizePages() = 0;
		virtual unsigned int GetSamplesPerPage() = 0;
		virtual unsigned int GetUSBAddress() = 0;
		virtual unsigned long GetPCIeAddress() = 0;
		virtual unsigned int GetBcdDevice() = 0;
		virtual int GetStreamStatus() = 0;
		virtual int SetStreamStatus(int status) = 0;
		virtual int GetStreamOverflow() = 0;
		virtual char* GetBoardSerialNumber() = 0;
		virtual int* GetRevision() = 0;
		virtual int GetTriggerInformation() = 0;
		virtual int GetTrigPoint() = 0;
		virtual unsigned int GetTrigType() = 0;
		virtual int GetOverflow() = 0;
		virtual unsigned int GetRecordSize() = 0;
		virtual unsigned int GetNofRecords() = 0;
		virtual int IsUSBDevice() = 0;
		virtual int IsPCIeDevice() = 0;
		virtual unsigned int SendProcessorCommand(int command, int argument) = 0;
		virtual unsigned int SendProcessorCommand(unsigned int command, unsigned int addr, unsigned int mask, unsigned int data) = 0;
		virtual unsigned int WriteRegister(unsigned int addr, unsigned int mask, unsigned int data) = 0;
		virtual unsigned int ReadRegister(unsigned int addr) = 0;
		virtual unsigned int WriteAlgoRegister(unsigned int addr, unsigned int mask, unsigned int data) = 0;
		virtual unsigned int ReadAlgoRegister(unsigned int addr) = 0;
		virtual unsigned int WriteI2C(unsigned int addr, unsigned int nbytes, unsigned int data) = 0;
		virtual unsigned int ReadI2C(unsigned int addr, unsigned int nbytes) = 0;
		virtual unsigned int GetTemperature(unsigned int addr) = 0;
		virtual unsigned int MultiRecordSetup(unsigned int NumberOfRecords, unsigned int SamplesPerRecord) = 0;
		virtual unsigned int MultiRecordGetRecord(int RecordNumber) = 0;
		virtual unsigned int MultiRecordClose(void) = 0;
		virtual unsigned int GetAcquiredAll(void) = 0;
		virtual unsigned int IsStartedOK(void) = 0;
		virtual unsigned int MemoryDump(unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr) = 0;
		virtual unsigned int GetDataFormat() = 0;
        virtual unsigned int SetTestPatternMode(unsigned int mode) = 0;
        virtual unsigned int SetTestPatternConstant(unsigned int value) = 0;
		virtual int SetDirectionTrig(int direction) = 0;
		virtual int WriteTrig(int data) = 0;
		virtual int SetDirectionGPIO(unsigned int direction, unsigned int mask) = 0;
		virtual int WriteGPIO(unsigned int data, unsigned int mask) = 0;
		virtual unsigned int ReadGPIO() = 0;
		virtual unsigned int* GetMultiRecordHeader(void) = 0;
		virtual unsigned long long GetTrigTime(void) = 0;
		virtual unsigned long long GetTrigTimeCycles(void) = 0;
		virtual unsigned int GetTrigTimeSyncs(void) = 0;
		virtual unsigned int GetTrigTimeStart(void) = 0;
		virtual int SetTrigTimeMode(int TrigTimeMode) = 0;
		virtual int ResetTrigTimer(int TrigTimeRestart) = 0;
    
    protected:
        virtual int ParseSampleData(unsigned int wordNbOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxSamples) = 0;
    
};
#endif //ADQINTERFACE_H
#endif //__cplusplus
