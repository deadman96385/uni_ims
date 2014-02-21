#include "MONKEY_NewPredictor.h"
#include "MONKEY_Prepare.h"
#include "MONKEY_APEDecompress.h"

#define MIN(a,b) (a)>(b) ? (b):(a)

static int MONKEY_APEDecompress_FillFrameBuffer(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
static int MONKEY_APEDecompress_DecodeBlocksToFrameBuffer(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, int nBlocks);
static int MONKEY_APEDecompress_StartFrame(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);
static void MONKEY_APEDecompress_EndFrame(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo);


int MONKEY_APEDecompress_Init(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo)
{
	int nTotalBlocks;

	// check if we have anything to do
    if (m_APEDecompress->m_bDecompressorInitialized)
        return ERROR_SUCCESS;
	
    // version check (this implementation only works with 3.93 and later files)
    if (m_APEFileInfo->nVersion < 3930)
    {
        return ERROR_UPSUPPORTED_FILE_VERSION;
    }
	if ((m_APEFileInfo->nBytesPerSample != 2)||(m_APEFileInfo->nBitsPerSample != 16))
	{
		return ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH;
	}
	if ((m_APEFileInfo->nChannels != 1)&&(m_APEFileInfo->nChannels != 2))
	{
		return ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT;
	}

	nTotalBlocks = (m_APEFileInfo->nTotalFrames == 0) ? 0 : ((m_APEFileInfo->nTotalFrames -  1) * m_APEFileInfo->nBlocksPerFrame) 
									+ m_APEFileInfo->nFinalFrameBlocks;

    // initialize other stuff
    m_APEDecompress->m_bDecompressorInitialized = 0;
    m_APEDecompress->m_nCurrentFrame = 0;
    m_APEDecompress->m_nCurrentBlock = 0;
    m_APEDecompress->m_nCurrentFrameBufferBlock = 0;
    m_APEDecompress->m_bErrorDecodingCurrentFrame = 0;
    m_APEDecompress->m_nCurrentTimeUs = 1;
	
    // set the "real" start and finish blocks
    m_APEDecompress->m_nStartBlock = 0;
    m_APEDecompress->m_nFinishBlock = nTotalBlocks;
	m_APEDecompress->nBlocksToOutput = nTotalBlocks;
    m_APEDecompress->m_bIsRanged = 0;

    // create a frame buffer
	if (MONKEY_CircleBuffer_CreateBuffer(&(m_APEDecompress->m_cbFrameBufferX), (BLOCKS_TEMP_OUTPUT) * m_APEFileInfo->nBytesPerSample, m_APEFileInfo->nBytesPerSample * 64))
		return ERROR_INSUFFICIENT_MEMORY;
	if (m_APEFileInfo->nChannels == 2)
	{
		if (MONKEY_CircleBuffer_CreateBuffer(&(m_APEDecompress->m_cbFrameBufferY), (BLOCKS_TEMP_OUTPUT) * m_APEFileInfo->nBytesPerSample, m_APEFileInfo->nBytesPerSample * 64))
			return ERROR_INSUFFICIENT_MEMORY;
	}
    
    // create decoding components
   	if (m_APEFileInfo->nVersion > 3900)
	{
		MONKEY_CUnBitArray(&(m_APEDecompress->m_spUnBitArray), m_APEFileInfo->nVersion);
	}
	else
	{} // to be implemented

    if (m_APEFileInfo->nVersion >= 3950)
    {
		m_APEDecompress->m_spNewPredictorX = (CPredictorDecompress3950toCurrent *) malloc(sizeof(CPredictorDecompress3950toCurrent));
		m_APEDecompress->m_spNewPredictorY = (CPredictorDecompress3950toCurrent *) malloc(sizeof(CPredictorDecompress3950toCurrent));

		if ((m_APEDecompress->m_spNewPredictorX == 0)||(m_APEDecompress->m_spNewPredictorY == 0))
			return ERROR_INSUFFICIENT_MEMORY;

		MONKEY_CPredictorDecompress3950toCurrent((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX, m_APEFileInfo->nCompressionLevel, m_APEFileInfo->nVersion);
       	MONKEY_CPredictorDecompress3950toCurrent((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorY, m_APEFileInfo->nCompressionLevel, m_APEFileInfo->nVersion);
    }
    else
    {
		m_APEDecompress->m_spNewPredictorX = (CPredictorDecompressNormal3930to3950 *) malloc(sizeof(CPredictorDecompressNormal3930to3950));
		m_APEDecompress->m_spNewPredictorY = (CPredictorDecompressNormal3930to3950 *) malloc(sizeof(CPredictorDecompressNormal3930to3950));

		if ((m_APEDecompress->m_spNewPredictorX == 0)||(m_APEDecompress->m_spNewPredictorY == 0))
			return ERROR_INSUFFICIENT_MEMORY;

		MONKEY_CPredictorDecompressNormal3930to3950((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX, m_APEFileInfo->nCompressionLevel, m_APEFileInfo->nVersion);
       	MONKEY_CPredictorDecompressNormal3930to3950((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorY, m_APEFileInfo->nCompressionLevel, m_APEFileInfo->nVersion);
    }

	// update the initialized flag
    m_APEDecompress->m_bDecompressorInitialized = 1; 
    
    return ERROR_SUCCESS;
}

int MONKEY_APEDecompress_Term(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo)
{	
	if((!m_APEDecompress)||(!m_APEFileInfo))
		return ERROR_UNDEFINED;

	MONKEY_CircleBuffer_Finish(&(m_APEDecompress->m_cbFrameBufferX));
	if (m_APEFileInfo->nChannels == 2)
		MONKEY_CircleBuffer_Finish(&(m_APEDecompress->m_cbFrameBufferY));

	if (m_APEFileInfo->nVersion > 3900)
	{
		MONKEY_CUnBitArray_Finish(&(m_APEDecompress->m_spUnBitArray));
	}
	else
	{} // to be implemented

	if (m_APEFileInfo->nVersion >= 3950)
    {
		MONKEY_CPredictorDecompress3950toCurrent_Finish((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX);
       	MONKEY_CPredictorDecompress3950toCurrent_Finish((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorY);
    }
    else
    {
		MONKEY_CPredictorDecompressNormal3930to3950_Finish((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX);
       	MONKEY_CPredictorDecompressNormal3930to3950_Finish((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorY);
    }

	return ERROR_SUCCESS;
}

int MONKEY_APEDecompress_Dec(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, 
	MONKEY_APE_DEC_DATA * Input, MONKEY_APE_DEC_DATA * Output)
{
    int nRetVal = ERROR_SUCCESS;

	int nBlocksUntilFinish, nBlocksToRetrieve, nBlocksRetrieved;

	nBlocksUntilFinish = m_APEDecompress->m_nFinishBlock - m_APEDecompress->m_nCurrentBlock;

	nBlocksToRetrieve = BLOCKS_PER_DECODE - Output->counter;
    nBlocksToRetrieve = MIN(nBlocksToRetrieve, nBlocksUntilFinish);

	// fill up the input temp buffer
	m_APEDecompress->m_spUnBitArray.Input = Input;
	nRetVal = MONKEY_CUnBitArray_FillBitArray(&(m_APEDecompress->m_spUnBitArray));
	if (nRetVal != ERROR_SUCCESS)
        return nRetVal;

    // decode and fill up the output temp buffer
    nRetVal = MONKEY_APEDecompress_FillFrameBuffer(m_APEDecompress, m_APEFileInfo);
    if (nRetVal != ERROR_SUCCESS)
        return nRetVal;

    // fill up the output from the output temp buffer
   
    nBlocksRetrieved = MONKEY_CircleBuffer_Get(&(m_APEDecompress->m_cbFrameBufferX), 
    		((unsigned char*)Output->pchData + Output->counter * m_APEFileInfo->nBytesPerSample), nBlocksToRetrieve * m_APEFileInfo->nBytesPerSample)/m_APEFileInfo->nBytesPerSample;
	if (m_APEFileInfo->nChannels == 2)
		{
			int nBlocksRetrieved_Y;
			nBlocksRetrieved_Y = MONKEY_CircleBuffer_Get(&(m_APEDecompress->m_cbFrameBufferY), 
			((unsigned char*)Output->pchData + Output->nLenAlloc/2 + Output->counter * m_APEFileInfo->nBytesPerSample), nBlocksToRetrieve * m_APEFileInfo->nBytesPerSample)/m_APEFileInfo->nBytesPerSample;
			if (nBlocksRetrieved_Y != nBlocksRetrieved)
				nRetVal = ERROR_TEMP_OUTBUF_UNDERFLOW;
		}
	
	Output->counter += nBlocksRetrieved;
	Output->nLen += nBlocksRetrieved * m_APEFileInfo->nBytesPerSample * m_APEFileInfo->nChannels;

    // update position
    m_APEDecompress->m_nCurrentBlock += nBlocksRetrieved;
    if (nBlocksRetrieved != nBlocksToRetrieve)
		nRetVal = ERROR_TEMP_OUTBUF_UNDERFLOW;
	m_APEDecompress->nBlocksToOutput -= nBlocksRetrieved;

	// fill up the input temp buffer
	MONKEY_CUnBitArray_FillBitArray(&(m_APEDecompress->m_spUnBitArray));

    return nRetVal;
}


/*****************************************************************************************
Decodes blocks of data
*****************************************************************************************/
static int MONKEY_APEDecompress_FillFrameBuffer(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo)
{
    int nRetVal = ERROR_SUCCESS;

    // determine the maximum blocks we can decode
    // note that we won't do end capping because we can't use data
    // until EndFrame(...) successfully handles the frame
    // that means we may decode a little extra in end capping cases
    // but this allows robust error handling of bad frames
    int nMaxBlocks = MONKEY_CircleBuffer_MaxAdd(&(m_APEDecompress->m_cbFrameBufferX)) / m_APEFileInfo->nBytesPerSample;

    // loop and decode data
    int nBlocksLeft = nMaxBlocks;

    while (nBlocksLeft > 0)
    {
		int nFrameBlocks, nFrameOffsetBlocks, nFrameBlocksLeft, nBlocksThisPass;
		
		if((m_APEDecompress->m_nCurrentFrame < 0)||(m_APEDecompress->m_nCurrentFrame >= m_APEFileInfo->nTotalFrames))
			break;
		else if (m_APEDecompress->m_nCurrentFrame != (m_APEFileInfo->nTotalFrames - 1))
			nFrameBlocks = m_APEFileInfo->nBlocksPerFrame;
		else
			nFrameBlocks = m_APEFileInfo->nFinalFrameBlocks;

        nFrameOffsetBlocks = m_APEDecompress->m_nCurrentFrameBufferBlock % m_APEFileInfo->nBlocksPerFrame;
        nFrameBlocksLeft = nFrameBlocks - nFrameOffsetBlocks;
        nBlocksThisPass = MIN(nFrameBlocksLeft, nBlocksLeft);

        // start the frame if we need to
        if (nFrameOffsetBlocks == 0)
		{
            nRetVal = MONKEY_APEDecompress_StartFrame(m_APEDecompress, m_APEFileInfo);
			if (nRetVal != ERROR_SUCCESS)
				return nRetVal;
        }
        // decode data
        nRetVal = MONKEY_APEDecompress_DecodeBlocksToFrameBuffer(m_APEDecompress, m_APEFileInfo, nBlocksThisPass);
		if (nRetVal != ERROR_SUCCESS)
				return nRetVal;
            
        // end the frame if we need to
        if ((nFrameOffsetBlocks + nBlocksThisPass) >= nFrameBlocks)
        {
            MONKEY_APEDecompress_EndFrame(m_APEDecompress, m_APEFileInfo);
            if (m_APEDecompress->m_bErrorDecodingCurrentFrame)
            {
                nRetVal = ERROR_INVALID_CHECKSUM;
				return nRetVal;
            }
        }

        nBlocksLeft -= nBlocksThisPass;
    }

    return nRetVal;
}

static int MONKEY_APEDecompress_DecodeBlocksToFrameBuffer(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, int nBlocks)
{
    // decode the samples
    int nRetVal = ERROR_SUCCESS;
    int nBlocksProcessed = 0;

        if (m_APEFileInfo->nChannels == 2)
        {
            if ((m_APEDecompress->m_nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && 
                (m_APEDecompress->m_nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    MONKEY_Unprepare(0, 0, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
						MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferY)), &m_APEDecompress->m_nCRC);

					MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
					MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferY), m_APEFileInfo->nBytesPerSample);
                }
            }
            else if (m_APEDecompress->m_nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
					int X;
					if (m_APEFileInfo->nVersion >= 3950)
					{
						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX), &X);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						
						X = MONKEY_CPredictorDecompress3950toCurrent_DecompressValue((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX, X, 0);
					}
					else
					{
						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX), &X);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						
						X = MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX, X);
					}

					MONKEY_Unprepare(X, 0, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
						MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferY)), &m_APEDecompress->m_nCRC);

					MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
					MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferY), m_APEFileInfo->nBytesPerSample);
                }
            }    
            else
            {
                if (m_APEFileInfo->nVersion >= 3950)
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
						int nY, nX, Y, X;

						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateY), &nY);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						
						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX), &nX);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						

						Y = MONKEY_CPredictorDecompress3950toCurrent_DecompressValue((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorY, nY, m_APEDecompress->m_nLastX);
						X = MONKEY_CPredictorDecompress3950toCurrent_DecompressValue((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX, nX, Y);
						m_APEDecompress->m_nLastX = X;

						MONKEY_Unprepare(X, Y, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
							MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferY)), &m_APEDecompress->m_nCRC);

						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferY), m_APEFileInfo->nBytesPerSample);
                    }
                }
                else
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int X, Y;
	
						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX),&X);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;

						X = MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX, X);

						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateY), &Y);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;

						Y = MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorY, Y);
	
						MONKEY_Unprepare(X, Y, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
							MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferY)), &m_APEDecompress->m_nCRC);

						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferY), m_APEFileInfo->nBytesPerSample);      
                    }
                }
            }
        }
        else
        {
            if (m_APEDecompress->m_nSpecialCodes & SPECIAL_FRAME_MONO_SILENCE)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
					MONKEY_Unprepare(0, 0, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
							0, &m_APEDecompress->m_nCRC);

					MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
                }
            }
            else
            {
				if (m_APEFileInfo->nVersion >= 3950)
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
						int X;

						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX), &X);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						
						X = MONKEY_CPredictorDecompress3950toCurrent_DecompressValue((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX, X, 0);

						MONKEY_Unprepare(X, 0, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
								0, &m_APEDecompress->m_nCRC);

						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);
                    }
                }
                else
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int X;
	
						nRetVal = MONKEY_CUnBitArray_DecodeValueRange(&(m_APEDecompress->m_spUnBitArray), &(m_APEDecompress->m_BitArrayStateX),&X);
						if (nRetVal != ERROR_SUCCESS)
							return nRetVal;
						
						X = MONKEY_CPredictorDecompressNormal3930to3950_DecompressValue((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX, X);

						MONKEY_Unprepare(X, 0, (void*)m_APEFileInfo, MONKEY_CircleBuffer_GetDirectWritePointer(&(m_APEDecompress->m_cbFrameBufferX)), 
								0, &m_APEDecompress->m_nCRC);

						MONKEY_CircleBuffer_UpdateAfterDirectWrite(&(m_APEDecompress->m_cbFrameBufferX), m_APEFileInfo->nBytesPerSample);           
                    }
                }
            }
        }

    m_APEDecompress->m_nCurrentFrameBufferBlock += nBlocks;
	return nRetVal;
}

