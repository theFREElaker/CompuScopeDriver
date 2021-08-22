// Project: ADQ-API
// File: ADQ112.h

#ifndef ADQ112_H
#define ADQ112_H

#include "ADQ.h"
#include "ADQAPI_definitions.h"
#include "HWIF.h"

class ADQ112 : public ADQ
    {
    public:
		ADQ112();
		~ADQ112();
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
#endif //ADQ112_H