
#include "MONKEY_CircleBuffer.h"

#define MIN(a,b) (a)>(b) ? (b):(a)

void MONKEY_CircleBuffer_CircleBuffer(CircleBuffer * CCircleBuffer)
{
    CCircleBuffer->m_pBuffer = 0;
    CCircleBuffer->m_nTotal = 0;
    CCircleBuffer->m_nHead = 0;
    CCircleBuffer->m_nTail = 0;
    CCircleBuffer->m_nEndCap = 0;
    CCircleBuffer->m_nMaxDirectWriteBytes = 0;
}

int MONKEY_CircleBuffer_CreateBuffer(CircleBuffer * CCircleBuffer, int nBytes, int nMaxDirectWriteBytes)
{
    SAFE_ARRAY_DELETE(CCircleBuffer->m_pBuffer)
    
    CCircleBuffer->m_nMaxDirectWriteBytes = nMaxDirectWriteBytes;
    CCircleBuffer->m_nTotal = nBytes + 1 + nMaxDirectWriteBytes;
    CCircleBuffer->m_pBuffer = (unsigned char *)malloc(CCircleBuffer->m_nTotal);
	if (CCircleBuffer->m_pBuffer == 0)
		return ERROR_INSUFFICIENT_MEMORY;
    
	CCircleBuffer->m_nHead = 0;
    CCircleBuffer->m_nTail = 0;
    CCircleBuffer->m_nEndCap = CCircleBuffer->m_nTotal;

	return ERROR_SUCCESS;
}

int MONKEY_CircleBuffer_Finish(CircleBuffer * CCircleBuffer)
{
    if (!CCircleBuffer)
		return ERROR_UNDEFINED;

	SAFE_ARRAY_DELETE(CCircleBuffer->m_pBuffer)
	return ERROR_SUCCESS;
}

int MONKEY_CircleBuffer_MaxAdd(CircleBuffer * CCircleBuffer)
{
    int nMaxAdd = (CCircleBuffer->m_nTail >= CCircleBuffer->m_nHead) ? (CCircleBuffer->m_nTotal - 1 - CCircleBuffer->m_nMaxDirectWriteBytes) - (CCircleBuffer->m_nTail - CCircleBuffer->m_nHead) : CCircleBuffer->m_nHead - CCircleBuffer->m_nTail - 1;
    return nMaxAdd;
}

int MONKEY_CircleBuffer_MaxGet(CircleBuffer * CCircleBuffer)
{
    return (CCircleBuffer->m_nTail >= CCircleBuffer->m_nHead) ? CCircleBuffer->m_nTail - CCircleBuffer->m_nHead : (CCircleBuffer->m_nEndCap - CCircleBuffer->m_nHead) + CCircleBuffer->m_nTail;
}

int MONKEY_CircleBuffer_Get(CircleBuffer * CCircleBuffer, unsigned char * pBuffer, int nBytes)
{
    int nTotalGetBytes = 0;

    if (pBuffer != 0 && nBytes > 0)
    {
        int nHeadBytes = MIN(CCircleBuffer->m_nEndCap - CCircleBuffer->m_nHead, nBytes);
        int nFrontBytes = nBytes - nHeadBytes;

        memcpy(&pBuffer[0], &CCircleBuffer->m_pBuffer[CCircleBuffer->m_nHead], nHeadBytes);
        nTotalGetBytes = nHeadBytes;

        if (nFrontBytes > 0)
        {
            memcpy(&pBuffer[nHeadBytes], &CCircleBuffer->m_pBuffer[0], nFrontBytes);
            nTotalGetBytes += nFrontBytes;
        }

        MONKEY_CircleBuffer_RemoveHead(CCircleBuffer, nBytes);
    }

    return nTotalGetBytes;
}

void MONKEY_CircleBuffer_Empty(CircleBuffer * CCircleBuffer)
{
    CCircleBuffer->m_nHead = 0;
    CCircleBuffer->m_nTail = 0;
    CCircleBuffer->m_nEndCap = CCircleBuffer->m_nTotal;
}

int MONKEY_CircleBuffer_RemoveHead(CircleBuffer * CCircleBuffer, int nBytes)
{
    nBytes = MIN(MONKEY_CircleBuffer_MaxGet(CCircleBuffer), nBytes);
    CCircleBuffer->m_nHead += nBytes;
    if (CCircleBuffer->m_nHead >= CCircleBuffer->m_nEndCap)
        CCircleBuffer->m_nHead -= CCircleBuffer->m_nEndCap;
    return nBytes;
}

int MONKEY_CircleBuffer_RemoveTail(CircleBuffer * CCircleBuffer, int nBytes)
{
    nBytes = MIN(MONKEY_CircleBuffer_MaxGet(CCircleBuffer), nBytes);
    CCircleBuffer->m_nTail -= nBytes;
    if (CCircleBuffer->m_nTail < 0)
        CCircleBuffer->m_nTail += CCircleBuffer->m_nEndCap;
    return nBytes;
}
