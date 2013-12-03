/*************************************************************************
** File Name:      sbr_fbt.c                                             *
** Author:         Reed zhang                                            *
** Date:           09/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    this file is use to Calculate frequency band tables
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_common.h"
#ifdef AAC_SBR_DEC

#include "AAC_sbr_syntax.h"
#include "AAC_sbr_fbt.h"
#define REAL_PRECISION (1 << 14)

extern const uint8 sbr_start_minTable[12];
extern const uint8 sbr_start_offsetIndexTable[12];
extern const int8 sbr_start_offset[7][16];
extern const uint8 sbr_stop_minTable[12];
extern const int8 sbr_stop_offset[12][14];
extern const uint16 INV_tab[];
extern const uint16 sbr_log2Table[];	
extern const uint16 limiterBandsCompare[];
extern const uint8 temp1[];
/* static function declarations */
static int32 AAC_FindBands(uint8 warp, 
						  uint8 bands,
						  uint8 a0,
						  uint8 a1);
static int32 AAC_SortData(int32 *data_ptr, int16 len);

/* calculate the start QMF channel for the master frequency band table */
/* parameter is also called k0 */
uint8 AAC_QmfStartChannel(uint8 bs_start_freq, 
						  uint8 bs_samplerate_mode,
                          uint8 sr_index)
{    
    uint8 startMin    = sbr_start_minTable[sr_index];
    uint8 offsetIndex = sbr_start_offsetIndexTable[sr_index];
    if (bs_samplerate_mode)
    {
        return startMin + sbr_start_offset[offsetIndex][bs_start_freq];
    } else {
        return startMin + sbr_start_offset[6][bs_start_freq];
    }
}
#ifdef __VC_ENVIRONMENT__
static int AAC_LongCmp(void *a,
				       void *b)
{
    return ((int)(*(int32*)a - *(int32*)b));
}
#endif

/* calculate the stop QMF channel for the master frequency band table */
/* parameter is also called k2 */
uint8 AAC_QmfStopChannel(uint8 bs_stop_freq,
						 uint8 sr_index,
                         uint8 k0)
{
    if (bs_stop_freq == 15)
    {
        return AAC_DEC_MIN(64, k0 * 3);
    } else if (bs_stop_freq == 14) {
        return AAC_DEC_MIN(64, k0 * 2);
    } else 
	{       
        uint8 stopMin = sbr_stop_minTable[sr_index];
        /* bs_stop_freq <= 13 */
        return AAC_DEC_MIN(64, stopMin + sbr_stop_offset[sr_index][AAC_DEC_MIN(bs_stop_freq, 13)]);
    }

    //return 0;
}

/* calculate the master frequency table from k0, k2, bs_freq_scale
   and bs_alter_scale

   version for bs_freq_scale = 0
*/
uint8 AAC_MasterFrequencyTable_fs0(AAC_SBR_INFO_T *sbr_ptr, 
								   uint8 k0, 
								   uint8 k2,
                                   uint8 bs_alter_scale,
								   int32 *tmp_buf_ptr)
{
    int8 incr;
    uint8 k;
    uint8 dk;
    uint32 nrBands, k2Achieved;
    int32 k2Diff;
    int32 *vDk = tmp_buf_ptr;//g_tmp_buffer_ptr;

    /* mft only defined for k2 > k0 */
    if (k2 <= k0)
    {
        sbr_ptr->N_master = 0;
        return 0;
    }

    dk = bs_alter_scale ? 2 : 1;

    if (bs_alter_scale)
    {
        nrBands = (((k2-k0+2)>>2)<<1);
    } else {
        nrBands = (((k2-k0)>>1)<<1);
    }

    nrBands    = AAC_DEC_MIN(nrBands, 63);
    k2Achieved = k0 + nrBands * dk;
    k2Diff     = k2 - k2Achieved;
    for (k = 0; k < nrBands; k++)
        vDk[k] = dk;

    if (k2Diff)
    {
        incr = (k2Diff > 0) ? -1 : 1;
        k = (uint8)((k2Diff > 0) ? (nrBands-1) : 0);
        while (k2Diff != 0)
        {
            vDk[k] -= incr;
            k += incr;
            k2Diff += incr;
        }
    }

    sbr_ptr->f_master[0] = k0;
    for (k = 1; k <= nrBands; k++)
        sbr_ptr->f_master[k] =(uint8)(sbr_ptr->f_master[k-1] + vDk[k-1]);

    sbr_ptr->N_master = (uint8)nrBands;
    sbr_ptr->N_master = AAC_DEC_MIN(sbr_ptr->N_master, 64);
    return 0;
}

