// Project: ADQ-API
// File: ADQ.h
//


#ifndef ADQ_H
#define ADQ_H

#include "ADQInterface.h"
#include "ADQAPI_definitions.h"
#include "HWIF.h"

class ADQ : public ADQInterface
    {
    public:
        ADQ();
		~ADQ();
		
        // Must overrides
        virtual int Open(HWIF* device) = 0; 
        virtual void* GetPtrData(unsigned int channel) = 0;
        virtual int SetDataFormat(unsigned int format) = 0;
        virtual int GetADQType(void) = 0;

        // Selected overrides (default returns NULL)
        virtual int* GetPtrData();
        virtual int* GetPtrDataChA();
        virtual int* GetPtrDataChB();
		virtual int SetLvlTrigChannel(int channel);
		virtual int GetLvlTrigChannel();
		virtual int GetTriggedCh();
        virtual int SetAlgoNyquistBand(unsigned int band);
        virtual int SetAlgoStatus(int status);
        virtual unsigned int SetSampleSkip(unsigned int);
		virtual unsigned int GetSampleSkip();
        virtual unsigned int SetSampleDecimation(unsigned int);
		virtual unsigned int GetSampleDecimation();
		virtual int SetAfeSwitch(unsigned int afe);

        // Non-overrides (generic implementation for all ADQs)
        int GetData(void** target_buffers, 
                            unsigned int target_buffer_size,
                            unsigned char target_bytes_per_sample,
                            unsigned int StartRecordNumber,
                            unsigned int NumberOfRecords,
                            unsigned char ChannelsMask,
                            unsigned int StartSample,
                            unsigned int nSamples,
                            unsigned char TransferMode);
        unsigned int GetLastError(void);
        void* GetPtrStream(void);
		unsigned int GetErrorVector(void);
		unsigned int IsAlive(void);
		unsigned int ReadEEPROM(unsigned int addr);
        unsigned int ResetDevice(int);
        int SetCacheSize(unsigned int);
		int SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize);
		unsigned int WriteEEPROM(unsigned int addr, unsigned int data, unsigned int accesscode);
		int SetLvlTrigLevel(int level);
		int SetLvlTrigEdge(int edge);
		int SetClockSource(int source);
		int SetClockFrequencyMode(unsigned int clockmode);
		int SetPllFreqDivider(int divider);
		int SetPll(int n_divider, int r_divider, int vco_divider, int channel_divider);
		int SetTriggerMode(int trig_mode);
		int SetPreTrigSamples(unsigned int PreTrigSamples);
		int SetTriggerHoldOffSamples(unsigned int TriggerHoldOffSamples);
		int SetBufferSizePages(unsigned int pages);
		int SetBufferSizeWords(unsigned int words);
		int SetBufferSize(unsigned int samples);
		int SetNofBits(unsigned int NofBits);
		int SetSampleWidth(unsigned int SampleWidth);
		int SetWordsPerPage(unsigned int WordsPerPage);
		int SetPreTrigWords(unsigned int PreTrigWords);
		int SetWordsAfterTrig(unsigned int WordsAfterTrig);
		int SetTrigLevelResetValue(int OffsetValue);
		int SetTrigMask1(unsigned int TrigMask);
		int SetTrigLevel1(int TrigLevel);
        int SetTrigPreLevel1(int TrigLevel);
		int SetTrigCompareMask1(unsigned int TrigCompareMask);
		int SetTrigMask2(unsigned int TrigMask);
		int SetTrigLevel2(int TrigLevel);
        int SetTrigPreLevel2(int TrigLevel);
		int SetTrigCompareMask2(unsigned int TrigCompareMask);
		int ArmTrigger();
		int DisarmTrigger();
		int SWTrig();
		int CollectDataNextPage();
		int GetWaitingForTrigger();
        int GetAcquired();
		unsigned int GetPageCount();
		int GetLvlTrigLevel();
		int GetLvlTrigEdge();
		int GetPllFreqDivider();
		int GetClockSource();
		int GetTriggerMode();
		unsigned int GetBufferSizePages();
		unsigned int GetBufferSize();
		unsigned int GetMaxBufferSize();
		unsigned int GetMaxBufferSizePages();
		unsigned int GetSamplesPerPage();
		unsigned int GetUSBAddress();
		unsigned long GetPCIeAddress();
		unsigned int GetBcdDevice();
		int GetStreamStatus();
		int SetStreamStatus(int status);
		int GetStreamOverflow();
		char* GetBoardSerialNumber();
		int* GetRevision();
		int GetTriggerInformation();
		int GetTrigPoint();
		unsigned int GetTrigType();
		int GetOverflow();
		unsigned int GetRecordSize();
		unsigned int GetNofRecords();
		int IsUSBDevice();
		int IsPCIeDevice();
		unsigned int SendProcessorCommand(int command, int argument);
		unsigned int SendProcessorCommand(unsigned int command, unsigned int addr, unsigned int mask, unsigned int data);
		unsigned int WriteRegister(unsigned int addr, unsigned int mask, unsigned int data);
		unsigned int ReadRegister(unsigned int addr);
		unsigned int WriteAlgoRegister(unsigned int addr, unsigned int mask, unsigned int data);
		unsigned int ReadAlgoRegister(unsigned int addr);
		unsigned int WriteI2C(unsigned int addr, unsigned int nbytes, unsigned int data);
		unsigned int ReadI2C(unsigned int addr, unsigned int nbytes);
		unsigned int GetTemperature(unsigned int addr);
		unsigned int MultiRecordSetup(unsigned int NumberOfRecords, unsigned int SamplesPerRecord);
		unsigned int MultiRecordGetRecord(int RecordNumber);
		unsigned int MultiRecordClose(void);
		unsigned int GetAcquiredAll(void);
		unsigned int IsStartedOK(void);
		unsigned int MemoryDump(unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr);
		unsigned int GetDataFormat();
        unsigned int SetTestPatternMode(unsigned int mode);
        unsigned int SetTestPatternConstant(unsigned int value);
		int SetDirectionTrig(int direction);
		int WriteTrig(int data);
		int SetDirectionGPIO(unsigned int direction, unsigned int mask);
		int WriteGPIO(unsigned int data, unsigned int mask);
		unsigned int ReadGPIO();
		unsigned int* GetMultiRecordHeader(void);
		unsigned long long GetTrigTime(void);
		unsigned long long GetTrigTimeCycles(void);
		unsigned int GetTrigTimeSyncs(void);
		unsigned int GetTrigTimeStart(void);
		int SetTrigTimeMode(int TrigTimeMode);
		int ResetTrigTimer(int TrigTimeRestart);
        
    protected:
        int CollectDataNextPageInternal(
                                     unsigned int StartRelativeToTriggerOffsetWordNb,
                                     unsigned char reset
                                     );
        unsigned int MultiRecordGetRecordInternal(int RecordNumber,
                                               unsigned int StartRelativeToTriggerOffsetWordNb,
                                               unsigned char reset);
        virtual int ParseSampleData(unsigned int wordNbOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxSamples) = 0;
        unsigned int GetMemoryNumberOfWords128b(void);
		int OptimizeTransferSettings(void);
		int SetupDevice(void);
		void RetrieveBoardSerialNumber(void);
		int CollectDataPage(unsigned int addr_begin, unsigned int addr_end);
		int CollectDataStream();

		HWIF* m_Device;
		unsigned char* m_temp_data_buffer;
		unsigned char* m_temp_data_buffer_ptr;
		int* m_data_buffer;
		int* m_revision;
		char* m_serial;
		unsigned int* m_MultiRecordHeader;
		unsigned int m_status;
		unsigned int m_no_bytes_last_received;
		unsigned int m_BcdDevice;
		int m_trig_lvl;
		int m_trig_edge;
		int m_clock_source;
		int m_freq_divider;
		int m_trig_mode;
		int m_trig_point;
		int m_overflow;
		int m_MultiRecordMode;
		unsigned int m_NumberOfInterleavedChannels;
        unsigned int m_IsStartedOK;
		unsigned int m_dram_phys_start_address128b;
		unsigned int m_dram_phys_end_address128b;
		unsigned int m_mr_backup_dram_phys_end_address128b;
		unsigned int m_read_nofbytes;
		unsigned int m_read_nof_pages;
		unsigned int m_SamplesPerRecord;
		unsigned int m_PagesPerRecord;
		unsigned int m_NofRecords;
		unsigned int m_buffersize;
		unsigned int m_SamplesPerWordNb;
		int m_SampleWidth;
		unsigned int m_page_count;
		unsigned int m_CacheWord128bStart;
		unsigned int m_CacheWord128bEnd;
		unsigned int m_WordNbBits;
		unsigned int m_WordsNbPerPage;
		unsigned int m_Words128bPerPage;
		unsigned int m_MaxPages;
		unsigned int m_DramLastAddress;
		unsigned int m_PreTrigWordsNb;
		unsigned int m_TriggerHoldOffWordsNb;
		unsigned int m_WordsNbAfterTrig;
		unsigned int m_TrigMask1;
        int m_TrigLevelResetValue;
		int m_TrigLevel1;
        int m_TrigPreLevel1;
		unsigned int m_TrigCompareMask1;
		unsigned int m_TrigMask2;
		int m_TrigLevel2;
        int m_TrigPreLevel2;
		unsigned int m_TrigCompareMask2;
		int m_StreamStatus;
		unsigned int m_DataFormat;
		int m_pageAddress128b; //CollectDataNextPage
		int m_wordNbOffset; //CollectDataNextPage & MultiRecordGetRecord
		int m_sampleOffset; //CollectDataNextPage & MultiRecordGetRecord
		int m_CurrentRecordNumber; //MultiRecordGetRecord
		unsigned int m_read_start_address; //MultiRecordGetRecord
		unsigned long long m_TrigTimeCycles;
		unsigned int m_TrigTimeSyncs;
		unsigned int m_TrigTimeStart;
		int m_TrigTimeMode;
        unsigned int m_TrigType;
        unsigned int m_TrigSyncForSampleSkip;
		unsigned int m_TrigVector[2];
		unsigned int m_TrigCounter;
		unsigned int m_RecordsCollected;
        unsigned int m_CacheSizeIn128BWords;
        int m_trigged_ch;
    
        unsigned __int64 m_SampleSkip;

        // Below are parameters needed to be able to use exact same function bodies 
        // for most functions across ADQ112, ADQ114 and ADQ214
        unsigned char parameter_pll_vcoset;
        float parameter_pll_basefreq;
        int parameter_interleaved_leaves;

        unsigned int m_typeOfADQ;
        unsigned int m_last_error;

        int* m_pData_int[8];
        char* m_pData_char[8];
        short int* m_pData_int16[8];

        unsigned char m_channels_mask;
        unsigned char m_target_bytes_per_sample;
        int m_max_samples_to_parse;

        unsigned int m_rev_fpga1;

	};
#endif //ADQ112_H
