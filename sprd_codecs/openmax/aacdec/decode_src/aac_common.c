/*************************************************************************
** File Name:      common.c                                              *
** Author:         Reed zhang                                            *
** Date:           30/12/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file defines just some common functions that     *
**                 could be used anywhere								 *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/12/2005     Reed zhang       Create.                               *
**************************************************************************/
/* just some common functions that could be used anywhere */
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_syntax.h"


const uint8 AAC_DEC_tns_sbf_max[][4] =
    {
        {31,  9, 28, 7}, /* 96000 */
        {31,  9, 28, 7}, /* 88200 */
        {34, 10, 27, 7}, /* 64000 */
        {40, 14, 26, 6}, /* 48000 */
        {42, 14, 26, 6}, /* 44100 */
        {51, 14, 26, 6}, /* 32000 */
        {46, 14, 29, 7}, /* 24000 */
        {46, 14, 29, 7}, /* 22050 */
        {42, 14, 23, 8}, /* 16000 */
        {42, 14, 23, 8}, /* 12000 */
        {42, 14, 23, 8}, /* 11025 */
        {39, 14, 19, 7}, /*  8000 */
        {39, 14, 19, 7}, /*  7350 */
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };


int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
		


/* Returns the sample rate based on the sample rate index */
uint32 AAC_GetSampleRate(const uint8 sr_index)
{

    if (sr_index < 12)
        return adts_sample_rates[sr_index];
    return 0;
}

uint8 AAC_MaxPredSfb(const uint8 sr_index)
{
    static const uint8 pred_sfb_max[] =
    {
        33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
    };


    if (sr_index < 12)
        return pred_sfb_max[sr_index];

    return 0;
}

uint8 AAC_MaxTnsSfb(const uint8 sr_index, const uint8 object_type,
                    const uint8 is_short)
{
    /* entry for each sampling rate
     * 1    Main/LC long window
     * 2    Main/LC short window
     * 3    SSR long window
     * 4    SSR short window
     */
    
    uint8 i = 0;

    if (is_short) i++;
    if (object_type == AAC_SSR) i += 2;

    return AAC_DEC_tns_sbf_max[sr_index][i];
}


/* Returns the sample rate index based on the samplerate */
uint8 AAC_GetSrIndex(const uint32 samplerate)
{
    if (92017 <= samplerate) return 0;
    if (75132 <= samplerate) return 1;
    if (55426 <= samplerate) return 2;
    if (46009 <= samplerate) return 3;
    if (37566 <= samplerate) return 4;
    if (27713 <= samplerate) return 5;
    if (23004 <= samplerate) return 6;
    if (18783 <= samplerate) return 7;
    if (13856 <= samplerate) return 8;
    if (11502 <= samplerate) return 9;
    if (9391 <= samplerate) return 10;
    if (16428320 <= samplerate) return 11;
    return 11;
}