/*
   This function finds the number of bands using this formula:
    bands * log(a1/a0)/log(2.0) + 0.5
*/
extern  const int32 log2_int_tab[];
static int32 AAC_FindBands(uint8 warp,
						  uint8 bands,
						  uint8 a0, 
						  uint8 a1)
{
    int32 r0 = log2_int_tab[a0]; /* coef */
    int32 r1 = log2_int_tab[a1]; /* coef */
    int32 r2 = (r1 - r0); /* coef */
    if (warp)
        r2 = AAC_DEC_MultiplyShiftR32( r2<<4, 206488812);//MUL_C(r2, COEF_CONST(1.0/1.3));
	
    /* convert r2 to real and then multiply and round */
    r2 = (r2) * bands + (1<<(14-1));
	
    return (r2 >> 14);
}


/************************************************************************/
// the function find_initial_power1() is used to calculate the formula is shown below:
// output = (a1 / a0) ^ (1 / bands).
/* standard Taylor polynomial coefficients for exp(x) around 0 */
/* a polynomial around x=1 is more precise, as most values are around 1.07,
   but this is just fine already */
/************************************************************************/
static int32 AAC_FindInitialPower(uint8 bands,
								 uint8 a0,
								 uint8 a1)
{
    int32 c1 = 268435456;  // Q0.28
    int32 c2 = 134217728;   // Q0.28
    int32 c3 = 44739243;   // Q0.28
    int32 c4 = 11184811;    // Q0.28
    int32 med_var;
    int32 rexp;
    int32 r0 = log2_int_tab[a0]; /* coef */   // Q0.14
    int32 r1 = log2_int_tab[a1]; /* coef */   // Q0.14
    int32 r2 = (r1 - r0) * INV_tab[bands-1] >> 12;

    r2           = AAC_DEC_MultiplyShiftR32(r2<<4, 186065280);	
    med_var = AAC_DEC_MultiplyShiftR32(c4<<4,  r2); // Q0.28

    med_var = (med_var) + c3;
    med_var = (int32)((int64)med_var * r2 >> 14);
    med_var = med_var + c2;
    med_var = (int32)((int64)med_var * r2 >> 14);
    med_var = med_var + c1;
    med_var = (int32)((int64)med_var * r2 >> 14);
    rexp    = c1 + med_var + 8192;
    return rexp >> 14; /* real */
}