static int MONKEY_APEDecompress_StartFrame(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo)
{
	int ret = ERROR_SUCCESS;
	m_APEDecompress->m_nCRC = 0xFFFFFFFF;
    
    // get the frame header
    ret = MONKEY_CUnBitArray_DecodeValue(&(m_APEDecompress->m_spUnBitArray), DECODE_VALUE_METHOD_UNSIGNED_INT, 
    			(unsigned int *)&(m_APEDecompress->m_nStoredCRC));
	if (ret != ERROR_SUCCESS)
		return ret;
	
    m_APEDecompress->m_bErrorDecodingCurrentFrame = 0;

    // get any 'special' codes if the file uses them (for silence, FALSE stereo, etc.)
    m_APEDecompress->m_nSpecialCodes = 0;

    if (m_APEFileInfo->nVersion > 3820)
    {
        if (m_APEDecompress->m_nStoredCRC & 0x80000000) 
        {
			ret = MONKEY_CUnBitArray_DecodeValue(&(m_APEDecompress->m_spUnBitArray), DECODE_VALUE_METHOD_UNSIGNED_INT, 
    			(unsigned int *)&(m_APEDecompress->m_nSpecialCodes));
			if (ret != ERROR_SUCCESS)
				return ret;
		}
        m_APEDecompress->m_nStoredCRC &= 0x7FFFFFFF;
    }

	if (m_APEFileInfo->nVersion >= 3950) {
		MONKEY_CPredictorDecompress3950toCurrent_Flush((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorX);
		MONKEY_CPredictorDecompress3950toCurrent_Flush((CPredictorDecompress3950toCurrent *)m_APEDecompress->m_spNewPredictorY);		
	} else {
		MONKEY_CPredictorDecompressNormal3930to3950_Flush((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorX);
		MONKEY_CPredictorDecompressNormal3930to3950_Flush((CPredictorDecompressNormal3930to3950 *)m_APEDecompress->m_spNewPredictorY);
	}

//    m_spNewPredictorX->Flush();
//    m_spNewPredictorY->Flush(); 

    MONKEY_CUnBitArray_FlushState(&(m_APEDecompress->m_BitArrayStateX));
    MONKEY_CUnBitArray_FlushState(&(m_APEDecompress->m_BitArrayStateY));

    MONKEY_CUnBitArray_FlushBitArray(&(m_APEDecompress->m_spUnBitArray));

    m_APEDecompress->m_nLastX = 0;

	return ret;
}

static void MONKEY_APEDecompress_EndFrame(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo)
{
    m_APEDecompress->m_nCurrentFrame++;

    // finalize
    MONKEY_CUnBitArray_Finalize(&(m_APEDecompress->m_spUnBitArray));

    // check the CRC
    m_APEDecompress->m_nCRC = m_APEDecompress->m_nCRC ^ 0xFFFFFFFF;
    m_APEDecompress->m_nCRC >>= 1;
    if (m_APEDecompress->m_nCRC != m_APEDecompress->m_nStoredCRC)
        m_APEDecompress->m_bErrorDecodingCurrentFrame = 1;
}

int MONKEY_APEDecompress_Dec_sprd(MONKEY_APE_DEC_HANDLE * m_APEDecompress, MONKEY_APE_DEC_CONFIG * m_APEFileInfo, int64_t TimeUs, char remainder, MONKEY_APE_DEC_DATA *input,MONKEY_APE_DEC_DATA *output)
{
	int nBaseFrame, nBlocksToSkip, nBytesToSkip;
	char *TempBuffer;
	int nBlocksRetrived;
	int nSeekRemainder;
	int remain_bytes;
	int nBlockOffset;

	
 	if(m_APEDecompress->m_nCurrentTimeUs != TimeUs)
	{
            nBlockOffset=(int)(((float)TimeUs/m_APEFileInfo->nSampleRate/1000000));
	     nBlockOffset=(nBlockOffset+512)&0xfffffc00;
	     nBlockOffset+=m_APEDecompress->m_nStartBlock;

		nBaseFrame=nBlockOffset/m_APEFileInfo->nBlocksPerFrame;
		nBlocksToSkip=nBlockOffset%m_APEFileInfo->nBlocksPerFrame;
		nBytesToSkip=nBlocksToSkip*m_APEFileInfo->nBytesPerSample*m_APEFileInfo->nChannels;
		//ALOGV("APEDecompress ----------------------------------begin");
		m_APEDecompress->m_nCurrentBlock=nBaseFrame*m_APEFileInfo->nBlocksPerFrame;
		m_APEDecompress->m_nCurrentFrameBufferBlock=nBaseFrame*m_APEFileInfo->nBlocksPerFrame;
		m_APEDecompress->m_nCurrentFrame=nBaseFrame;
		m_APEDecompress->nBlocksToOutput=m_APEDecompress->m_nFinishBlock-nBaseFrame*m_APEFileInfo->nBlocksPerFrame;
		m_APEDecompress->m_cbFrameBufferX.m_nHead=0;
		m_APEDecompress->m_cbFrameBufferX.m_nTail=0;
		m_APEDecompress->m_cbFrameBufferX.m_nEndCap=m_APEDecompress->m_cbFrameBufferX.m_nTotal;
		m_APEDecompress->m_cbFrameBufferY.m_nHead=0;
		m_APEDecompress->m_cbFrameBufferY.m_nTail=0;
		m_APEDecompress->m_cbFrameBufferY.m_nEndCap=m_APEDecompress->m_cbFrameBufferY.m_nTotal;
		m_APEDecompress->m_spUnBitArray.m_nBytes=0;
		m_APEDecompress->m_spUnBitArray.m_nCurrentBitIndex=0;
		m_APEDecompress->m_spUnBitArray.m_nCurrentBitIndex=remainder<<3;
	
		while(nBlocksToSkip>0){
			//ALOGV("APEDecompress ----------------------------------ing");
			m_APEDecompress->m_spUnBitArray.Input=input;
			MONKEY_APEDecompress_Dec(m_APEDecompress, m_APEFileInfo, input, output);
			nBlocksToSkip-=1024;
			output->nLen=0;
			output->counter=0;
		}
 		//ALOGV("APEDecompress ----------------------------------end");
		m_APEDecompress->m_nCurrentTimeUs = TimeUs;
	}
 	MONKEY_APEDecompress_Dec(m_APEDecompress, m_APEFileInfo, input, output);
	return ERROR_SUCCESS;
}