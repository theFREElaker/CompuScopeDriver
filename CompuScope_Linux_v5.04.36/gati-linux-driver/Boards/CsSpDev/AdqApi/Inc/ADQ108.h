// Project: ADQ-API
// File: ADQ108.h

#ifndef ADQ108_H
#define ADQ108_H

#include "ADQ.h"
#include "ADQAPI_definitions.h"
#include "HWIF.h"

class ADQ108 : public ADQ
    {
    public:
		ADQ108();
		~ADQ108();
		int Open(HWIF* device);
        int GetADQType(void);
		void* GetPtrData(unsigned int channel);
        int SetDataFormat(unsigned int format);
        int SetAlgoNyquistBand(unsigned int band);
        int SetAlgoStatus(int status);

    private:
		int ParseSampleData(unsigned int wordOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxsamples);

        char* m_data_buffer;
        int m_trig_channel;
};
#endif //ADQ108_H