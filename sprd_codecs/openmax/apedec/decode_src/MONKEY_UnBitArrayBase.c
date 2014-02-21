#include "MONKEY_APECommon.h"
#include "MONKEY_UnBitArray.h"
#include "MONKEY_APEDecompress.h"
#ifdef SPEEDOPT
#include "MONKEY_DIVTab.h"
#endif

#define MIN(a,b) (a)>(b) ? (b):(a)
#define MAX(a,b) (a)>(b) ? (a):(b)

const uint32 POWERS_OF_TWO_MINUS_ONE[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295UL};

const uint32 K_SUM_MIN_BOUNDARY[32] = {0,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648UL,0,0,0,0};

const uint32 RANGE_TOTAL_1[65] = {0,14824,28224,39348,47855,53994,58171,60926,62682,63786,64463,64878,65126,65276,65365,65419,65450,65469,65480,65487,65491,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,65536};
const uint32 RANGE_WIDTH_1[64] = {14824,13400,11124,8507,6139,4177,2755,1756,1104,677,415,248,150,89,54,31,19,11,7,4,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
const uint32 RANGE_TOTAL_2[65] = {0,19578,36160,48417,56323,60899,63265,64435,64971,65232,65351,65416,65447,65466,65476,65482,65485,65488,65490,65491,65492,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,65536};
const uint32 RANGE_WIDTH_2[64] = {19578,16582,12257,7906,4576,2366,1170,536,261,119,65,31,19,10,6,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int ) 1 << (CODE_BITS - 1))
#define BOTTOM_VALUE (TOP_VALUE >> 8)
#define MODEL_ELEMENTS 64
#define RANGE_OVERFLOW_SHIFT 16

void MONKEY_CUnBitArray_AdvanceToByteBoundary(CUnBitArray * CCUnBitArray) 
{
    int nMod = CCUnBitArray->m_nCurrentBitIndex % 8;
    if (nMod != 0) { CCUnBitArray->m_nCurrentBitIndex += 8 - nMod; }
}

int MONKEY_CUnBitArray_DecodeValue(CUnBitArray * CCUnBitArray, int DecodeMethod, unsigned int * value)
{
    switch (DecodeMethod)
    {
    case DECODE_VALUE_METHOD_UNSIGNED_INT:
        return MONKEY_CUnBitArray_DecodeValueXBits(CCUnBitArray, 32, value);
    }
    
    return 0;
}

