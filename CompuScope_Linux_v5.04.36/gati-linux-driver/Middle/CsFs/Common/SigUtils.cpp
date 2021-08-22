
#include "StdAfx.h"
#include "SigUtils.h"
#include <math.h>


TCHAR* szVersion[SIG_VERSION_COUNT] = {(TCHAR*)_T("GS V.1.20"),
									   (TCHAR*)_T("GS V.2.00"),
									   (TCHAR*)_T("GS V.2.05"),
									   (TCHAR*)_T("GS V.2.10"),
									   (TCHAR*)_T("GS V.2.15"),
									   (TCHAR*)_T("GS V.2.20"),
									   (TCHAR*)_T("GS V.2.25"),
									   (TCHAR*)_T("GS V.2.50"),
									   (TCHAR*)_T("GS V.2.60"),
									   (TCHAR*)_T("GS V.2.65"),
									   (TCHAR*)_T("GS V.2.70"),
									   (TCHAR*)_T("GS V.2.75"),
									   (TCHAR*)_T("GS V.2.80"),
									   (TCHAR*)_T("GS V.2.85"),
									   (TCHAR*)_T("GS V.2.95"),
									   (TCHAR*)_T("GS V.3.00")};


short GetSRIndex(float fTBS)
{
	short i;
	for(i = 0; i < SRTI_EXTERNAL; i++ )
	{
		if(fabs(sample_rate_table[i].sr_calc - fTBS) < 0.0001*fTBS )
			return i;
	}
	return SRTI_EXTERNAL;
}

unsigned short MakeBoardType(uInt32 u32SampleBits)
{
	unsigned short usBoardType = GAGE_ASSUME_CS8500;

	if (8 == u32SampleBits)
		usBoardType = GAGE_ASSUME_CS82G;
	else if (12 == u32SampleBits)
		usBoardType = GAGE_ASSUME_CS12100;
	else if (14 == u32SampleBits)
		usBoardType = GAGE_ASSUME_CS14100;
	else if (16 == u32SampleBits)
		usBoardType = GAGE_ASSUME_CS1610;
	else if (32 == u32SampleBits)
		usBoardType = GAGE_ASSUME_CS3200;

	return usBoardType;
}

short GetProbe(short shAdjust)
{
	short shRetProb = 0;
	short shActualProb = shAdjust;
	switch(shActualProb)
	{
		case 1000: shRetProb++; 
		case 500:  shRetProb++;
		case 200:  shRetProb++;
		case 100:  shRetProb++;
		case 50:   shRetProb++; 
		case 20:   shRetProb++;
		case 10:   shRetProb++;
		case 1:    break;
		default:   shRetProb = -shActualProb;
	}
	return shRetProb;
}

short ConstructGainConstant(uInt32 u32InputRange, int16 __unaligned *CapturedGain)
{

	short shAdjust = 1;
	char  chPointPosition = 0;

	//Use half of input value
	long nlInputRange = (long)u32InputRange / 2;

	*CapturedGain=6;
	
	switch(nlInputRange)
	{
		case 10000: (*CapturedGain)--;
		case 5000:  (*CapturedGain)--;
		case 2000:  (*CapturedGain)--;
		case 1000:  (*CapturedGain)--;
		case 500:   (*CapturedGain)--;
		case 200:   (*CapturedGain)--;
		case 100:   break;
			//New format;
		default:    if(nlInputRange<1) // resolution requirement
						nlInputRange = 2;
					
					while(nlInputRange > 998000) //scale down to supported range
					{
						nlInputRange /= 10;
						shAdjust     *= 10;
					}
					while(nlInputRange > 998) //determine point position
					{
						nlInputRange /= 10;
						chPointPosition++;
					}
					*CapturedGain = 0x8000U;  //Indicator of new format;
					*CapturedGain |= ((int16)chPointPosition)<<11;   //put in  point position
					*CapturedGain |= (int16)(nlInputRange/100)<<7;   //first digit
					*CapturedGain |= (int16)((nlInputRange%100)/10)<<3;    //second digit
					*CapturedGain |= (int16)(((nlInputRange%10)>>1)&0x7); //third digit
					break;
	}
	return shAdjust;
}

short ConstructDcOffset(int32 i32DcOffset, uInt32 u32InputRange)
{
	// GageScope saves the DC Offset in the header as 10 * the percentage, that is,
	// a value of 1000 means 100%, -500 mean -50%, etc. The ASCII file header assumes
	// that DC Offset is in millivolts. So we'll do the conversion here, and the opposite
	// when we convert from Sig to ASCII.

	float fPerCent;

	// if input range is 0 (should never happen) return 0 so we don't crash
	if ( 0 == u32InputRange )
	{
		return 0;
	}

	fPerCent = ((float(i32DcOffset) * 200) / float(u32InputRange)) * 10.;
	
	if(fPerCent > 1000)
		fPerCent = 1000.;
	else if (fPerCent < -1000 )
		fPerCent = -1000.;

	return (int16)fPerCent;
}


