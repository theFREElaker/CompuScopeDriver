#include "LegacyAddresses.h"

int32 GAGEAPI	CalculateMrFileAddresses (int32 group, int16 board_type, int32 depth, int32 in_memory, int16, int16 op_mode, float tbs, int32 *trig, int32 *start, int32 *end)
{
	uInt16	mulrec_adjust = (uInt16)(op_mode) & GAGE_MODE_MULREC_ADJUST;
	uInt16	version_adjust = (uInt16)(op_mode) & GAGE_VERSION_ADJUST;		/*	EXACTLY the same as above.	*/
	uInt32	memory = (uInt32) in_memory;
	uInt16	external_clock_adjust = (uInt16)(op_mode) & GAGE_MODE_EXT_CLK_ADJ;
	int32	DepthTmp;
	int32	MaxRecordCount;

	op_mode &= ~GAGE_MODE_OPTIONS;

	if ( board_type == GAGE_ASSUME_CS8500 )
	{
		int32	MulRecDepthAdj = 128;

		DepthTmp = depth + MulRecDepthAdj;

		// Calculate the maximum record count
		// This calculation should be the same in cs85_card_set_number_of_records
		if ( DepthTmp < 256 )
			MaxRecordCount = memory / 256;
		else
			MaxRecordCount = memory / DepthTmp;

		if ( MaxRecordCount > 0x1000000 )
			MaxRecordCount = 0x100000;		// Limit to 24 bits number

		if ((group == -1) || (group > MaxRecordCount) )
		{
			group = MaxRecordCount;
		}

		if (group == 0)
		{
	    	if ( !mulrec_adjust )
			    *trig = 8;
			else
				    if ( (tbs == (float)2) || (tbs == (float)5) ) 	// 500,200 MHz
					*trig = 9;
			    else if ( (tbs == (float)10) || (tbs == (float)4) )	// 100,250 MHz
					*trig = 4;
			    else if ( (tbs == (float)20) || ( tbs == (float)8) )// 125,50 MHz
					*trig = 2;
			    else												// 62.5*,25 MHz
					*trig = 1;
			
			*start = 0;
			*end = memory - 1L;
			return (0);
		}
		if ( !mulrec_adjust )
			*trig = (*start = ((group - 1L) * (depth ))) + 8;
		else
		{
		    if ( (tbs == (float)2) || (tbs == (float)5) )  		// 500,200 MHz
				*trig = (*start = ((group - 1L) * (depth ))) + 9;
		    else if ( (tbs == (float)10) || (tbs == (float)4) )	// 100,250 MHz
				*trig = (*start = ((group - 1L) * (depth ))) + 4;
		    else if ( (tbs == (float)20) || ( tbs == (float)8) )// 125,50 MHz
				*trig = (*start = ((group - 1L) * (depth ))) + 2;
		    else												// 62.5*,25 MHz
				*trig = (*start = ((group - 1L) * (depth ))) + 1;
		}
		*end = ((*start + (depth)) % memory) - 1;
		return (group);
	}
	else if (board_type == GAGE_ASSUME_CS12100)
	{
		int32	goffset = ((4 * ((op_mode == GAGE_DUAL_CHAN) && (tbs < (float)40 )))
						+ (8 * ((op_mode == GAGE_SINGLE_CHAN) && (tbs < (float)20))));

		DepthTmp = depth + goffset;

		if( (DepthTmp < 128) && ( op_mode == GAGE_DUAL_CHAN)) 
			MaxRecordCount =  memory / 128;
		else if ( (DepthTmp < 256) && ( op_mode == GAGE_SINGLE_CHAN) )
			MaxRecordCount = memory / 256;
		else
			MaxRecordCount = memory / DepthTmp;

		if ( MaxRecordCount > 0x1000000 )
			MaxRecordCount = 0x1000000;		// Limit to 24 bits number

		if ( (group == -1) ||  (group > MaxRecordCount ) )
		{
			group = MaxRecordCount;
		}

		if (group == 0)
		{
			*trig  = 0;
			*start = 0;
			*end = memory - 1L;
			return (0);
		}
		*start = ((group - 1L) * (depth + goffset));
		*trig = *start;
		*end = (*trig + depth) - 1;
		return (group);
	}
	else if ((uInt16)board_type == GAGE_ASSUME_CS1610)
	{
		int32	offset = 0;

		if( depth < 128 )
			MaxRecordCount = (memory - 8) / 128;
		else
			MaxRecordCount = (memory - 8) / depth;

		if ( MaxRecordCount > 0x1000000 )
			MaxRecordCount = 0x1000000;		// Limit to 24 bits number

		if ((group == -1) || (group > MaxRecordCount) )
		{
			group = MaxRecordCount;
		}
		if (group == 0)
		{
			*trig  = 0;
			*start = 0;
			*end = memory - 1L;
			return (0);
		}

		*start = ((group - 1L) * (depth + offset));
		*trig = *start;
		*end = (*trig + depth + offset) - 1;

		if( version_adjust ) //cs1220
		{
			*trig += 2;
		}
		else
		{
			if ( ! external_clock_adjust )
			{
				if (tbs <= 50.0)		/*	   20 MHz.	*/
					*trig += 4;
				else if (tbs <= 100.0)	/*	   10 MHz.	*/
					*trig += 8;
				else if (tbs <= 200.0)	/*	   5 MHz.	*/
					*trig += 16;
				else					/*	<= 2.5 MHz.	*/
					*trig += 25;
			}
			else
			{	// External clock mode
				*trig += 8;
			}
		}
		return (group);
	}
	else if (board_type == (int16)(GAGE_ASSUME_CS14100))
	{
		// Calculate the maximum record count
		// This calculation should be the same in cs4100_card_set_number_of_records
		DepthTmp = depth;

		if(board_type == (int16)(GAGE_ASSUME_CS14100) && version_adjust)
		{
			DepthTmp += PRETRIG_DELAY_14100 << (int)(op_mode == GAGE_SINGLE_CHAN);
		}

		if( (DepthTmp < 128) && ( op_mode == GAGE_DUAL_CHAN) )
			MaxRecordCount = memory / 128;
		else if ( (DepthTmp < 256) && (op_mode == GAGE_SINGLE_CHAN) )
			MaxRecordCount = memory / 256;
		else			
			MaxRecordCount = memory / DepthTmp;
		if ( MaxRecordCount > 0x1000000 )
			MaxRecordCount = 0x1000000;		// Limit to 24 bits number

		if ( group == -1 || group >  MaxRecordCount )
			group = MaxRecordCount;

		if (group == 0)
		{
			*trig  = 0;
			*start = 0;
			*end = memory - 1L;
			return (0);
		}
		*start = ((group - 1L) * DepthTmp);
		int		toffset = 0;
		if ( board_type == (int16)(GAGE_ASSUME_CS14100) && (op_mode == GAGE_SINGLE_CHAN) )
			toffset = 4;
		*trig = *start + toffset;
		*end = (*start + DepthTmp) - 1;

		// CS14100 HW bug
		//  Dual mode:  Sample rate below 25MHZ, the last group has 4 invalid sample at the end
		//  Single mode:  Sample rate below 50MHZ, the last group has 8 invalid sample at the end
		//  To simplify, cut off on every group
		if( op_mode == GAGE_DUAL_CHAN )
			*end -= 4;
		else	
			*end -= 8;		
		return (group);
	}
	else if	(board_type == GAGE_ASSUME_CS3200)
	{ 
		int32	start_offset[3] = {16, 8, 4};
		int32	end_offset[3] = {8, 4, 2};
		
		if (group > 0)
		{
			*start = ((group - 1L) * (depth+start_offset[op_mode-1]));
			*trig = *start;
			*end = *start + depth -1L + end_offset[op_mode-1];
		}
		else if (group == 0)
		{
			*trig = 0;
			*start = 0;
			*end = memory-1;
		}
		else 
		{ 
			if ((group == -1) || (((uInt32)group * (uInt32)(depth+start_offset[op_mode-1])) > memory))
			{
				group = memory / (depth+start_offset[op_mode-1]);
			}
			*start = ((group - 1L) * (depth+start_offset[op_mode-1]));
			*end = *start + depth -1L + end_offset[op_mode-1];
			*trig = *start;
		}
		return(group);

	}else if	(board_type == GAGE_ASSUME_CS82G){ //nat310700

		int32 trigger_res = 128 * (1 + (op_mode == GAGE_SINGLE_CHAN));

		if (group > 0)
		{
			*trig = group;
			*start = group;
			*end = depth -1L;
		}
		else if (group == 0)
		{
			*trig = 0;
			*start = 0;
			*end = memory-1;
		}
		else 
		{ 
			if ((group == -1) || (group == -2) || (((uInt32)group * (uInt32)(depth+trigger_res)) > memory))  
			{
				// for that particular case the apps should pass depth = depth + ptm, to get the right value of *start= group
				group = memory / (depth+trigger_res);
			}

			*start = group;
			*trig = *start;
			*end = *start + depth -1L;
		}
		return(group);
	}
	else 
	{
		int32 MaxRecordCount = in_memory / depth;

		if ( MaxRecordCount > 0x1000000 )
			MaxRecordCount = 0x1000000;		// Limit to 24 bits number

		if ((group == -1) || (group > MaxRecordCount) )
		{
			group = MaxRecordCount;
		}

		if (group == 0)
		{
			*trig  = 0;
			*start = 0;
			*end = memory - 1L;
			return (0);
		}

		*start = ((group - 1L) * depth);
		*trig = *start;
		*end = (*trig + depth) - 1;
		return (group);		
	}
}