int MONKEY_CUnBitArray_DecodeValueXBits(CUnBitArray * CCUnBitArray, uint32 nBits,  unsigned int * value) 
{
	int ret = ERROR_SUCCESS;
	int nRightBits;
	unsigned int nLeftBits, nBitArrayIndex, nLeftValue, nRightValue;

    // get more data if necessary
    if ((CCUnBitArray->m_nCurrentBitIndex + nBits) >= (CCUnBitArray->m_nBytes*8))
	{
		ret = MONKEY_CUnBitArray_FillBitArray(CCUnBitArray);
		if (ret != ERROR_SUCCESS)
			return ret;
	}
    // variable declares
    nLeftBits = 32 - (CCUnBitArray->m_nCurrentBitIndex & 31);
    nBitArrayIndex = CCUnBitArray->m_nCurrentBitIndex >> 5;
    CCUnBitArray->m_nCurrentBitIndex += nBits;
    
    // if their isn't an overflow to the right value, get the value and exit
    if (nLeftBits >= nBits)
	{
		* value = (CCUnBitArray->m_pBitArray[nBitArrayIndex] & (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);
		return ERROR_SUCCESS;
	}
    // must get the "split" value from left and right
    nRightBits = nBits - nLeftBits;
    
    nLeftValue = ((CCUnBitArray->m_pBitArray[nBitArrayIndex] & POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
    nRightValue = (CCUnBitArray->m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
    * value = (nLeftValue | nRightValue);
	return ERROR_SUCCESS;
}

#ifdef SPEEDOPT
#ifdef ARM9E
__inline int MONKEY_Div_ASM(unsigned int a, unsigned int b)
{
	int hi;
	unsigned int lo;
	
	//__asm
	//{
	//	UMULL lo, hi, a, b
	//}
	__asm__
		(
		"UMULL %0, %1, %2, %3":"=r"(lo),"=r"(hi):"r"(a),"r"(b)
		);	
	return hi;
}
#endif

__inline int MONKEY_Div1(unsigned int a, unsigned int b)
{
	// assume 0<=b<=65535

	int result;
	unsigned int temp;

#ifdef ARM9E
	result = MONKEY_Div_ASM(a, MONKEY_DIVTab[b]);
#else
	result = (int)(((__int64) a * MONKEY_DIVTab[b]) >> 32);
#endif

	temp = a - ((unsigned int)result)*b; 
	
	while(temp >= b)
	{
		result++;
		temp = temp - b;
	}
	
	return result;
}
__inline int MONKEY_Div2(unsigned int a, unsigned int b)
{
	int result;
	unsigned int temp;
	unsigned int atemp = a;
	unsigned int btemp = b;

	while(btemp >= 65536)
	{
		atemp = atemp >> 8;
		btemp = (btemp >> 8) + 1;
	}

#ifdef ARM9E
	result = MONKEY_Div_ASM(atemp, MONKEY_DIVTab[btemp]);
#else		
	result = (int)(((__int64) atemp * MONKEY_DIVTab[btemp]) >> 32);
#endif

	temp = a - ((unsigned int)result)*b; 
	
	while(temp >= b)
	{
		result++;
		temp = temp - b;
	}
	
	return result;
}
#endif


__inline int MONKEY_CUnBitArray_RangeDecodeFast(CUnBitArray * CCUnBitArray, int nShift)
{
    while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        CCUnBitArray->m_RangeCoderInfo.buffer = (CCUnBitArray->m_RangeCoderInfo.buffer << 8) | ((CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31))) & 0xFF);
        CCUnBitArray->m_nCurrentBitIndex += 8;
        CCUnBitArray->m_RangeCoderInfo.low = (CCUnBitArray->m_RangeCoderInfo.low << 8) | ((CCUnBitArray->m_RangeCoderInfo.buffer >> 1) & 0xFF);
        CCUnBitArray->m_RangeCoderInfo.range <<= 8;
    }

    // decode
       CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range >> nShift;

#ifndef SPEEDOPT
    return CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
#else
	return MONKEY_Div1(CCUnBitArray->m_RangeCoderInfo.low, CCUnBitArray->m_RangeCoderInfo.range);
#endif
}

__inline int MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CUnBitArray * CCUnBitArray, int nShift)
{
	int nRetVal=0;

    while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
    {   
        CCUnBitArray->m_RangeCoderInfo.buffer = (CCUnBitArray->m_RangeCoderInfo.buffer << 8) | ((CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31))) & 0xFF);
        CCUnBitArray->m_nCurrentBitIndex += 8;
        CCUnBitArray->m_RangeCoderInfo.low = (CCUnBitArray->m_RangeCoderInfo.low << 8) | ((CCUnBitArray->m_RangeCoderInfo.buffer >> 1) & 0xFF);
        CCUnBitArray->m_RangeCoderInfo.range <<= 8;
    }

    // decode
    CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range >> nShift;
#ifndef SPEEDOPT
    nRetVal = CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
#else
	nRetVal = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.low, CCUnBitArray->m_RangeCoderInfo.range);
#endif

    CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * nRetVal;

    return nRetVal;
}