BOOL GetSIGVersion(LPCTSTR Version)
{
	for (int i = 0; i < SIG_VERSION_COUNT; i++)
	{
		if ( !_tcscmp(Version, szVersion[i]) )
		{
			return TRUE;
		}
	}
	return FALSE;
}


uInt32 ConstructDataRange(short shGain)
{
	uInt32 u32InputRange = 1;

	if(shGain & 0x8000) //New format
	{
		char chPointPosition = char((shGain >> 11) & 0x3);
		u32InputRange = (shGain & 0x7) << 1;				//third digit
		u32InputRange += ((shGain & 0x78) >> 3) * 10;		//second digit
		u32InputRange += ((shGain & 0x780) >> 7) * 100;	//first digit
		while (chPointPosition--)
		{
			u32InputRange *= 10;
		}
		u32InputRange *= 2;
	}
	else //old format
	{
		switch(shGain)
		{
			case 0: u32InputRange = 20000; break;
			case 1: u32InputRange = 10000; break;
			case 2: u32InputRange = 4000;  break;
			case 3: u32InputRange = 2000;  break;
			case 4: u32InputRange = 1000;  break;
			case 5: u32InputRange = 400;   break;
			case 6: u32InputRange = 200;   break;
		}
	}
	return u32InputRange;
}


int32 ConstructDcOffsetPercent(short sDcOffset, uInt32 u32Range)
{
	return int32(float(sDcOffset) * float(u32Range) / 2000.);
}

short CheckDiskBoardType (disk_file_header *header)
{
	if (header->board_type)
		return (header->board_type);
	if (header->inverted_data)
		return (GAGE_ASSUME_CS220);
	if (header->sample_rate_index == 26 ||
		header->sample_rate_index == 25 ||
		header->sample_rate_index == 23)
		return (GAGE_ASSUME_CS250);
	if (header->sample_rate_index == 24 ||
		header->sample_rate_index == 22)
		return (GAGE_ASSUME_CSLITE);
	if (header->captured_gain < 2)
		return (GAGE_ASSUME_CS250);
	if (header->sample_depth == GAGE_POST_128K ||
		header->sample_depth == GAGE_POST_32K)
		return (GAGE_ASSUME_CS250);
	if (header->sample_depth == GAGE_POST_64K ||
		header->sample_depth == GAGE_POST_16K)
		return (GAGE_ASSUME_CSLITE);
	if (header->sample_depth == GAGE_POST_8M ||
		header->sample_depth == GAGE_POST_4M ||
		header->sample_depth == GAGE_POST_2M ||
		header->sample_depth == GAGE_POST_1M ||
		header->sample_depth == GAGE_POST_256K ||
		header->sample_depth == GAGE_POST_32K)
		return (GAGE_ASSUME_CS220);
	if (header->sample_depth <= GAGE_POST_16K)
		return (GAGE_ASSUME_CSLITE);
	if (header->sample_depth <= GAGE_POST_32K)
		return (GAGE_ASSUME_CS250);
	if (header->sample_depth <= GAGE_POST_64K)
		return (GAGE_ASSUME_CSLITE);
	if (header->sample_depth <= GAGE_POST_128K)
		return (GAGE_ASSUME_CS250);
	if (header->sample_depth <= GAGE_POST_8M)
		return (GAGE_ASSUME_CS220);

	return (GAGE_ASSUME_CSLITE);
}

void CheckDiskSampleRateIndex (short sCheckV270, 
							   short sCheckV280, 
							   short sCheckV285, 
							   short sCheckV300, 
							   short __unaligned *sSampleRateIndex)
{
	if (sCheckV270)  {
		if (*sSampleRateIndex == 26)		/*	100 MHz.	*/
			*sSampleRateIndex += 3;
		else if (*sSampleRateIndex >= 24)	/*	40 or 50 MHz.	*/
			*sSampleRateIndex += 2;
		else if (*sSampleRateIndex >= 20)	/*	5, 10, 20 or 25 MHz.	*/
			*sSampleRateIndex += 1;
	}
	if (sCheckV280)  {
		if (*sSampleRateIndex >= 39)	/*	5 GHz or External.	*/
			*sSampleRateIndex += 1;
	}
	if (sCheckV285)  {
		if (*sSampleRateIndex >= 23)	/*	20 MHz to External.	*/
			*sSampleRateIndex += 1;
	}
	if (sCheckV300)  {
		if (*sSampleRateIndex >= 42)	/*	External.	*/
			*sSampleRateIndex += 5;
		else if (*sSampleRateIndex >= 33)	/*	150 MHz to External.	*/
			*sSampleRateIndex += 3;
		else if (*sSampleRateIndex >= 30)	/*	100 MHz to External.	*/
			*sSampleRateIndex += 2;
	}
}

