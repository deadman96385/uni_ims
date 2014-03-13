#include "MONKEY_NNFilter.h"

#define ABS(a) (a)>0 ? (a):(-a)

static __inline void MONKEY_CNNFilter_AdaptNoMMX(short * pM, short * pAdapt, int nDirection, int nOrder)
{
#ifdef ARM9E
    MONKEY_CNNFilter_Adapt_ASM(pM, pAdapt, nDirection, nOrder);
#else
    nOrder >>= 4;

    if (nDirection < 0) 
    {    
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
#endif
}

static __inline int MONKEY_CNNFilter_CalculateDotProductNoMMX(short * pA, short * pB, int nOrder)
{
    int nDotProduct = 0;
#ifdef ARM9E
    nDotProduct = MONKEY_CNNFilter_CalcDotProduct_ASM(pB, pA, nOrder);
#else
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }
#endif
    
    return nDotProduct;
}

static __inline void MONKEY_CRollBuffer_IncrementSafe(CRollBuffer * CCRollBuffer)
{
    CCRollBuffer->m_pCurrent++;
    if (CCRollBuffer->m_pCurrent == &CCRollBuffer->m_pData[CCRollBuffer->m_nWindowElements + CCRollBuffer->m_nHistoryElements])
        MONKEY_CRollBuffer_Roll(CCRollBuffer);
}

static __inline void MONKEY_CRollBuffer_IncrementFast(CRollBuffer * CCRollBuffer)
{
    CCRollBuffer->m_pCurrent++;
}

int MONKEY_CNNFilter(CNNFilter * CCNNFilter, int nOrder, int nShift, int nVersion)
{
    if ((nOrder <= 0) || ((nOrder % 16) != 0)) 
		return ERROR_UNDEFINED;

	memset(CCNNFilter, 0, sizeof(CNNFilter));

    CCNNFilter->m_nOrder = nOrder;
    CCNNFilter->m_nShift = nShift;
    CCNNFilter->m_nVersion = nVersion;
    
    CCNNFilter->m_bMMXAvailable = 0; //GetMMXAvailable();
    
    if (MONKEY_CRollBuffer_Create(&CCNNFilter->m_rbInput, NN_WINDOW_ELEMENTS, CCNNFilter->m_nOrder))
		return ERROR_INSUFFICIENT_MEMORY;
    if (MONKEY_CRollBuffer_Create(&CCNNFilter->m_rbDeltaM, NN_WINDOW_ELEMENTS, CCNNFilter->m_nOrder))
		return ERROR_INSUFFICIENT_MEMORY;

	// assume CCNNFilter->m_nOrder is even
    CCNNFilter->m_paryM0 = (int *) malloc (CCNNFilter->m_nOrder * sizeof(int) / 2);
	CCNNFilter->m_paryM = (short *) CCNNFilter->m_paryM0;

	if (CCNNFilter->m_paryM == 0)
		return ERROR_INSUFFICIENT_MEMORY;

	return ERROR_SUCCESS;
}

int MONKEY_CNNFilter_Finish(CNNFilter * CCNNFilter)
{
	if (!CCNNFilter)
		return ERROR_UNDEFINED;

	MONKEY_CRollBuffer_Finish(&CCNNFilter->m_rbInput);
	MONKEY_CRollBuffer_Finish(&CCNNFilter->m_rbDeltaM);
    SAFE_ARRAY_DELETE(CCNNFilter->m_paryM0)
	return ERROR_SUCCESS;
}

void MONKEY_CNNFilter_Flush(CNNFilter * CCNNFilter)
{
    memset(&CCNNFilter->m_paryM[0], 0, CCNNFilter->m_nOrder * sizeof(short));
    MONKEY_CRollBuffer_Flush(&CCNNFilter->m_rbInput);
    MONKEY_CRollBuffer_Flush(&CCNNFilter->m_rbDeltaM);
    CCNNFilter->m_nRunningAverage = 0;
}

