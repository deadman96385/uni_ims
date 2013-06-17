/******************************************************************************
 ** File Name:    h264dec_bitstream.c                                         *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

uint32 BitstreamReadBits (DEC_BS_T * stream, int32 nbits)
{
	uint32 temp;

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
	//l = BitstreamShowBits (stream, nbits);
	temp = BITSTREAMSHOWBITS(stream, nbits);
	BITSTREAMFLUSHBITS(stream, nbits);

	return temp;	
}	
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream)
{
	
	uint32 ret;
    uint32 tmp;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 1,"BSM_rd_UE cmd");	
	tmp =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_UE check error");
    if(tmp&4)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	ret =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_UE dara");
	
	return ret;
}

PUBLIC int32 READ_SE_V (DEC_BS_T * stream)
{
	int32 ret;
	
	int32 tmp;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 2,"BSM_rd_SE cmd");	
	tmp =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_SE check error");
    if(tmp&4)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	ret =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_SE dara");
	

	return ret;
}

PUBLIC int32 H264Dec_Long_UEV (DEC_BS_T * stream)
{
	uint32 tmp;
	int32 leading_zero = 0;
	int32 ret;
	
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
	tmp = SHOW_FLC (stream, 16);
	
	if (tmp == 0)
	{
		READ_FLC (stream, 16);
		leading_zero = 16;
		
		do {
			tmp = READ_FLC (stream, 1);
			leading_zero++;
			
			if (leading_zero > 32)//weihu ?//>=16
			{
				g_image_ptr->error_flag = TRUE;
				return 0;
			}
		} while(!tmp);
		
		leading_zero--;
		tmp = READ_FLC (stream, leading_zero);		
		
		ret = (1 << leading_zero) + tmp - 1;
		
			
		return ret;
	}else
	{
		return READ_UE_V (stream);
	}
}

PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream)
{
	uint32 tmp;
	int32 leading_zero = 0;
	int32 ret;

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
	tmp = SHOW_FLC (stream, 16);

	if (tmp == 0)
	{
		READ_FLC (stream, 16);
		leading_zero = 16;

		do {
			tmp = READ_FLC (stream, 1);
			leading_zero++;

			if (leading_zero > 32)//weihu ?//>=16
			{
				g_image_ptr->error_flag = TRUE;
				return 0;
			}
		} while(!tmp);

		leading_zero--;
		tmp = READ_FLC (stream, leading_zero);


		tmp = (1 << leading_zero) + tmp - 1;
		
		ret = (tmp + 1) / 2;
		
		if ( (tmp & 1) == 0 )
			ret = -ret;

		return ret;
	}else
	{
		return READ_SE_V (stream);
	}
}

PUBLIC void H264Dec_flush_left_byte (void)
{
	int32 i;
	int32 dec_len;
	int32 left_bytes;
	uint32 nDecTotalBits;// = stream->bitcnt;
	//DEC_BS_T * stream;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 8,"BSM byte align");
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
	nDecTotalBits =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF,"BSM flushed bits cnt");
	//bytealign
	
	

	dec_len = nDecTotalBits>>3;

	left_bytes = g_nalu_ptr->len - dec_len;

	for (i = 0; i < left_bytes; i++)
	{
        READ_FLC(NULL, 8);
	}

	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

