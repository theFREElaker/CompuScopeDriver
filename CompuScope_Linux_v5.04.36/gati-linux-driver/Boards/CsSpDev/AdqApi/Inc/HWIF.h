// Project: SphinxAPIEXPRESS
// File: HWIF.h


#ifndef HWIF_H
#define HWIF_H

class HWIF
    {
    public:
		virtual   		~HWIF();
		virtual int     SetTransferBuffers(unsigned int nOfBuffers, unsigned int bufferSize);
		virtual unsigned int GetMaxTransferSize() = 0;
		virtual int		IsUSBDevice();
		virtual int		IsPCIeDevice();
		virtual int		SendCommand(unsigned char* message) = 0;
		virtual int		ReceiveStatusInit();
		virtual int		ReceiveStatusFinish(unsigned int* data) = 0;
		virtual int		ReceiveDataInit(int transfer_size_bytes, unsigned int maxPrefetch);
		virtual int		ReceiveDataFinish(int transfer_size_bytes, unsigned char **data_buffer_ptr) = 0;
		virtual int		ReceiveDataAbort() = 0;
		virtual int		FlushBuffers();
        virtual unsigned int ResetDevice(int);
		virtual unsigned int Address() = 0;
		virtual unsigned int HwVersion() = 0;
	};

#endif //HWIF_H
