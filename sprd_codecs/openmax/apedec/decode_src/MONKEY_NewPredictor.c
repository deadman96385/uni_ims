
#include "MONKEY_NewPredictor.h"
#include "MONKEY_RollBuffer.h"
#include "MONKEY_APECommon.h"

static __inline void MONKEY_CRollBufferFast_IncrementSafe(CRollBufferFast * CCRollBufferFast)
{
    CCRollBufferFast->m_pCurrent++;
    if (CCRollBufferFast->m_pCurrent == &CCRollBufferFast->m_pData[CCRollBufferFast->m_nWindowElements + CCRollBufferFast->m_nHistoryElements])
        MONKEY_CRollBufferFast_Roll(CCRollBufferFast);
}

static __inline void MONKEY_CRollBufferFast_IncrementFast(CRollBufferFast * CCRollBufferFast)
{
    CCRollBufferFast->m_pCurrent++;
}

/*****************************************************************************************
CPredictorCompressNormal
*****************************************************************************************/
int MONKEY_CPredictorCompressNormal(CPredictorCompressNormal * CCPredictorCompressNormal, int nCompressionLevel)
{
	if (MONKEY_CRollBufferFast(&CCPredictorCompressNormal->m_rbPrediction, WINDOW_BLOCKS, 10))
		return ERROR_INSUFFICIENT_MEMORY;
	if (MONKEY_CRollBufferFast(&CCPredictorCompressNormal->m_rbAdapt, WINDOW_BLOCKS, 9))
		return ERROR_INSUFFICIENT_MEMORY;

    if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
    {
        CCPredictorCompressNormal->m_pNNFilter = 0;
        CCPredictorCompressNormal->m_pNNFilter1 = 0;
        CCPredictorCompressNormal->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
    {
        //CCPredictorCompressNormal->m_pNNFilter = new CNNFilter(16, 11, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter, 16, 11, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorCompressNormal->m_pNNFilter1 = 0;
        CCPredictorCompressNormal->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
    {
        //CCPredictorCompressNormal->m_pNNFilter = new CNNFilter(64, 11, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter, 64, 11, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorCompressNormal->m_pNNFilter1 = 0;
        CCPredictorCompressNormal->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
    {
        //CCPredictorCompressNormal->m_pNNFilter = new CNNFilter(256, 13, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter, 256, 13, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        //CCPredictorCompressNormal->m_pNNFilter1 = new CNNFilter(32, 10, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter1 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter1 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter1, 32, 10, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorCompressNormal->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_INSANE)
    {
        //CCPredictorCompressNormal->m_pNNFilter = new CNNFilter(1024 + 256, 15, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter, 1024 + 256, 15, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        //CCPredictorCompressNormal->m_pNNFilter1 = new CNNFilter(256, 13, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter1 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter1 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter1, 256, 13, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

        //CCPredictorCompressNormal->m_pNNFilter2 = new CNNFilter(16, 11, MAC_VERSION_NUMBER);
		CCPredictorCompressNormal->m_pNNFilter2 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorCompressNormal->m_pNNFilter2 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorCompressNormal->m_pNNFilter2, 16, 11, MAC_VERSION_NUMBER))
			return ERROR_INSUFFICIENT_MEMORY;

    }
    else
    {
        //throw(1);
    }

	return ERROR_SUCCESS;
}

int MONKEY_CPredictorCompressNormal_Finish(CPredictorCompressNormal * CCPredictorCompressNormal)
{
	if (!CCPredictorCompressNormal)
		return ERROR_UNDEFINED;

    MONKEY_CNNFilter_Finish(CCPredictorCompressNormal->m_pNNFilter);
    MONKEY_CNNFilter_Finish(CCPredictorCompressNormal->m_pNNFilter1);
    MONKEY_CNNFilter_Finish(CCPredictorCompressNormal->m_pNNFilter2);

	MONKEY_CRollBufferFast_Finish(&CCPredictorCompressNormal->m_rbPrediction);
	MONKEY_CRollBufferFast_Finish(&CCPredictorCompressNormal->m_rbAdapt);

	return ERROR_SUCCESS;
}
    
int MONKEY_CPredictorCompressNormal_Flush(CPredictorCompressNormal * CCPredictorCompressNormal)
{
    if (CCPredictorCompressNormal->m_pNNFilter) 
		MONKEY_CNNFilter_Flush(CCPredictorCompressNormal->m_pNNFilter);
    if (CCPredictorCompressNormal->m_pNNFilter1) 
		MONKEY_CNNFilter_Flush(CCPredictorCompressNormal->m_pNNFilter1);
    if (CCPredictorCompressNormal->m_pNNFilter2) 
		MONKEY_CNNFilter_Flush(CCPredictorCompressNormal->m_pNNFilter2);

    MONKEY_CRollBufferFast_Flush(&(CCPredictorCompressNormal->m_rbPrediction));
	MONKEY_CRollBufferFast_Flush(&(CCPredictorCompressNormal->m_rbAdapt));

    MONKEY_CScaledFirstOrderFilter_Flush(&(CCPredictorCompressNormal->m_Stage1FilterA));
	MONKEY_CScaledFirstOrderFilter_Flush(&(CCPredictorCompressNormal->m_Stage1FilterB));

    memset(CCPredictorCompressNormal->m_aryM, 0, sizeof(CCPredictorCompressNormal->m_aryM));

	{
		int * paryM = &(CCPredictorCompressNormal->m_aryM[8]);
		paryM[0] = 360;
		paryM[-1] = 317;
		paryM[-2] = -109;
		paryM[-3] = 98;
	}

    CCPredictorCompressNormal->m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

int MONKEY_CPredictorCompressNormal_CompressValue(CPredictorCompressNormal * CCPredictorCompressNormal, int nA, int nB)
{
    int *m_rbPrediction_p;
	int *m_rbAdapt_p;
	int *paryM;
	int nPredictionA, nPredictionB, nOutput;

	// roll the buffers if necessary
    if (CCPredictorCompressNormal->m_nCurrentIndex == WINDOW_BLOCKS)
    {
		MONKEY_CRollBufferFast_Roll(&(CCPredictorCompressNormal->m_rbPrediction));
		MONKEY_CRollBufferFast_Roll(&(CCPredictorCompressNormal->m_rbAdapt));

        CCPredictorCompressNormal->m_nCurrentIndex = 0;
    }

	m_rbPrediction_p = CCPredictorCompressNormal->m_rbPrediction.m_pCurrent;
	m_rbAdapt_p = CCPredictorCompressNormal->m_rbAdapt.m_pCurrent;

    // stage 1: simple, non-adaptive order 1 prediction
    nA = MONKEY_CScaledFirstOrderFilter_Compress(&CCPredictorCompressNormal->m_Stage1FilterA, nA);
    nB = MONKEY_CScaledFirstOrderFilter_Compress(&CCPredictorCompressNormal->m_Stage1FilterB, nB);

    // stage 2: adaptive offset filter(s)
    m_rbPrediction_p[0] = nA;
    m_rbPrediction_p[-2] = m_rbPrediction_p[-1] - m_rbPrediction_p[-2];
    
    m_rbPrediction_p[-5] = nB;
    m_rbPrediction_p[-6] = m_rbPrediction_p[-5] - m_rbPrediction_p[-6];

    paryM = &CCPredictorCompressNormal->m_aryM[8];

    nPredictionA = (m_rbPrediction_p[-1] * paryM[0]) + (m_rbPrediction_p[-2] * paryM[-1]) + (m_rbPrediction_p[-3] * paryM[-2]) + (m_rbPrediction_p[-4] * paryM[-3]);
	nPredictionB = (m_rbPrediction_p[-5] * paryM[-4]) + (m_rbPrediction_p[-6] * paryM[-5]) + (m_rbPrediction_p[-7] * paryM[-6]) + (m_rbPrediction_p[-8] * paryM[-7]) + (m_rbPrediction_p[-9] * paryM[-8]);

    nOutput = nA - ((nPredictionA + (nPredictionB >> 1)) >> 10);

    // adapt
    m_rbAdapt_p[0] = (m_rbPrediction_p[-1]) ? ((m_rbPrediction_p[-1] >> 30) & 2) - 1 : 0;
    m_rbAdapt_p[-1] = (m_rbPrediction_p[-2]) ? ((m_rbPrediction_p[-2] >> 30) & 2) - 1 : 0;
    m_rbAdapt_p[-4] = (m_rbPrediction_p[-5]) ? ((m_rbPrediction_p[-5] >> 30) & 2) - 1 : 0;
    m_rbAdapt_p[-5] = (m_rbPrediction_p[-6]) ? ((m_rbPrediction_p[-6] >> 30) & 2) - 1 : 0;

    if (nOutput > 0) 
    {
        int * pM = &paryM[-8]; int * pAdapt = &m_rbAdapt_p[-8];
        EXPAND_9_TIMES(*pM++ -= *pAdapt++;)
    }
    else if (nOutput < 0) 
    {
        int * pM = &paryM[-8]; int * pAdapt = &m_rbAdapt_p[-8];
        EXPAND_9_TIMES(*pM++ += *pAdapt++;)
    }

    // stage 3: NNFilters
    if (CCPredictorCompressNormal->m_pNNFilter)
    {
        nOutput = MONKEY_CNNFilter_Compress(CCPredictorCompressNormal->m_pNNFilter, nOutput);

        if (CCPredictorCompressNormal->m_pNNFilter1)
        {
            nOutput = MONKEY_CNNFilter_Compress(CCPredictorCompressNormal->m_pNNFilter1, nOutput);

            if (CCPredictorCompressNormal->m_pNNFilter2)
                nOutput = MONKEY_CNNFilter_Compress(CCPredictorCompressNormal->m_pNNFilter2, nOutput);
        }
    }

    MONKEY_CRollBufferFast_IncrementFast(&CCPredictorCompressNormal->m_rbPrediction);    
	MONKEY_CRollBufferFast_IncrementFast(&CCPredictorCompressNormal->m_rbAdapt);

    CCPredictorCompressNormal->m_nCurrentIndex++;

    return nOutput;
}

/*****************************************************************************************
CPredictorDecompressNormal3930to3950
*****************************************************************************************/
int MONKEY_CPredictorDecompressNormal3930to3950(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950, int nCompressionLevel, int nVersion)
{
    CCPredictorDecompressNormal3930to3950->m_pBuffer[0] = (int *) malloc((HISTORY_ELEMENTS + WINDOW_BLOCKS)*sizeof(int));
	if (CCPredictorDecompressNormal3930to3950->m_pBuffer[0] == 0)
		return ERROR_INSUFFICIENT_MEMORY;

    if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
    {
        CCPredictorDecompressNormal3930to3950->m_pNNFilter = 0;
        CCPredictorDecompressNormal3930to3950->m_pNNFilter1 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
    {
        CCPredictorDecompressNormal3930to3950->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompressNormal3930to3950->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompressNormal3930to3950->m_pNNFilter, 16, 11, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;
        
		CCPredictorDecompressNormal3930to3950->m_pNNFilter1 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
    {
		CCPredictorDecompressNormal3930to3950->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompressNormal3930to3950->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;
		
		if (MONKEY_CNNFilter(CCPredictorDecompressNormal3930to3950->m_pNNFilter, 64, 11, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorDecompressNormal3930to3950->m_pNNFilter1 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
    {
		CCPredictorDecompressNormal3930to3950->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompressNormal3930to3950->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompressNormal3930to3950->m_pNNFilter, 256, 13, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;
		
		CCPredictorDecompressNormal3930to3950->m_pNNFilter1 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompressNormal3930to3950->m_pNNFilter1 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompressNormal3930to3950->m_pNNFilter1, 32, 10, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;
    }
    else
    {
        //throw(1);
    }

	return ERROR_SUCCESS;
}

int MONKEY_CPredictorDecompressNormal3930to3950_Finish(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950)
{
	if (!CCPredictorDecompressNormal3930to3950)
		return ERROR_UNDEFINED;

    MONKEY_CNNFilter_Finish(CCPredictorDecompressNormal3930to3950->m_pNNFilter);
    MONKEY_CNNFilter_Finish(CCPredictorDecompressNormal3930to3950->m_pNNFilter1);

	SAFE_ARRAY_DELETE(CCPredictorDecompressNormal3930to3950->m_pBuffer[0])

	return ERROR_SUCCESS;
}
    
int MONKEY_CPredictorDecompressNormal3930to3950_Flush(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950)
{
    if (CCPredictorDecompressNormal3930to3950->m_pNNFilter)
		MONKEY_CNNFilter_Flush(CCPredictorDecompressNormal3930to3950->m_pNNFilter);
    if (CCPredictorDecompressNormal3930to3950->m_pNNFilter1)
		MONKEY_CNNFilter_Flush(CCPredictorDecompressNormal3930to3950->m_pNNFilter1);

    ZeroMemory(CCPredictorDecompressNormal3930to3950->m_pBuffer[0], (HISTORY_ELEMENTS + 1) * sizeof(int));    
    ZeroMemory(&CCPredictorDecompressNormal3930to3950->m_aryM[0], M_COUNT * sizeof(int));

    CCPredictorDecompressNormal3930to3950->m_aryM[0] = 360;
    CCPredictorDecompressNormal3930to3950->m_aryM[1] = 317;
    CCPredictorDecompressNormal3930to3950->m_aryM[2] = -109;
    CCPredictorDecompressNormal3930to3950->m_aryM[3] = 98;

    CCPredictorDecompressNormal3930to3950->m_pInputBuffer = &CCPredictorDecompressNormal3930to3950->m_pBuffer[0][HISTORY_ELEMENTS];
    
    CCPredictorDecompressNormal3930to3950->m_nLastValue = 0;
    CCPredictorDecompressNormal3930to3950->m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

int MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue(CPredictorDecompressNormal3930to3950 * CCPredictorDecompressNormal3930to3950, int nInput)
{
	int p1,p2,p3,p4,nRetVal;

    if (CCPredictorDecompressNormal3930to3950->m_nCurrentIndex == WINDOW_BLOCKS)
    {
        // copy forward and adjust pointers
        memcpy(&CCPredictorDecompressNormal3930to3950->m_pBuffer[0][0], &CCPredictorDecompressNormal3930to3950->m_pBuffer[0][WINDOW_BLOCKS], HISTORY_ELEMENTS * sizeof(int));
        CCPredictorDecompressNormal3930to3950->m_pInputBuffer = &CCPredictorDecompressNormal3930to3950->m_pBuffer[0][HISTORY_ELEMENTS];

        CCPredictorDecompressNormal3930to3950->m_nCurrentIndex = 0;
    }

    // stage 2: NNFilter
    if (CCPredictorDecompressNormal3930to3950->m_pNNFilter1)
        nInput = MONKEY_CNNFilter_Decompress(CCPredictorDecompressNormal3930to3950->m_pNNFilter1, nInput);
    if (CCPredictorDecompressNormal3930to3950->m_pNNFilter)
        nInput = MONKEY_CNNFilter_Decompress(CCPredictorDecompressNormal3930to3950->m_pNNFilter, nInput);;

    // stage 1: multiple predictors (order 2 and offset 1)

    p1 = CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-1];
    p2 = CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-1] - CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-2];
    p3 = CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-2] - CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-3];
    p4 = CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-3] - CCPredictorDecompressNormal3930to3950->m_pInputBuffer[-4];
    
    CCPredictorDecompressNormal3930to3950->m_pInputBuffer[0] = nInput + (((p1 * CCPredictorDecompressNormal3930to3950->m_aryM[0]) + (p2 * CCPredictorDecompressNormal3930to3950->m_aryM[1]) + (p3 * CCPredictorDecompressNormal3930to3950->m_aryM[2]) + (p4 * CCPredictorDecompressNormal3930to3950->m_aryM[3])) >> 9);
    
    if (nInput > 0) 
    {
        CCPredictorDecompressNormal3930to3950->m_aryM[0] -= ((p1 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[1] -= ((p2 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[2] -= ((p3 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[3] -= ((p4 >> 30) & 2) - 1;
    }
    else if (nInput < 0) 
    {
        CCPredictorDecompressNormal3930to3950->m_aryM[0] += ((p1 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[1] += ((p2 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[2] += ((p3 >> 30) & 2) - 1;
        CCPredictorDecompressNormal3930to3950->m_aryM[3] += ((p4 >> 30) & 2) - 1;
    }

    nRetVal = CCPredictorDecompressNormal3930to3950->m_pInputBuffer[0] + ((CCPredictorDecompressNormal3930to3950->m_nLastValue * 31) >> 5);
    CCPredictorDecompressNormal3930to3950->m_nLastValue = nRetVal;

    CCPredictorDecompressNormal3930to3950->m_nCurrentIndex++;
    CCPredictorDecompressNormal3930to3950->m_pInputBuffer++;

    return nRetVal;
}

/*****************************************************************************************
CPredictorDecompress3950toCurrent
*****************************************************************************************/
int MONKEY_CPredictorDecompress3950toCurrent(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent, int nCompressionLevel, int nVersion)
{
    CCPredictorDecompress3950toCurrent->m_nVersion = nVersion;

	if (MONKEY_CRollBufferFast(&CCPredictorDecompress3950toCurrent->m_rbPredictionA, WINDOW_BLOCKS, 8))
		return ERROR_INSUFFICIENT_MEMORY;

	if (MONKEY_CRollBufferFast(&CCPredictorDecompress3950toCurrent->m_rbPredictionB, WINDOW_BLOCKS, 8))
		return ERROR_INSUFFICIENT_MEMORY;

	if (MONKEY_CRollBufferFast(&CCPredictorDecompress3950toCurrent->m_rbAdaptA, WINDOW_BLOCKS, 8))
		return ERROR_INSUFFICIENT_MEMORY;

	if (MONKEY_CRollBufferFast(&CCPredictorDecompress3950toCurrent->m_rbAdaptB, WINDOW_BLOCKS, 8))
		return ERROR_INSUFFICIENT_MEMORY;

    if (nCompressionLevel == COMPRESSION_LEVEL_FAST)
    {
        CCPredictorDecompress3950toCurrent->m_pNNFilter = 0;
        CCPredictorDecompress3950toCurrent->m_pNNFilter1 = 0;
        CCPredictorDecompress3950toCurrent->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_NORMAL)
    {
		CCPredictorDecompress3950toCurrent->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter, 16, 11, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorDecompress3950toCurrent->m_pNNFilter1 = 0;
        CCPredictorDecompress3950toCurrent->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_HIGH)
    {
       	CCPredictorDecompress3950toCurrent->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;
		
		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter, 64, 11, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorDecompress3950toCurrent->m_pNNFilter1 = 0;
        CCPredictorDecompress3950toCurrent->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH)
    {
		CCPredictorDecompress3950toCurrent->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter, 256, 13, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

		CCPredictorDecompress3950toCurrent->m_pNNFilter1 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter1 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter1, 32, 10, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

        CCPredictorDecompress3950toCurrent->m_pNNFilter2 = 0;
    }
    else if (nCompressionLevel == COMPRESSION_LEVEL_INSANE)
    {
		CCPredictorDecompress3950toCurrent->m_pNNFilter = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter, (1024 + 256), 15, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

		CCPredictorDecompress3950toCurrent->m_pNNFilter1 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter1 == 0)
			return ERROR_INSUFFICIENT_MEMORY;

		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter1, 256, 13, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;

		CCPredictorDecompress3950toCurrent->m_pNNFilter2 = (CNNFilter *) malloc(sizeof(CNNFilter));
		if (CCPredictorDecompress3950toCurrent->m_pNNFilter2 == 0)
			return ERROR_INSUFFICIENT_MEMORY;
		
		if (MONKEY_CNNFilter(CCPredictorDecompress3950toCurrent->m_pNNFilter2, 16, 11, nVersion))
			return ERROR_INSUFFICIENT_MEMORY;
    }
    else
    {
        //throw(1);
    }

	return ERROR_SUCCESS;
}

int MONKEY_CPredictorDecompress3950toCurrent_Finish(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent)
{
	if (!CCPredictorDecompress3950toCurrent)
		return ERROR_UNDEFINED;

    MONKEY_CNNFilter_Finish(CCPredictorDecompress3950toCurrent->m_pNNFilter);
    MONKEY_CNNFilter_Finish(CCPredictorDecompress3950toCurrent->m_pNNFilter1);
    MONKEY_CNNFilter_Finish(CCPredictorDecompress3950toCurrent->m_pNNFilter2);

	MONKEY_CRollBufferFast_Finish(&CCPredictorDecompress3950toCurrent->m_rbPredictionA);
	MONKEY_CRollBufferFast_Finish(&CCPredictorDecompress3950toCurrent->m_rbPredictionB);
	MONKEY_CRollBufferFast_Finish(&CCPredictorDecompress3950toCurrent->m_rbAdaptA);
	MONKEY_CRollBufferFast_Finish(&CCPredictorDecompress3950toCurrent->m_rbAdaptB);

	return ERROR_SUCCESS;
}
    
int MONKEY_CPredictorDecompress3950toCurrent_Flush(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent)
{
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter) 
		MONKEY_CNNFilter_Flush(CCPredictorDecompress3950toCurrent->m_pNNFilter);
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter1) 
		MONKEY_CNNFilter_Flush(CCPredictorDecompress3950toCurrent->m_pNNFilter1);
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter2) 
		MONKEY_CNNFilter_Flush(CCPredictorDecompress3950toCurrent->m_pNNFilter2);

    ZeroMemory(CCPredictorDecompress3950toCurrent->m_aryMA, sizeof(CCPredictorDecompress3950toCurrent->m_aryMA));
    ZeroMemory(CCPredictorDecompress3950toCurrent->m_aryMB, sizeof(CCPredictorDecompress3950toCurrent->m_aryMB));

    MONKEY_CRollBufferFast_Flush(&CCPredictorDecompress3950toCurrent->m_rbPredictionA);
    MONKEY_CRollBufferFast_Flush(&CCPredictorDecompress3950toCurrent->m_rbPredictionB);
    MONKEY_CRollBufferFast_Flush(&CCPredictorDecompress3950toCurrent->m_rbAdaptA);
    MONKEY_CRollBufferFast_Flush(&CCPredictorDecompress3950toCurrent->m_rbAdaptB);

    CCPredictorDecompress3950toCurrent->m_aryMA[0] = 360;
    CCPredictorDecompress3950toCurrent->m_aryMA[1] = 317;
    CCPredictorDecompress3950toCurrent->m_aryMA[2] = -109;
    CCPredictorDecompress3950toCurrent->m_aryMA[3] = 98;

    MONKEY_CScaledFirstOrderFilter_Flush(&CCPredictorDecompress3950toCurrent->m_Stage1FilterA);
    MONKEY_CScaledFirstOrderFilter_Flush(&CCPredictorDecompress3950toCurrent->m_Stage1FilterB);
    
    CCPredictorDecompress3950toCurrent->m_nLastValueA = 0;
    
    CCPredictorDecompress3950toCurrent->m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

int MONKEY_CPredictorDecompress3950toCurrent_DecompressValue(CPredictorDecompress3950toCurrent * CCPredictorDecompress3950toCurrent, int nA, int nB)
{
	int *m_rbPredictionA_p;
	int *m_rbPredictionB_p;
	int *m_rbAdaptA_p;
	int *m_rbAdaptB_p;
	int nPredictionA, nPredictionB, nCurrentA, nRetVal;

    if (CCPredictorDecompress3950toCurrent->m_nCurrentIndex == WINDOW_BLOCKS)
    {
        // copy forward and adjust pointers
        MONKEY_CRollBufferFast_Roll(&CCPredictorDecompress3950toCurrent->m_rbPredictionA);    
		MONKEY_CRollBufferFast_Roll(&CCPredictorDecompress3950toCurrent->m_rbPredictionB);
        MONKEY_CRollBufferFast_Roll(&CCPredictorDecompress3950toCurrent->m_rbAdaptA); 
		MONKEY_CRollBufferFast_Roll(&CCPredictorDecompress3950toCurrent->m_rbAdaptB);

        CCPredictorDecompress3950toCurrent->m_nCurrentIndex = 0;
    }

	m_rbPredictionA_p = CCPredictorDecompress3950toCurrent->m_rbPredictionA.m_pCurrent;
	m_rbPredictionB_p = CCPredictorDecompress3950toCurrent->m_rbPredictionB.m_pCurrent;
	m_rbAdaptA_p = CCPredictorDecompress3950toCurrent->m_rbAdaptA.m_pCurrent;
	m_rbAdaptB_p = CCPredictorDecompress3950toCurrent->m_rbAdaptB.m_pCurrent;

    // stage 2: NNFilter
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter2)
        nA = MONKEY_CNNFilter_Decompress(CCPredictorDecompress3950toCurrent->m_pNNFilter2, nA);
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter1)
        nA = MONKEY_CNNFilter_Decompress(CCPredictorDecompress3950toCurrent->m_pNNFilter1, nA);
    if (CCPredictorDecompress3950toCurrent->m_pNNFilter)
        nA = MONKEY_CNNFilter_Decompress(CCPredictorDecompress3950toCurrent->m_pNNFilter, nA);

    // stage 1: multiple predictors (order 2 and offset 1)
    m_rbPredictionA_p[0] = CCPredictorDecompress3950toCurrent->m_nLastValueA;
    m_rbPredictionA_p[-1] = m_rbPredictionA_p[0] - m_rbPredictionA_p[-1];
    
    m_rbPredictionB_p[0] = MONKEY_CScaledFirstOrderFilter_Compress(&CCPredictorDecompress3950toCurrent->m_Stage1FilterB, nB);
    m_rbPredictionB_p[-1] = m_rbPredictionB_p[0] - m_rbPredictionB_p[-1];

    nPredictionA = (m_rbPredictionA_p[0] * CCPredictorDecompress3950toCurrent->m_aryMA[0]) + (m_rbPredictionA_p[-1] * CCPredictorDecompress3950toCurrent->m_aryMA[1]) + (m_rbPredictionA_p[-2] * CCPredictorDecompress3950toCurrent->m_aryMA[2]) + (m_rbPredictionA_p[-3] * CCPredictorDecompress3950toCurrent->m_aryMA[3]);
    nPredictionB = (m_rbPredictionB_p[0] * CCPredictorDecompress3950toCurrent->m_aryMB[0]) + (m_rbPredictionB_p[-1] * CCPredictorDecompress3950toCurrent->m_aryMB[1]) + (m_rbPredictionB_p[-2] * CCPredictorDecompress3950toCurrent->m_aryMB[2]) + (m_rbPredictionB_p[-3] * CCPredictorDecompress3950toCurrent->m_aryMB[3]) + (m_rbPredictionB_p[-4] * CCPredictorDecompress3950toCurrent->m_aryMB[4]);

    nCurrentA = nA + ((nPredictionA + (nPredictionB >> 1)) >> 10);

    m_rbAdaptA_p[0] = (m_rbPredictionA_p[0]) ? ((m_rbPredictionA_p[0] >> 30) & 2) - 1 : 0;
    m_rbAdaptA_p[-1] = (m_rbPredictionA_p[-1]) ? ((m_rbPredictionA_p[-1] >> 30) & 2) - 1 : 0;
    
    m_rbAdaptB_p[0] = (m_rbPredictionB_p[0]) ? ((m_rbPredictionB_p[0] >> 30) & 2) - 1 : 0;
    m_rbAdaptB_p[-1] = (m_rbPredictionB_p[-1]) ? ((m_rbPredictionB_p[-1] >> 30) & 2) - 1 : 0;

    if (nA > 0) 
    {
        CCPredictorDecompress3950toCurrent->m_aryMA[0] -= m_rbAdaptA_p[0];
        CCPredictorDecompress3950toCurrent->m_aryMA[1] -= m_rbAdaptA_p[-1];
        CCPredictorDecompress3950toCurrent->m_aryMA[2] -= m_rbAdaptA_p[-2];
        CCPredictorDecompress3950toCurrent->m_aryMA[3] -= m_rbAdaptA_p[-3];

        CCPredictorDecompress3950toCurrent->m_aryMB[0] -= m_rbAdaptB_p[0];
        CCPredictorDecompress3950toCurrent->m_aryMB[1] -= m_rbAdaptB_p[-1];
        CCPredictorDecompress3950toCurrent->m_aryMB[2] -= m_rbAdaptB_p[-2];
        CCPredictorDecompress3950toCurrent->m_aryMB[3] -= m_rbAdaptB_p[-3];
        CCPredictorDecompress3950toCurrent->m_aryMB[4] -= m_rbAdaptB_p[-4];
    }
    else if (nA < 0) 
    {
        CCPredictorDecompress3950toCurrent->m_aryMA[0] += m_rbAdaptA_p[0];
        CCPredictorDecompress3950toCurrent->m_aryMA[1] += m_rbAdaptA_p[-1];
        CCPredictorDecompress3950toCurrent->m_aryMA[2] += m_rbAdaptA_p[-2];
        CCPredictorDecompress3950toCurrent->m_aryMA[3] += m_rbAdaptA_p[-3];

        CCPredictorDecompress3950toCurrent->m_aryMB[0] += m_rbAdaptB_p[0];
        CCPredictorDecompress3950toCurrent->m_aryMB[1] += m_rbAdaptB_p[-1];
        CCPredictorDecompress3950toCurrent->m_aryMB[2] += m_rbAdaptB_p[-2];
        CCPredictorDecompress3950toCurrent->m_aryMB[3] += m_rbAdaptB_p[-3];
        CCPredictorDecompress3950toCurrent->m_aryMB[4] += m_rbAdaptB_p[-4];
    }

    nRetVal = MONKEY_CScaledFirstOrderFilter_Decompress(&CCPredictorDecompress3950toCurrent->m_Stage1FilterA, nCurrentA);
    CCPredictorDecompress3950toCurrent->m_nLastValueA = nCurrentA;
    
    MONKEY_CRollBufferFast_IncrementFast(&CCPredictorDecompress3950toCurrent->m_rbPredictionA); 
	MONKEY_CRollBufferFast_IncrementFast(&CCPredictorDecompress3950toCurrent->m_rbPredictionB);
    MONKEY_CRollBufferFast_IncrementFast(&CCPredictorDecompress3950toCurrent->m_rbAdaptA); 
	MONKEY_CRollBufferFast_IncrementFast(&CCPredictorDecompress3950toCurrent->m_rbAdaptB);

    CCPredictorDecompress3950toCurrent->m_nCurrentIndex++;

    return nRetVal;
}