uint8 AAC_MasterFrequencyTable(AAC_SBR_INFO_T *sbr_ptr, 
                                                       uint8 k0,
                                                       uint8 k2,
                                                       uint8 bs_freq_scale, 
                                                       uint8 bs_alter_scale,
                                                       int32 *tmp_buf_ptr)
{
#define BIT 6	
    int32 *vDk0 = tmp_buf_ptr;//g_tmp_buffer_ptr;
    int32 *vDk1 = tmp_buf_ptr + 64;//g_tmp_buffer_ptr + 64;
    int32 *vk0  = tmp_buf_ptr + 128;//g_tmp_buffer_ptr + 128;
    int32 *vk1  = tmp_buf_ptr + 192;//g_tmp_buffer_ptr + 192;
	
    uint8 k, bands, twoRegions;
    uint8 k1;
    uint8 nrBand0, nrBand1;    
    int32  q, qk;
    int32 A_1;
    /* mft only defined for k2 > k0 */
    if (k2 <= k0)
    {
        sbr_ptr->N_master = 0;
        return 0;
    }
    bands = temp1[bs_freq_scale-1];
    q     = (int32)k2 * REAL_PRECISION;
    qk   = (int32)k0 * 36780;
    if (q > qk)
    {
        twoRegions = 1;
        k1 = k0 << 1;
    } else {
        twoRegions = 0;
        k1 = k2;
    }
    nrBand0 = 2 * (uint8)(AAC_FindBands(0, bands, k0, k1));
    nrBand0 = AAC_DEC_MIN(nrBand0, 63);
    q   = AAC_FindInitialPower(nrBand0, k0, k1);  // q: Q0.10
    qk  = k0 << 14 ; // qk: Q0.BIT
    A_1 = (int32)(k0);

    for (k = 0; k <= nrBand0; k++)
    {
        int32 A_0 = A_1;
        qk      = (int32)(((int64)qk*q) >> 14);
        A_1     = (int32)((qk + 8192) >> 14);
        vDk0[k] = A_1 - A_0;
    }
    /* needed? */
    //qsort(vDk0, nrBand0, sizeof(vDk0[0]), AAC_LongCmp);   // ÅÅÐòº¯Êý
    AAC_SortData(vDk0, nrBand0);
    vk0[0] = k0;
    for (k = 1; k <= nrBand0; k++)
    {
        vk0[k] = vk0[k-1] + vDk0[k-1];
        if (vDk0[k-1] == 0)
            return 1;
    }
    if (!twoRegions)
    {
        for (k = 0; k <= nrBand0; k++)
            sbr_ptr->f_master[k] =(uint8) vk0[k];
        sbr_ptr->N_master = AAC_DEC_MIN(nrBand0, 64);
        return 0;
    }

    nrBand1 = (uint8) (2 * AAC_FindBands(1 /* warped */, bands, k1, k2));
    nrBand1 = AAC_DEC_MIN(nrBand1, 63);

    q   = AAC_FindInitialPower(nrBand1, k1, k2); // q: Q0.14
    qk  = k1 << 14;  // Q6.10
    A_1 = k1;
    for (k = 0; k <= nrBand1 - 1; k++)
    {
        int32 A_0 = A_1;
        qk      = (int32)((int64)qk * q >> 14);
        A_1     = (int32)((qk + 8192) >> 14);
        vDk1[k] = A_1 - A_0;
    }

    if (vDk1[0] < vDk0[nrBand0 - 1])
    {
        int32 change;

        /* needed? */
        //qsort(vDk1, nrBand1 + 1, sizeof(vDk1[0]), AAC_LongCmp);
		AAC_SortData(vDk1, (int16)(nrBand1 + 1));
        change  = vDk0[nrBand0 - 1] - vDk1[0];
        vDk1[0] = vDk0[nrBand0 - 1];
        vDk1[nrBand1 - 1] = vDk1[nrBand1 - 1] - change;
    }

    /* needed? */
    //qsort(vDk1, nrBand1, sizeof(vDk1[0]), AAC_LongCmp);
	AAC_SortData(vDk1, nrBand1);
    vk1[0] = k1;
    for (k = 1; k <= nrBand1; k++)
    {
        vk1[k] = vk1[k-1] + vDk1[k-1];
        if (vDk1[k-1] == 0)
            return 1;
    }

    sbr_ptr->N_master = nrBand0 + nrBand1;
    sbr_ptr->N_master = AAC_DEC_MIN(sbr_ptr->N_master, 64);
    for (k = 0; k <= nrBand0; k++)
    {
        sbr_ptr->f_master[k] = (uint8)vk0[k];
    }
    for (k = nrBand0 + 1; k <= sbr_ptr->N_master; k++)
    {
        sbr_ptr->f_master[k] = (uint8)vk1[k - nrBand0];
    }
    return 0;
}

