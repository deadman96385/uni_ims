/*************************************************************************
** File Name:      ps_syntax.c                                           *
** Author:         Reed zhang                                            *
** Date:           18/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR signal analyzing                          *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 18/04/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#ifdef AAC_PS_DEC
#include "aac_bits.h"
#include "AAC_ps_dec.h"

/* type definitaions */
typedef const int8 (*ps_huff_tab)[2];

extern  const uint8 nr_iid_par_tab[];
extern  const uint8 nr_ipdopd_par_tab[];
extern  const uint8 nr_icc_par_tab[];
extern  const uint8 num_env_tab[][4];
extern  const int8 f_huff_iid_def[][2];
extern  const int8 t_huff_iid_def[][2];
extern  const int8 f_huff_iid_fine[][2];
extern  const int8 t_huff_iid_fine[][2];
extern  const int8 f_huff_icc[][2];
extern  const int8 t_huff_icc[][2];
extern  const int8 f_huff_ipd[][2];
extern  const int8 t_huff_ipd[][2];

/* static function declarations */
static uint16 AAC_PsExtension(AAC_PS_INFO_T *ps_ptr, AAC_BIT_FIFO_FORMAT_T *ld_ptr,
                             const uint8 ps_extension_id,
                             const uint16 num_bits_left);
static void AAC_HuffData(AAC_BIT_FIFO_FORMAT_T *ld_ptr, const uint8 dt, const uint8 nr_par,
                      ps_huff_tab t_huff, ps_huff_tab f_huff, int8 *par_ptr);
static   int8 AAC_PsHuffDec(AAC_BIT_FIFO_FORMAT_T *ld_ptr, ps_huff_tab t_huff);


uint16 AAC_PsData(AAC_PS_INFO_T *ps_ptr, AAC_BIT_FIFO_FORMAT_T *ld_ptr, uint8 *header_ptr)
{
    uint8 tmp, n;
    uint16 bits = (uint16)AAC_GetProcessedBits(ld_ptr);

    *header_ptr = 0;
    
    /* check for new ps_ptr header_ptr */
    if (AAC_Get1Bit(ld_ptr))
    {
        *header_ptr = 1;

        ps_ptr->header_read = 1;

        ps_ptr->use34hybrid_bands = 0;

        /* Inter-channel Intensity Difference (IID) parameters enabled */
        ps_ptr->enable_iid = (uint8)AAC_Get1Bit(ld_ptr);

        if (ps_ptr->enable_iid)
        {
            ps_ptr->iid_mode = (uint8)AAC_GetBits(ld_ptr, 3);

            ps_ptr->nr_iid_par = nr_iid_par_tab[ps_ptr->iid_mode];
            ps_ptr->nr_ipdopd_par = nr_ipdopd_par_tab[ps_ptr->iid_mode];

            if (ps_ptr->iid_mode == 2 || ps_ptr->iid_mode == 5)
                ps_ptr->use34hybrid_bands = 1;

            /* IPD freq res equal to IID freq res */
            ps_ptr->ipd_mode = ps_ptr->iid_mode;
        }

        /* Inter-channel Coherence (ICC) parameters enabled */
        ps_ptr->enable_icc = (uint8)AAC_Get1Bit(ld_ptr);

        if (ps_ptr->enable_icc)
        {
            ps_ptr->icc_mode = (uint8)AAC_GetBits(ld_ptr, 3);

            ps_ptr->nr_icc_par = nr_icc_par_tab[ps_ptr->icc_mode];

            if (ps_ptr->icc_mode == 2 || ps_ptr->icc_mode == 5)
                ps_ptr->use34hybrid_bands = 1;
        }

        /* ps_ptr extension layer enabled */
        ps_ptr->enable_ext = (uint8)AAC_Get1Bit(ld_ptr);
    }

    /* we are here, but no header_ptr has been read yet */
    if (ps_ptr->header_read == 0)
    {
        return 1;
    }

    ps_ptr->frame_class = (uint8)AAC_Get1Bit(ld_ptr);
    tmp = (uint8)AAC_GetBits(ld_ptr, 2);

    ps_ptr->num_env = num_env_tab[ps_ptr->frame_class][tmp];

    if (ps_ptr->num_env > AAC_PS_MAX_PS_ENVELOPES)
    {
	return 1;
    }

    if (ps_ptr->frame_class)
    {
        for (n = 1; n < ps_ptr->num_env+1; n++)
        {
            ps_ptr->border_position[n] = (uint8) (AAC_GetBits(ld_ptr, 5) + 1);
        }
    }

    if (ps_ptr->enable_iid)
    {
        for (n = 0; n < ps_ptr->num_env; n++)
        {
            ps_ptr->iid_dt[n] = (uint8)AAC_Get1Bit(ld_ptr);

            /* iid_data */
            if (ps_ptr->iid_mode < 3)
            {
                AAC_HuffData(ld_ptr, ps_ptr->iid_dt[n], ps_ptr->nr_iid_par, t_huff_iid_def,
                    f_huff_iid_def, ps_ptr->iid_index[n]);
            } else {
                AAC_HuffData(ld_ptr, ps_ptr->iid_dt[n], ps_ptr->nr_iid_par, t_huff_iid_fine,
                    f_huff_iid_fine, ps_ptr->iid_index[n]);
            }
        }
    }

    if (ps_ptr->enable_icc)
    {
        for (n = 0; n < ps_ptr->num_env; n++)
        {
            ps_ptr->icc_dt[n] = (uint8)AAC_Get1Bit(ld_ptr);

            /* icc_data */
            AAC_HuffData(ld_ptr, ps_ptr->icc_dt[n], ps_ptr->nr_icc_par, t_huff_icc,
                f_huff_icc, ps_ptr->icc_index[n]);
        }
    }


    if (ps_ptr->enable_ext)
    {
        int16 num_bits_left;
        uint16 cnt = (uint16)AAC_GetBits(ld_ptr, 4);
        if (cnt == 15)
        {
            cnt = (uint16) (cnt+ AAC_GetBits(ld_ptr, 8));
        }

        num_bits_left = (int16) (8 * cnt);
        while (num_bits_left > 7)
        {
            uint8 ps_extension_id = (uint8)AAC_GetBits(ld_ptr, 2);

            num_bits_left = (int16) (num_bits_left-2);
            num_bits_left = (int16) (num_bits_left - AAC_PsExtension(ps_ptr, ld_ptr, ps_extension_id, (const uint16)num_bits_left));
        }

        AAC_GetBits(ld_ptr, num_bits_left);
    }

    bits = (uint16) (AAC_GetProcessedBits(ld_ptr) - bits);

    ps_ptr->ps_data_available = 1;

    return bits;
}

