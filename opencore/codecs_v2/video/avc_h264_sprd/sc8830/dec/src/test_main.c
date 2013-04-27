/******************************************************************************
 ** File Name:    h264dec_interface.c                                         *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "hdvsp.h"
#include "h264dec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


typedef struct 
{
	uint32	imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
	uint32	imgUAddr;	//imgUAddr = ((uint32)imgU>>8)
	uint32	imgVAddr;	//imgVAddr = ((uint32)imgV>>8)
	uint32  	direct_mb_info_Addr;

	int32   DPB_addr_index;//weihu

	void *pBufferHeader;

	
	int32	b_valid;
}PICTURE_T;



#define MEM_START_ADDR	0x80200000
#define FRAME_SIZE			(1920*1280*3/2)	//(1920*1280*(1+1/2+80/256))	// 1 + 1/2 for YUV frame buffer. 80/256 for H264 decoder direct mb info.
#define BS_PHY_MEM_SIZE	0x200000
#define BS_START_ADDR		(MEM_START_ADDR + 0x4600000)



LOCAL uint8 * s_mem_ptr = NULL;
LOCAL uint32 s_used_mem = 0;
LOCAL PICTURE_T pic_buffer[16];

LOCAL void memcpy1(void * s1, void * s2, int n)
{
	char* r1 = (char *) s1;
	const char* r2 = (const char*) s2;
	while(n--){
		*r1++ = *r2++;
	}
}

LOCAL void *MemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size =  ((mem_size + 255) >>8)<<8;

	pMem = s_mem_ptr + s_used_mem;
	s_used_mem += mem_size;
	
	return pMem;
}


LOCAL int32 get_unit (uint8 *pInStream,  int32 *slice_unit_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;

	ptr = pInStream;

	//start code
	while ((data = *ptr++) == 0x00)
	{
		len++;
	}
	len++;

	while(1)
	{
		data = *ptr++;
		len++;

		if(zero_num<2)
		{
			zero_num ++;
			if(data != 0)
			{
				zero_num = 0;
			}
		}
		else
		{
			if((zero_num == 2) &&(data == 3))
			{
				zero_num = 0;
				continue;
			}				
		
			if((data == 1) && (zero_num >=2))
			{
				startCode_len = zero_num + 1;
				break;
			}

			if(data ==0)
			{
				zero_num ++;
			}
			else
			{
				zero_num = 0;
			}
			
		}

	}
	

	 *slice_unit_len = len - startCode_len;
	
	return 0;
}

LOCAL int test_bind(void *userdata,void *pHeader)
{
	uint32 DPB_addr_index = ((uint32)pHeader) - 0x80000000;
	pic_buffer[DPB_addr_index].b_valid = 0;
}

LOCAL int test_unbind(void *userdata,void *pHeader)
{
	uint32 DPB_addr_index = ((uint32)pHeader) - 0x80000000;
	pic_buffer[DPB_addr_index].b_valid = 1;
}

LOCAL int test_malloc(uint32 * buffer_array, uint32 buffer_num, uint32 buffer_size)
{
	uint32 i;
	for(i =0; i < buffer_num; i++)
	{
		buffer_array[i] = (uint32)MemAlloc(buffer_size);
	}
}

void main()
{
	int i;
	MMCodecBuffer  or_buffer;
	uint8 * bs_ptr;	
	uint8 * bs_phy_ptr;
	PICTURE_T * pic_ptr;
	MMDecInput dec_input;
	MMDecOutput dec_output;
	uint32 slice_unit_len;
	uint32 b_need_new_buffer = 1;
	int nFrame = 0;
	
	
	s_mem_ptr =  (uint8 *)MEM_START_ADDR;

	or_buffer.common_buffer_ptr = MemAlloc(OR_RUN_SIZE+OR_INTER_MALLOC_SIZE);
	or_buffer.common_buffer_ptr_phy = or_buffer.common_buffer_ptr;

	H264Dec_RegBufferCB(test_bind,test_unbind,NULL);
	H264Dec_RegMallocCB( test_malloc);

	for(i = 0; i < 16; i ++)
	{
		pic_buffer[i].imgYAddr = (uint32 )MemAlloc(FRAME_SIZE);
		pic_buffer[i].pBufferHeader = (void *)pic_buffer[i].imgYAddr;
		pic_buffer[i].DPB_addr_index = i;
		pic_buffer[i].b_valid = 1;
	}

	H264DecInit(&or_buffer, NULL);

	//Load BS here.
	bs_phy_ptr = MemAlloc(BS_PHY_MEM_SIZE);
	bs_ptr = (uint8 *)BS_START_ADDR;

	while(nFrame < 100)
	{
		if(b_need_new_buffer)
		{
			// Get valid buffer for decoder buffer.
			for(i =0; i< 16; i++)
			{
				if(pic_buffer[i].b_valid)
				{
					pic_ptr = & pic_buffer[i];
					break;
				}
			}		
		
			H264Dec_SetCurRecPic((uint8 *)pic_ptr->imgYAddr,(uint8 *)pic_ptr->imgYAddr,(void *)(0x80000000+pic_ptr->DPB_addr_index));
			b_need_new_buffer = 0;
		}
		
		get_unit (bs_ptr, &slice_unit_len);

		memcpy1(bs_phy_ptr, bs_ptr, slice_unit_len);

		dec_input.expected_IVOP= 0;
		dec_input.pStream_phy = bs_phy_ptr;
		dec_input.dataLen = slice_unit_len;
		
		bs_ptr += slice_unit_len;	
		
		H264DecDecode(&dec_input, &dec_output);

		if(dec_output.frameEffective)
		{
			b_need_new_buffer = 1;
			nFrame ++;
		}
		
		
		
	}

	

	

	
	
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

