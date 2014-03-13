
#include "MONKEY_UnBitArray.h"
#include "MONKEY_APEDecompress.h"

const uint32 POWERS_OF_TWO_MINUS_ONE_REVERSED[33] = {4294967295,2147483647,1073741823,536870911,268435455,134217727,67108863,33554431,16777215,8388607,4194303,2097151,1048575,524287,262143,131071,65535,32767,16383,8191,4095,2047,1023,511,255,127,63,31,15,7,3,1,0};

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int ) 1 << (CODE_BITS - 1))
#define BOTTOM_VALUE (TOP_VALUE >> 8)
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS - 2) % 8 + 1)

/***********************************************************************************

***********************************************************************************/
int MONKEY_CUnBitArray(CUnBitArray * CCUnBitArray, int nVersion) 
{
    CCUnBitArray->m_nFlushCounter = 0;
    CCUnBitArray->m_nFinalizeCounter = 0;
	CCUnBitArray->m_pBitArray = 0;

	return MONKEY_CUnBitArray_CreateHelper(CCUnBitArray, BUFFER_TEMP_INPUT_BYTES, nVersion);
}

int MONKEY_CUnBitArray_CreateHelper(CUnBitArray * CCUnBitArray, int nBytes, int nVersion)
{
    // check the parameters
    if (nBytes <= 0)
		return ERROR_BAD_PARAMETER;

    // save the size
    CCUnBitArray->m_nLen = (nBytes / 4) * 4;
    
    // set the variables
    CCUnBitArray->m_nVersion = nVersion;
	CCUnBitArray->m_nBytes = 0;
    CCUnBitArray->m_nCurrentBitIndex = 0;
	CCUnBitArray->m_nRefillBitThreshold = BitArray_RefillBitThreshold;
    
    // create the bitarray
    CCUnBitArray->m_pBitArray = (uint32 *) malloc(sizeof(uint32)*(CCUnBitArray->m_nLen/4));
    
    return (CCUnBitArray->m_pBitArray != 0) ? 0 : ERROR_INSUFFICIENT_MEMORY;
}

int MONKEY_CUnBitArray_Finish(CUnBitArray * CCUnBitArray)
{
	if (!CCUnBitArray)
		return ERROR_UNDEFINED;

    SAFE_ARRAY_DELETE(CCUnBitArray->m_pBitArray)
	return ERROR_SUCCESS;
}


int MONKEY_CUnBitArray_GenerateArrayRange(CUnBitArray * CCUnBitArray, int * pOutputArray, int nElements)
{
	int ret = ERROR_SUCCESS;
	int z;
    UNBIT_ARRAY_STATE BitArrayState;
    
	MONKEY_CUnBitArray_FlushState(&BitArrayState);
    MONKEY_CUnBitArray_FlushBitArray(CCUnBitArray);
    
    for (z = 0; z < nElements; z++)
    {
        ret = MONKEY_CUnBitArray_DecodeValueRange(CCUnBitArray, &BitArrayState, &(pOutputArray[z]));
		if (ret != ERROR_SUCCESS)
			return ret;
    }

    MONKEY_CUnBitArray_Finalize(CCUnBitArray);
	return ret;
}

int MONKEY_CUnBitArray_GenerateArray(CUnBitArray * CCUnBitArray, int * pOutputArray, int nElements) 
{
	int ret = ERROR_SUCCESS;
    ret = MONKEY_CUnBitArray_GenerateArrayRange(CCUnBitArray, pOutputArray, nElements);
	return ret;
}

void MONKEY_CUnBitArray_FlushState(UNBIT_ARRAY_STATE * BitArrayState)
{
    BitArrayState->k = 10;
    BitArrayState->nKSum = (1 << BitArrayState->k) * 16;
}

__inline unsigned char MONKEY_CUnBitArray_GetC(CUnBitArray * CCUnBitArray)
{
    unsigned char nValue = (unsigned char) (CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31)));
    CCUnBitArray->m_nCurrentBitIndex += 8;
    return nValue;
}    

void MONKEY_CUnBitArray_FlushBitArray(CUnBitArray * CCUnBitArray)
{
    MONKEY_CUnBitArray_AdvanceToByteBoundary(CCUnBitArray);
    CCUnBitArray->m_nCurrentBitIndex += 8; // ignore the first byte... (slows compression too much to not output this dummy byte)
    CCUnBitArray->m_RangeCoderInfo.buffer = MONKEY_CUnBitArray_GetC(CCUnBitArray);
    CCUnBitArray->m_RangeCoderInfo.low = CCUnBitArray->m_RangeCoderInfo.buffer >> (8 - EXTRA_BITS);
    CCUnBitArray->m_RangeCoderInfo.range = (unsigned int) 1 << EXTRA_BITS;
}

void MONKEY_CUnBitArray_Finalize(CUnBitArray * CCUnBitArray)
{
    // normalize
    while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        CCUnBitArray->m_nCurrentBitIndex += 8;
        CCUnBitArray->m_RangeCoderInfo.range <<= 8;
    }
    
    // used to back-pedal the last two bytes out
    // this should never have been a problem because we've outputted and normalized beforehand
    // but stopped doing it as of 3.96 in case it accounted for rare decompression failures
    if (CCUnBitArray->m_nVersion <= 3950)
        CCUnBitArray->m_nCurrentBitIndex -= 16;
}