int MONKEY_CUnBitArray_DecodeValueRange(CUnBitArray * CCUnBitArray, UNBIT_ARRAY_STATE * BitArrayState, int * Value)
{
	int ret = ERROR_SUCCESS;
	int nValue = 0;
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (CCUnBitArray->m_nCurrentBitIndex + CCUnBitArray->m_nRefillBitThreshold
		> CCUnBitArray->m_nBytes*8)
    {
        ret = MONKEY_CUnBitArray_FillBitArray(CCUnBitArray);
		if (ret!= ERROR_SUCCESS)
			return ret;
    }

    if (CCUnBitArray->m_nVersion >= 3990)
    {
        // figure the pivot value
        int nPivotValue = MAX((BitArrayState->nKSum>>5), 1);
        
        // get the overflow
        int nOverflow = 0;

		int nBase = 0;
        {
            // decode
            int nRangeTotal = MONKEY_CUnBitArray_RangeDecodeFast(CCUnBitArray, RANGE_OVERFLOW_SHIFT);
            
            // lookup the symbol (must be a faster way than this)
            while (nRangeTotal >= RANGE_TOTAL_2[nOverflow + 1]) 
			{ 
				nOverflow++; 
			}
            
            // update
            CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * RANGE_TOTAL_2[nOverflow];
            CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range * RANGE_WIDTH_2[nOverflow];
            
            // get the working k
            if (nOverflow == (MODEL_ELEMENTS - 1))
            {
                nOverflow = MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, 16);
                nOverflow <<= 16;
                nOverflow |= MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, 16);
            }
        }

        // get the value
        {
            int nShift = 0;
            if (nPivotValue >= (1 << 16))
            {
				int nSplitFactor, nPivotValueA, nPivotValueB;
				int nBaseA, nBaseB;
                int nPivotValueBits = 0;

                while ((nPivotValue >> nPivotValueBits) > 0) 
					{ 
						nPivotValueBits++; 
					}
                nSplitFactor = 1 << (nPivotValueBits - 16);
                nPivotValueA = (nPivotValue / nSplitFactor) + 1;
                nPivotValueB = nSplitFactor;

                while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    CCUnBitArray->m_RangeCoderInfo.buffer = (CCUnBitArray->m_RangeCoderInfo.buffer << 8) | ((CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31))) & 0xFF);
                    CCUnBitArray->m_nCurrentBitIndex += 8;
                    CCUnBitArray->m_RangeCoderInfo.low = (CCUnBitArray->m_RangeCoderInfo.low << 8) | ((CCUnBitArray->m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    CCUnBitArray->m_RangeCoderInfo.range <<= 8;
                }

#ifndef SPEEDOPT
				CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range / nPivotValueA;
                nBaseA = CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
#else
				CCUnBitArray->m_RangeCoderInfo.range = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.range, nPivotValueA);
				nBaseA = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.low, CCUnBitArray->m_RangeCoderInfo.range);
#endif
                CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * nBaseA;

                while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    CCUnBitArray->m_RangeCoderInfo.buffer = (CCUnBitArray->m_RangeCoderInfo.buffer << 8) | ((CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31))) & 0xFF);
                    CCUnBitArray->m_nCurrentBitIndex += 8;
                    CCUnBitArray->m_RangeCoderInfo.low = (CCUnBitArray->m_RangeCoderInfo.low << 8) | ((CCUnBitArray->m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    CCUnBitArray->m_RangeCoderInfo.range <<= 8;
                }

#ifndef SPEEDOPT
                CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range / nPivotValueB;
                nBaseB = CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
#else
				CCUnBitArray->m_RangeCoderInfo.range = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.range, nPivotValueB);
                nBaseB = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.low, CCUnBitArray->m_RangeCoderInfo.range);