static uint16 AAC_PsExtension(AAC_PS_INFO_T *ps_ptr, AAC_BIT_FIFO_FORMAT_T *ld_ptr,
                             const uint8 ps_extension_id,
                             const uint16 num_bits_left)
{
    uint16 bits = (uint16)AAC_GetProcessedBits(ld_ptr);

    if (ps_extension_id == 0)
    {
        ps_ptr->enable_ipdopd = (uint8)AAC_Get1Bit(ld_ptr);




        AAC_Get1Bit(ld_ptr);
    }

    /* return number of bits read */
    bits = (uint16) (AAC_GetProcessedBits(ld_ptr) - bits);

    return bits;
}

/* read huffman data coded in either the frequency or the time direction */
static void AAC_HuffData(AAC_BIT_FIFO_FORMAT_T      *ld_ptr,
						 const uint8 dt, 
						 const uint8 nr_par,
                                           ps_huff_tab   t_huff, 
						 ps_huff_tab   f_huff, 
						 int8       *par_ptr)
{
    uint8 n;
    if (dt)
    {
        /* coded in time direction */
        for (n = 0; n < nr_par; n++)
        {
            par_ptr[n] = AAC_PsHuffDec(ld_ptr, t_huff);
        }
    } else {
        /* coded in frequency direction */
        par_ptr[0] = AAC_PsHuffDec(ld_ptr, f_huff);
        for (n = 1; n < nr_par; n++)
        {
            par_ptr[n] = AAC_PsHuffDec(ld_ptr, f_huff);
        }
    }
}
/* binary search huffman decoding */
static   int8 AAC_PsHuffDec(AAC_BIT_FIFO_FORMAT_T *ld_ptr, ps_huff_tab t_huff)
{
    uint8 bit;
    int16 index = 0;
    while (index >= 0)
    {
        bit = (uint8)AAC_Get1Bit(ld_ptr);
        index = t_huff[index][bit];
    }
    return  (int8)(index + 31);
}

#endif
