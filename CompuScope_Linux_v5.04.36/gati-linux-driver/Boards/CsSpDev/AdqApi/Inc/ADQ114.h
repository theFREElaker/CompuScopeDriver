// Project: ADQ-API
// File: ADQ114.h


#ifndef ADQ114_H
#define ADQ114_H

#include "ADQ.h"
#include "ADQAPI_definitions.h"
#include "HWIF.h"

class ADQ114 : public ADQ
    {
    public:
		ADQ114();
		~ADQ114();
		int Open(HWIF* device);
        int GetADQType(void);
		void* GetPtrData(unsigned int channel);
        int SetDataFormat(unsigned int format);
        int SetAlgoNyquistBand(unsigned int band);
        int SetAlgoStatus(int status);
        unsigned int SetSampleSkip(unsigned int SkipFactor);
        unsigned int GetSampleSkip(void);

    private:
		int ParseSampleData(unsigned int wordOffset, unsigned int sampleOffset, unsigned int destOffset, unsigned int maxsamples);

        int* m_data_buffer;
        int m_trig_channel;
};
#endif //ADQ114_H