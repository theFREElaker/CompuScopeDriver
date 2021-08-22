// Project: ADQ-API
// File: ADQAPI.cpp
//
// latest revision: 2008-10-27
// by Peter Johansson

#include "ADQInterface.h"
#include "ADQControlUnit.h"
#include "ADQ112.h"
#include "ADQ114.h"
#include "ADQ214.h"
#include "ADQAPI_definitions.h"
#include "swrev.h"


extern "C" __declspec(dllexport) void* CreateADQControlUnit()
{
	ADQControlUnit* ADQCU_ptr = new ADQControlUnit(NULL);
	return (void*)ADQCU_ptr;
}

extern "C" __declspec(dllexport) void* CreateADQControlUnitWN(HANDLE ReceiverWnd)
{
	ADQControlUnit* ADQCU_ptr = new ADQControlUnit(ReceiverWnd);
	return (void*)ADQCU_ptr;
}

extern "C" __declspec(dllexport) void DeleteADQControlUnit(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	delete adq_cu;
}

extern "C" __declspec(dllexport) int ADQAPI_GetRevision(void)
{
	return (int)SWREV_ADQAPI;
}

extern "C" __declspec(dllexport) int ADQControlUnit_FindDevices(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;

	return adq_cu->FindDevices();
}

extern "C" __declspec(dllexport) int ADQControlUnit_GetFailedDeviceCount(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;

	return adq_cu->GetNumberOfFailedDevices();
}

extern "C" __declspec(dllexport) int ADQControlUnit_NofADQ(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_ADQ = 0;
    n_of_ADQ = adq_cu->NofADQ();
	return n_of_ADQ;
}

extern "C" __declspec(dllexport) int ADQControlUnit_NofADQ112(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_ADQ112 = 0;
	n_of_ADQ112 = adq_cu->NofADQ112();
	return n_of_ADQ112;
}

extern "C" __declspec(dllexport) int ADQControlUnit_NofADQ114(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_adq114 = 0;
	n_of_adq114 = adq_cu->NofADQ114();
	return n_of_adq114;
}

extern "C" __declspec(dllexport) int ADQControlUnit_NofADQ214(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_adq214 = 0;
	n_of_adq214 = adq_cu->NofADQ214();
	return n_of_adq214;
}

extern "C" __declspec(dllexport) void ADQControlUnit_DeleteADQ112(void* adq_cu_ptr, int ADQ112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	adq_cu->DeleteADQ112(ADQ112_num);
	return;
}

extern "C" __declspec(dllexport) void ADQControlUnit_DeleteADQ114(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	adq_cu->DeleteADQ114(adq114_num);
	return;
}

extern "C" __declspec(dllexport) void ADQControlUnit_DeleteADQ214(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	adq_cu->DeleteADQ214(adq214_num);
	return;
}

extern "C" __declspec(dllexport) ADQInterface* ADQControlUnit_GetADQ(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	return adq_cu->GetADQ(adq_num);
}



//------------------------- ADQ112 --------------------------------------------------------------------------

