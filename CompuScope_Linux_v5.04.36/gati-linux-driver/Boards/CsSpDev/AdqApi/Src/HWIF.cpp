// Project: SphinxAPIEXPRESS
// File: PCIe.cpp


#include "HWIF.h"
#include "ADQAPI_definitions.h"

HWIF::~HWIF(){}

int	HWIF::IsUSBDevice()
{
	return 0;
}

int	HWIF::IsPCIeDevice()
{
	return 0;
}

int HWIF::ReceiveStatusInit()
{
	return 1;
}

int HWIF::ReceiveDataInit(int , unsigned int )
{
	return 1;
}

int HWIF::FlushBuffers()
{
	return 1;
}

unsigned int HWIF::ResetDevice(int)
{
	return 0;
}

int HWIF::SetTransferBuffers(unsigned int , unsigned int )
{
	return 0;
}