/* calculate the derived frequency border tables from f_master */
uint8 AAC_DerivedFrequencyTable(AAC_SBR_INFO_T *sbr_ptr, 
								uint8 bs_xover_band,
                                uint8 k2)
{
    uint8  k, i=0;
    uint32 minus;

	uint8 N_master = sbr_ptr->N_master;
	uint8 N_high;
	uint8 N_low;

    /* The following relation shall be satisfied: bs_xover_band < N_Master */
    if (N_master <= bs_xover_band)
        return 1;

    N_high      = sbr_ptr->N_master - bs_xover_band;
	sbr_ptr->N_high = N_high;
    N_low       = (N_high>>1) + (N_high - ((N_high>>1)<<1));
	sbr_ptr->N_low  = N_low;
    sbr_ptr->n[0]   = N_low;
    sbr_ptr->n[1]   = N_high;

    for (k = 0; k <= N_high; k++)
    {
        sbr_ptr->f_table_res[AAC_HI_RES][k] = sbr_ptr->f_master[k + bs_xover_band];
    }

    sbr_ptr->M  = sbr_ptr->f_table_res[AAC_HI_RES][N_high] - sbr_ptr->f_table_res[AAC_HI_RES][0];
    sbr_ptr->kx = sbr_ptr->f_table_res[AAC_HI_RES][0];
    if (sbr_ptr->kx > 32)
        return 1;
    if (sbr_ptr->kx + sbr_ptr->M > 64)
        return 1;

    minus = (N_high & 1) ? 1 : 0;

    for (k = 0; k <= N_low; k++)
    {
        if (k == 0)
            i = 0;
        else
            i = (uint8)(2*k - minus);
        sbr_ptr->f_table_res[AAC_LO_RES][k] = sbr_ptr->f_table_res[AAC_HI_RES][i];
    }
    sbr_ptr->N_Q = 0;
    if (sbr_ptr->bs_noise_bands == 0)
    {
        sbr_ptr->N_Q = 1;
    } else {

        sbr_ptr->N_Q = (uint8)(AAC_DEC_MAX(1, AAC_FindBands(0, sbr_ptr->bs_noise_bands, sbr_ptr->kx, k2)));
        sbr_ptr->N_Q = AAC_DEC_MIN(5, sbr_ptr->N_Q);
    }

	N_low = sbr_ptr->N_Q;
    for (k = 0; k <= N_low; k++)
    {
        if (k == 0)
        {
            i = 0;
        } else 
		{
			uint8 tmp;
			tmp = sbr_ptr->N_Q - k;
			if (tmp)
			{
				i = i + ((sbr_ptr->N_low - i) * INV_tab[tmp] >> AAC_INV_TAB_BIT);
			}
			else
			{
				i = sbr_ptr->N_low;
			}
        }
        sbr_ptr->f_table_noise[k] = sbr_ptr->f_table_res[AAC_LO_RES][i];
    }

    /* build table for mapping k to g in hf patching */
    for (k = 0; k < 64; k++)
    {
        uint8 g;
        for (g = 0; g < N_low; g++)
        {
            if ((sbr_ptr->f_table_noise[g] <= k) &&
                (k < sbr_ptr->f_table_noise[g+1]))
            {
                sbr_ptr->table_map_k_to_g[k] = g;
                break;
            }
        }
    }
    return 0;
}

/* TODO: blegh, ugly */
/* Modified to calculate for all possible bs_limiter_bands always
 * This reduces the number calls to this functions needed (now only on
 * header reset)
 */
