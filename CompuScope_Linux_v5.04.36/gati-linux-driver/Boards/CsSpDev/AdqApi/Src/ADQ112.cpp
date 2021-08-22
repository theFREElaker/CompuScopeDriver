// Project: ADQ-API
// File: ADQ112.cpp
//
// latest revision: 2008-10-27
// by Peter Johansson

#include "ADQ112.h"
#include "ADQAPI_definitions.h"
#include <math.h>
#include <windows.h>

ADQ112::ADQ112()
  :	m_data_buffer(NULL)
{
}

int ADQ112::Open(HWIF* device)
{
	int success;
	int* rev;
	
    parameter_pll_basefreq = ADQ112_DEFAULT_PLL_FREQUENCY;
    parameter_pll_vcoset = ADQ112_DEFAULT_PLL_VCOSET; 
    parameter_interleaved_leaves = 2;

    m_Device = device;
	m_temp_data_buffer = NULL;
	m_temp_data_buffer_ptr = NULL;
	m_dram_phys_start_address128b = DRAM_PHYS_START_ADDRESS;
	m_dram_phys_end_address128b = DRAM_PHYS_END_ADDRESS;

    m_trig_edge = RISING_EDGE;
	m_status = 0;
	m_page_count = 0;
	m_CacheWord128bStart = ~0u;//no data in buffer
	m_CacheWord128bEnd = 0u;//no data in buffer
	m_trig_point = 0;
	m_overflow = 0;
	m_revision = new int[6];
	m_serial = new char[16];
	m_MultiRecordMode = 0;
	m_data_buffer = new int[MAX_DMA_TRANSFER]; //Oversize to be safe by assuming samples are 8-bit
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
    m_rev_fpga1 = rev[3];

	//Try to set up card with default values
    SetCacheSize(DEFAULT_CACHE_SIZE_IN_128B_WORDS*16); // 16 is due to byte/128bit relation
    SetTrigLevelResetValue(ADQ112_DEFAULT_LEVEL_TRIGGER_RESET);
    success = success && SetDataFormat(0);
	success = success && SetLvlTrigLevel(DEF_TRIG_LVL);
	success = success && SetLvlTrigEdge(DEF_TRIG_EDGE);
	success = success && WriteRegister((unsigned int)TRIG_SETUP_ADDR,(unsigned int)(~(LEVELTRIG_CHA_EN_BIT | LEVELTRIG_CHB_EN_BIT)),(unsigned int)(ANY_CH<<LEVELTRIG_CHA_EN_POS));
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

    // Set Trigger Delay circuit to synch external trigger and data stream
    // Note: Other content in FPGA #1 will require other setting of this.
    //       Allowed is 0-31 (counted in the clk domain)
    success = success && WriteRegister(TRIGGER_DELAY_ADDR, 0x0, ADQ112_DEFAULT_TRIGGER_DELAY);

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

ADQ112::~ADQ112()
{
	if (m_data_buffer != NULL)
		delete[] m_data_buffer;
}

int ADQ112::GetADQType(void)
{
    return (int)112;
}

unsigned int ADQ112::SetSampleSkip(unsigned int SkipFactor)
{
    unsigned int success = 0;
    unsigned short int SetVar = 0;

    // 0 = all samples
    // 1 = everyother sample (only ref channel)
    // 2 = every 4th sample (only ref channel)
    // 3 = every 8th sample (only ref channel)
    if (m_rev_fpga1 >= REV_FCN_FPGA1_SET_SAMPLE_SKIP)
    {
        if (    (SkipFactor == 1) 
            || (SkipFactor == 2)
            || ((SkipFactor % 4) == 0)
            && (SkipFactor <= (65535-1)*4))
        {
            if (SkipFactor == 1)
            {
                SetVar = 0;
            }
            else if (SkipFactor == 2)
            {
                SetVar = 1;
            }
            else if (SkipFactor > 2)
            {
                SetVar = (unsigned short int)(SkipFactor >> 2) + 1;        
            }
            success = WriteAlgoRegister(ALGO_SAMPLE_SKIP_ADDR, 0, SetVar);
            if (success)
            {
              m_SampleSkip = SkipFactor;
            }
        }
        else
        {
            success = 0;
        }
    }

    return success;
}

unsigned int ADQ112::GetSampleSkip(void)
{
    unsigned int value;
    unsigned int SkipFactor = 0;

    value = ReadAlgoRegister(ALGO_SAMPLE_SKIP_ADDR);

    if (value == 0)
    {
        SkipFactor = 1;
    }
    else if (value == 1)
    {
        SkipFactor = 2;
    }
    else
    {
        SkipFactor = (value - 1)*4;
    }

    return SkipFactor;

}


void* ADQ112::GetPtrData(unsigned int channel)
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

int ADQ112::ParseSampleData(unsigned int wordNbOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxSamples)
{
	unsigned int temp_A = 0;
	unsigned int temp_B = 0;
	int sampleOffsetStore = m_WordNbBits/m_SamplesPerWordNb; //Storage size for sample
	unsigned int bitOffset = wordNbOffset*m_WordNbBits +        //bit address to first sample (considering only one channel's data)
							 (sampleOffset/2)*sampleOffsetStore; // /2 for ADQ112
	unsigned int word128Offset = (bitOffset / 64) * 8 * 2;     //byte address to first 128 bit word
	unsigned int wordNOffset = (bitOffset % 64) / 8;       //byte address to first m_WordNbBits bit word relative 64 bit word
	unsigned int parsedSamples = wordNbOffset * m_SamplesPerWordNb * 2 + sampleOffset; //Count how many samples have been consumed in page // *2 for ADQ112
	bitOffset = bitOffset % 8;                             //bit address relative first byte
	sampleOffset = sampleOffset % 2;                       //for ADQ112 Even/odd starting point
  

	//Start parsing samples from first sample position
	int consumedBits = 0;
	unsigned int sampleMask = (1ull<<m_SampleWidth)-1;
	unsigned int sampleMax = sampleMask >> 1;
	unsigned int sampleMin = 1<<(m_SampleWidth-1);
	unsigned int j = wordNOffset;

    int signextend_shift_factor = (m_target_bytes_per_sample << 3) - m_SampleWidth; 

	for (unsigned int i=word128Offset; i<m_Words128bPerPage*16; i+=16)
	{
		while (j<8)
		{
			temp_A |= ((((unsigned int)m_temp_data_buffer_ptr[i+j  ])>>bitOffset) & (sampleMask >> consumedBits)) << consumedBits;
			temp_B |= ((((unsigned int)m_temp_data_buffer_ptr[i+j+8])>>bitOffset) & (sampleMask >> consumedBits)) << consumedBits;
			consumedBits += (8-bitOffset);

			if (consumedBits >= sampleOffsetStore)
			{
				bitOffset = 8-(consumedBits-sampleOffsetStore);
				consumedBits = 0;
				if (bitOffset == 8)
				{
					bitOffset = 0;
					j++;
				}

				if ((temp_A == sampleMax) || (temp_A == sampleMin) ||
					(temp_B == sampleMax) || (temp_B == sampleMin))
				{
					m_overflow = 1;
				}
				 //Sign extend (m_WordNbBits/m_SamplesPerWordNb) bit data (in 32 bit CPU)
				if (sampleOffset == 0) //for ADQ112
				{

                if (m_channels_mask & 0x1)
                {
                    switch(m_target_bytes_per_sample)
                    {
                    case 2:
                        *(m_pData_int16[0]+destOffset) = (short int)(temp_A << signextend_shift_factor) >> signextend_shift_factor;
                        break;
                    case 4:
                        *(m_pData_int[0]+destOffset) = (int)(temp_A << signextend_shift_factor) >> signextend_shift_factor;
                        break;
                    }
                }
					if (++destOffset >= maxSamples)
					{
						return destOffset;
					}
					if (++parsedSamples >= m_WordsNbPerPage*m_SamplesPerWordNb)
					{
						return destOffset;
					}
				}
				sampleOffset = 0;  //only skip samples in the first iteration

                if (m_channels_mask & 0x1)
                {
                    switch(m_target_bytes_per_sample)
                    {
                    case 2:
                        *(m_pData_int16[0]+destOffset) = (short int)(temp_B << signextend_shift_factor) >> signextend_shift_factor;
                        break;
                    case 4:
                        *(m_pData_int[0]+destOffset) = (int)(temp_B << signextend_shift_factor) >> signextend_shift_factor;
                        break;
                    }
                }

				if (++destOffset >= maxSamples)
				{
					return destOffset;
				}
				if (++parsedSamples >= m_WordsNbPerPage*m_SamplesPerWordNb)
				{
					return destOffset;
				}
				temp_A = 0;
				temp_B = 0;
			}
			else
			{
				bitOffset = 0;
				j++;
			}
		}
		j = 0; //only skip samples (in the 64bit words) in the first iteration
	}
	// Returning from here should never happen
	return destOffset;
}

int ADQ112::SetAlgoStatus(int status)
{
	status = (status)? 1 : 0;
	return WriteAlgoRegister(ALGO_ALGO_ENABLED_ADDR, 0, status);
}

int ADQ112::SetAlgoNyquistBand(unsigned int band)
{
	int status = 0;
	if (band <= 1)
		status = WriteAlgoRegister(ALGO_ALGO_NYQUIST_ADDR, 0, band);
	return status;
}

int ADQ112::SetDataFormat(unsigned int format)
{
	int success;

	success = WriteAlgoRegister(ALGO_DATA_FORMAT_ADDR, 0, format);
	m_DataFormat = format;

	switch(format)
	{
	case 0: //Packed N-bit data
		success = success && SetNofBits(N_WORDBITS_ADQ112); //Default for ADQ214 R0910
		success = success && SetSampleWidth(N_BITS_ADQ112); //Default for ADQ214 R0910
		break;
	case 1: //Unpacked N-bit data
		success = success && SetNofBits(32); //Default for ADQ214 R0910
		success = success && SetSampleWidth(N_BITS_ADQ112); //Default for ADQ214 R0910
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
				success = SetWordsPerPage(DEF_WORDS_PER_PAGE_112);
	return success;
}