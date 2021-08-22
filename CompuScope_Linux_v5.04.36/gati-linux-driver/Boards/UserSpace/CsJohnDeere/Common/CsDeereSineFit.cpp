#ifdef _WINDOWS_
#include "stdafx.h"
#else
#include <math.h>
#include <string.h>
#endif
#include "CsErrors.h"
#include "CsDeereSineFit.h"

int LinearEquationsSolving(int nDim, double* pfMatr, double* pfVect, double* pfSolution);

int32 SplitSineFit3(const double cdPPC, void* const pDataIn, int32 i32SqrAdj, long lSampleSize, const long lDataCount, const long clSplit, SINEFIT3* const pSol)
{
#ifdef _WINDOWS_
	if( ::IsBadReadPtr( pDataIn, lSampleSize*lDataCount ) )
		return CS_INVALID_PARAMETER;
	if( ::IsBadWritePtr( pSol, sizeof(SINEFIT3)*clSplit ) )
		return CS_BUFFER_TOO_SMALL;
#endif

	int iRet(CS_SUCCESS);
	const long clFitCount = lDataCount/clSplit;
	PARTIAL_SUMS* pSums = new PARTIAL_SUMS[clSplit];
	const double cdPhaseStep = (2.0 * M_PI) / cdPPC;
	const double cdPhaseOff = cdPhaseStep / clSplit;
#ifdef _WINDOWS_
	::ZeroMemory(pSums, clSplit*sizeof(PARTIAL_SUMS) );
#else
	memset(pSums, 0, clSplit*sizeof(PARTIAL_SUMS));
#endif
	long lIx(i32SqrAdj);

	int16* const p16DataIn = (int16*) pDataIn;
	int32* const p32DataIn = (int32*) pDataIn;

	for (long i = 0; i < clFitCount; i++)
	{
		for( long j = 0; j < clSplit; j++ )
		{
			double dPhase = cdPhaseOff*j + i*cdPhaseStep;
			// Calculate Cos and sin for further usage
			double dSin = sin(dPhase);
			double dCos = cos(dPhase);
			
			double dY;
			if ( sizeof(uInt32) == lSampleSize )
				dY = p32DataIn[lIx++];
			else
				dY = p16DataIn[lIx++];

			pSums[j].dSinSin += dSin*dSin;
			pSums[j].dCosSin += dSin*dCos;
			pSums[j].dCosCos += dCos*dCos;
			pSums[j].dSin    += dSin;
			pSums[j].dCos    += dCos;
			pSums[j].dSinY   += dSin*dY;
			pSums[j].dCosY   += dCos*dY;
			pSums[j].dY      += dY;
		}
	}

	double dVector[3] = {0.0};
	double dMatrix[3*3] = {0.0};
	double dSolution[3] = {0.0};

	for (long i = 0; i < clSplit; i++)
	{
		// Create vector of 3 elements
		dVector[0] = pSums[i].dSinY;
		dVector[1] = pSums[i].dCosY;
		dVector[2] = pSums[i].dY;

		// Create matrix 3x3 elements
		dMatrix[0] = pSums[i].dSinSin;   dMatrix[3] = pSums[i].dCosSin;  dMatrix[6] = pSums[i].dSin;
		dMatrix[1] = pSums[i].dCosSin;   dMatrix[4] = pSums[i].dCosCos;	 dMatrix[7] = pSums[i].dCos;
		dMatrix[2] = pSums[i].dSin;	     dMatrix[5] = pSums[i].dCos;	 dMatrix[8] = clFitCount;
		// Solve Linear Equations to get result
		pSol[i].dSnr = 0.;
		if (0 == LinearEquationsSolving(3, dMatrix, dVector, dSolution))
		{
			pSol[i].dOff = dSolution[2];
			pSol[i].dAmp = sqrt( dSolution[1]*dSolution[1] + dSolution[0]*dSolution[0] );

			if( 0.0 != dSolution[0] )
				pSol[i].dPhase = atan (dSolution[1] /dSolution[0]);
			else
				pSol[i].dPhase = M_PI_2;
			if (0 > dSolution[0] )
				pSol[i].dPhase += M_PI;
		}
		else
		{
			pSol[i].dOff =	pSol[i].dAmp =	pSol[i].dPhase = 0.0;
			iRet = CS_INVALID_REQUEST;
		}
	}

	lIx = i32SqrAdj;
	for (long i = 0; i < clFitCount; i++)
	{
		for( long j = 0; j < clSplit; j++ )
		{
			double dPhase = cdPhaseOff*j + i*cdPhaseStep +  pSol[j].dPhase;
			double dF = pSol[j].dAmp * sin( dPhase ) + pSol[j].dOff;
			double dY;
			if ( sizeof(uInt32) == lSampleSize )
				dY = p32DataIn[lIx++];
			else
				dY = p16DataIn[lIx++];
			dF -= dY;
			pSol[j].dSnr += dF * dF;
		}
	}
	for( long j = 0; j < clSplit; j++ )
		pSol[j].dSnr = 20.*log10(pSol[j].dAmp/sqrt(pSol[j].dSnr/clFitCount));
	
	delete[] pSums; 
	return iRet;
}

int LinearEquationsSolving(int nDim, double* pfMatr, double* pfVect, double* pfSolution)
{
	double fMaxElem;
	double fAcc;
	int i , j, k, m;
	for (k=0; k<(nDim-1); k++) // base row of matrix
	{
		// search of line with max element
		fMaxElem = fabs( pfMatr[k*nDim + k] );
		m = k;
		for (i=k+1; i<nDim; i++)
		{
			if (fMaxElem < fabs(pfMatr[i*nDim + k]))
			{
				fMaxElem = pfMatr[i*nDim + k];
				m = i;
			}
		}
    		// permutation of base line (index k) and max element line(index m)
		if (m != k)
		{
			for (i=k; i<nDim; i++)
			{
				fAcc               = pfMatr[k*nDim + i];
				pfMatr[k*nDim + i] = pfMatr[m*nDim + i];
				pfMatr[m*nDim + i] = fAcc;
			}
			fAcc = pfVect[k];
			pfVect[k] = pfVect[m];
			pfVect[m] = fAcc;
		}

		if (pfMatr[k*nDim + k] == 0.) 
			return -1; // needs improvement !!!

		// triangulation of matrix with coefficients
		for (j=(k+1); j<nDim; j++) // current row of matrix
		{
			fAcc = - pfMatr[j*nDim + k] / pfMatr[k*nDim + k];
			for (i=k; i<nDim; i++)
			{
				pfMatr[j*nDim + i] = pfMatr[j*nDim + i] + fAcc*pfMatr[k*nDim + i];
			}

			pfVect[j] = pfVect[j] + fAcc*pfVect[k]; // free member recalculation
		}
	}

	for (k=(nDim-1); k>=0; k--)
	{
		pfSolution[k] = pfVect[k];
		for (i=(k+1); i<nDim; i++)
		{
			pfSolution[k] -= (pfMatr[k*nDim + i]*pfSolution[i]);
		}

		pfSolution[k] = pfSolution[k] / pfMatr[k*nDim + k];
	}

	return 0;
}