// Project: ADQ-API
// File: ADQ214.cpp
//
// latest revision: 2008-10-27
// by Peter Johansson

#include "ADQ214.h"
#include "ADQAPI_definitions.h"
#include <math.h>
#include <windows.h>

ADQ::ADQ()
  :	m_revision(NULL),
	m_serial(NULL),
	m_MultiRecordHeader(NULL),
	m_Device(NULL)
{
    m_SampleSkip = 1;
}

ADQ::~ADQ()
{
	if (m_revision != NULL)
		delete[] m_revision;

	if (m_serial != NULL)
		delete[] m_serial;
	
	if (m_MultiRecordHeader != NULL)
		delete[] m_MultiRecordHeader;

	if (m_Device != NULL)
		delete m_Device;
}

int*    ADQ::GetPtrData(void)               
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return NULL;
};

int*    ADQ::GetPtrDataChA(void)            
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return NULL;
};

int*    ADQ::GetPtrDataChB(void)            
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return NULL;
};

int     ADQ::SetLvlTrigChannel(int ) 
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

int     ADQ::GetLvlTrigChannel(void)        
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

int     ADQ::GetTriggedCh(void)             
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

int     ADQ::SetAlgoNyquistBand(unsigned int )
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

int     ADQ::SetAlgoStatus(int )
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

unsigned int ADQ::SetSampleSkip(unsigned int )
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

unsigned int ADQ::GetSampleSkip(void)
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

unsigned int ADQ::SetSampleDecimation(unsigned int )
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