#endif
                CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * nBaseB;

                nBase = nBaseA * nSplitFactor + nBaseB;
            }
            else
            {
				int nBaseLower;

                while (CCUnBitArray->m_RangeCoderInfo.range <= BOTTOM_VALUE)
                {   
                    CCUnBitArray->m_RangeCoderInfo.buffer = (CCUnBitArray->m_RangeCoderInfo.buffer << 8) | ((CCUnBitArray->m_pBitArray[CCUnBitArray->m_nCurrentBitIndex >> 5] >> (24 - (CCUnBitArray->m_nCurrentBitIndex & 31))) & 0xFF);
                    CCUnBitArray->m_nCurrentBitIndex += 8;
                    CCUnBitArray->m_RangeCoderInfo.low = (CCUnBitArray->m_RangeCoderInfo.low << 8) | ((CCUnBitArray->m_RangeCoderInfo.buffer >> 1) & 0xFF);
                    CCUnBitArray->m_RangeCoderInfo.range <<= 8;
                }

                // decode
				if (nPivotValue >= 2)
				{
#ifndef SPEEDOPT
					CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range / nPivotValue;
					nBaseLower = CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
#else
					CCUnBitArray->m_RangeCoderInfo.range = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.range, nPivotValue);
					nBaseLower = MONKEY_Div2(CCUnBitArray->m_RangeCoderInfo.low, CCUnBitArray->m_RangeCoderInfo.range);
#endif
				}
				else // (nPivotValue == 1)
				{
					//CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range / nPivotValue;
					nBaseLower = CCUnBitArray->m_RangeCoderInfo.low / CCUnBitArray->m_RangeCoderInfo.range;
				}

                CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * nBaseLower;

                nBase = nBaseLower;
            }
        }

        // build the value
        nValue = nBase + (nOverflow * nPivotValue);
    }
    else
    {
        // decode
        int nRangeTotal = MONKEY_CUnBitArray_RangeDecodeFast(CCUnBitArray, RANGE_OVERFLOW_SHIFT);
        
        // lookup the symbol (must be a faster way than this)
        int nOverflow = 0;
		int nTempK;

        while (nRangeTotal >= RANGE_TOTAL_1[nOverflow + 1]) { nOverflow++; }
        
        // update
        CCUnBitArray->m_RangeCoderInfo.low -= CCUnBitArray->m_RangeCoderInfo.range * RANGE_TOTAL_1[nOverflow];
        CCUnBitArray->m_RangeCoderInfo.range = CCUnBitArray->m_RangeCoderInfo.range * RANGE_WIDTH_1[nOverflow];
        
        // get the working k
        if (nOverflow == (MODEL_ELEMENTS - 1))
        {
            nTempK = MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, 5);
            nOverflow = 0;
        }
        else
        {
            nTempK = (BitArrayState->k < 1) ? 0 : BitArrayState->k - 1;
        }
        
        // figure the extra bits on the left and the left value
        if (nTempK <= 16 || CCUnBitArray->m_nVersion < 3910)
        {
            nValue = MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, nTempK);
        }                    
        else
        {    
            int nX1 = MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, 16);
            int nX2 = MONKEY_CUnBitArray_RangeDecodeFastWithUpdate(CCUnBitArray, nTempK - 16);
            nValue = nX1 | (nX2 << 16);
        }
            
        // build the value and output it
        nValue += (nOverflow << nTempK);
    }

    // update nKSum
    BitArrayState->nKSum += ((nValue + 1) / 2) - ((BitArrayState->nKSum + 16) >> 5);
    
    // update k
    if (BitArrayState->nKSum < K_SUM_MIN_BOUNDARY[BitArrayState->k]) 
        BitArrayState->k--;
    else if (BitArrayState->nKSum >= K_SUM_MIN_BOUNDARY[BitArrayState->k + 1]) 
        BitArrayState->k++;

    // output the value (converted to signed)
    * Value = (nValue & 1) ? (nValue >> 1) + 1 : -(nValue >> 1);
	return ret;
}


