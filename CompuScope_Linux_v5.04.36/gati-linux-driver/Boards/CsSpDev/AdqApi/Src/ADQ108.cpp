// Project: ADQ-API
// File: ADQ108.cpp

#include "ADQ108.h"
#include "ADQAPI_definitions.h"
#include <math.h>
#include <windows.h>

ADQ108::ADQ108()
  :	m_data_buffer(NULL)
{
}

int ADQ108::Open(HWIF* device)
{
	int success;
	int* rev;
	
    parameter_pll_basefreq = ADQ108_DEFAULT_PLL_FREQUENCY;
    parameter_pll_vcoset = ADQ108_DEFAULT_PLL_VCOSET; 
    parameter_interleaved_leaves = 2;

    m_Device = device;
	m_temp_data_buffer = NULL;
	m_temp_data_buffer_ptr = NULL;
	m_dram_phys_start_address128b = DRAM_PHYS_START_ADDRESS;
	m_dram_phys_end_address128b = DRAM_PHYS_END_ADDRESS;

	m_status = 0;
	m_page_count = 0;
	m_CacheWord128bStart = ~0u;//no data in buffer
	m_CacheWord128bEnd = 0u;//no data in buffer
	m_trig_point = 0;
	m_overflow = 0;
	m_revision = new int[6];
	m_serial = new char[16];
	m_MultiRecordMode = 0;
	m_data_buffer = new char[MAX_DMA_TRANSFER]; //Oversize to be safe by assuming samples are 8-bit
	m_MultiRecordHeader = new unsigned int[8];
	m_TrigTimeMode = 0;
    m_max_samples_to_parse = 0;
	// Check if new type of version numbering
	if (m_Device->HwVersion() < 5)
		success = 0;
	else
		success = 1;
	
	rev = GetRevision();
	m_BcdDevice = rev[0];

	//Try to set up card with default values
    SetCacheSize(DEFAULT_CACHE_SIZE_IN_128B_WORDS*16); // 16 is due to byte/128bit relation
    SetTrigLevelResetValue(ADQ108_DEFAULT_LEVEL_TRIGGER_RESET);
    success = success && SetDataFormat(0);
	success = success && SetLvlTrigLevel(DEF_TRIG_LVL);
	success = success && SetLvlTrigEdge(DEF_TRIG_EDGE);
	success = success && WriteRegister(TRIG_SETUP_ADDR,~(LEVELTRIG_CHA_EN_BIT | LEVELTRIG_CHB_EN_BIT),ANY_CH<<LEVELTRIG_CHA_EN_POS);
	success = success && SetClockSource(DEF_CLOCK_SOURCE);
	success = success && SetPllFreqDivider(DEF_FREQ_DIV); // Backwards compatible
	//success = success && SetPll(DEF_FREQ_N_DIV_112, DEF_FREQ_R_DIV, DEF_FREQ_VCO_DIV, DEF_FREQ_CH_DIV);
	success = success && SetTriggerMode(DEF_TRIG_MODE);
	success = success && SetPreTrigWords(0);
	success = success && SetTriggerHoldOffSamples(0);
	success = success && SetWordsPerPage(DEF_WORDS_PER_PAGE_112); //Default for ADQ214 0902_B
	success = success && SetBufferSize(DEF_BUFF_SIZE_SAMPLES);
   	success = success && SetStreamStatus(0);
	m_mr_backup_dram_phys_end_address128b = m_dram_phys_end_address128b;
   	success = success && MultiRecordClose();
   	success = success && SetAlgoStatus(1);
   	success = success && SetAlgoNyquistBand(0);

	// Get Serial number string from board
	if (success)
	{
		RetrieveBoardSerialNumber();
	}

    if (success)
    {
        m_IsStartedOK = 1;
    }
    else
    {
        m_IsStartedOK = 0;
    }
	return success;
}

ADQ108::~ADQ108()
{
	if (m_data_buffer != NULL)
		delete[] m_data_buffer;
}

int ADQ108::GetADQType(void)
{
    return (int)108;
}

void* ADQ108::GetPtrData(unsigned int channel)
{
    void* rtnptr = NULL;

    switch(channel)
    {
    case 1:
        rtnptr = (void*)m_data_buffer;
        break;
    default:
        m_last_error = ERROR_CODE_CHANNEL_NOT_AVAILABLE_ON_DEVICE;
        break;
    }

    return rtnptr;
}

int ADQ108::ParseSampleData(unsigned int , unsigned int , unsigned int destOffset, unsigned int )
{
	return destOffset;
}

int ADQ108::SetAlgoStatus(int status)
{
	status = (status)? 1 : 0;
	return WriteAlgoRegister(ALGO_ALGO_ENABLED_ADDR, 0, status);
}

int ADQ108::SetAlgoNyquistBand(unsigned int band)
{
	int status = 0;
	if (band <= 1)
		status = WriteAlgoRegister(ALGO_ALGO_NYQUIST_ADDR, 0, band);
	return status;
}

int ADQ108::SetDataFormat(unsigned int format)
{
	int success;

	success = WriteAlgoRegister(ALGO_DATA_FORMAT_ADDR, 0, format);
	m_DataFormat = format;

	switch(format)
	{
	case 0: //Packed N-bit data
		success = success && SetNofBits(N_WORDBITS_ADQ108); //Default for ADQ214 R0910
		success = success && SetSampleWidth(N_BITS_ADQ108); //Default for ADQ214 R0910
		break;
	case 1: //Unpacked N-bit data
		success = success && SetNofBits(32); //Default for ADQ214 R0910
		success = success && SetSampleWidth(N_BITS_ADQ108); //Default for ADQ214 R0910
		break;
	case 2: //16-bit data
		success = success && SetNofBits(32);
		success = success && SetSampleWidth(16);
		break;
	case 3: //32-bit data
		success = success && SetNofBits(32);
		success = success && SetSampleWidth(32);
		break;
	default:
		success = 0;
	}
	// Re-configure wordsPerPage
	if (success && !SetWordsPerPage(m_WordsNbPerPage))
				success = SetWordsPerPage(DEF_WORDS_PER_PAGE_214);
	return success;
}