void AdjustHeaderToVersion (disk_file_header *header)
{
	CheckDiskSampleRateIndex (strcmp(header->file_version, SIG_VERSION_270) < 0 ? 1 : 0,
							  strcmp(header->file_version, SIG_VERSION_280) < 0 ? 1 : 0,
							  strcmp(header->file_version, SIG_VERSION_285) < 0 ? 1 : 0,
							  strcmp(header->file_version, SIG_VERSION_300) < 0 ? 1 : 0,
							  &(header->sample_rate_index)
							 );

	if (strcmp(header->file_version, SIG_VERSION_270) < 0)
	{
		if (header->resolution_12_bits)  {
			header->sample_offset = GAGE_12_BIT_SAMPLE_OFFSET;
			header->sample_resolution = GAGE_12_BIT_SAMPLE_RESOLUTION;
			header->sample_bits = GAGE_12_BIT_SAMPLE_BITS;
		}else{
			header->sample_offset = GAGE_8_BIT_SAMPLE_OFFSET;
			header->sample_resolution = GAGE_8_BIT_SAMPLE_RESOLUTION;
			header->sample_bits = GAGE_8_BIT_SAMPLE_BITS;
		}
	}

	if (!strcmp(header->file_version, SIG_VERSION_120))
	{
		header->captured_gain = (int16)((header->captured_gain * 2) + 3);
	}

    if (strcmp(header->file_version, SIG_VERSION_215) < 0)
	{
		if (header->captured_gain > 2)
			header->captured_gain++;
	}

	header->board_type = CheckDiskBoardType (header);
	
	if(header->sample_bits == 0 )
	{
		if(header->resolution_12_bits)
			header->sample_bits = 12;
		else
			header->sample_bits = 8;
	}
}

int32 GetSampleResFromHeader(disk_file_header* header)
{
	return ((0 != header->sample_resolution_32) ? header->sample_resolution_32 : int32(header->sample_resolution));
}

int32 GetSampleOffsetFromHeader(disk_file_header* header)
{
	return (((1 == header->sample_resolution) || (3 == header->resolution_12_bits)) ? header->sample_offset_32 : int32(header->sample_offset));
}

uInt32 GetMulRecCountFromHeader(disk_file_header* header)
{
	uInt32 u32RecordCount;
	if (0 == header->multiple_record_count) // some older version of GageScope didn't set this value
	{
		u32RecordCount = (0 == header->multiple_record) ? 1 : uInt32(header->sample_depth / (header->ending_address - header->starting_address + 1));
	}
	else
	{
		u32RecordCount = uInt32(header->multiple_record_count);
	}	
	return u32RecordCount;
}

int64 GetSampleRateFromHeader(disk_file_header* header)
{
	if (SRTI_EXTERNAL == header->sample_rate_index)
	{
		if (0.0 == header->external_tbs || 0.0 != header->external_clock_rate)
		{
			return int64(header->external_clock_rate);
		}
		else
		{
			double divisor = 1.;
			double dValue = (1.0e9/(double)header->external_tbs);
			if (dValue >= 1000000000.)
			{
				divisor = 1000000000.;
			}
			else if (dValue >= 1000000.)
			{
				divisor = 1000000.;
			}
			else if (dValue >= 1000.)
			{
				divisor = 1000.;
			}

			dValue = dValue / divisor;
			if (floor(dValue) == dValue)
			{
				dValue = floor(dValue + 0.5);
			}
			else
			{
				dValue = floor((dValue + 0.05) * 10.) / 10.;
			}

			return int64(dValue);
		}

	}
	else
	{
		int64 i64Multiplier;
		if (GAGE_GHZ == sample_rate_table[header->sample_rate_index].mult)
			i64Multiplier = 1000000000;
		else if (GAGE_MHZ == sample_rate_table[header->sample_rate_index].mult)
			i64Multiplier = 1000000;
		else if (GAGE_KHZ == sample_rate_table[header->sample_rate_index].mult)
			i64Multiplier = 1000;
		else
			i64Multiplier = 1;

		int64 i64SampleRate = i64Multiplier * sample_rate_table[header->sample_rate_index].rate;

		return i64SampleRate;
	}
}
