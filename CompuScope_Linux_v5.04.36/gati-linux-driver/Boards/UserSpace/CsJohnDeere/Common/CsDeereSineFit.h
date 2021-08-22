#ifndef __CS_DEERE_SINE_FIT_H__
#define __CS_DEERE_SINE_FIT_H__
#include "CsTypes.h"


#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#ifndef M_PI_4
#define M_PI_4 0.785398163397448309616
#endif

typedef struct _PARTIAL_SUMS
{
	double dSinSin;
	double dCosSin;
	double dCosCos;
	double dSin;
	double dCos;
	double dSinY;
	double dCosY;
	double dY;
	double dX;
}PARTIAL_SUMS;

typedef struct _SINEFIT_3_SOLUTION
{
	double	dOff;
	double	dAmp;
	double	dPhase;
	double  dSnr;
} SINEFIT3;

// Functions
int32 SplitSineFit3(const double cdPPC, void* const pDataIn, int32 i32SqrAdj, const long lSampleSize, const long lDataCount, const long clSplit, SINEFIT3* const pSolution);

#endif