extern "C" __declspec(dllexport) int ADQ112_SetTransferBuffers(void* adq_cu_ptr, int adq112_num, unsigned int nOfBuffers, unsigned int bufferSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetTransferBuffers(nOfBuffers, bufferSize);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetCacheSize(void* adq_cu_ptr, int adq112_num, unsigned int newCacheSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetCacheSize(newCacheSize);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq112_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
        success = adq112_ptr->SetTrigLevelResetValue(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SetSampleSkip(void* adq_cu_ptr, int adq112_num, unsigned int skipfactor)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
        success = adq112_ptr->SetSampleSkip(skipfactor);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetLvlTrigLevel(void* adq_cu_ptr, int adq112_num, int level)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetLvlTrigLevel(level);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetLvlTrigFlank(void* adq_cu_ptr, int adq112_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetLvlTrigEdge(void* adq_cu_ptr, int adq112_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetClockSource(void* adq_cu_ptr, int adq112_num, int source)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetClockSource(source);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetClockFrequencyMode(void* adq_cu_ptr, int adq112_num, int clockmode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetClockFrequencyMode(clockmode);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetPllFreqDivider(void* adq_cu_ptr, int adq112_num, int divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetPllFreqDivider(divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetPll(void* adq_cu_ptr, int adq112_num, int n_divider, int r_divider, int vco_divider, int channel_divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetPll(n_divider, r_divider, vco_divider, channel_divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetTriggerMode(void* adq_cu_ptr, int adq112_num, int trig_mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetTriggerMode(trig_mode);
	}
	return success;
}

//extern "C" __declspec(dllexport) int ADQ112_SetupMemory(void* adq_cu_ptr, int adq112_num, unsigned int RecordSize, unsigned int NofRecords)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
//	int success = 0;
//	if (adq112_ptr != NULL)
//	{
//		success = adq112_ptr->MultiRecordSetup(NofRecords, RecordSize);
//	}
//	return success;
//}

extern "C" __declspec(dllexport) int ADQ112_SetSampleWidth(void* adq_cu_ptr, int adq112_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetSampleWidth(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetNofBits(void* adq_cu_ptr, int adq112_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetNofBits(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetPreTrigSamples(void* adq_cu_ptr, int adq112_num, unsigned int PreTrigSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetPreTrigSamples(PreTrigSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq112_num, unsigned int TriggerHoldOffSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetTriggerHoldOffSamples(TriggerHoldOffSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetBufferSizePages(void* adq_cu_ptr, int adq112_num, unsigned int pages)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetBufferSizePages(pages);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SetBufferSize(void* adq_cu_ptr, int adq112_num, unsigned int samples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetBufferSize(samples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_ArmTrigger(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->ArmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_DisarmTrigger(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->DisarmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_USBTrig(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_SWTrig(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_CollectDataNextPage(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->CollectDataNextPage();
	}
	return success;
}


extern "C" __declspec(dllexport) int ADQ112_CollectRecord(void* adq_cu_ptr, int adq112_num, unsigned int record_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->MultiRecordGetRecord((int)record_num);
	}
	return success;
}


extern "C" __declspec(dllexport) int* ADQ112_GetPtrData(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int* data_ptr = 0;
	if (adq112_ptr != NULL)
	{
		data_ptr = (int*)adq112_ptr->GetPtrData(1);
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) void* ADQ112_GetPtrStream(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	void* data_ptr = 0;
	if (adq112_ptr != NULL)
	{
		data_ptr = adq112_ptr->GetPtrStream();
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) int ADQ112_GetWaitingForTrigger(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int wait4trig = 0;
	if (adq112_ptr != NULL)
	{
		wait4trig = adq112_ptr->GetWaitingForTrigger();
	}
	return wait4trig;
}

extern "C" __declspec(dllexport) int ADQ112_GetTrigged(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trigged = 0;
	if (adq112_ptr != NULL)
	{
		trigged = adq112_ptr->GetAcquired();
	}
	return trigged;
}

extern "C" __declspec(dllexport) int ADQ112_GetTriggedAll(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trigged = 0;
	if (adq112_ptr != NULL)
	{
		trigged = adq112_ptr->GetAcquiredAll();
	}
	return trigged;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetPageCount(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int page_count = 0;
	if (adq112_ptr != NULL)
	{
		page_count = adq112_ptr->GetPageCount();
	}
	return page_count;
}

extern "C" __declspec(dllexport) int ADQ112_GetLvlTrigLevel(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int level = 0;
	if (adq112_ptr != NULL)
	{
		level = adq112_ptr->GetLvlTrigLevel();
	}
	return level;
}

extern "C" __declspec(dllexport) int ADQ112_GetLvlTrigFlank(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int edge = 0;
	if (adq112_ptr != NULL)
	{
		edge = adq112_ptr->GetLvlTrigEdge();
	}
	return edge;
}


extern "C" __declspec(dllexport) int ADQ112_GetPllFreqDivider(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int divider = 0;
	if (adq112_ptr != NULL)
	{
		divider = adq112_ptr->GetPllFreqDivider();
	}
	return divider;
}

extern "C" __declspec(dllexport) int ADQ112_GetClockSource(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int clk_source = 0;
	if (adq112_ptr != NULL)
	{
		clk_source = adq112_ptr->GetClockSource();
	}
	return clk_source;
}

extern "C" __declspec(dllexport) int ADQ112_GetTriggerMode(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trig_mode = 0;
	if (adq112_ptr != NULL)
	{
		trig_mode = adq112_ptr->GetTriggerMode();
	}
	return trig_mode;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetBufferSizePages(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int buffersize_pages = 0;
	if (adq112_ptr != NULL)
	{
		buffersize_pages = adq112_ptr->GetBufferSizePages();
	}
	return buffersize_pages;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetBufferSize(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int buffersize = 0;
	if (adq112_ptr != NULL)
	{
		buffersize = adq112_ptr->GetBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetMaxBufferSize(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int buffersize = 0;
	if (adq112_ptr != NULL)
	{
		buffersize = adq112_ptr->GetMaxBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetMaxBufferSizePages(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int buffersize = 0;
	if (adq112_ptr != NULL)
	{
		buffersize = adq112_ptr->GetMaxBufferSizePages();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetSamplesPerPage(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int buffersize = 0;
	if (adq112_ptr != NULL)
	{
		buffersize = adq112_ptr->GetSamplesPerPage();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetUSBAddress(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int usb_address = 0;
	if (adq112_ptr != NULL)
	{
		usb_address = adq112_ptr->GetUSBAddress();
	}
	return usb_address;
}


extern "C" __declspec(dllexport) unsigned int ADQ112_GetPCIeAddress(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int pcie_address = 0;
	if (adq112_ptr != NULL)
	{
		pcie_address = adq112_ptr->GetPCIeAddress();
	}
	return pcie_address;
}



extern "C" __declspec(dllexport) unsigned int ADQ112_GetBcdDevice(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int BcdDevice = 0;
	if (adq112_ptr != NULL)
	{
		BcdDevice = adq112_ptr->GetBcdDevice();
	}
	return BcdDevice;
}

extern "C" __declspec(dllexport) int* ADQ112_GetRevision(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int* revision = 0;
	if (adq112_ptr != NULL)
	{
		revision = adq112_ptr->GetRevision();
	}
	return revision;
}

extern "C" __declspec(dllexport) int ADQ112_GetTriggerInformation(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trig_info = 0;
	if (adq112_ptr != NULL)
	{
		trig_info = adq112_ptr->GetTriggerInformation();
	}
	return trig_info;
}

extern "C" __declspec(dllexport) int ADQ112_GetTrigPoint(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trig_point = 0;
	if (adq112_ptr != NULL)
	{
		trig_point = adq112_ptr->GetTrigPoint();
	}
	return trig_point;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetTrigType(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int trig_type = 0;
	if (adq112_ptr != NULL)
	{
		trig_type = adq112_ptr->GetTrigType();
	}
	return trig_type;
}



extern "C" __declspec(dllexport) int ADQ112_GetOverflow(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int overflow = 0;
	if (adq112_ptr != NULL)
	{
		overflow = adq112_ptr->GetOverflow();
	}
	return overflow;
}

extern "C" __declspec(dllexport) int ADQ112_GetRecordSize(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int output = 0;
	if (adq112_ptr != NULL)
	{
		output = adq112_ptr->GetRecordSize();
	}
	return output;
}

extern "C" __declspec(dllexport) int ADQ112_GetNofRecords(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int output = 0;
	if (adq112_ptr != NULL)
	{
		output = adq112_ptr->GetNofRecords();
	}
	return output;
}
extern "C" __declspec(dllexport) int ADQ112_SetTrigTimeMode(void* adq_cu_ptr, int adq112_num, int TrigTimeMode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetTrigTimeMode(TrigTimeMode);
	}
	return success;
}
extern "C" __declspec(dllexport) int ADQ112_ResetTrigTimer(void* adq_cu_ptr, int adq112_num, int TrigTimeRestart)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->ResetTrigTimer(TrigTimeRestart);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned long long ADQ112_GetTrigTime(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned long long TrigTime = 0;
	if (adq112_ptr != NULL)
	{
		TrigTime = adq112_ptr->GetTrigTime();
	}
	return TrigTime;
}
extern "C" __declspec(dllexport) unsigned long long ADQ112_GetTrigTimeCycles(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned long long TrigTimeCycles = 0;
	if (adq112_ptr != NULL)
	{
		TrigTimeCycles = adq112_ptr->GetTrigTimeCycles();
	}
	return TrigTimeCycles;
}
extern "C" __declspec(dllexport) unsigned int ADQ112_GetTrigTimeSyncs(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int TrigTimeSyncs = 0;
	if (adq112_ptr != NULL)
	{
		TrigTimeSyncs = adq112_ptr->GetTrigTimeSyncs();
	}
	return TrigTimeSyncs;
}
extern "C" __declspec(dllexport) unsigned int ADQ112_GetTrigTimeStart(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int TrigTimeStart = 0;
	if (adq112_ptr != NULL)
	{
		TrigTimeStart = adq112_ptr->GetTrigTimeStart();
	}
	return TrigTimeStart;
}
extern "C" __declspec(dllexport) unsigned int* ADQ112_GetMultiRecordHeader(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int* p_MultiRecordHeader = NULL;
	if (adq112_ptr != NULL)
	{
		p_MultiRecordHeader = adq112_ptr->GetMultiRecordHeader();
	}
	return p_MultiRecordHeader;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_ResetDevice(void* adq_cu_ptr, int adq112_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
        success = adq112_ptr->ResetDevice(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ112_IsUSBDevice(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int usb_device = 0;
	if (adq112_ptr != NULL)
	{
		usb_device = adq112_ptr->IsUSBDevice();
	}
	return usb_device;
}


extern "C" __declspec(dllexport) int ADQ112_IsPCIeDevice(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int PCIe_device = 0;
	if (adq112_ptr != NULL)
	{
		PCIe_device = adq112_ptr->IsPCIeDevice();
	}
	return PCIe_device;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_IsAlive(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int result = 0;
	if (adq112_ptr != NULL)
	{
		result = adq112_ptr->IsAlive();
	}
	return result;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SendProcessorCommand(void* adq_cu_ptr, int adq112_num, int command, int argument)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->SendProcessorCommand(command, argument);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SendLongProcessorCommand(void* adq_cu_ptr, int adq112_num, int command, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->SendProcessorCommand(command, addr, mask, data);
	}
	return message;
}

//extern "C" __declspec(dllexport) unsigned int ADQ112_WriteRegister(void* adq_cu_ptr, int adq112_num, int FPGA_num, unsigned int addr, unsigned int mask, unsigned int data)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
//	unsigned int message = 0;
//	if (adq112_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq112_ptr->WriteAlgoRegister(addr, mask, data);
//		else if (FPGA_num == 2)
//			message = adq112_ptr->WriteRegister(addr, mask, data);
//	}
//	return message;
//}

//extern "C" __declspec(dllexport) unsigned int ADQ112_ReadRegister(void* adq_cu_ptr, int adq112_num, int FPGA_num, unsigned int addr)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
//	unsigned int message = 0;
//	if (adq112_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq112_ptr->ReadAlgoRegister(addr);
//		else if (FPGA_num == 2)
//			message = adq112_ptr->ReadRegister(addr);
//	}
//	return message;
//}

extern "C" __declspec(dllexport) unsigned int ADQ112_WriteRegister(void* adq_cu_ptr, int adq112_num, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->WriteRegister(addr, mask, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_ReadRegister(void* adq_cu_ptr, int adq112_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->ReadRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_WriteAlgoRegister(void* adq_cu_ptr, int adq112_num, int addr, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->WriteAlgoRegister(addr, 0, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_ReadAlgoRegister(void* adq_cu_ptr, int adq112_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->ReadAlgoRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetErrorVector(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->GetErrorVector();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetStreamStatus(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->GetStreamStatus();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SetStreamStatus(void* adq_cu_ptr, int adq112_num, unsigned int status)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->SetStreamStatus(status);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetStreamOverflow(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->GetStreamOverflow();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_ReadEEPROM(void* adq_cu_ptr, int adq112_num, int unsigned addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->ReadEEPROM(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_WriteEEPROM(void* adq_cu_ptr, int adq112_num, unsigned int addr, unsigned char data, unsigned int accesscode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = NULL;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->WriteEEPROM(addr, data, accesscode);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetTemperature(void* adq_cu_ptr, int adq112_num, unsigned int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	unsigned int message = 0;
	if (adq112_ptr != NULL)
	{
		message = adq112_ptr->GetTemperature(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) char* ADQ112_GetBoardSerialNumber(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->GetBoardSerialNumber();
	}
	else
	{
		return NULL;
	}
}

extern "C" __declspec(dllexport) int ADQ112_SetDataFormat(void* adq_cu_ptr, int adq112_num, unsigned int format)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int success = 0;
	if (adq112_ptr != NULL)
	{
		success = adq112_ptr->SetDataFormat(format);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ112_GetDataFormat(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	int format = 0;
	if (adq112_ptr != NULL)
	{
		format = adq112_ptr->GetDataFormat();
	}
	return format;
}

extern "C" __declspec(dllexport) unsigned int   ADQ112_MultiRecordSetup(void* adq_cu_ptr, int adq112_num,int NumberOfRecords, int SamplesPerRecord)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->MultiRecordSetup(NumberOfRecords, SamplesPerRecord);
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ112_MultiRecordGetRecord(void* adq_cu_ptr, int adq112_num,int RecordNumber)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
//	if (adq112_ptr != NULL)
//	{
//		return adq112_ptr->MultiRecordGetRecord(RecordNumber);
//	}
//	else
//	{
//		return NULL;
//	}
//}
//
extern "C" __declspec(dllexport) unsigned int   ADQ112_MultiRecordClose(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->MultiRecordClose();
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ112_MultiRecordGetTrigged(void* adq_cu_ptr, int adq112_num)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
//	if (adq112_ptr != NULL)
//	{
//		return adq112_ptr->MultiRecordGetTrigged();
//	}
//	else
//	{
//		return NULL;
//	}
//}

extern "C" __declspec(dllexport) unsigned int ADQ112_MemoryDump(void* adq_cu_ptr, int adq112_num, 
																unsigned int StartAddress, unsigned int EndAddress, 
																unsigned char* buffer, unsigned int *bufctr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->MemoryDump(StartAddress, EndAddress, buffer, bufctr);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_WriteTrig(void* adq_cu_ptr, int adq112_num, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->WriteTrig(data);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_SetTestPatternMode(void* adq_cu_ptr, int adq112_num, int mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
        return adq112_ptr->SetTestPatternMode(mode);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_SetTestPatternConstant(void* adq_cu_ptr, int adq112_num, int value)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
        return adq112_ptr->SetTestPatternConstant(value);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_SetDirectionTrig(void* adq_cu_ptr, int adq112_num, int direction)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->SetDirectionTrig(direction);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ112_ReadGPIO(void* adq_cu_ptr, int adq112_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->ReadGPIO();
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_WriteGPIO(void* adq_cu_ptr, int adq112_num, 
													  unsigned int data, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->WriteGPIO(data, mask);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ112_SetDirectionGPIO(void* adq_cu_ptr, int adq112_num, 
															 unsigned int direction, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->SetDirectionGPIO(direction, mask);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SetAlgoStatus(void* adq_cu_ptr, int adq112_num, int status)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->SetAlgoStatus(status);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ112_SetAlgoNyquistBand(void* adq_cu_ptr, int adq112_num, unsigned int band)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ112* adq112_ptr = adq_cu->GetADQ112(adq112_num);
	
	if (adq112_ptr != NULL)
	{
		return adq112_ptr->SetAlgoNyquistBand(band);
	}
	else
	{
		return 0;
	}
}


//------------------------- ADQ114 --------------------------------------------------------------------------

extern "C" __declspec(dllexport) int ADQ114_SetTransferBuffers(void* adq_cu_ptr, int adq114_num, unsigned int nOfBuffers, unsigned int bufferSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetTransferBuffers(nOfBuffers, bufferSize);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetCacheSize(void* adq_cu_ptr, int adq114_num, unsigned int newCacheSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetCacheSize(newCacheSize);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq114_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
        success = adq114_ptr->SetTrigLevelResetValue(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SetSampleSkip(void* adq_cu_ptr, int adq114_num, unsigned int skipfactor)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
        success = adq114_ptr->SetSampleSkip(skipfactor);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetLvlTrigLevel(void* adq_cu_ptr, int adq114_num, int level)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetLvlTrigLevel(level);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetLvlTrigFlank(void* adq_cu_ptr, int adq114_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetLvlTrigEdge(void* adq_cu_ptr, int adq114_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetClockSource(void* adq_cu_ptr, int adq114_num, int source)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetClockSource(source);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetClockFrequencyMode(void* adq_cu_ptr, int adq114_num, int clockmode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetClockFrequencyMode(clockmode);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetPllFreqDivider(void* adq_cu_ptr, int adq114_num, int divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetPllFreqDivider(divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetPll(void* adq_cu_ptr, int adq114_num, int n_divider, int r_divider, int vco_divider, int channel_divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetPll(n_divider, r_divider, vco_divider, channel_divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetupMemory(void* adq_cu_ptr, int adq114_num, unsigned int RecordSize, unsigned int NofRecords)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->MultiRecordSetup(NofRecords, RecordSize);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetTriggerMode(void* adq_cu_ptr, int adq114_num, int trig_mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetTriggerMode(trig_mode);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetSampleWidth(void* adq_cu_ptr, int adq114_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetSampleWidth(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetNofBits(void* adq_cu_ptr, int adq114_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetNofBits(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetPreTrigSamples(void* adq_cu_ptr, int adq114_num, unsigned int PreTrigSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetPreTrigSamples(PreTrigSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq114_num, unsigned int TriggerHoldOffSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetTriggerHoldOffSamples(TriggerHoldOffSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetBufferSizePages(void* adq_cu_ptr, int adq114_num, unsigned int pages)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetBufferSizePages(pages);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SetBufferSize(void* adq_cu_ptr, int adq114_num, unsigned int samples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetBufferSize(samples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_ArmTrigger(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->ArmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_DisarmTrigger(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->DisarmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_USBTrig(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_SWTrig(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_CollectDataNextPage(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->CollectDataNextPage();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_CollectRecord(void* adq_cu_ptr, int adq114_num, unsigned int record_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->MultiRecordGetRecord(record_num);
	}
	return success;
}


extern "C" __declspec(dllexport) int* ADQ114_GetPtrData(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int* data_ptr = 0;
	if (adq114_ptr != NULL)
	{
		data_ptr = (int*)adq114_ptr->GetPtrData(1);
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) void* ADQ114_GetPtrStream(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	void* data_ptr = 0;
	if (adq114_ptr != NULL)
	{
		data_ptr = adq114_ptr->GetPtrStream();
	}
	return data_ptr;
}


extern "C" __declspec(dllexport) int ADQ114_GetWaitingForTrigger(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int wait4trig = 0;
	if (adq114_ptr != NULL)
	{
		wait4trig = adq114_ptr->GetWaitingForTrigger();
	}
	return wait4trig;
}

extern "C" __declspec(dllexport) int ADQ114_GetTrigged(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trigged = 0;
	if (adq114_ptr != NULL)
	{
		trigged = adq114_ptr->GetAcquired();
	}
	return trigged;
}

extern "C" __declspec(dllexport) int ADQ114_GetTriggedAll(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trigged = 0;
	if (adq114_ptr != NULL)
	{
		trigged = adq114_ptr->GetAcquiredAll();
	}
	return trigged;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetPageCount(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int page_count = 0;
	if (adq114_ptr != NULL)
	{
		page_count = adq114_ptr->GetPageCount();
	}
	return page_count;
}

extern "C" __declspec(dllexport) int ADQ114_GetLvlTrigLevel(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int level = 0;
	if (adq114_ptr != NULL)
	{
		level = adq114_ptr->GetLvlTrigLevel();
	}
	return level;
}

extern "C" __declspec(dllexport) int ADQ114_GetLvlTrigFlank(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int edge = 0;
	if (adq114_ptr != NULL)
	{
		edge = adq114_ptr->GetLvlTrigEdge();
	}
	return edge;
}


extern "C" __declspec(dllexport) int ADQ114_GetPllFreqDivider(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int divider = 0;
	if (adq114_ptr != NULL)
	{
		divider = adq114_ptr->GetPllFreqDivider();
	}
	return divider;
}

extern "C" __declspec(dllexport) int ADQ114_GetClockSource(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int clk_source = 0;
	if (adq114_ptr != NULL)
	{
		clk_source = adq114_ptr->GetClockSource();
	}
	return clk_source;
}

extern "C" __declspec(dllexport) int ADQ114_GetTriggerMode(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trig_mode = 0;
	if (adq114_ptr != NULL)
	{
		trig_mode = adq114_ptr->GetTriggerMode();
	}
	return trig_mode;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetBufferSizePages(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int buffersize_pages = 0;
	if (adq114_ptr != NULL)
	{
		buffersize_pages = adq114_ptr->GetBufferSizePages();
	}
	return buffersize_pages;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetBufferSize(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int buffersize = 0;
	if (adq114_ptr != NULL)
	{
		buffersize = adq114_ptr->GetBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetMaxBufferSize(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int buffersize = 0;
	if (adq114_ptr != NULL)
	{
		buffersize = adq114_ptr->GetMaxBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetMaxBufferSizePages(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int buffersize = 0;
	if (adq114_ptr != NULL)
	{
		buffersize = adq114_ptr->GetMaxBufferSizePages();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetSamplesPerPage(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int buffersize = 0;
	if (adq114_ptr != NULL)
	{
		buffersize = adq114_ptr->GetSamplesPerPage();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetUSBAddress(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int usb_address = 0;
	if (adq114_ptr != NULL)
	{
		usb_address = adq114_ptr->GetUSBAddress();
	}
	return usb_address;
}


extern "C" __declspec(dllexport) unsigned int ADQ114_GetPCIeAddress(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int pcie_address = 0;
	if (adq114_ptr != NULL)
	{
		pcie_address = adq114_ptr->GetPCIeAddress();
	}
	return pcie_address;
}



extern "C" __declspec(dllexport) unsigned int ADQ114_GetBcdDevice(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int BcdDevice = 0;
	if (adq114_ptr != NULL)
	{
		BcdDevice = adq114_ptr->GetBcdDevice();
	}
	return BcdDevice;
}

extern "C" __declspec(dllexport) int* ADQ114_GetRevision(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int* revision = 0;
	if (adq114_ptr != NULL)
	{
		revision = adq114_ptr->GetRevision();
	}
	return revision;
}

extern "C" __declspec(dllexport) int ADQ114_GetTriggerInformation(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trig_info = 0;
	if (adq114_ptr != NULL)
	{
		trig_info = adq114_ptr->GetTriggerInformation();
	}
	return trig_info;
}

extern "C" __declspec(dllexport) int ADQ114_GetTrigPoint(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trig_point = 0;
	if (adq114_ptr != NULL)
	{
		trig_point = adq114_ptr->GetTrigPoint();
	}
	return trig_point;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetTrigType(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int trig_type = 0;
	if (adq114_ptr != NULL)
	{
		trig_type = adq114_ptr->GetTrigType();
	}
	return trig_type;
}

extern "C" __declspec(dllexport) int ADQ114_GetOverflow(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int overflow = 0;
	if (adq114_ptr != NULL)
	{
		overflow = adq114_ptr->GetOverflow();
	}
	return overflow;
}


extern "C" __declspec(dllexport) int ADQ114_GetRecordSize(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int output = 0;
	if (adq114_ptr != NULL)
	{
		output = adq114_ptr->GetRecordSize();
	}
	return output;
}


extern "C" __declspec(dllexport) int ADQ114_GetNofRecords(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int output = 0;
	if (adq114_ptr != NULL)
	{
		output = adq114_ptr->GetNofRecords();
	}
	return output;
}

extern "C" __declspec(dllexport) int ADQ114_SetTrigTimeMode(void* adq_cu_ptr, int adq114_num, int TrigTimeMode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetTrigTimeMode(TrigTimeMode);
	}
	return success;
}
extern "C" __declspec(dllexport) int ADQ114_ResetTrigTimer(void* adq_cu_ptr, int adq114_num, int TrigTimeRestart)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->ResetTrigTimer(TrigTimeRestart);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned long long ADQ114_GetTrigTime(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned long long TrigTime = 0;
	if (adq114_ptr != NULL)
	{
		TrigTime = adq114_ptr->GetTrigTime();
	}
	return TrigTime;
}
extern "C" __declspec(dllexport) unsigned long long ADQ114_GetTrigTimeCycles(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned long long TrigTimeCycles = 0;
	if (adq114_ptr != NULL)
	{
		TrigTimeCycles = adq114_ptr->GetTrigTimeCycles();
	}
	return TrigTimeCycles;
}
extern "C" __declspec(dllexport) unsigned int ADQ114_GetTrigTimeSyncs(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int TrigTimeSyncs = 0;
	if (adq114_ptr != NULL)
	{
		TrigTimeSyncs = adq114_ptr->GetTrigTimeSyncs();
	}
	return TrigTimeSyncs;
}
extern "C" __declspec(dllexport) unsigned int ADQ114_GetTrigTimeStart(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int TrigTimeStart = 0;
	if (adq114_ptr != NULL)
	{
		TrigTimeStart = adq114_ptr->GetTrigTimeStart();
	}
	return TrigTimeStart;
}
extern "C" __declspec(dllexport) unsigned int* ADQ114_GetMultiRecordHeader(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int* p_MultiRecordHeader = NULL;
	if (adq114_ptr != NULL)
	{
		p_MultiRecordHeader = adq114_ptr->GetMultiRecordHeader();
	}
	return p_MultiRecordHeader;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_ResetDevice(void* adq_cu_ptr, int adq114_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
        success = adq114_ptr->ResetDevice(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ114_IsUSBDevice(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int usb_device = 0;
	if (adq114_ptr != NULL)
	{
		usb_device = adq114_ptr->IsUSBDevice();
	}
	return usb_device;
}


extern "C" __declspec(dllexport) int ADQ114_IsPCIeDevice(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int PCIe_device = 0;
	if (adq114_ptr != NULL)
	{
		PCIe_device = adq114_ptr->IsPCIeDevice();
	}
	return PCIe_device;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_IsAlive(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int result = 0;
	if (adq114_ptr != NULL)
	{
		result = adq114_ptr->IsAlive();
	}
	return result;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SendProcessorCommand(void* adq_cu_ptr, int adq114_num, int command, int argument)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->SendProcessorCommand(command, argument);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SendLongProcessorCommand(void* adq_cu_ptr, int adq114_num, int command, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->SendProcessorCommand(command, addr, mask, data);
	}
	return message;
}

//extern "C" __declspec(dllexport) unsigned int ADQ114_WriteRegister(void* adq_cu_ptr, int adq114_num, int FPGA_num, unsigned int addr, unsigned int mask, unsigned int data)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
//	unsigned int message = 0;
//	if (adq114_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq114_ptr->WriteAlgoRegister(addr, mask, data);
//		else if (FPGA_num == 2)
//			message = adq114_ptr->WriteRegister(addr, mask, data);
//	}
//	return message;
//}

//extern "C" __declspec(dllexport) unsigned int ADQ114_ReadRegister(void* adq_cu_ptr, int adq114_num, int FPGA_num, unsigned int addr)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
//	unsigned int message = 0;
//	if (adq114_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq114_ptr->ReadAlgoRegister(addr);
//		else if (FPGA_num == 2)
//			message = adq114_ptr->ReadRegister(addr);
//	}
//	return message;
//}

extern "C" __declspec(dllexport) unsigned int ADQ114_WriteRegister(void* adq_cu_ptr, int adq114_num, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->WriteRegister(addr, mask, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_ReadRegister(void* adq_cu_ptr, int adq114_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->ReadRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_WriteAlgoRegister(void* adq_cu_ptr, int adq114_num, int addr, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->WriteAlgoRegister(addr, 0, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_ReadAlgoRegister(void* adq_cu_ptr, int adq114_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->ReadAlgoRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetErrorVector(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->GetErrorVector();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetStreamStatus(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->GetStreamStatus();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SetStreamStatus(void* adq_cu_ptr, int adq114_num, unsigned int status)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->SetStreamStatus(status);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetStreamOverflow(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->GetStreamOverflow();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_ReadEEPROM(void* adq_cu_ptr, int adq114_num, unsigned int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->ReadEEPROM(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_WriteEEPROM(void* adq_cu_ptr, int adq114_num, unsigned int addr, unsigned int data, unsigned int accesscode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = NULL;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->WriteEEPROM(addr, data, accesscode);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetTemperature(void* adq_cu_ptr, int adq114_num, unsigned int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	unsigned int message = 0;
	if (adq114_ptr != NULL)
	{
		message = adq114_ptr->GetTemperature(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) char* ADQ114_GetBoardSerialNumber(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->GetBoardSerialNumber();
	}
	else
	{
		return NULL;
	}
}

extern "C" __declspec(dllexport) int ADQ114_SetDataFormat(void* adq_cu_ptr, int adq114_num, unsigned int format)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int success = 0;
	if (adq114_ptr != NULL)
	{
		success = adq114_ptr->SetDataFormat(format);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ114_GetDataFormat(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	int format = 0;
	if (adq114_ptr != NULL)
	{
		format = adq114_ptr->GetDataFormat();
	}
	return format;
}

extern "C" __declspec(dllexport) unsigned int   ADQ114_MultiRecordSetup(void* adq_cu_ptr, int adq114_num,int NumberOfRecords, int SamplesPerRecord)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->MultiRecordSetup(NumberOfRecords, SamplesPerRecord);
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ114_MultiRecordGetRecord(void* adq_cu_ptr, int adq114_num,int RecordNumber)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
//	if (adq114_ptr != NULL)
//	{
//		return adq114_ptr->MultiRecordGetRecord(RecordNumber);
//	}
//	else
//	{
//		return NULL;
//	}
//}

extern "C" __declspec(dllexport) unsigned int   ADQ114_MultiRecordClose(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->MultiRecordClose();
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ114_MultiRecordGetTrigged(void* adq_cu_ptr, int adq114_num)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
//	if (adq114_ptr != NULL)
//	{
//		return adq114_ptr->MultiRecordGetTrigged();
//	}
//	else
//	{
//		return NULL;
//	}
//}

extern "C" __declspec(dllexport) unsigned int ADQ114_MemoryDump(void* adq_cu_ptr, int adq114_num, 
																unsigned int StartAddress, unsigned int EndAddress, 
																unsigned char* buffer, unsigned int *bufctr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->MemoryDump(StartAddress, EndAddress, buffer, bufctr);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_WriteTrig(void* adq_cu_ptr, int adq114_num, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->WriteTrig(data);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_SetTestPatternMode(void* adq_cu_ptr, int adq114_num, int mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
        return adq114_ptr->SetTestPatternMode(mode);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_SetTestPatternConstant(void* adq_cu_ptr, int adq114_num, int value)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
        return adq114_ptr->SetTestPatternConstant(value);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_SetDirectionTrig(void* adq_cu_ptr, int adq114_num, int direction)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->SetDirectionTrig(direction);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ114_ReadGPIO(void* adq_cu_ptr, int adq114_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->ReadGPIO();
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_WriteGPIO(void* adq_cu_ptr, int adq114_num, 
													  unsigned int data, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->WriteGPIO(data, mask);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ114_SetDirectionGPIO(void* adq_cu_ptr, int adq114_num, 
															 unsigned int direction, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->SetDirectionGPIO(direction, mask);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SetAlgoStatus(void* adq_cu_ptr, int adq114_num, int status)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->SetAlgoStatus(status);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ114_SetAlgoNyquistBand(void* adq_cu_ptr, int adq114_num, unsigned int band)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ114* adq114_ptr = adq_cu->GetADQ114(adq114_num);
	
	if (adq114_ptr != NULL)
	{
		return adq114_ptr->SetAlgoNyquistBand(band);
	}
	else
	{
		return 0;
	}
}


//------------------------- ADQ214 --------------------------------------------------------------------

extern "C" __declspec(dllexport) int ADQ214_SetTransferBuffers(void* adq_cu_ptr, int adq214_num, unsigned int nOfBuffers, unsigned int bufferSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetTransferBuffers(nOfBuffers, bufferSize);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetCacheSize(void* adq_cu_ptr, int adq214_num, unsigned int newCacheSize)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetCacheSize(newCacheSize);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SetSampleSkip(void* adq_cu_ptr, int adq214_num, unsigned int skipfactor)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
        success = adq214_ptr->SetSampleSkip(skipfactor);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SetSampleDecimation(void* adq_cu_ptr, int adq214_num, unsigned int decimationfactor)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
        success = adq214_ptr->SetSampleDecimation(decimationfactor);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SetLvlTrigResetLevel(void* adq_cu_ptr, int adq214_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
        success = adq214_ptr->SetTrigLevelResetValue(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetLvlTrigLevel(void* adq_cu_ptr, int adq214_num, int level)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetLvlTrigLevel(level);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetLvlTrigFlank(void* adq_cu_ptr, int adq214_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetLvlTrigEdge(void* adq_cu_ptr, int adq214_num, int edge)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetLvlTrigEdge(edge);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetLvlTrigChannel(void* adq_cu_ptr, int adq214_num,int channel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetLvlTrigChannel(channel);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetClockSource(void* adq_cu_ptr, int adq214_num, int source)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetClockSource(source);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetClockFrequencyMode(void* adq_cu_ptr, int adq214_num, int clockmode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetClockFrequencyMode(clockmode);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetPllFreqDivider(void* adq_cu_ptr, int adq214_num, int divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetPllFreqDivider(divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetPll(void* adq_cu_ptr, int adq214_num, int n_divider, int r_divider, int vco_divider, int channel_divider)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetPll(n_divider, r_divider, vco_divider, channel_divider);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetupMemory(void* adq_cu_ptr, int adq214_num, unsigned int RecordSize, unsigned int NofRecords)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->MultiRecordSetup(NofRecords, RecordSize);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetTriggerMode(void* adq_cu_ptr, int adq214_num, int trig_mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetTriggerMode(trig_mode);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetSampleWidth(void* adq_cu_ptr, int adq214_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetSampleWidth(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetNofBits(void* adq_cu_ptr, int adq214_num, unsigned int NofBits)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetNofBits(NofBits);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetPreTrigSamples(void* adq_cu_ptr, int adq214_num, unsigned int PreTrigSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetPreTrigSamples(PreTrigSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetTriggerHoldOffSamples(void* adq_cu_ptr, int adq214_num, unsigned int TriggerHoldOffSamples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetTriggerHoldOffSamples(TriggerHoldOffSamples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetBufferSizePages(void* adq_cu_ptr, int adq214_num, unsigned int pages)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetBufferSizePages(pages);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SetBufferSize(void* adq_cu_ptr, int adq214_num, unsigned int samples)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetBufferSize(samples);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_ArmTrigger(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->ArmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_DisarmTrigger(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->DisarmTrigger();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_USBTrig(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_SWTrig(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SWTrig();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_CollectDataNextPage(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->CollectDataNextPage();
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_CollectRecord(void* adq_cu_ptr, int adq214_num, unsigned int record_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->MultiRecordGetRecord((int)record_num);
	}
	return success;
}

extern "C" __declspec(dllexport) int* ADQ214_GetPtrDataChA(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int* data_ptr = 0;
	if (adq214_ptr != NULL)
	{
		data_ptr = (int*)adq214_ptr->GetPtrData(1);
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) int* ADQ214_GetPtrDataChB(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int* data_ptr = 0;
	if (adq214_ptr != NULL)
	{
		data_ptr = (int*)adq214_ptr->GetPtrData(2);
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) void* ADQ214_GetPtrStream(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	void* data_ptr = 0;
	if (adq214_ptr != NULL)
	{
		data_ptr = adq214_ptr->GetPtrStream();
	}
	return data_ptr;
}

extern "C" __declspec(dllexport) int ADQ214_GetWaitingForTrigger(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int wait4trig = 0;
	if (adq214_ptr != NULL)
	{
		wait4trig = adq214_ptr->GetWaitingForTrigger();
	}
	return wait4trig;
}

extern "C" __declspec(dllexport) int ADQ214_GetTrigged(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trigged = 0;
	if (adq214_ptr != NULL)
	{
		trigged = adq214_ptr->GetAcquired();
	}
	return trigged;
}

extern "C" __declspec(dllexport) int ADQ214_GetTriggedAll(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trigged = 0;
	if (adq214_ptr != NULL)
	{
		trigged = adq214_ptr->GetAcquiredAll();
	}
	return trigged;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetPageCount(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int page_count = 0;
	if (adq214_ptr != NULL)
	{
		page_count = adq214_ptr->GetPageCount();
	}
	return page_count;
}

extern "C" __declspec(dllexport) int ADQ214_GetLvlTrigLevel(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int level = 0;
	if (adq214_ptr != NULL)
	{
		level = adq214_ptr->GetLvlTrigLevel();
	}
	return level;
}

extern "C" __declspec(dllexport) int ADQ214_GetLvlTrigFlank(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int edge = 0;
	if (adq214_ptr != NULL)
	{
		edge = adq214_ptr->GetLvlTrigEdge();
	}
	return edge;
}

extern "C" __declspec(dllexport) int ADQ214_GetLvlTrigChannel(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int channel = 0;
	if (adq214_ptr != NULL)
	{
		channel = adq214_ptr->GetLvlTrigChannel();
	}
	return channel;
}

extern "C" __declspec(dllexport) int ADQ214_GetPllFreqDivider(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int divider = 0;
	if (adq214_ptr != NULL)
	{
		divider = adq214_ptr->GetPllFreqDivider();
	}
	return divider;
}

extern "C" __declspec(dllexport) int ADQ214_GetClockSource(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int clk_source = 0;
	if (adq214_ptr != NULL)
	{
		clk_source = adq214_ptr->GetClockSource();
	}
	return clk_source;
}

extern "C" __declspec(dllexport) int ADQ214_GetTriggerMode(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trig_mode = 0;
	if (adq214_ptr != NULL)
	{
		trig_mode = adq214_ptr->GetTriggerMode();
	}
	return trig_mode;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetBufferSizePages(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int buffersize_pages = 0;
	if (adq214_ptr != NULL)
	{
		buffersize_pages = adq214_ptr->GetBufferSizePages();
	}
	return buffersize_pages;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetBufferSize(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int buffersize = 0;
	if (adq214_ptr != NULL)
	{
		buffersize = adq214_ptr->GetBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetMaxBufferSize(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int buffersize = 0;
	if (adq214_ptr != NULL)
	{
		buffersize = adq214_ptr->GetMaxBufferSize();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetMaxBufferSizePages(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int buffersize = 0;
	if (adq214_ptr != NULL)
	{
		buffersize = adq214_ptr->GetMaxBufferSizePages();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetSamplesPerPage(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int buffersize = 0;
	if (adq214_ptr != NULL)
	{
		buffersize = adq214_ptr->GetSamplesPerPage();
	}
	return buffersize;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetUSBAddress(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int usb_address = 0;
	if (adq214_ptr != NULL)
	{
		usb_address = adq214_ptr->GetUSBAddress();
	}
	return usb_address;
}


extern "C" __declspec(dllexport) unsigned int ADQ214_GetPCIeAddress(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int pcie_address = 0;
	if (adq214_ptr != NULL)
	{
		pcie_address = adq214_ptr->GetPCIeAddress();
	}
	return pcie_address;
}



extern "C" __declspec(dllexport) unsigned int ADQ214_GetBcdDevice(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int BcdDevice = 0;
	if (adq214_ptr != NULL)
	{
		BcdDevice = adq214_ptr->GetBcdDevice();
	}
	return BcdDevice;
}

extern "C" __declspec(dllexport) int* ADQ214_GetRevision(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int* revision = 0;
	if (adq214_ptr != NULL)
	{
		revision = adq214_ptr->GetRevision();
	}
	return revision;
}

extern "C" __declspec(dllexport) int ADQ214_GetTriggerInformation(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trig_info = 0;
	if (adq214_ptr != NULL)
	{
		trig_info = adq214_ptr->GetTriggerInformation();
	}
	return trig_info;
}

extern "C" __declspec(dllexport) int ADQ214_GetTrigPoint(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trig_point = 0;
	if (adq214_ptr != NULL)
	{
		trig_point = adq214_ptr->GetTrigPoint();
	}
	return trig_point;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetTrigType(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trig_type = 0;
	if (adq214_ptr != NULL)
	{
		trig_type = adq214_ptr->GetTrigType();
	}
	return trig_type;
}

extern "C" __declspec(dllexport) int ADQ214_GetTriggedCh(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int trigged_ch = 0;
	if (adq214_ptr != NULL)
	{
		trigged_ch = adq214_ptr->GetTriggedCh();
	}
	return trigged_ch;
}


extern "C" __declspec(dllexport) int ADQ214_GetOverflow(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int overflow = 0;
	if (adq214_ptr != NULL)
	{
		overflow = adq214_ptr->GetOverflow();
	}
	return overflow;
}

extern "C" __declspec(dllexport) int ADQ214_GetRecordSize(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int output = 0;
	if (adq214_ptr != NULL)
	{
		output = adq214_ptr->GetRecordSize();
	}
	return output;
}


extern "C" __declspec(dllexport) int ADQ214_GetNofRecords(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int output = 0;
	if (adq214_ptr != NULL)
	{
		output = adq214_ptr->GetNofRecords();
	}
	return output;
}

extern "C" __declspec(dllexport) int ADQ214_SetTrigTimeMode(void* adq_cu_ptr, int adq214_num, int TrigTimeMode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetTrigTimeMode(TrigTimeMode);
	}
	return success;
}
extern "C" __declspec(dllexport) int ADQ214_ResetTrigTimer(void* adq_cu_ptr, int adq214_num, int TrigTimeRestart)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->ResetTrigTimer(TrigTimeRestart);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned long long ADQ214_GetTrigTime(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned long long TrigTime = 0;
	if (adq214_ptr != NULL)
	{
		TrigTime = adq214_ptr->GetTrigTime();
	}
	return TrigTime;
}
extern "C" __declspec(dllexport) unsigned long long ADQ214_GetTrigTimeCycles(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned long long TrigTimeCycles = 0;
	if (adq214_ptr != NULL)
	{
		TrigTimeCycles = adq214_ptr->GetTrigTimeCycles();
	}
	return TrigTimeCycles;
}
extern "C" __declspec(dllexport) unsigned int ADQ214_GetTrigTimeSyncs(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int TrigTimeSyncs = 0;
	if (adq214_ptr != NULL)
	{
		TrigTimeSyncs = adq214_ptr->GetTrigTimeSyncs();
	}
	return TrigTimeSyncs;
}
extern "C" __declspec(dllexport) unsigned int ADQ214_GetTrigTimeStart(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int TrigTimeStart = 0;
	if (adq214_ptr != NULL)
	{
		TrigTimeStart = adq214_ptr->GetTrigTimeStart();
	}
	return TrigTimeStart;
}
extern "C" __declspec(dllexport) unsigned int* ADQ214_GetMultiRecordHeader(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int* p_MultiRecordHeader = NULL;
	if (adq214_ptr != NULL)
	{
		p_MultiRecordHeader = adq214_ptr->GetMultiRecordHeader();
	}
	return p_MultiRecordHeader;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_ResetDevice(void* adq_cu_ptr, int adq214_num, int resetlevel)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
        success = adq214_ptr->ResetDevice(resetlevel);
	}
	return success;
}

extern "C" __declspec(dllexport) int ADQ214_IsUSBDevice(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int usb_device = 0;
	if (adq214_ptr != NULL)
	{
		usb_device = adq214_ptr->IsUSBDevice();
	}
	return usb_device;
}


extern "C" __declspec(dllexport) int ADQ214_IsPCIeDevice(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int PCIe_device = 0;
	if (adq214_ptr != NULL)
	{
		PCIe_device = adq214_ptr->IsPCIeDevice();
	}
	return PCIe_device;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_IsAlive(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int result = 0;
	if (adq214_ptr != NULL)
	{
		result = adq214_ptr->IsAlive();
	}
	return result;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SendProcessorCommand(void* adq_cu_ptr, int adq214_num, int command, int argument)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->SendProcessorCommand(command, argument);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SendLongProcessorCommand(void* adq_cu_ptr, int adq214_num, int command, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->SendProcessorCommand(command, addr, mask, data);
	}
	return message;
}


//extern "C" __declspec(dllexport) unsigned int ADQ214_WriteRegister(void* adq_cu_ptr, int adq214_num, int FPGA_num, unsigned int addr, unsigned int mask, unsigned int data)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
//	unsigned int message = 0;
//	if (adq214_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq214_ptr->WriteAlgoRegister(addr, mask, data);
//		else if (FPGA_num == 2)
//			message = adq214_ptr->WriteRegister(addr, mask, data);
//	}
//	return message;
//}
//
//extern "C" __declspec(dllexport) unsigned int ADQ214_ReadRegister(void* adq_cu_ptr, int adq214_num, int FPGA_num, unsigned int addr)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
//	unsigned int message = 0;
//	if (adq214_ptr != NULL)
//	{
//		if (FPGA_num == 1)
//			message = adq214_ptr->ReadAlgoRegister(addr);
//		else if (FPGA_num == 2)
//			message = adq214_ptr->ReadRegister(addr);
//	}
//	return message;
//}

extern "C" __declspec(dllexport) unsigned int ADQ214_WriteRegister(void* adq_cu_ptr, int adq214_num, int addr, int mask, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->WriteRegister(addr, mask, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_ReadRegister(void* adq_cu_ptr, int adq214_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->ReadRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_WriteAlgoRegister(void* adq_cu_ptr, int adq214_num, int addr, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->WriteAlgoRegister(addr, 0, data);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_ReadAlgoRegister(void* adq_cu_ptr, int adq214_num, int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->ReadAlgoRegister(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetErrorVector(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->GetErrorVector();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetStreamStatus(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->GetStreamStatus();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_SetStreamStatus(void* adq_cu_ptr, int adq214_num, unsigned int status)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->SetStreamStatus(status);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetStreamOverflow(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->GetStreamOverflow();
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_ReadEEPROM(void* adq_cu_ptr, int adq214_num, unsigned int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->ReadEEPROM(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_WriteEEPROM(void* adq_cu_ptr, int adq214_num, unsigned int addr, unsigned int data, unsigned int accesscode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = NULL;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->WriteEEPROM(addr, data, accesscode);
	}
	return message;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetTemperature(void* adq_cu_ptr, int adq214_num, unsigned int addr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	unsigned int message = 0;
	if (adq214_ptr != NULL)
	{
		message = adq214_ptr->GetTemperature(addr);
	}
	return message;
}

extern "C" __declspec(dllexport) char* ADQ214_GetBoardSerialNumber(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->GetBoardSerialNumber();
	}
	else
	{
		return NULL;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetDataFormat(void* adq_cu_ptr, int adq214_num, unsigned int format)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int success = 0;
	if (adq214_ptr != NULL)
	{
		success = adq214_ptr->SetDataFormat(format);
	}
	return success;
}

extern "C" __declspec(dllexport) unsigned int ADQ214_GetDataFormat(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	int format = 0;
	if (adq214_ptr != NULL)
	{
		format = adq214_ptr->GetDataFormat();
	}
	return format;
}

extern "C" __declspec(dllexport) unsigned int   ADQ214_MultiRecordSetup(void* adq_cu_ptr, int adq214_num,int NumberOfRecords, int SamplesPerRecord)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->MultiRecordSetup(NumberOfRecords, SamplesPerRecord);
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ214_MultiRecordGetRecord(void* adq_cu_ptr, int adq214_num,int RecordNumber)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
//	if (adq214_ptr != NULL)
//	{
//		return adq214_ptr->MultiRecordGetRecord(RecordNumber);
//	}
//	else
//	{
//		return NULL;
//	}
//}

extern "C" __declspec(dllexport) unsigned int   ADQ214_MultiRecordClose(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->MultiRecordClose();
	}
	else
	{
		return NULL;
	}
}

//extern "C" __declspec(dllexport) unsigned int   ADQ214_MultiRecordGetTrigged(void* adq_cu_ptr, int adq214_num)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
//	if (adq214_ptr != NULL)
//	{
//		return adq214_ptr->MultiRecordGetTrigged();
//	}
//	else
//	{
//		return NULL;
//	}
//}

extern "C" __declspec(dllexport) unsigned int ADQ214_MemoryDump(void* adq_cu_ptr, int adq214_num, 
																unsigned int StartAddress, unsigned int EndAddress, 
																unsigned char* buffer, unsigned int *bufctr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->MemoryDump(StartAddress, EndAddress, buffer, bufctr);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_WriteTrig(void* adq_cu_ptr, int adq214_num, int data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->WriteTrig(data);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetTestPatternMode(void* adq_cu_ptr, int adq214_num, int mode)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
        return adq214_ptr->SetTestPatternMode(mode);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetTestPatternConstant(void* adq_cu_ptr, int adq214_num, int value)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
        return adq214_ptr->SetTestPatternConstant(value);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetDirectionTrig(void* adq_cu_ptr, int adq214_num, int direction)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->SetDirectionTrig(direction);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) unsigned int ADQ214_ReadGPIO(void* adq_cu_ptr, int adq214_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->ReadGPIO();
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_WriteGPIO(void* adq_cu_ptr, int adq214_num, 
													  unsigned int data, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->WriteGPIO(data, mask);
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetDirectionGPIO(void* adq_cu_ptr, int adq214_num, 
															 unsigned int direction, unsigned int mask)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->SetDirectionGPIO(direction,  mask);	
	}
	else
	{
		return 0;
	}
}

extern "C" __declspec(dllexport) int ADQ214_SetAfeSwitch(void* adq_cu_ptr, int adq214_num, 
														 unsigned int afe)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ214* adq214_ptr = adq_cu->GetADQ214(adq214_num);
	
	if (adq214_ptr != NULL)
	{
		return adq214_ptr->SetAfeSwitch(afe);	
	}
	else
	{
		return 0;
	}
}