// Project: ADQ-API_Lite
// File: ADQAPI_Lite.cpp

#include "AdqLiteControlUnit.h"
#include "Adqlite.h"
#include "AdqApiLite_definitions.h"


extern "C"  void* CreateADQControlUnit()
{
	ADQControlUnit* ADQCU_ptr = new ADQControlUnit();
	return (void*)ADQCU_ptr;
}

extern "C"  void DeleteADQControlUnit(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	delete adq_cu;
}

extern "C"  int ADQControlUnit_FindDevices(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_devices = 0;
	n_of_devices = adq_cu->FindDevices();
	return n_of_devices;
}

extern "C"  int ADQControlUnit_NofADQ(void* adq_cu_ptr)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	int n_of_adq = 0;
	n_of_adq = adq_cu->NofADQ();
	return n_of_adq;
}


extern "C"  void ADQControlUnit_DeleteADQ(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	adq_cu->DeleteADQ(adq_num);
	return;
}

extern "C"  unsigned int ADQ_GetUSBAddress(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	unsigned int usb_address;
	usb_address = adq_ptr->GetUSBAddress();
	return usb_address;
}

extern "C"  int* ADQ_GetRevision(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int* revision;
	revision = adq_ptr->GetRevision();
	return revision;
}

extern "C"  int ADQ_EnableSpi(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->EnableSpi();
	return success;
}
extern "C"  int ADQ_DisableSpi(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->DisableSpi();
	return success;
}
extern "C"  int ADQ_FlashRead(void* adq_cu_ptr, int adq_num,  unsigned int pagenum, unsigned int buf_address, long len, char *buf)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashRead(pagenum, buf_address, len, buf);
	return success;
}
extern "C"  int ADQ_FlashBufRead(void* adq_cu_ptr, int adq_num, char buf_num, unsigned int buf_address, long len, char *buf)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashBufRead(buf_num, buf_address, len, buf);
	return success;
}
//extern "C"  int ADQ_FlashUploadBitfile(void* adq_cu_ptr, int adq_num, char *filename)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
//	int success;
//	success = adq_ptr->FlashUploadBitfile(filename);
//	return success;
//}
//
//extern "C"  int ADQ_FlashDownloadBitfile(void* adq_cu_ptr, int adq_num, unsigned int len, char *filename)
//{
//	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
//	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
//	int success;
//	success = adq_ptr->FlashDownloadBitfile(len, filename);
//	return success;
//}

extern "C"  int ADQ_FlashUploadStart(void* adq_cu_ptr, int adq_num, int fpga_num, int start_page)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashUploadStart(fpga_num, start_page);
	return success;
}

extern "C"  int ADQ_FlashUploadPart(void* adq_cu_ptr, int adq_num, long len, char * data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashUploadPart(len, data);
	return success;
}

extern "C"  int ADQ_FlashUploadEnd(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashUploadEnd();
	return success;
}


extern "C"  int ADQ_FlashDownloadStart(void* adq_cu_ptr, int adq_num, int fpga_num, int start_page)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashDownloadStart(fpga_num, start_page);
	return success;
}

extern "C"  int ADQ_FlashDownloadPart(void* adq_cu_ptr, int adq_num, long len, char * data)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashDownloadPart(len, data);
	return success;
}

extern "C"  int ADQ_FlashDownloadEnd(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashDownloadEnd();
	return success;
}

extern "C"  int ADQ_FlashGetStatus(void* adq_cu_ptr, int adq_num, char *buf)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashGetStatus(buf);
	return success;
}

extern "C"  int ADQ_FlashBlockErase(void* adq_cu_ptr, int adq_num, unsigned char fpga_number, int block)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashBlockErase(fpga_number, block);
	return success;
}

extern "C"  int ADQ_FlashGetModelInfo(void* adq_cu_ptr, int adq_num, int fpga_number, char *buf)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashGetModelInfo(fpga_number, buf);
	return success;
}

extern "C"  int ADQ_FlashGetUniqueId(void* adq_cu_ptr, int adq_num, int fpga_number, char *buf)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->FlashGetUniqueId(fpga_number, buf);
	return success;
}

extern "C"  int ADQ_GetVID(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->GetVID();
	return success;
}

extern "C"  int ADQ_GetPID(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->GetPID();
	return success;
}

extern "C"  int ADQ_GetBcdUSB(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->GetBcdUSB();
	return success;
}

extern "C"  int ADQ_GetBcdDevice(void* adq_cu_ptr, int adq_num)
{
	ADQControlUnit* adq_cu = (ADQControlUnit*)adq_cu_ptr;
	ADQ* adq_ptr = adq_cu->GetADQ(adq_num);
	int success;
	success = adq_ptr->GetBcdDevice();
	return success;
}