void AAC_LimiterFrequencyTable(AAC_SBR_INFO_T *sbr_ptr,
							   int32 *tmp_buf_ptr)
{

	
	int32 *limTable     = tmp_buf_ptr;//g_tmp_buffer_ptr;
    uint8 *patchBorders = (uint8 (*)) (tmp_buf_ptr + 100);//g_tmp_buffer_ptr
    uint8 k, s;
    int8 nrLim;
	
    sbr_ptr->f_table_lim[0][0] = sbr_ptr->f_table_res[AAC_LO_RES][0] - sbr_ptr->kx;
    sbr_ptr->f_table_lim[0][1] = sbr_ptr->f_table_res[AAC_LO_RES][sbr_ptr->N_low] - sbr_ptr->kx;
    sbr_ptr->N_L[0] = 1;

    for (s = 1; s < 4; s++)
    {	
        patchBorders[0] = sbr_ptr->kx;
        for (k = 1; k <= sbr_ptr->noPatches; k++)
        {
            patchBorders[k] = patchBorders[k-1] + sbr_ptr->patchNoSubbands[k-1];
        }

        for (k = 0; k <= sbr_ptr->N_low; k++)
        {
            limTable[k] = sbr_ptr->f_table_res[AAC_LO_RES][k];
        }
        for (k = 1; k < sbr_ptr->noPatches; k++)
        {
            limTable[k+sbr_ptr->N_low] = patchBorders[k];
        }

        /* needed */
        //qsort(limTable, sbr_ptr->noPatches + sbr_ptr->N_low, sizeof(limTable[0]), AAC_LongCmp);
		AAC_SortData(limTable, (int16)(sbr_ptr->noPatches + sbr_ptr->N_low));
        k = 1;
        nrLim = sbr_ptr->noPatches + sbr_ptr->N_low - 1;

        if (nrLim < 0)  // TODO: BIG FAT PROBLEM
            return;

restart:
        if (k <= nrLim)
        {
            int32 nOctaves;

            if (limTable[k-1] != 0)
			{                

				nOctaves = limTable[k] * 16384 / limTable[k-1];
			}
            else
                nOctaves = 0;

            if (nOctaves < limiterBandsCompare[s - 1])
            {
                uint8 i;
                if (limTable[k] != limTable[k-1])
                {
                    uint8 found = 0, found2 = 0;
                    for (i = 0; i <= sbr_ptr->noPatches; i++)
                    {
                        if (limTable[k] == patchBorders[i])
                            found = 1;
                    }
                    if (found)
                    {
                        found2 = 0;
                        for (i = 0; i <= sbr_ptr->noPatches; i++)
                        {
                            if (limTable[k-1] == patchBorders[i])
                                found2 = 1;
                        }
                        if (found2)
                        {
                            k++;
                            goto restart;
                        } else {
                            /* remove (k-1)th element */
                            limTable[k-1] = sbr_ptr->f_table_res[AAC_LO_RES][sbr_ptr->N_low];
                            //qsort(limTable, sbr_ptr->noPatches + sbr_ptr->N_low, sizeof(limTable[0]), AAC_LongCmp);
							AAC_SortData(limTable, (int16) (sbr_ptr->noPatches + sbr_ptr->N_low));
                            nrLim--;
                            goto restart;
                        }
                    }
                }
                /* remove kth element */
                limTable[k] = sbr_ptr->f_table_res[AAC_LO_RES][sbr_ptr->N_low];
                //qsort(limTable, nrLim, sizeof(limTable[0]), AAC_LongCmp);
				AAC_SortData(limTable, nrLim);
                nrLim--;
                goto restart;
            } else {
                k++;
                goto restart;
            }
        }

        sbr_ptr->N_L[s] = nrLim;
        for (k = 0; k <= nrLim; k++)
        {
            sbr_ptr->f_table_lim[s][k] = limTable[k] - sbr_ptr->kx;
        }
    }
}


static int32 AAC_SortData(int32 *data_ptr, int16 len)
{
	int16 i, j;

	for (i = 0; i < len; i++)
	{
		for(j = i+1; j < len; j++)
		{
			int32 t1, t2;
			t1 = data_ptr[i];
			t2 = data_ptr[j];
			if (t1 > t2)
			{
				data_ptr[j] = t1;
				data_ptr[i] = t2;
			}
		}
	}
	return 0;
}
#endif