int MONKEY_CNNFilter_Compress(CNNFilter * CCNNFilter, int nInput)
{
	int nDotProduct;
	int nTempABS;
	int nOutput;

    // convert the input to a short and store it
    CCNNFilter->m_rbInput.m_pCurrent[0] = GetSaturatedShortFromInt(nInput);

    // figure a dot product
    if (CCNNFilter->m_bMMXAvailable)
        nDotProduct = 0;//MONKEY_CNNFilter_CalculateDotProduct(&CCNNFilter->m_rbInput.m_pCurrent[-m_nOrder], &CCNNFilter->m_paryM[0], CCNNFilter->m_nOrder);
    else
        nDotProduct = MONKEY_CNNFilter_CalculateDotProductNoMMX(&CCNNFilter->m_rbInput.m_pCurrent[-CCNNFilter->m_nOrder], &CCNNFilter->m_paryM[0], CCNNFilter->m_nOrder);

    // calculate the output
    nOutput = nInput - ((nDotProduct + (1 << (CCNNFilter->m_nShift - 1))) >> CCNNFilter->m_nShift);

    // adapt
    if (CCNNFilter->m_bMMXAvailable)
	{}//MONKEY_CNNFilter_Adapt(&CCNNFilter->m_paryM[0], &CCNNFilter->m_rbDeltaM.m_pCurrent[-m_nOrder], -nOutput, CCNNFilter->m_nOrder);
    else
        MONKEY_CNNFilter_AdaptNoMMX(&CCNNFilter->m_paryM[0], &CCNNFilter->m_rbDeltaM.m_pCurrent[-CCNNFilter->m_nOrder], nOutput, CCNNFilter->m_nOrder);

    nTempABS = ABS(nInput);

    if (nTempABS > (CCNNFilter->m_nRunningAverage * 3))
        CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nInput >> 25) & 64) - 32;
    else if (nTempABS > (CCNNFilter->m_nRunningAverage * 4) / 3)
        CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nInput >> 26) & 32) - 16;
    else if (nTempABS > 0)
        CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nInput >> 27) & 16) - 8;
    else
        CCNNFilter->m_rbDeltaM.m_pCurrent[0] = 0;

    CCNNFilter->m_nRunningAverage += (nTempABS - CCNNFilter->m_nRunningAverage) / 16;

    CCNNFilter->m_rbDeltaM.m_pCurrent[-1] >>= 1;
    CCNNFilter->m_rbDeltaM.m_pCurrent[-2] >>= 1;
    CCNNFilter->m_rbDeltaM.m_pCurrent[-8] >>= 1;
        
    // increment and roll if necessary
    MONKEY_CRollBuffer_IncrementSafe(&CCNNFilter->m_rbInput);
    MONKEY_CRollBuffer_IncrementSafe(&CCNNFilter->m_rbDeltaM);

    return nOutput;
}

int MONKEY_CNNFilter_Decompress(CNNFilter * CCNNFilter, int nInput)
{
    // figure a dot product
    int nDotProduct;
	int nOutput;
    
    nDotProduct = MONKEY_CNNFilter_CalculateDotProductNoMMX(&CCNNFilter->m_rbInput.m_pCurrent[-CCNNFilter->m_nOrder], &CCNNFilter->m_paryM[0], CCNNFilter->m_nOrder);
    MONKEY_CNNFilter_AdaptNoMMX(&CCNNFilter->m_paryM[0], &CCNNFilter->m_rbDeltaM.m_pCurrent[-CCNNFilter->m_nOrder], nInput, CCNNFilter->m_nOrder);

    // store the output value
    nOutput = nInput + ((nDotProduct + (1 << (CCNNFilter->m_nShift - 1))) >> CCNNFilter->m_nShift);

    // update the input buffer
    CCNNFilter->m_rbInput.m_pCurrent[0] = GetSaturatedShortFromInt(nOutput);

    if (CCNNFilter->m_nVersion >= 3980)
    {
        int nTempABS = ABS(nOutput);

        if (nTempABS > (CCNNFilter->m_nRunningAverage * 3))
            CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nOutput >> 25) & 64) - 32;
        else if (nTempABS > (CCNNFilter->m_nRunningAverage * 4) / 3)
            CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nOutput >> 26) & 32) - 16;
        else if (nTempABS > 0)
            CCNNFilter->m_rbDeltaM.m_pCurrent[0] = ((nOutput >> 27) & 16) - 8;
        else
            CCNNFilter->m_rbDeltaM.m_pCurrent[0] = 0;

        CCNNFilter->m_nRunningAverage += (nTempABS - CCNNFilter->m_nRunningAverage) / 16;

        CCNNFilter->m_rbDeltaM.m_pCurrent[-1] >>= 1;
        CCNNFilter->m_rbDeltaM.m_pCurrent[-2] >>= 1;
        CCNNFilter->m_rbDeltaM.m_pCurrent[-8] >>= 1;
    }
    else
    {
        CCNNFilter->m_rbDeltaM.m_pCurrent[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
        CCNNFilter->m_rbDeltaM.m_pCurrent[-4] >>= 1;
        CCNNFilter->m_rbDeltaM.m_pCurrent[-8] >>= 1;
    }

    // increment and roll if necessary
    MONKEY_CRollBuffer_IncrementSafe(&CCNNFilter->m_rbInput);
    MONKEY_CRollBuffer_IncrementSafe(&CCNNFilter->m_rbDeltaM);

    return nOutput;
}