int32 GAGEAPI	CalculateMraFileAddresses (int16 board_type, uInt16 version, int16 channel, int16 op_mode, float tbs, int32 *trig, int32 *start, int32 *end, int16 data)
{
	int16	bits = 0;
	uInt16	version_adjust = (uInt16)(op_mode) & GAGE_VERSION_ADJUST;

	// Retrieve the channel 
	int8	chan = (int8) (channel & 0xFF);

	version;
	op_mode &= ~GAGE_MODE_OPTIONS;

	if (board_type == (int16)(GAGE_ASSUME_CS12100))  
	{
		*trig = *start;	/*	Always in dual channel mode.	*/
	}
	else if ( board_type == (int16)(GAGE_ASSUME_CS14100))
	{
		uInt16 trigger_address_offset = (channel >> 8) & 0xFF;

		if (op_mode & GAGE_SINGLE_CHAN)
		{
			*trig = *start + (data & 1);
		}
		else
		{
			*trig = *start;	/*	ETB's are meaningless in dual channel mode.	*/
		}
		*trig += trigger_address_offset;

		if(version_adjust)
		{
			int32 i32off = PRETRIG_DELAY_14100;
			if (op_mode & GAGE_DUAL_CHAN)
			{
				i32off /= 2;
			}
			*trig += i32off;
		}
	}
	else if ((uInt16)board_type == GAGE_ASSUME_CS1610)  
	{
		*trig = *start + ((data & 1) >> (int)(op_mode == GAGE_DUAL_CHAN));

		if (tbs <= 50.0)		/*	   20 MHz.	*/
			*trig += 4;
		else if (tbs <= 100.0)	/*	   10 MHz.	*/
			*trig += 8;
		else if (tbs <= 200.0)	/*	   5 MHz.	*/
			*trig += 16;
		else					/*	<= 2.5 MHz.	*/
			*trig += 24;
	}
	else if (board_type == GAGE_ASSUME_CS8500)
	{
		uInt16	SC = 1;
		uInt32	addr;

		if ( tbs == (float)10 || tbs == (float)4 )
		{
			SC = 2;
		}else if ( tbs == (float)20 || tbs == (float)8 )
		{
			SC = 4;
		}else if ( tbs == (float)40  )
		{
			SC = 8;
		}
		if ( chan == 0 )
		{
			bits = data & 0x7;
			bits = 7-bits;
			if (SC == 8)
				bits = 0;
			addr = bits;

			if ( bits < 3 )
				addr += 8;

			addr += ((uInt32)(SC/2));
			addr /= ((uInt32)SC);
			*start = *trig+1;
			*trig += addr;
		}
	}
	else if (board_type == GAGE_ASSUME_CS82G) //nat310700
	{
		uInt32	group, depth;
		uInt16	pretrig_depth = (uInt16) data;	// data maximum 32K


		group = *start;
		depth = (*end) +1;

		//----- data = resolution = pretrigger
		*start = ((group - 1) *  (depth + pretrig_depth + 128*(1+(op_mode == GAGE_SINGLE_CHAN))));// give me the address of the group

		*trig = *start + pretrig_depth; //so trig - start = data
		*end += *trig;
	}
	else if (board_type == GAGE_ASSUME_CS3200) //nat201000
	{
		int32 trig_offset[3] = {8, 4, 2};
		
		bits = data & 0x7;
		if (op_mode == GAGE_SINGLE_CHAN)
			*trig = *start + bits + trig_offset[op_mode-1];
		else if (op_mode == GAGE_DUAL_CHAN)
			*trig = *start + (bits >>1)+ trig_offset[op_mode-1];
		else if (op_mode == GAGE_QUAD_CHAN)
			*trig = *start + (bits>>2)+ trig_offset[op_mode-1];

		*trig = *trig + 1;
	}
	else if ((board_type == GAGE_ASSUME_CS14200) || (board_type == GAGE_ASSUME_CS14105))
	{
		bits = 0;
	}
	return ((int32)(bits));
}