int MONKEY_CUnBitArray_FillAndResetBitArray(CUnBitArray * CCUnBitArray) 
{
	int ret = ERROR_SUCCESS;
	unsigned int nBytesToRead = 0;

	MONKEY_APE_DEC_DATA * CCUnBitArray_Input = (MONKEY_APE_DEC_DATA *)CCUnBitArray->Input;
    // reset the bit index
    CCUnBitArray->m_nCurrentBitIndex = 0;

	nBytesToRead = MIN((CCUnBitArray_Input->nLen - CCUnBitArray_Input->counter),CCUnBitArray->m_nLen);	
	if (nBytesToRead > 0)
	{
		memcpy((unsigned char *) CCUnBitArray->m_pBitArray, (CCUnBitArray_Input->pchData+CCUnBitArray_Input->counter), nBytesToRead);
    	CCUnBitArray->m_nBytes = nBytesToRead;
		CCUnBitArray_Input->counter += nBytesToRead;
	} else {
		//if (CCUnBitArray->m_nCurrentBitIndex + CCUnBitArray->m_nRefillBitThreshold
		//	> CCUnBitArray->m_nBytes*8)
		if (CCUnBitArray->m_nCurrentBitIndex >= CCUnBitArray->m_nBytes*8)
			ret = ERROR_TEMP_INBUF_UNDERFLOW;
	}
	
    return ret;
}

#if 0
void memmove(void *dst, const void *src, unsigned int length)
{
	if ((dst+length) < src)
		memcpy(dst, src, length);
	else {
		int i;
		char *dst_temp = dst;
		char *src_temp = src;

		for (i=0;i<length;i++)
			*dst_temp++ = *src_temp++;	
	}
}
#endif
//#define test_input
#ifdef test_input
FILE *fptest=0; 
#endif
int MONKEY_CUnBitArray_FillBitArray(CUnBitArray * CCUnBitArray) 
{
    // get the bit array index
	int ret = ERROR_SUCCESS;
    unsigned int nBitArrayIndex = CCUnBitArray->m_nCurrentBitIndex >> 5;
	unsigned int nBytesToRead = 0;

	MONKEY_APE_DEC_DATA * CCUnBitArray_Input = (MONKEY_APE_DEC_DATA *)CCUnBitArray->Input;

#ifdef test_input
if(!fptest)
{
	fptest = fopen("test.bin", "wb");
}
fwrite(CCUnBitArray->m_pBitArray, 1, nBitArrayIndex*4, fptest);
#endif
    // move the remaining data to the front
	if (nBitArrayIndex)
	{
		memmove((void *) (CCUnBitArray->m_pBitArray), (const void *) (CCUnBitArray->m_pBitArray + nBitArrayIndex), CCUnBitArray->m_nBytes - (nBitArrayIndex * 4));
		CCUnBitArray->m_nBytes -= nBitArrayIndex * 4;
		// adjust the m_Bit pointer
		CCUnBitArray->m_nCurrentBitIndex -= nBitArrayIndex * 4 * 8;
	}

    // read the new data
    nBytesToRead = MIN((CCUnBitArray_Input->nLen - CCUnBitArray_Input->counter),CCUnBitArray->m_nLen - CCUnBitArray->m_nBytes);
    
	if (nBytesToRead>0)
	{
		memcpy(((unsigned char *) CCUnBitArray->m_pBitArray) + CCUnBitArray->m_nBytes, 
			(CCUnBitArray_Input->pchData+CCUnBitArray_Input->counter), nBytesToRead);
		CCUnBitArray->m_nBytes += nBytesToRead;
		CCUnBitArray_Input->counter += nBytesToRead;
	} else {
		//if (CCUnBitArray->m_nCurrentBitIndex + CCUnBitArray->m_nRefillBitThreshold
		//	> CCUnBitArray->m_nBytes*8)
		//	ret = ERROR_TEMP_INBUF_UNDERFLOW;
		if (CCUnBitArray->m_nCurrentBitIndex >= CCUnBitArray->m_nBytes*8)
			ret = ERROR_TEMP_INBUF_UNDERFLOW;
	}
    // return
    return ret;
}

