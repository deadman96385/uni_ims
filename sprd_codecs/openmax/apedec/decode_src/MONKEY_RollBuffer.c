#include "MONKEY_RollBuffer.h"
#include "MONKEY_APECommon.h"

void MONKEY_CRollBuffer(CRollBuffer * CCRollBuffer)
{
    CCRollBuffer->m_pData = 0;
    CCRollBuffer->m_pCurrent = 0;
}

void MONKEY_CRollBuffer_Flush(CRollBuffer * CCRollBuffer)
{
    ZeroMemory(CCRollBuffer->m_pData, (CCRollBuffer->m_nHistoryElements + 1) * sizeof(short));
    CCRollBuffer->m_pCurrent = &CCRollBuffer->m_pData[CCRollBuffer->m_nHistoryElements];
}

int MONKEY_CRollBuffer_Create(CRollBuffer * CCRollBuffer, int nWindowElements, int nHistoryElements)
{
    SAFE_ARRAY_DELETE(CCRollBuffer->m_pData)

    CCRollBuffer->m_nWindowElements = nWindowElements;
    CCRollBuffer->m_nHistoryElements = nHistoryElements;

    CCRollBuffer->m_pData = (short *) malloc((CCRollBuffer->m_nWindowElements + CCRollBuffer->m_nHistoryElements)*sizeof(short));
    if (CCRollBuffer->m_pData == 0)
        return ERROR_INSUFFICIENT_MEMORY;

    MONKEY_CRollBuffer_Flush(CCRollBuffer);
    return 0;
}

int MONKEY_CRollBuffer_Finish(CRollBuffer * CCRollBuffer)
{
	if (!CCRollBuffer)
		return ERROR_UNDEFINED;

    SAFE_ARRAY_DELETE(CCRollBuffer->m_pData)
	return ERROR_SUCCESS;
}

void MONKEY_CRollBuffer_Roll(CRollBuffer * CCRollBuffer)
{
    memcpy(&CCRollBuffer->m_pData[0], &CCRollBuffer->m_pCurrent[-CCRollBuffer->m_nHistoryElements], CCRollBuffer->m_nHistoryElements * sizeof(short));
    CCRollBuffer->m_pCurrent = &CCRollBuffer->m_pData[CCRollBuffer->m_nHistoryElements];
}


void MONKEY_CRollBufferFast_Flush(CRollBufferFast * CCRollBufferFast)
{
    ZeroMemory(CCRollBufferFast->m_pData, (CCRollBufferFast->m_nHistoryElements + 1) * sizeof(int));
    CCRollBufferFast->m_pCurrent = &CCRollBufferFast->m_pData[CCRollBufferFast->m_nHistoryElements];
}

int MONKEY_CRollBufferFast(CRollBufferFast * CCRollBufferFast, int WINDOW_ELEMENTS, int HISTORY_ELEMENTS)
{
    CCRollBufferFast->m_pData = (int *) malloc((WINDOW_ELEMENTS + HISTORY_ELEMENTS)*sizeof(int));
	if(CCRollBufferFast->m_pData == 0)
		return ERROR_INSUFFICIENT_MEMORY;

	CCRollBufferFast->m_nHistoryElements = HISTORY_ELEMENTS;
	CCRollBufferFast->m_nWindowElements = WINDOW_ELEMENTS;
    MONKEY_CRollBufferFast_Flush(CCRollBufferFast);
		return 0;
}

int MONKEY_CRollBufferFast_Finish(CRollBufferFast * CCRollBufferFast)
{
	if (!CCRollBufferFast)
		return ERROR_UNDEFINED;

    SAFE_ARRAY_DELETE(CCRollBufferFast->m_pData)
	return ERROR_SUCCESS;
}

void MONKEY_CRollBufferFast_Roll(CRollBufferFast * CCRollBufferFast)
{
    memcpy(&CCRollBufferFast->m_pData[0], &(CCRollBufferFast->m_pCurrent[-CCRollBufferFast->m_nHistoryElements]), CCRollBufferFast->m_nHistoryElements * sizeof(int));
    CCRollBufferFast->m_pCurrent = &CCRollBufferFast->m_pData[CCRollBufferFast->m_nHistoryElements];
}