unsigned int ADQ::GetSampleDecimation(void)
{
    m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

int ADQ::SetAfeSwitch(unsigned int )
{
	m_last_error = ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE;
    return 0;
};

unsigned int ADQ::GetLastError(void)
{
    unsigned int returnvalue = m_last_error;

    m_last_error = ERROR_CODE_NO_ERROR_OCCURRED;
    
    return returnvalue;
}


void* ADQ::GetPtrStream(void)
{
	return (void*)m_temp_data_buffer;
}

unsigned int ADQ::IsStartedOK(void)
{
	return m_IsStartedOK;
}



int ADQ::SetLvlTrigLevel(int level)
{
	int success = 0;
	unsigned int sampleOffset = m_WordNbBits/m_SamplesPerWordNb; //Sample storage size (may differ from sample width)
	unsigned int sampleMask = (1ull<<m_SampleWidth)-1;
	unsigned int sampleSign = 1<<(m_SampleWidth-1);
	int sampleMax = (int)(sampleMask >> 1);
	int sampleMin = ((int)sampleSign) << -m_SampleWidth >> -m_SampleWidth;
	unsigned int sampleMask2 = sampleMask << sampleOffset;
	unsigned int sampleSign2 = sampleSign << sampleOffset;
    unsigned int InterleavingFlag = ((parameter_interleaved_leaves - 1) > 0);

	if ((level >= sampleMin) && (level <= sampleMax))
	{
		if (m_SamplesPerWordNb == 2)
		{
			// Trig level is set in N-bit binary offset value
			success = SetTrigLevel1((level & sampleMask)^sampleSign);
			success = success && SetTrigMask1(sampleMask);
			// TrigCompareMask is used to convert data to N-bit binary offset
			success = success && SetTrigCompareMask1(sampleSign);
			success = success && SetTrigLevel2(((level<<sampleOffset) & sampleMask2)^sampleSign2);
			success = success && SetTrigMask2(sampleMask2);
			success = success && SetTrigCompareMask2(sampleSign2);
			success = success && WriteRegister(TRIG_SETUP_ADDR,
				~(LEVELTRIG_2SAMPLESPERWORD_BIT | LEVELTRIG_INTERLEAVED_BIT),
				((1<<LEVELTRIG_2SAMPLESPERWORD_POS) + (InterleavingFlag<<LEVELTRIG_INTERLEAVED_POS)));
        
        }
		else //m_SamplesPerWordNb == 1
		{
			success = SetTrigLevel1(~0);
			success = success && SetTrigMask1(0);
			success = success && SetTrigCompareMask1(0);
			success = success && SetTrigLevel2(level^sampleSign);
			success = success && SetTrigMask2(sampleMask);
			success = success && SetTrigCompareMask2(sampleSign);
			success = success && WriteRegister(TRIG_SETUP_ADDR,
				~(LEVELTRIG_2SAMPLESPERWORD_BIT | LEVELTRIG_INTERLEAVED_BIT),
				((0<<LEVELTRIG_2SAMPLESPERWORD_POS) + (InterleavingFlag<<LEVELTRIG_INTERLEAVED_POS)));
		}

        if (success) m_trig_lvl = level;

        if (success && (m_BcdDevice >= REV_FCN_SET_TRIGGER_PRE_LEVEL))
        {
            success = success && SetLvlTrigEdge(m_trig_edge);
        }

	}
	return success;
}


int ADQ::SetLvlTrigEdge(int edge)
{
	int success = 0;
    int PreLevel;
	unsigned int sampleOffset = m_WordNbBits/m_SamplesPerWordNb; //Sample storage size (may differ from sample width)
	unsigned int sampleMask = (1ull<<m_SampleWidth)-1;
	unsigned int sampleSign = 1<<(m_SampleWidth-1);
	int sampleMax = (int)(sampleMask >> 1);
	int sampleMin = ((int)sampleSign) << -m_SampleWidth >> -m_SampleWidth;
	unsigned int sampleMask2 = sampleMask << sampleOffset;
	unsigned int sampleSign2 = sampleSign << sampleOffset;

    if ((edge == RISING_EDGE) || (edge == FALLING_EDGE))
    {
        success = WriteRegister(TRIG_SETUP_ADDR,~LEVELTRIG_POSEDGE_BIT,edge<<LEVELTRIG_POSEDGE_POS);

        if (success && (m_BcdDevice >= REV_FCN_SET_TRIGGER_PRE_LEVEL))
        {
            if (edge == RISING_EDGE)
            {
                PreLevel = m_trig_lvl - m_TrigLevelResetValue;
                if (PreLevel <= sampleMin) 
                {
                    PreLevel = sampleMin + 1;
                }
            }
            else
            {
                PreLevel = m_trig_lvl + m_TrigLevelResetValue;
                if (PreLevel >= sampleMax) 
                {
                    PreLevel = sampleMax - 1;
                }            
            }

       		if (m_SamplesPerWordNb == 2)
       		{
                success = success && SetTrigPreLevel1((PreLevel & sampleMask)^sampleSign);  
                success = success && SetTrigPreLevel2(((PreLevel<<sampleOffset) & sampleMask2)^sampleSign2); 
            }
            else
            {
                success = success && SetTrigPreLevel1(~0);  
                success = success && SetTrigPreLevel2(PreLevel^sampleSign);           
            }
        }

        if (success)
        {
            m_trig_edge = edge;
        }
    }
	return success;
}

int ADQ::SetClockSource(int source)
{
	int success = 0;
	if ((source == INT_INTREF) || (source == INT_EXTREF))
	{
		Sleep(10);
		unsigned char message[6];
		message[0] = 6;
		message[1] = PROCESSOR_CMD;
		message[2] = 0;
		message[3] = SET_CLOCK_REFERENCE;
		message[4] = 0;
		message[5] = (unsigned char)source;
		m_clock_source = source;
		m_Device->ReceiveStatusInit();
		success = m_Device->SendCommand(message);
		success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
		success = success && (m_status != 0);
		Sleep(10);
	}
	else if (source == EXT)
	{
		Sleep(10);
		unsigned char message[6];
		message[0] = 6;
		message[1] = PROCESSOR_CMD;
		message[2] = 0;
		message[3] = SET_EXT_CLK_SOURCE;
		message[4] = 0;
		message[5] = 0; 
		m_clock_source = source;
		m_Device->ReceiveStatusInit();
		success = m_Device->SendCommand(message);
		success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
		success = success && (m_status != 0);
		Sleep(10);
	}
	return success;
}

int ADQ::SetClockFrequencyMode(unsigned int clockmode)
{
    int success = 0;
	
	if (m_BcdDevice < REV_FCN_SET_CLOCK_FREQUENCY_MODE)
	{
		// Handle non-supported function
		// Always high frequency
		if (clockmode == 1)
		{
			return 1;
		}
		return 0;
	}

    if (clockmode == 0 || clockmode == 1)
    {
        unsigned int mcu2drpreg = 0;
        unsigned int mcu2drpreg_e = 0;
        unsigned int answer;
        unsigned int answer_valid;
		unsigned int dcm_locked;
        unsigned int newdata;
        unsigned int timeoutctr;
        unsigned int clockmodenow;
        unsigned int addr_to_dcm = 0x51;

        success = 1;
        // Read DCM current status
        mcu2drpreg = (addr_to_dcm << 16);
        success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP address
        mcu2drpreg_e = mcu2drpreg | (1L << 25);
        success = success && WriteRegister(23, 0, mcu2drpreg_e); // Write DRP enable
        timeoutctr = 0;
        answer = ReadRegister(24);
        answer_valid = ((answer & 0x00010000) >> 16);

        while(!answer_valid && timeoutctr < 100)
        {
            answer = ReadRegister(24);
            answer_valid = ((answer & 0x00010000) >> 16);
            Sleep(1);
            timeoutctr++;
        }
        success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP no enable
		if (timeoutctr > 100) success = 0;
        clockmodenow = ((answer & 0x00000000C) >> 2);
        clockmodenow >>= 1;
        
        if (clockmodenow != clockmode)
        {
            if (clockmode == 1)
            {
                newdata = answer | 0x000C;
            }
            else
            {
                newdata = answer & 0xFFF3;
            }
			newdata &= 0x0000FFFF;
            // Write new mode
            //              address             data       Reset         WE
            mcu2drpreg = (addr_to_dcm << 16) | newdata | (1L << 23) | (1L << 24);
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP address, data, reset and WE
            mcu2drpreg_e = mcu2drpreg | (1L << 25);
            success = success && WriteRegister(23, 0, mcu2drpreg_e); // Write DRP enable
            timeoutctr = 0;
            answer = ReadRegister(24);
            answer_valid = ((answer & 0x00010000) >> 16);
			dcm_locked = ((answer & 0x00020000) >> 17);
            while(success && !answer_valid && timeoutctr < 100)
            {
                answer = ReadRegister(24);
                answer_valid = ((answer & 0x00010000) >> 16);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP no enable
			if (timeoutctr > 100) success = 0;

            // Read DCM current status
            //              address              Reset
            mcu2drpreg = (addr_to_dcm << 16) | (1L << 23);
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP address
            mcu2drpreg_e = mcu2drpreg | (1L << 25);
            success = success && WriteRegister(23, 0, mcu2drpreg_e); // Write DRP enable
            timeoutctr = 0;
            answer = ReadRegister(24);
            answer_valid = ((answer & 0x00010000) >> 16);
			dcm_locked = ((answer & 0x00020000) >> 17);

            while(success && !answer_valid && timeoutctr < 100)
            {
                answer = ReadRegister(24);
                answer_valid = ((answer & 0x00010000) >> 16);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP no enable
			if (timeoutctr > 100) success = 0;
            clockmodenow = ((answer & 0x00000000C) >> 2);
            clockmodenow >>= 1;

            // Read address 0 to reset DCM DRP output
            //            address       Reset
            mcu2drpreg = (0 << 16) | (1L << 23);
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP address
            mcu2drpreg_e = mcu2drpreg | (1L << 25);
            success = success && WriteRegister(23, 0, mcu2drpreg_e); // Write DRP enable
            timeoutctr = 0;
            answer = ReadRegister(24);
            answer_valid = ((answer & 0x00010000) >> 16);

            while(success && !answer_valid && timeoutctr < 100)
            {
                answer = ReadRegister(24);
                answer_valid = ((answer & 0x00010000) >> 16);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP no enable
			if (timeoutctr > 100) success = 0;

            // Release reset
            success = success && WriteRegister(23, 0, 0);

            timeoutctr = 0;
            while(success && !dcm_locked && timeoutctr < 100)
            {
                answer = ReadRegister(24);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
			if (timeoutctr > 100) success = 0;
			if (!dcm_locked) success = 0;

            if (clockmodenow != clockmode)
            {
                success = 0;
            }
/*
			if (success && (clockmode == 0)) // Fix phase offset in low frequency mode.
			{
				for(int j=0; j<256; j++)
				{
					timeoutctr = 0;
					success = success && WriteRegister(0,~(1<<11),(1<<11)); //INC R
					success = success && WriteRegister(0,~(1<<11),(0<<11)); //INC F
					if (success) while(!(ReadRegister(1) & (1<<2)) && (++timeoutctr < 100));
					if (timeoutctr >= 100)
					{
						success = 0;
						break;
					}
				}
			}
*/

        }
		else
		{
		    // Read address 0 to reset DCM DRP output
            //            address 
            mcu2drpreg = (0 << 16);
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP address
            mcu2drpreg_e = mcu2drpreg | (1L << 25);
            success = success && WriteRegister(23, 0, mcu2drpreg_e); // Write DRP enable
            timeoutctr = 0;
            answer = ReadRegister(24);
            answer_valid = ((answer & 0x00010000) >> 16);
			dcm_locked = ((answer & 0x00020000) >> 17);

            while(success && !answer_valid && timeoutctr < 100)
            {
                answer = ReadRegister(24);
                answer_valid = ((answer & 0x00010000) >> 16);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
            success = success && WriteRegister(23, 0, mcu2drpreg); // Write DRP no enable
			if (timeoutctr > 100) success = 0;

            timeoutctr = 0;
			while(success && !dcm_locked && timeoutctr < 100)
            {
                answer = ReadRegister(24);
				dcm_locked = ((answer & 0x00020000) >> 17);
                Sleep(1);
                timeoutctr++;
            }
			if (timeoutctr > 100) success = 0;
			if (!dcm_locked) success = 0;
		}
    }
    return success;
}

int ADQ::SetPllFreqDivider(int divider)
{
	int success = 0;
	if ((divider >= 2) && (divider <= 20))
	{
		Sleep(10);

		unsigned char m = 0;
		unsigned char n = 0;

		m = (unsigned char) ceil((divider-2)/2.0f);
		n = (unsigned char) floor((divider-2)/2.0f);
		
		unsigned char message[6];
		message[0] = 6;
		message[1] = PROCESSOR_CMD;
		message[2] = 0;
		message[3] = SET_PLL_DIVIDER;
        message[4] = parameter_pll_vcoset;
		message[5] = (m << 4) | n; 
		m_freq_divider = divider;
		m_Device->ReceiveStatusInit();
		success = m_Device->SendCommand(message);
		success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
		success = success && (m_status != 0);

		Sleep(10);
		// Now, select low or high frequency mode
		// Only possible to know with internal clock and internal reference
		// otherwise user must by himself use the SetClockFrequencyMode function
		
		if (success)
		{
			if (m_clock_source == INT_INTREF)
			{
                float BaseFreq = parameter_pll_basefreq;
				float RequiredDDRFrequency = BaseFreq / (float)divider / 2.0f;

				if (RequiredDDRFrequency < DCM_FREQUENCY_LIMIT)
				{
					success = success && SetClockFrequencyMode(0); // Go to low freqeuncy mode
				}
				else
				{
					success = success && SetClockFrequencyMode(1); // Go to high frequency mode
				}
			}
			Sleep(10);
		}
	}
	return success;
}

int ADQ::SetPll(int n_divider, int r_divider, int vco_divider, int channel_divider) // REF*M/D = SMPS
{
	int success = 1;
	int a(1);
	int b(1);
	int p = 8; //Minimum div by 8 (max 32)
	int bp_mn = 0;
	int mn;
	int invout2 = 0; // Rev E inverts clock on PCB
	unsigned int arg1, arg2, arg3;

	if ((r_divider < 1) || (r_divider > 16383)) success = 0;
	if ((channel_divider < 1) || (channel_divider > 32)) success = 0;
	if ((vco_divider < 2) || (vco_divider > 6)) success = 0;

	// Find P and B values
	if (success)
	{
		b = n_divider / p;
		while (b > 8191)
		{
			p *= 2;
			b /= 2;
			if (p > 32) success = 0;
		}
		if (b < 3) success = 0;
	}

	// Find A value
	if (success)
	{
		a = n_divider - b * p;
		if (a > 63) success = 0;

		// Convert P value to HW
		switch(p)
		{
		case 8:
			p = 4;
			break;
		case 16:
			p = 5;
			break;
		case 32:
			p = 6;
			break;
		default:
			success = 0;
			break;
		}
	}

	// Find channel M & N and set hardware
	if (success)
	{
		if (channel_divider == 1)
		{
			bp_mn = 1;
			mn = 0;
		}
		else
		{
			mn = ((unsigned char)ceil((float)(channel_divider - 2) / 2.0f) << 4)
				| (unsigned char)floor((float)(channel_divider - 2) / 2.0f);
		}

		arg1 = r_divider & 0x00003FFF;
		arg2 = (a & 0x0000003F)
			| ((b << 8) & 0x001FFF00)
			| ((p << 24) & 0x0F000000);
		arg3 = ((vco_divider-2) & 0x00000007)
			| ((bp_mn << 4) & 0x00000010)
			| ((mn << 8) & 0x0000FF00)
			| ((invout2 << 16) & 0x00FF0000);
		success = success && SendProcessorCommand(SET_PLL_ADV, arg1, arg2, arg3);
		m_freq_divider = channel_divider;
		
		if (success && (m_clock_source == INT_INTREF))
		{
			float RequiredDDRFrequency = (INTERNAL_REFERENCE_FREQUENCY * (float)n_divider) / ((float)r_divider * (float)vco_divider * (float)channel_divider * 2.0f);

			if (RequiredDDRFrequency < DCM_FREQUENCY_LIMIT)
			{
				success = success && SetClockFrequencyMode(0); // Go to low freqeuncy mode
			}
			else
			{
				success = success && SetClockFrequencyMode(1); // Go to high frequency mode
			}
		}
	}

	return success;
}

int ADQ::SetTriggerMode(int trig_mode)
{
	int success = 0;
    int trig_mode_to_set;
	if ((trig_mode == SET_SW_TRIGGER_MODE) || (trig_mode == SET_EXT_TRIGGER_MODE) || (trig_mode == SET_LEVEL_TRIGGER_MODE))
	{       
        trig_mode_to_set = (1 << trig_mode) | (1 << SET_SW_TRIGGER_MODE); // Always allow software trigger
		success = WriteRegister(TRIG_SETUP_ADDR,~(HOST_TRIG_ALLOWED_BIT | EXTERN_TRIG_ALLOWED_BIT | LEVELTRIG_ALLOWED_BIT),trig_mode_to_set);
		m_trig_mode = trig_mode;
	}
	return success;
}

int ADQ::SetTrigTimeMode(int TrigTimeMode)
{
	int success = 0;
	if ((TrigTimeMode == TRIGTIME_SYNC_OFF) || (TrigTimeMode == TRIGTIME_SYNC_ON))
	{
		success = WriteRegister(TIMER_CTRL_ADDR, ~(TRIG_TIME_SYNC_BIT), TrigTimeMode << TRIG_TIME_SYNC_POS);
		m_TrigTimeMode = TrigTimeMode;
	}
	return success;
}
int ADQ::ResetTrigTimer(int TrigTimeRestart)
{
	int success = 0;
	success = WriteRegister(TIMER_CTRL_ADDR, ~(TRIG_TIME_SETUP_BIT), 1 << TRIG_TIME_SETUP_POS); // Reset high
	if (TrigTimeRestart == TRIGTIME_RESTART)
	{
		success += WriteRegister(TIMER_CTRL_ADDR, ~(TRIG_TIME_SETUP_BIT | TRIG_TIME_START_BIT), 1 << TRIG_TIME_START_POS); // Reset low and start high
		success += WriteRegister(TIMER_CTRL_ADDR, ~(TRIG_TIME_START_BIT), 0); // start low
	}
	else
	{
		success += WriteRegister(TIMER_CTRL_ADDR, ~(TRIG_TIME_SETUP_BIT), 0); // Reset low
	}
	return success;

}
int ADQ::SetBufferSizePages(unsigned int pages)
{
	int success = 0;
	if ((pages >= 1) && (pages <= m_MaxPages))
		success = SetBufferSizeWords(m_WordsNbPerPage*pages);  
	return success;
}

int ADQ::SetBufferSizeWords(unsigned int words)
{
	int success = 0;
	if (words > m_PreTrigWordsNb)
	{
		success = SetWordsAfterTrig(words-m_PreTrigWordsNb);
		m_buffersize = words;
	}
	return success;
}

int ADQ::SetBufferSize(unsigned int samples)
{
	int success = 0;
	unsigned int wordsize;

    if (samples <= GetMemoryNumberOfWords128b() * 64 / m_WordNbBits * m_SamplesPerWordNb * parameter_interleaved_leaves)
	{
		wordsize = (samples+m_SamplesPerWordNb-1)/m_SamplesPerWordNb; //round up
		success = SetBufferSizeWords(wordsize);
	}
	return success;
}

int ADQ::SetNofBits(unsigned int NofBits)
{
	int success = 0;
	if ((NofBits >= 1) && (NofBits <= 32))
	{
		success = WriteRegister(N_OF_BITS_ADDR,0,NofBits-1);
		m_WordNbBits = NofBits;
	}
	return success;
}

int ADQ::SetSampleWidth(unsigned int SampleWidth)
{
	int success = 0;
	if ((SampleWidth >= 1) && (SampleWidth <= 32)
		&& (m_WordNbBits/SampleWidth >= 1) && (m_WordNbBits/SampleWidth <= 2))
	{
		m_SamplesPerWordNb = m_WordNbBits/SampleWidth;
		m_SampleWidth = SampleWidth;
		success = 1;
	}
	return success;
}

unsigned int ADQ::GetMemoryNumberOfWords128b(void)
{
	return ((m_dram_phys_end_address128b - m_dram_phys_start_address128b) + 1);
}


int ADQ::SetWordsPerPage(unsigned int WordsPerPage)
{
	int success = 0;
    unsigned int DRAMWordsPerPage = ((WordsPerPage/parameter_interleaved_leaves) * m_WordNbBits / 64);

    // Samples per page must also be aligned
	if ((WordsPerPage >= 1)
		&& (DRAMWordsPerPage <= MAX_PAGE_SIZE128B)
        && (DRAMWordsPerPage * 64 / m_WordNbBits == (WordsPerPage/parameter_interleaved_leaves))
        && (WordsPerPage % parameter_interleaved_leaves == 0))
	{
        WordsPerPage /= parameter_interleaved_leaves;

		if (!m_MultiRecordMode)
			success = 1;

		// page alignment HAS to be on memory borders
		if (success) m_MaxPages = (DRAM_PHYS_END_ADDRESS + 1 - m_dram_phys_start_address128b) / (WordsPerPage * m_WordNbBits / 64);
		success = success && WriteRegister(MAX_PAGES_ADDR, 0, m_MaxPages-1);

		// Set new end address
		if (success) m_dram_phys_end_address128b = m_dram_phys_start_address128b + m_MaxPages * (WordsPerPage * m_WordNbBits / 64) - 1;

		success = success && WriteRegister(DRAM_PHYS_END_ADDR, 0, m_dram_phys_end_address128b);
		success = success && WriteRegister(DRAM_PHYS_START_ADDR, 0, m_dram_phys_start_address128b);
		success = success && WriteRegister(WORDS_PER_PAGE_ADDR,0,WordsPerPage-1);
		if (success)
		{
            m_WordsNbPerPage = WordsPerPage * parameter_interleaved_leaves;
			m_Words128bPerPage = (WordsPerPage * m_WordNbBits / 64);
		}
	}
	return success;
}

int ADQ::SetPreTrigSamples(unsigned int PreTrigSamples)
{
	int success;
	int PreTrigWords = (PreTrigSamples+m_SamplesPerWordNb-1)/m_SamplesPerWordNb; //Round up samples to whole words
	success = SetPreTrigWords(PreTrigWords);
	success = success && SetWordsAfterTrig(m_buffersize-PreTrigWords);
	return success;
}

int ADQ::SetTriggerHoldOffSamples(unsigned int TriggerHoldOffSamples)
{
	int success = 1;
	int RoundUpValue;

	// First handle unsupported function
	if (m_BcdDevice < REV_FCN_SET_TRIGGER_HOLDOFF_SAMPLES)
	{
		// Trigger hold off is always zero
		if (TriggerHoldOffSamples == 0)
		{
			return 1;
		}
		return 0;
	}
	
	int TriggerHoldOffWords = (TriggerHoldOffSamples + m_SamplesPerWordNb-1)/m_SamplesPerWordNb;
    RoundUpValue = (TriggerHoldOffWords+parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
	success = success && WriteRegister(TRIGGER_HOLD_OFF_ADDR, 0, RoundUpValue);
	
	if (success)
	{
        m_TriggerHoldOffWordsNb = ((TriggerHoldOffWords+parameter_interleaved_leaves-1)/parameter_interleaved_leaves)
            *parameter_interleaved_leaves;
		
		if (m_TriggerHoldOffWordsNb > 0 && m_PreTrigWordsNb > 0)
		{
			// Need to reset pretrigger when using TriggerHoldOff function
			success = success && SetPreTrigWords(0);
			success = success && SetWordsAfterTrig(m_buffersize); 
		}
	}
	return success;
}

int ADQ::SetPreTrigWords(unsigned int PreTrigWords)
{
	int success = 0;
	int RoundUpValue;
    if (PreTrigWords <= ((1<<30)-1))
    {
        
        RoundUpValue = (PreTrigWords+parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
		success = WriteRegister(PRE_TRIG_WORDS_ADDR,0,RoundUpValue);
        m_PreTrigWordsNb = RoundUpValue*parameter_interleaved_leaves;     

		if (success)
		{
			if (m_PreTrigWordsNb > 0 && m_TriggerHoldOffWordsNb > 0)
			{
				// Need to reset TriggerHoldOff when using Pretrigger function
				success = success && SetTriggerHoldOffSamples(0);
			}
		}
	} 
	return success;
}

int ADQ::SetWordsAfterTrig(unsigned int WordsAfterTrig)
{
	int success = 0;
	int RoundUpValue;
    WordsAfterTrig += (parameter_interleaved_leaves+parameter_interleaved_leaves); // Work-around for last sample bugs // add extra word since trigger may happen in two positions
	if (WordsAfterTrig <= ((1<<30)-1))
	{
        RoundUpValue = (WordsAfterTrig + parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
        success = WriteRegister(WORDS_AFTER_TRIG_ADDR,0,RoundUpValue-1);
        m_WordsNbAfterTrig = RoundUpValue*parameter_interleaved_leaves;
	}
	return success;
}

int ADQ::SetTrigLevelResetValue(int OffsetValue)
{
    m_TrigLevelResetValue = OffsetValue;
    return 1;
}

int ADQ::SetTrigMask1(unsigned int TrigMask)
{
	m_TrigMask1 = TrigMask;
	return WriteRegister(TRIG_MASK1_ADDR,0,TrigMask);
}

int ADQ::SetTrigLevel1(int TrigLevel)
{
	m_TrigLevel1 = TrigLevel;
	return WriteRegister(TRIG_LEVEL1_ADDR,0,TrigLevel);
}

int ADQ::SetTrigPreLevel1(int TrigLevel)
{
	m_TrigPreLevel1 = TrigLevel;
    return WriteRegister(TRIG_PRE_LEVEL1_ADDR,0,TrigLevel);
}

int ADQ::SetTrigCompareMask1(unsigned int TrigCompareMask)
{
	m_TrigCompareMask1 = TrigCompareMask;
	return WriteRegister(TRIG_COMPARE_MASK1_ADDR,0,TrigCompareMask);
}

int ADQ::SetTrigMask2(unsigned int TrigMask)
{
	m_TrigMask2 = TrigMask;
	return WriteRegister(TRIG_MASK2_ADDR,0,TrigMask);
}

int ADQ::SetTrigLevel2(int TrigLevel)
{
	m_TrigLevel2 = TrigLevel;
	return WriteRegister(TRIG_LEVEL2_ADDR,0,TrigLevel);
}

int ADQ::SetTrigPreLevel2(int TrigLevel)
{
	m_TrigPreLevel2 = TrigLevel;
    return WriteRegister(TRIG_PRE_LEVEL2_ADDR,0,TrigLevel);
}

int ADQ::SetTrigCompareMask2(unsigned int TrigCompareMask)
{
	m_TrigCompareMask2 = TrigCompareMask;
	return WriteRegister(TRIG_COMPARE_MASK2_ADDR,0,TrigCompareMask);
}

int ADQ::ArmTrigger()
{
	int success = 1;

	if (m_StreamStatus)
	{
		success = m_Device->ReceiveDataInit(m_Device->GetMaxTransferSize(), 100);
	}

	if (success)
	{
		success = WriteRegister(TRIG_SETUP_ADDR,~(ARMED_TRIGGER_BIT),ARMED_TRIGGER_BIT);
		m_page_count = 0;
		//invalidate page in buffer
		m_CacheWord128bStart = ~0u;
		m_CacheWord128bEnd = 0u;
		m_trig_point = 0;
		m_trigged_ch = 0;
	}
	return success;
}


int ADQ::DisarmTrigger()
{
	int success = 1;
	if (m_StreamStatus)	// Wait until DMA controller is idle before stopping data flow.
		success = m_Device->ReceiveDataAbort();
	success = WriteRegister(TRIG_SETUP_ADDR,~(ARMED_TRIGGER_BIT),~(ARMED_TRIGGER_BIT)) && success; //always write bit
	if (m_StreamStatus) // Reset data fifos in HW
	{
		success = m_Device->FlushBuffers();
		success = WriteRegister(USB_SENDBATCH_ADDR,~(USB_FIFO_RST_BIT),USB_FIFO_RST_BIT) && success; //always write bit
		success = WriteRegister(PCIEDMA_CTRL_ADDR,~(PCIE_DMAFIFO_RST_BIT),PCIE_DMAFIFO_RST_BIT) && success;
		success = WriteRegister(USB_SENDBATCH_ADDR,~(USB_FIFO_RST_BIT),~(USB_FIFO_RST_BIT)) && success;
		success = WriteRegister(PCIEDMA_CTRL_ADDR,~(PCIE_DMAFIFO_RST_BIT),~(PCIE_DMAFIFO_RST_BIT)) && success;
	}
	return success;
}


int ADQ::SWTrig()
{
    int success = 1;

    success = WriteRegister(HOST_TRIG_ADDR,0,0);
    success = success && WriteRegister(HOST_TRIG_ADDR,0,1);
	return success;
}


int ADQ::GetData(void** target_buffers, 
                            unsigned int target_buffer_size,
                            unsigned char target_bytes_per_sample,
                            unsigned int StartRecordNumber,
                            unsigned int NumberOfRecords,
                            unsigned char ChannelsMask,
                            unsigned int StartSample,
                            unsigned int nSamples,
                            unsigned char TransferMode)
{
    unsigned int SamplesPerPage = m_WordsNbPerPage*m_SamplesPerWordNb;
    unsigned int SamplesCollected;
    unsigned int LoopVar;
    unsigned int EndRecordNumber; 
    unsigned int StartRelativeToTriggerOffsetWordNb;
    unsigned int OptimumCacheSizeInBytes(0);
    unsigned int OldCacheSizeIn128bWords(0);
    unsigned int wordsize;
    int success = 1;

    void* cp_tb[8];

    m_max_samples_to_parse = nSamples;

    wordsize = (StartSample + nSamples +m_SamplesPerWordNb-1)/m_SamplesPerWordNb; //round up

    // Check input arguments
    if (TransferMode == ADQ_TRANSFER_MODE_NORMAL)
    {
        OldCacheSizeIn128bWords = m_CacheSizeIn128BWords;
        
        if (m_MultiRecordMode)
        {
            OptimumCacheSizeInBytes = 1024 * (((wordsize*m_WordNbBits / 8) + 16 + 1023)/ 1024);
            if ((StartSample + nSamples) > m_SamplesPerRecord)
            {
                success = 0;
            }
        }
        else
        {
            OptimumCacheSizeInBytes = 1024 * (((wordsize*m_WordNbBits / 8) + 1023)/ 1024);

            if (wordsize > m_buffersize)
            {
                success = 0;
            }
        }
    }
    else if (TransferMode == ADQ_TRANSFER_MODE_HEADER_ONLY)
    {
        OldCacheSizeIn128bWords = m_CacheSizeIn128BWords;
        OptimumCacheSizeInBytes = 1024;    
        nSamples = 1;
    }
    SetCacheSize(OptimumCacheSizeInBytes);

    m_channels_mask = ChannelsMask;
    m_target_bytes_per_sample = target_bytes_per_sample;

    cp_tb[0] = GetPtrData(1);
    cp_tb[1] = GetPtrData(2);
    // Copy pointers to the target buffers
    if (ChannelsMask & 0x1)
    {
        cp_tb[0] = target_buffers[0];
    }

    if (ChannelsMask & 0x2)
    {
        cp_tb[1] = target_buffers[1];
    }

    m_pData_int[0] = (int*)cp_tb[0];  
    m_pData_int[1] = (int*)cp_tb[1];
    m_pData_int16[0] = (short int*)cp_tb[0];  
    m_pData_int16[1] = (short int*)cp_tb[1];
    m_pData_char[0] = (char*)cp_tb[0];  
    m_pData_char[1] = (char*)cp_tb[1];

    StartRelativeToTriggerOffsetWordNb = StartSample / m_SamplesPerWordNb;

    if (!m_MultiRecordMode)
    {
        StartRecordNumber = 0;
        EndRecordNumber = 0;
    }
    else
    {
        EndRecordNumber = StartRecordNumber + NumberOfRecords - 1;
        if (EndRecordNumber >= m_NofRecords)
        {
            success = 0;
        }
    }

    if (success)
    {
        for (LoopVar = StartRecordNumber; LoopVar <= EndRecordNumber; LoopVar++)
        {
            m_max_samples_to_parse = nSamples;
            SamplesCollected = 0;
            while(success && (SamplesCollected < nSamples))
            {
                if (SamplesCollected == 0)
                {
                    if (m_MultiRecordMode)
                    {
                        success = success && MultiRecordGetRecordInternal(LoopVar, StartRelativeToTriggerOffsetWordNb, 1);
                    }
                    else
                    {
                        success = success && CollectDataNextPageInternal(StartRelativeToTriggerOffsetWordNb, 1);                
                    }
                }
                else
                {
                    if (m_MultiRecordMode)
                    {
                        success = success && MultiRecordGetRecordInternal(LoopVar, StartRelativeToTriggerOffsetWordNb, 0);
                    }
                    else
                    {
                        success = success && CollectDataNextPageInternal(StartRelativeToTriggerOffsetWordNb, 0);                
                    }
                }

                SamplesCollected += MIN(nSamples-SamplesCollected, SamplesPerPage);
                m_max_samples_to_parse -= MIN(nSamples-SamplesCollected, SamplesPerPage);
                if (SamplesCollected > target_buffer_size)
                {
                    success = 0;
                }

                if (ChannelsMask & 0x01) 
                {
                    m_pData_char[0] += SamplesPerPage;
                    m_pData_int[0] += SamplesPerPage;
                    m_pData_int16[0] += SamplesPerPage;
                }

                if (ChannelsMask & 0x02) 
                {
                    m_pData_char[1] += SamplesPerPage;
                    m_pData_int[1] += SamplesPerPage;
                    m_pData_int16[1] += SamplesPerPage;
                }
            }
        }
    }

    SetCacheSize(OldCacheSizeIn128bWords*16);
    m_page_count = 0;
    return success;
}

int ADQ::CollectDataNextPage()
{
    int success = 0;

    m_max_samples_to_parse = m_WordsNbPerPage*m_SamplesPerWordNb;

    if (m_StreamStatus)
	{
		success = CollectDataStream();
	}
	else
	{
        if (!m_MultiRecordMode)
        {
            m_pData_int[0] = (int*)GetPtrData(1);  
            m_pData_int[1] = (int*)GetPtrData(2);
        
            m_target_bytes_per_sample = 4;
            m_channels_mask = 0x3;

            success = CollectDataNextPageInternal(0, 0);
        }
    }

    return success;
}

int ADQ::CollectDataNextPageInternal(
                                     unsigned int StartRelativeToTriggerOffsetWordNb,
                                     unsigned char reset
                                     )
{
    unsigned int samplesToParse;
    int success = 1;

    int destOffset = 0;

    samplesToParse = min((unsigned int)m_max_samples_to_parse, m_WordsNbPerPage*m_SamplesPerWordNb);

    // If first call, find staring page i ADQ mem
    if (m_page_count == 0 || reset)
    {
        unsigned int trig_vector, trig_info_6, trig_info_7;
        int RoundUpValue1, RoundUpValue2, RoundUpValue3, OffsetStep;
        m_overflow = 0;

        RoundUpValue1 = (m_PreTrigWordsNb+parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
        RoundUpValue2 = (m_TriggerHoldOffWordsNb + parameter_interleaved_leaves-1) / parameter_interleaved_leaves;
        RoundUpValue3 = (StartRelativeToTriggerOffsetWordNb + parameter_interleaved_leaves-1) / parameter_interleaved_leaves;

        m_wordNbOffset = ReadRegister(TRIG_INFO_0_ADDR) - RoundUpValue1 + RoundUpValue2 + RoundUpValue3;
        m_pageAddress128b = ReadRegister(TRIG_INFO_1_ADDR) * m_Words128bPerPage;

        // Get trig timestamp
        trig_info_6 = ReadRegister(TRIG_INFO_6_ADDR);
        trig_info_7 = ReadRegister(TRIG_INFO_7_ADDR);
        m_TrigTimeStart = (0x00000003 & trig_info_6);
        m_TrigTimeCycles = (unsigned long long)((0xfffffffc & trig_info_6)>>2) + ((unsigned long long)(0x000003ff & trig_info_7)<<30);
        m_TrigTimeSyncs  = (0xfffffC00 & trig_info_7)>>10;

        OffsetStep = m_WordsNbPerPage/parameter_interleaved_leaves;
        //Find address to page with first sample
        while (m_wordNbOffset < 0)
        {
            m_pageAddress128b -= m_Words128bPerPage; //Start of previous page
            if (m_pageAddress128b < 0) //Wrap around in memory block
                m_pageAddress128b += m_dram_phys_end_address128b - m_dram_phys_start_address128b;
            m_wordNbOffset += OffsetStep;
        }
        m_pageAddress128b += m_dram_phys_start_address128b;

        while (m_wordNbOffset >= OffsetStep)
        {
            m_pageAddress128b += m_Words128bPerPage; //Start of previous page
            if (m_pageAddress128b >= (int)m_dram_phys_end_address128b) //Wrap around in memory block
                m_pageAddress128b -= (m_dram_phys_end_address128b - m_dram_phys_start_address128b);
            m_wordNbOffset -= OffsetStep;
        }			

        // LSB bits are be one-hot coded
        // 0001 and 0010 are 1st sample A and B channel
        // 0100 and 1000 are 2nd sample A and B channel

        trig_vector = ReadRegister(TRIG_INFO_2_ADDR) & 0x0000000F;
        m_trigged_ch = ((trig_vector & 0x0Au) > 0) ? CH_B : CH_A;

        if (parameter_interleaved_leaves == 1)
        {
            if (m_SamplesPerWordNb == 1)
                m_sampleOffset = 0;
            else
                m_sampleOffset = (trig_vector > 2)? 1 : 0;
        }
        else if (parameter_interleaved_leaves == 2)
        {
            if (m_SamplesPerWordNb == 1)
            {
                m_sampleOffset = ((trig_vector & 0x0Au) > 0) ? 1 : 0;
            }
            else
            {
                switch(trig_vector & 0x0Fu)
                {
                default:
                    m_sampleOffset = 0;
                    break;
                case 2:
                    m_sampleOffset = 1;
                    break;
                case 4:
                    m_sampleOffset = 2;
                    break;
                case 8:
                    m_sampleOffset = 3;
                    break;
                }
            }
        }

        m_trig_point = m_PreTrigWordsNb*m_SamplesPerWordNb;
    }


    // Update driver buffer if necessary
    success = CollectDataPage(m_pageAddress128b, m_pageAddress128b+m_Words128bPerPage-1);

    if (success)
    {
        // Get valid samples from this hardware page
        destOffset = ParseSampleData(m_wordNbOffset, m_sampleOffset, 0, samplesToParse);

        // Increment address to next page
        m_page_count++;
        m_pageAddress128b += m_Words128bPerPage;
        if((unsigned int)m_pageAddress128b > m_dram_phys_end_address128b) //Should always be larger by one
        {
            m_pageAddress128b = m_dram_phys_start_address128b; //m_pageAddress128b is 128, phys addr is 64
        }

        // If previous hardware page did not fill the software (trigger-aligned) page, get next page 
        if ((unsigned int)destOffset < m_WordsNbPerPage*m_SamplesPerWordNb)
        {
            success = CollectDataPage(m_pageAddress128b, m_pageAddress128b+m_Words128bPerPage-1);

            if (success)
            {
                ParseSampleData(0, 0, destOffset, samplesToParse);
            }
        }
    }

    return success;
}

int ADQ::CollectDataPage(unsigned int addr_begin, unsigned int addr_end)
{
	int success = 1;

	if ((addr_begin >= m_CacheWord128bStart) // Cache hit
		&& (addr_end <= m_CacheWord128bEnd))
	{
		m_temp_data_buffer_ptr = &m_temp_data_buffer[(addr_begin-m_CacheWord128bStart)*16];
	}
	else // Cache miss
	{
		unsigned int pagesize = addr_end - addr_begin + 1;
		unsigned int new_addr_begin = addr_begin;
		unsigned int dram_last_data_address128b = m_dram_phys_end_address128b;

		if (!m_MultiRecordMode) // If normal mode, calculate how much data is available until wrap/end of mem
		{
			dram_last_data_address128b = m_PreTrigWordsNb+m_WordsNbAfterTrig; // number of words
			dram_last_data_address128b = (dram_last_data_address128b+m_WordsNbPerPage-1)/m_WordsNbPerPage + 1; // number pages (always get an extra due to trigger offset inside pages)
			dram_last_data_address128b -= m_page_count; // deduct number of pages already collected
			dram_last_data_address128b *= m_Words128bPerPage; // 128b word size
			dram_last_data_address128b += addr_begin; // 128b word end address
			dram_last_data_address128b = min(dram_last_data_address128b, m_dram_phys_end_address128b); // don't pass wrap address
		}
        else
        {
            dram_last_data_address128b = m_dram_phys_start_address128b + 
                (MULTIRECORD_HEADER_SIZE_128B + m_PagesPerRecord*m_Words128bPerPage)*m_NofRecords;
        }


		if ((dram_last_data_address128b - new_addr_begin) >= m_CacheSizeIn128BWords)
		{
			pagesize = ((pagesize + m_CacheSizeIn128BWords - 1) / m_CacheSizeIn128BWords) * m_CacheSizeIn128BWords; //Prefetch 8K data
			addr_end = new_addr_begin + pagesize - 1;
		}
		else // Memory ends before cache, decrease cache and move down start to guarantee size
		{
			pagesize = (dram_last_data_address128b - addr_begin + 1 + 31) / 32 * 32; //Round up to min 512 byte blocks (USB 512B, PCIe 128B packets)
			addr_end = new_addr_begin + pagesize - 1;
			if (addr_end > DRAM_PHYS_END_ADDRESS)
			{
				new_addr_begin -= addr_end - DRAM_PHYS_END_ADDRESS;
				addr_end = DRAM_PHYS_END_ADDRESS;
			}
		}

		if (pagesize > 0)
		{
			// Use asynchronous transfer since data may be lost if there is a
			// task switch between SendCommand() and ReceiveDataFinish().
			success = m_Device->ReceiveDataInit(pagesize*16,1);
			// Send addresses for fetching to board
			success = success && SendProcessorCommand(COLLECT_DATA, new_addr_begin, addr_end, 0);
			// Use asynchronous transfer since data may be lost if there is a
			// task switch between SendCommand() and ReceiveDataFinish().
			success = success && m_Device->ReceiveDataFinish(pagesize*16, &m_temp_data_buffer);	
		}
		else
		{
			success = 0;
		}

		if (success)
		{	
			m_CacheWord128bStart = new_addr_begin;
			m_CacheWord128bEnd = addr_end;
		}
		else
		{
			 //invalidate data in buffer
			m_CacheWord128bStart = ~0u;
			m_CacheWord128bEnd = 0u;
		}
		m_temp_data_buffer_ptr = &m_temp_data_buffer[addr_begin-new_addr_begin];
	}
	return success;
}

int ADQ::CollectDataStream()
{
	int success;
	success = m_Device->ReceiveDataInit(m_Device->GetMaxTransferSize(), 100);
	success = success && m_Device->ReceiveDataFinish(m_Device->GetMaxTransferSize(), &m_temp_data_buffer);
	return success;
}

unsigned int ADQ::MultiRecordSetup(unsigned int NumberOfRecords, unsigned int SamplesPerRecord)
{
	unsigned int success = 0;
	unsigned int numberOfWords128b;
    int AddForTrigger = parameter_interleaved_leaves*2 - 1;

	m_SamplesPerRecord = SamplesPerRecord;
	// Calculate number of pages needed to contain the number of samples.
	// Round up to nearest page count and make sure there is at least one 128b word padding for wrap-around packing problems.
	m_PagesPerRecord = (SamplesPerRecord
		+ AddForTrigger //Add extra sample since trigger can happen in two positions, last sample may land in this pos
        + (m_SamplesPerWordNb*parameter_interleaved_leaves) // Work-around for last word corrupt bug
        + (64+m_WordNbBits/m_SamplesPerWordNb-1)/(m_WordNbBits/m_SamplesPerWordNb)*parameter_interleaved_leaves // Packing padding
		+ (m_SamplesPerWordNb*m_WordsNbPerPage - 1)) // Round up constant
		/ (m_SamplesPerWordNb*m_WordsNbPerPage);
	m_NofRecords = NumberOfRecords;
	numberOfWords128b = NumberOfRecords * (m_PagesPerRecord*m_Words128bPerPage + MULTIRECORD_HEADER_SIZE_128B);

	if (numberOfWords128b <= DRAM_PHYS_END_ADDRESS-m_dram_phys_start_address128b+1)
	{
		m_MultiRecordMode = 1;
		m_mr_backup_dram_phys_end_address128b = m_dram_phys_end_address128b;
		m_dram_phys_end_address128b = numberOfWords128b-1;

		success = WriteRegister(MULTIRECORD_RECS_ADDR, 0, NumberOfRecords);
		success = success && WriteRegister(MULTIRECORD_EN_ADDR, 0, 1);
		success = success && WriteRegister(MAX_PAGES_ADDR, 0, m_PagesPerRecord-1);
        
        //Add extra sample since trigger can happen in two positions, last sample may land in this
        success = success && SetWordsAfterTrig((m_SamplesPerRecord+(2*parameter_interleaved_leaves-1)+m_SamplesPerWordNb-1)/m_SamplesPerWordNb - m_PreTrigWordsNb);
		success = success && WriteRegister(MULTIRECORD_DPR_ADDR, 0, m_PagesPerRecord*m_Words128bPerPage);
	}

	return success;
}
unsigned int ADQ::MultiRecordGetRecord(int RecordNumber)
{
    unsigned int success = 0;

    m_max_samples_to_parse = m_SamplesPerRecord;

    if (m_MultiRecordMode)
    {
        m_pData_int[0] = (int*)GetPtrData(1);  
        m_pData_int[1] = (int*)GetPtrData(2);
        
        m_target_bytes_per_sample = 4;
        m_channels_mask = 0x3;

        success = MultiRecordGetRecordInternal(RecordNumber, 0, 0);
    }

    return success;
}

unsigned int ADQ::MultiRecordGetRecordInternal(int RecordNumber,
                                               unsigned int StartRelativeToTriggerOffsetWordNb,
                                               unsigned char reset)
{
	unsigned int read_end_address, destOffset, sampleToParse;
	int success = 1;

	 // Reset page counter when requesting new record.
	if ((m_CurrentRecordNumber != RecordNumber) || reset)
	{
		m_CurrentRecordNumber = RecordNumber;
		m_page_count = 0;
	}

	sampleToParse = min(m_SamplesPerWordNb*m_WordsNbPerPage,
		m_SamplesPerRecord-m_page_count*m_SamplesPerWordNb*m_WordsNbPerPage);

    sampleToParse = min((unsigned int)m_max_samples_to_parse, sampleToParse);

	// Read trigger info from first page
	if (m_page_count == 0)
	{
		unsigned int trig_vector, header[8];
        int RoundUpValue1, RoundUpValue2, RoundUpValue3, OffsetStep;
		int trig_page;

		m_read_start_address = m_dram_phys_start_address128b + RecordNumber*(m_PagesPerRecord*m_Words128bPerPage + MULTIRECORD_HEADER_SIZE_128B);
		read_end_address = m_read_start_address + MULTIRECORD_HEADER_SIZE_128B - 1;
		success = CollectDataPage(m_read_start_address, read_end_address);

		for (int j = 0; j < 8; j++)
		{
			header[j] = ((unsigned int)m_temp_data_buffer_ptr[4*j+3]<<24)
						+ ((unsigned int)m_temp_data_buffer_ptr[4*j+2]<<16)
						+ ((unsigned int)m_temp_data_buffer_ptr[4*j+1]<<8)
						+ (unsigned int)m_temp_data_buffer_ptr[4*j];
		}
		// Sort header
		m_MultiRecordHeader[0]=header[1];
		m_MultiRecordHeader[1]=header[0];
		m_MultiRecordHeader[2]=header[3];
		m_MultiRecordHeader[3]=header[2];
		m_MultiRecordHeader[4]=header[5];
		m_MultiRecordHeader[5]=header[4];
		m_MultiRecordHeader[6]=header[7];
		m_MultiRecordHeader[7]=header[6];
		// Get header
		m_TrigVector[0] = (0x00000000f & m_MultiRecordHeader[2]);
		m_TrigVector[1] = (0x000f00000 & m_MultiRecordHeader[2])>>16;
		m_TrigCounter = m_MultiRecordHeader[3];
		m_RecordsCollected = m_MultiRecordHeader[4];
        m_TrigType = (0x0000000F & m_MultiRecordHeader[5]);
        m_TrigSyncForSampleSkip = ((0xFFFFFFF0 & m_MultiRecordHeader[5]) >> 4);
        m_TrigTimeStart = (0x00000003 & m_MultiRecordHeader[6]);
		m_TrigTimeCycles = (unsigned long long)((0xfffffffc & m_MultiRecordHeader[6])>>2) + ((unsigned long long)(0x000fffff & m_MultiRecordHeader[7])<<30);
		m_TrigTimeSyncs  = (0xfffffC00 & m_MultiRecordHeader[7])>>10;
		m_overflow = 0;

        RoundUpValue1 = (m_PreTrigWordsNb+parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
        RoundUpValue2 = (m_TriggerHoldOffWordsNb+parameter_interleaved_leaves-1)/parameter_interleaved_leaves;
        RoundUpValue3 = (StartRelativeToTriggerOffsetWordNb + parameter_interleaved_leaves-1) / parameter_interleaved_leaves;

		m_wordNbOffset = m_MultiRecordHeader[0] - RoundUpValue1 + RoundUpValue2 + RoundUpValue3;
		if (m_PagesPerRecord == 1) //Hardware does not wrap page counter if pages/record is 1
			trig_page = 0;
		else
			trig_page = m_MultiRecordHeader[1];

		// Find address to page with first sample
		OffsetStep = m_WordsNbPerPage/parameter_interleaved_leaves;
        while (m_wordNbOffset < 0)
		{
			trig_page -= 1;
			if (trig_page < 0) //Wrap around in record block
				trig_page += m_PagesPerRecord;
            m_wordNbOffset += OffsetStep;
		}

		while (m_wordNbOffset >= (int)OffsetStep)
		{
			trig_page += 1;
			if (trig_page >= (int)m_PagesPerRecord) //Wrap around in record block
				trig_page -= m_PagesPerRecord;
			m_wordNbOffset -= OffsetStep;
		}		
		m_read_start_address = m_read_start_address + MULTIRECORD_HEADER_SIZE_128B + trig_page * m_Words128bPerPage;
		
		// LSB bits are be one-hot coded
		// 0001 and 0010 are 1st sample A and B channel
		// 0100 and 1000 are 2nd sample A and B channel

		trig_vector = m_MultiRecordHeader[2] & 0x0000000F;
		m_trigged_ch = ((trig_vector & 0x0Au) > 0) ? CH_B : CH_A;

        if (parameter_interleaved_leaves == 1)
        {
            if (m_SamplesPerWordNb == 1)
                m_sampleOffset = 0;
            else
                m_sampleOffset = (trig_vector > 2)? 1 : 0;

            if (m_SampleSkip > 1)
            {
                int area=int((m_SamplesPerWordNb*m_TrigSyncForSampleSkip) / m_SampleSkip);
                m_sampleOffset = area;

            }

        }
        else if (parameter_interleaved_leaves == 2)
        {
            if (m_SamplesPerWordNb == 1)
            {
                m_sampleOffset = ((trig_vector & 0x0Au) > 0) ? 1 : 0;
            }
            else
            {
                switch(trig_vector & 0x0Fu)
                {
                default:
                    m_sampleOffset = 0;
                    break;
                case 2:
                    m_sampleOffset = 1;
                    break;
                case 4:
                    m_sampleOffset = 2;
                    break;
                case 8:
                    m_sampleOffset = 3;
                    break;
                }
            }

            if (m_SampleSkip > 1)
            {
                int area = int((parameter_interleaved_leaves * m_SamplesPerWordNb * m_TrigSyncForSampleSkip) / m_SampleSkip);
                m_sampleOffset = area;
            }
        }

		m_trig_point = m_PreTrigWordsNb*m_SamplesPerWordNb;
	}
	else if (m_page_count >= m_PagesPerRecord)
	{
		success = 0;
	}

	read_end_address = m_read_start_address + m_Words128bPerPage - 1;
	success = success && CollectDataPage(m_read_start_address, read_end_address);

	if (success)
	{
		// Get valid samples from this hardware page
		destOffset = ParseSampleData(m_wordNbOffset, m_sampleOffset, 0, sampleToParse);

		// Increment address to next page
		m_read_start_address += m_Words128bPerPage;
		if((unsigned int)m_read_start_address >=
			(m_dram_phys_start_address128b + (RecordNumber+1)*(m_PagesPerRecord*m_Words128bPerPage + MULTIRECORD_HEADER_SIZE_128B)))
		{
			m_read_start_address = m_dram_phys_start_address128b + RecordNumber*(m_PagesPerRecord*m_Words128bPerPage + MULTIRECORD_HEADER_SIZE_128B) + MULTIRECORD_HEADER_SIZE_128B;
		}
		read_end_address = m_read_start_address + m_Words128bPerPage - 1;

		// Now get all the samples that were in the beginning of the page
		if (destOffset < sampleToParse)
		{
			success = CollectDataPage(m_read_start_address, read_end_address);
			if (success)
			{
				destOffset = ParseSampleData(0, 0, destOffset, sampleToParse);
			}
		}
		m_page_count++;
	}
	return success;
}

unsigned int ADQ::MemoryDump(unsigned int StartAddress, unsigned int EndAddress, unsigned char* buffer, unsigned int *bufctr)
{
	unsigned int read_start_address;
	unsigned int read_end_address;
	unsigned int success = 1;
	unsigned int internal_success = 0;
	unsigned int retry_counter = 0;
	unsigned int read_bytes;

    unsigned char *buffer_c;

	unsigned int max_read_addresses;
	unsigned int to_read_chunk;

	*bufctr = 0;

	m_temp_data_buffer_ptr = m_temp_data_buffer;

	read_start_address = StartAddress;

	// Max transfer of 128b words
	max_read_addresses = (m_Device->GetMaxTransferSize() / 16);
	
	while (success && (read_start_address < EndAddress))
	{
		to_read_chunk = min(max_read_addresses, (EndAddress - read_start_address + 1));
			
		// Read next chunk
		read_end_address = read_start_address + to_read_chunk - 1;
		read_bytes = to_read_chunk * 16;

		internal_success = 0;
		retry_counter = 0;
		while (!internal_success && retry_counter < 2)
		{
			internal_success = m_Device->ReceiveDataInit(read_bytes,1);	
			internal_success = internal_success && SendProcessorCommand(COLLECT_DATA, read_start_address, read_end_address, 0);
            buffer_c = &buffer[*bufctr];
			internal_success = internal_success && m_Device->ReceiveDataFinish(read_bytes, &buffer_c);			
			retry_counter++;
		}

		success = success && internal_success;

		if (success)
		{
			retry_counter = 0;
			*bufctr += read_bytes;
		}
		else
		{
			break;
		}
		
		read_start_address = read_end_address + 1;
	}

	return success;
}

unsigned int ADQ::MultiRecordClose(void)
{
	unsigned int success;
	m_dram_phys_end_address128b = m_mr_backup_dram_phys_end_address128b;
	success = WriteRegister(MULTIRECORD_EN_ADDR, 0, 0);
	success = success && WriteRegister(MAX_PAGES_ADDR, 0, m_MaxPages-1);
	success = success && SetWordsAfterTrig(m_buffersize-m_PreTrigWordsNb);
	return success;
}

unsigned int ADQ::GetAcquiredAll()
{
	return (ReadRegister(COLLECT_STATUS_ADDR) & COLLECT_STATUS_ALL_RECORDS_BIT) >> COLLECT_STATUS_ALL_RECORDS_POS;	
}

int ADQ::GetWaitingForTrigger()
{
	return (ReadRegister(COLLECT_STATUS_ADDR) & COLLECT_STATUS_WAITINGFORTRIGGER_BIT) >> COLLECT_STATUS_WAITINGFORTRIGGER_POS;
}

int ADQ::GetAcquired()
{
	return (ReadRegister(COLLECT_STATUS_ADDR) & COLLECT_STATUS_DATA_COLLECTED_BIT) >> COLLECT_STATUS_DATA_COLLECTED_POS;
}


unsigned int ADQ::GetPageCount()
{
	return m_page_count;
}

int ADQ::GetLvlTrigLevel()
{
	return m_trig_lvl;
}

int ADQ::GetLvlTrigEdge()
{
	return m_trig_edge;
}

int ADQ::GetPllFreqDivider()
{
	return m_freq_divider;
}

int ADQ::GetTriggerMode()
{
	return m_trig_mode;
}

int ADQ::GetClockSource()
{
	return m_clock_source;
}

unsigned int ADQ::GetBufferSizePages()
{
	return (m_buffersize+m_WordsNbPerPage-1)/m_WordsNbPerPage; //Round up
}

unsigned int ADQ::GetMaxBufferSize()
{
	return m_MaxPages*m_WordsNbPerPage*m_SamplesPerWordNb;
}

unsigned int ADQ::GetMaxBufferSizePages()
{
	return m_MaxPages;
}

unsigned int ADQ::GetSamplesPerPage()
{
	if (m_StreamStatus)
		return m_SamplesPerWordNb*(m_Device->GetMaxTransferSize()/8)*2; //TODO: fix this?
	else
		return m_WordsNbPerPage*m_SamplesPerWordNb;
}

unsigned int ADQ::GetBufferSize()
{
	return (unsigned int)(m_buffersize*m_SamplesPerWordNb);
}

unsigned int ADQ::GetUSBAddress()
{
	return m_Device->Address();
}

unsigned long ADQ::GetPCIeAddress()
{
	return m_Device->Address();
}

unsigned int ADQ::GetBcdDevice()
{
	return m_Device->HwVersion();
}

int* ADQ::GetRevision()
{
	int success;
	unsigned int revision_lowbyte16 = 0;
	unsigned int revision_highbyte16 = 0;
	unsigned char message[4];
	message[0] = 4;
	message[1] = PROCESSOR_CMD;
	message[2] = 0;
	message[3] = SEND_REVISION_LSB;
	m_Device->ReceiveStatusInit();
	success = m_Device->SendCommand(message);
	success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
	if (success)
		revision_lowbyte16 = m_status;
	message[0] = 4;
	message[1] = PROCESSOR_CMD;
	message[2] = 0;
	message[3] = SEND_REVISION_MSB;
	if (success)
	{
		m_Device->ReceiveStatusInit();
		success = m_Device->SendCommand(message);
		success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
		if (success)
			revision_highbyte16 = m_status;
	}
	m_revision[0] = (int)((revision_highbyte16 & 0x3FFF)*65536 + revision_lowbyte16);
	m_revision[1] = (int)((revision_highbyte16 & 0x8000));
	m_revision[2] = (int)((revision_highbyte16 & 0x4000));

	revision_lowbyte16 = ReadAlgoRegister(4);
	revision_highbyte16 = ReadAlgoRegister(5);

	m_revision[3] = (int)(revision_lowbyte16);
	m_revision[4] = (int)((revision_highbyte16 & 0x8000));
	m_revision[5] = (int)((revision_highbyte16 & 0x4000));

	return m_revision;
}

int ADQ::GetTriggerInformation()
{	
	return ReadRegister(82);
}

int ADQ::GetTrigPoint()
{
	return m_trig_point;
}

unsigned int ADQ::GetTrigType()
{
	return m_TrigType;
}

unsigned int ADQ::GetRecordSize()
{
	unsigned int answer = 0;
	if (m_MultiRecordMode) answer = m_SamplesPerRecord;
	return answer;
}

unsigned int ADQ::GetNofRecords()
{
	unsigned int answer = 0;
	if (m_MultiRecordMode) answer = m_NofRecords;
	return answer;
}

int ADQ::GetOverflow()
{
	return m_overflow;
}

char* ADQ::GetBoardSerialNumber()
{
	return m_serial;
}
unsigned int* ADQ::GetMultiRecordHeader()
{
	return m_MultiRecordHeader;
}
unsigned long long ADQ::GetTrigTime()
{
	unsigned long long trigTime;
	if (parameter_interleaved_leaves == 1)
		trigTime = m_TrigTimeCycles*4 + m_TrigTimeStart + 2*m_sampleOffset;
	else
		trigTime = m_TrigTimeCycles*4 + m_TrigTimeStart + m_sampleOffset;
	if (m_TrigTimeMode == 0)
		trigTime += ((unsigned long long)m_TrigTimeSyncs << 42);
	return trigTime;
}
unsigned long long ADQ::GetTrigTimeCycles()
{
	return m_TrigTimeCycles;
}
unsigned int ADQ::GetTrigTimeSyncs()
{
	return m_TrigTimeSyncs;
}
unsigned int ADQ::GetTrigTimeStart()
{
	return m_TrigTimeStart;
}
void ADQ::RetrieveBoardSerialNumber(void)
{
	unsigned char tmp[4];

	m_serial[0] = 'N';
	m_serial[1] = 'O';
	m_serial[2] = 'T';
	m_serial[3] = 'P';
	m_serial[4] = 'R';
	m_serial[5] = 'O';
	m_serial[6] = 'G';
	m_serial[7] = 'R';
	m_serial[8] = 'A';
	m_serial[9] = 'M';
	m_serial[10] = 'M';
	m_serial[11] = 'E';
	m_serial[12] = 'D';
	m_serial[13] = 0;
	m_serial[14] = 0;
	m_serial[15] = 0;

	tmp[0] = (unsigned char)ReadEEPROM(0);
	if (tmp[0] == 48)
	{
	tmp[1] = (unsigned char)ReadEEPROM(1);
	tmp[2] = (unsigned char)ReadEEPROM(2);
	tmp[3] = (unsigned char)ReadEEPROM(3);

	// Valid is '0001' on these positions
	if (tmp[1] == 48 && tmp[2] == 48 && tmp[3] == 49)
	{
		// Valid serial number on board
		for (int j = 0; j < 16; j++)
		{
			m_serial[j] = (unsigned char)ReadEEPROM(82+j);
		}
	}
	}

}

int ADQ::IsUSBDevice()
{
	return m_Device->IsUSBDevice();
}

int ADQ::IsPCIeDevice()
{
	return m_Device->IsPCIeDevice();
}

unsigned int ADQ::IsAlive()
{
	unsigned char tmp[4];

	tmp[0] = (unsigned char)ReadEEPROM(0);
	if (tmp[0] == 48)
	{
		tmp[1] = (unsigned char)ReadEEPROM(1);
		tmp[2] = (unsigned char)ReadEEPROM(2);
		tmp[3] = (unsigned char)ReadEEPROM(3);

		// Valid is '0001' on these positions
		if (tmp[1] == 48 && tmp[2] == 48 && tmp[3] == 49)
		{
			return 1; // Is alive 
		}
	}

	return 0; // Isn't alive

}

unsigned int ADQ::SendProcessorCommand(int command,int argument)
{
	int success;
	unsigned char message[8];
	message[0] = 8;
	message[1] = PROCESSOR_CMD;
	message[2] = (command & 0xFF00) >> 8;
	message[3] = (command & 0x00FF);
	message[4] = (argument & 0xFF00) >> 8;
	message[5] = (argument & 0x00FF);
	message[6] = 0;
	message[7] = 0;

	m_Device->ReceiveStatusInit();
	success = m_Device->SendCommand(message);
	success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call

	if (success)
		return m_status;
	else
		return 0;
}

unsigned int ADQ::SendProcessorCommand(unsigned int command, unsigned int addr, unsigned int mask, unsigned int data)
{
	int success;
	int temp_word = 0;
	unsigned char message[16];

	message[ 0] = 16;
	message[ 1] = PROCESSOR_CMD;
	message[ 2] = (unsigned char)((command & 0xFF00) >> 8);
	message[ 3] = (unsigned char) (command & 0x00FF);
	message[ 4] = (unsigned char) (addr & 0x000000FF);
	message[ 5] = (unsigned char)((addr & 0x0000FF00) >>  8);
	message[ 6] = (unsigned char)((addr & 0x00FF0000) >> 16);
	message[ 7] = (unsigned char)((addr & 0xFF000000) >> 24);
	message[ 8] = (unsigned char) (mask & 0x000000FF);
	message[ 9] = (unsigned char)((mask & 0x0000FF00) >>  8);
	message[10] = (unsigned char)((mask & 0x00FF0000) >> 16);
	message[11] = (unsigned char)((mask & 0xFF000000) >> 24);
	message[12] = (unsigned char) (data & 0x000000FF);
	message[13] = (unsigned char)((data & 0x0000FF00) >>  8);
	message[14] = (unsigned char)((data & 0x00FF0000) >> 16);
	message[15] = (unsigned char)((data & 0xFF000000) >> 24);
	m_Device->ReceiveStatusInit();
	success = m_Device->SendCommand(message);
	success = m_Device->ReceiveStatusFinish(&m_status) && success; //Always do function call
	if (success)
		temp_word = m_status;

	return temp_word;
}

unsigned int ADQ::WriteI2C(unsigned int addr, unsigned int nbytes, unsigned int data)
{
	return SendProcessorCommand(WRITE_I2C, addr, nbytes, data);
}

unsigned int ADQ::ReadI2C(unsigned int addr, unsigned int nbytes)
{
	return SendProcessorCommand(READ_I2C, addr, nbytes, 0);
}

unsigned int ADQ::WriteRegister(unsigned int addr, unsigned int mask, unsigned int data)
{
	return SendProcessorCommand(WRITE_REG, addr, mask, data);
}

unsigned int ADQ::ReadRegister(unsigned int addr)
{
	return SendProcessorCommand(READ_REG, addr, 0, 0);
}

unsigned int ADQ::WriteAlgoRegister(unsigned int addr, unsigned int mask, unsigned int data)
{
	int temp_word = 0;
	if ((addr < (1<<15)) || (data < (1<<16)))
		temp_word = SendProcessorCommand(WRITE_ALGO_REG, addr, mask, data);
	return temp_word;
}

unsigned int ADQ::ReadAlgoRegister(unsigned int addr)
{
	int temp_word = 0;
	if ((addr < (1<<15)))
		temp_word = SendProcessorCommand(READ_ALGO_REG, addr, 0, 0);
	return temp_word;
}

unsigned int ADQ::WriteEEPROM(unsigned int addr, unsigned int data, unsigned int accesscode)
{
	unsigned int DeviceAddress = 0x50;
	unsigned char addr_high;
	unsigned char addr_low;
	unsigned int cmd;
	unsigned int success;

	addr_high = ((addr & 0x0000FF00) >> 8);
	addr_low = (addr & 0x000000FF);
	cmd = (addr_low << 8) + addr_high;

	// Check for writes in protected areas
	// 512 bytes are protected 
	if (addr_high < 64)
	{
		if (accesscode != 0x75A6A7A8)
		{
			return 0;
		}
	}
	
	// Ok, first pull write control low (IPIF register 4 - LSB)
	success = WriteRegister(4, 0, 0); 

	cmd |= ((data & 0x000000FF) << 16);
	// Write address and data to chip
	success &= WriteI2C(DeviceAddress, 3, cmd); // Sends LSB first	
	
	// Also, wait for 4ms (2ms is the required timing for EEPROM to write)
	Sleep(4);

	// Ok, finish with raising write control (IPIF register 4 - LSB)
	success &= WriteRegister(4, 0, 1); 

	return success;
}

unsigned int ADQ::GetErrorVector(void)
{
	return ReadRegister(ERROR_VECTOR_ADDR);
}

int ADQ::GetStreamStatus(void)
{
	return ReadRegister(STREAM_EN_ADDR) & 0x7;
}

int ADQ::SetStreamStatus(int status)
{
	m_StreamStatus = status;
	return WriteRegister(STREAM_EN_ADDR, 0, status);
}

int ADQ::GetStreamOverflow(void)
{
	return (ReadRegister(STREAM_EN_ADDR) & (1u<<31)) != 0;
}

unsigned int ADQ::ReadEEPROM(unsigned int addr)
{
	unsigned int DeviceAddress = 0x50;
	unsigned char addr_high;
	unsigned char addr_low;
	unsigned int cmd;
	unsigned char readbyte;
	
	addr_high = ((addr & 0x0000FF00) >> 8);
	addr_low = (addr & 0x000000FF);
	cmd = (addr_low << 8) + addr_high;

	// Assure write control is high
	WriteRegister(4, 0, 1); 

	// Write address to chip
	WriteI2C(DeviceAddress, 2, cmd); // Sends LSB first	
	
	// Read back information
	readbyte = (unsigned char)ReadI2C(DeviceAddress, 1);

	Sleep(1); // Release I2C bus

	return readbyte;
}

unsigned int ADQ::GetTemperature(unsigned int addr)
{
	unsigned int DeviceAddress = 0x18;
	unsigned int temperature;
	unsigned int TempMSB = 0x11, TempLSB = 0x11;

	if (addr == 0x01)
	{
		WriteI2C(DeviceAddress, 1, 0x19);
		TempMSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;
		WriteI2C(DeviceAddress, 1, 0x29);
		TempLSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;
	}
	else if (addr == 0x02)
	{
		WriteI2C(DeviceAddress, 1, 0x1A);
		TempMSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;
		WriteI2C(DeviceAddress, 1, 0x2A);
		TempLSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;	
	}
	else if (addr == 0x10)
	{
		WriteI2C(DeviceAddress, 1, 0xFE);
		TempMSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;
		WriteI2C(DeviceAddress, 1, 0xFF);
		TempLSB = ReadI2C(DeviceAddress, 1) & 0x000000FF;		
	}
	
	temperature = (TempMSB << 8) + TempLSB;

	return temperature;
}

unsigned int ADQ::GetDataFormat()
{
	return m_DataFormat;
}

unsigned int ADQ::SetTestPatternMode(unsigned int mode)
{
    if ((mode >= 0) && (mode <= 7))
    {
	    return WriteRegister(TEST_PATTERN_ADDR, ~(0x00000007u), mode);
    }
    else
    {
        return 0;
    }
}
unsigned int ADQ::SetTestPatternConstant(unsigned int value)
{
    return WriteRegister(TEST_PATTERN_ADDR, ~(0x00000007u), (value << 16));
}

int ADQ::SetDirectionTrig(int direction)
{
	return WriteRegister(TRIG_CONTROL_ADDR, ~TRIG_ENABLE_OUTPUT_BIT, direction);
}

int ADQ::WriteTrig(int data)
{
	return WriteRegister(TRIG_CONTROL_ADDR, ~TRIG_OUTPUT_VALUE_BIT, (data)<<TRIG_OUTPUT_VALUE_POS);
}

int ADQ::SetDirectionGPIO(unsigned int direction, unsigned int mask)
{
	return WriteRegister(COM_GPIO_EN_ADDRESS, mask, direction);
}

int ADQ::WriteGPIO(unsigned int data, unsigned int mask)
{
	return WriteRegister(COM_GPIO_O_ADDRESS, mask, data);
}

unsigned int ADQ::ReadGPIO()
{
	return ReadRegister(COM_GPIO_I_ADDRESS);
}

int ADQ::SetCacheSize(unsigned int newSizeInBytes)
{
    unsigned int newSizeIn128bWords = newSizeInBytes / 16;
    unsigned int success = 0;

    if (    (newSizeIn128bWords <= MAX_CACHE_SIZE_IN_128B_WORDS)
         && (newSizeIn128bWords >= MIN_CACHE_SIZE_IN_128B_WORDS)
         && ((newSizeInBytes % 1024) == 0))
    {
        m_CacheSizeIn128BWords = newSizeIn128bWords;

        // Invalidate current cache when setting new size
        m_CacheWord128bStart = ~0u;
		m_CacheWord128bEnd = 0u;

        success = 1;
    }
   
    return success;
}

int ADQ::SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize)
{
	return m_Device->SetTransferBuffers(nOfBuffers, bufferSize);
}

unsigned int ADQ::ResetDevice(int resetlevel)
{
    unsigned int success = 1;

    switch(resetlevel)
    {
    case 4: 
        // Reset DRAM (Only working for ADQ Classic)
        success = WriteRegister(GPIO_O_ADDR, ~(1uL << GPIO_O_BITS_RESET_SW), (1uL << GPIO_O_BITS_RESET_SW));
        Sleep(10);
        success = success && WriteRegister(GPIO_O_ADDR, ~(1uL << GPIO_O_BITS_RESET_SW), ~(1uL << GPIO_O_BITS_RESET_SW));
        Sleep(10);
        break;
    case 8:
        // Reset USB buffers
        success = m_Device->ResetDevice(resetlevel);
        break;
    case 16:
        // Reset all device by "power-cycling" internally
        success = m_Device->ResetDevice(resetlevel);
        break;
    default:
        // Undefined ResetLevel
        success = 0;
        break;
    }
    
    return success;

}
