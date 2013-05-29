#include "sc8810_video_header.h"


#include "spr-defs.h"
#include "or1k-support.h"

void __attribute__ ((section ("MAIN_FUNC")))main();

#define GLB_REG_BCK_SIZE (0x200)
#define FRM_RAM_BCK_SIZE (0x200)
#define DCT_RAM_BCK_SIZE (0x200)


PUBLIC uint32 or_addr_offset;
PUBLIC int32 bs_start_addr;
PUBLIC int32 bs_buffer_length;

PUBLIC uint32 * s_glb_reg_bck_ptr;
PUBLIC uint32 * s_frm_ram_bck_ptr;
PUBLIC uint32 * s_dct_ram_bck_ptr;

uint32 s_unbind_cnt =0;
uint32 s_bind_cnt = 0;

PUBLIC void OR_VSP_BIND(void *pHeader)
{
	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x6c,(uint32)pHeader,"bind_buffer_header");
	s_bind_cnt ++;
}

PUBLIC void OR_VSP_UNBIND(void *pHeader)
{
	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x74+s_unbind_cnt*4,(uint32)pHeader,"unbind_buffer_header");
	s_unbind_cnt ++;
}

/*Function Type for MPEG4 Decode*/
#define MPEG4DEC_DECODE				0
#define MPEG4DEC_INIT					1
#define MPEG4DEC_RELEASE				2
#define MPEG4DEC_GETLAST				3
#define MPEG4DEC_MEMINIT				4
#define MPEG4DEC_VOLHEADER			5


void main()
{
   	uint32 function_type;
	uint32 ret;
	uint32 i;
	function_type = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x58, "SHARE_RAM_BASE_ADDR 0x58: function_type.");
	if(function_type != MPEG4DEC_INIT)
	{
		// Restore registers.
		for(i= 0; i <(GLB_REG_BCK_SIZE >>2); i++)
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+(i<<2), *(s_glb_reg_bck_ptr +i),"restore glb reg.");
		}
		for(i= 0; i <(FRM_RAM_BCK_SIZE>>2); i++)
		{
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+(i<<2), *(s_frm_ram_bck_ptr+i),"restore glb reg.");
		}
		for(i= 0; i <(DCT_RAM_BCK_SIZE>>2); i++)
		{
			OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+(i<<2), *(s_dct_ram_bck_ptr+i),"restore dct reg.");
		}
	
	}	

	switch(function_type)
	{
		case MPEG4DEC_DECODE:
			{
				MMDecInput dec_input; 
				MMDecOutput  dec_output;
				uint32 FrameY, FrameY_phy,BufferHeader;
                               	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();


				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
				
				
				//set default value

                                
				vop_mode_ptr->video_std = (OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x0, "SHARE_RAM_BASE_ADDR 0x0: bit[3:0] video standard."))& 0xf;
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF,vop_mode_ptr->video_std,"VSP_MODE");
				dec_input.expected_IVOP =((OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x0,"shareRAM 0x0 bit 28: expected_IVOP"))>>28)& 0x1;
				dec_input.beLastFrm = FALSE;
				dec_input.beDisplayed = 1;

				
				dec_output.frameEffective = FALSE;
				dec_output.pOutFrameY = PNULL;
				dec_output.pOutFrameU = PNULL;
				dec_output.pOutFrameV = PNULL;

				// Rec Buffer.
				FrameY = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x60,"shareRame 0x60: rec buffer virtual address.");
				FrameY_phy = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x64,"shareRame 0x64: rec buffer physical address.");
				BufferHeader = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x68,"shareRame 0x68: rec buffer header.");

				MP4Dec_SetCurRecPic((uint8 *)FrameY, (uint8 *)FrameY_phy, (void *)BufferHeader);
				

				g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
				bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
				bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .
				
				OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

				dec_input.dataLen = bs_buffer_length;
				ret =  MP4DecDecode(&dec_input, &dec_output);
				
	
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, (dec_output.frame_height<<12)|(dec_output.frame_width),"shareRAM 0x4 IMAGE_SIZE");		

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70,((s_bind_cnt <<16)| s_unbind_cnt),"unbind_buffer_number");

				s_unbind_cnt = 0;
				s_bind_cnt = 0;		

				if( ret ||dec_output.frameEffective)
				{ 				

					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x5c,ret," Write output of OR function");
							
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c, dec_output.pOutFrameY,"display frame_Y_addr");
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30, dec_output.pOutFrameU,"display frame_UV_addr");
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4c, dec_output.frameEffective,"display en");		

					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xb4,dec_output.pBufferHeader,"pBufferHeader");
				}


									
			}

			break;

		case MPEG4DEC_INIT:
			{
				MMDecVideoFormat video_format;
				MMCodecBuffer dec_malloc_bfr;
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
				or_addr_offset = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x54, "or_addr_offset");//OPENRISC text+heap+stack

//				video_format.video_std = (OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x0, "SHARE_RAM_BASE_ADDR 0x0: bit[3:0] video standard."))& 0xf;
//				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, video_format.video_std,"VSP_MODE");

				dec_malloc_bfr.int_buffer_ptr = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+8, "shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
				dec_malloc_bfr.int_size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0xc,"shareRAM c CODE_RUN_SIZE"); //OPENRISC text+heap+stack

				s_frm_ram_bck_ptr = (uint32 *)(dec_malloc_bfr.int_buffer_ptr - FRM_RAM_BCK_SIZE);
				s_glb_reg_bck_ptr =  (uint32 *)(dec_malloc_bfr.int_buffer_ptr - FRM_RAM_BCK_SIZE - GLB_REG_BCK_SIZE);
				s_dct_ram_bck_ptr =  (uint32 *)(dec_malloc_bfr.int_buffer_ptr- FRM_RAM_BCK_SIZE - GLB_REG_BCK_SIZE - DCT_RAM_BCK_SIZE);

#if 0
				if(video_format.video_std ==VSP_MPEG4)
				{

					g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
					bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
					bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .
	
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
					OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.
			
					video_format.i_extra = bs_buffer_length;
				
				}else
#endif				
				{
					video_format.i_extra = 0;
					video_format.frame_width = 0;
					video_format.frame_height = 0;
				}				
				
				ret = MP4DecInit(&dec_malloc_bfr,& video_format);	


			}
			
			break;
		
		case MPEG4DEC_RELEASE:
			break;

		case MPEG4DEC_GETLAST:
			break;

		case MPEG4DEC_MEMINIT:
			{
				MMCodecBuffer dec_ext_malloc_bfr;

				dec_ext_malloc_bfr.common_buffer_ptr= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x10, "shareRAM 0x10 VSP_MEM1_ST_ADDR");
				dec_ext_malloc_bfr.size= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x14,"shareRAM 0x14 total_mem1_size");

				MP4DecMemInit(&dec_ext_malloc_bfr);			

			}
			break;

		case MPEG4DEC_VOLHEADER:
			{
				MMDecVideoFormat video_format;



				video_format.video_std = VSP_MPEG4;

				if(video_format.video_std ==VSP_MPEG4)
				{

					g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
					bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
					bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .
					
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
					OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.
			
					video_format.i_extra = bs_buffer_length;
				}else
				{
					video_format.i_extra = 0;
					video_format.frame_width = 0;
					video_format.frame_height = 0;
				}	


				ret = MP4DecVolHeader(& video_format);	

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, (video_format.frame_height <<12)|(video_format.frame_width),"shareRAM 0x4 IMAGE_SIZE");
			}
			break;
			

		default :
			break;			

	}


	// Store registers.
	for(i= 0; i <(GLB_REG_BCK_SIZE>>2); i++)
	{
		 *(s_glb_reg_bck_ptr +i) = OR1200_READ_REG(GLB_REG_BASE_ADDR+(i<<2),"restore glb reg.");
	}
	for(i= 0; i <(FRM_RAM_BCK_SIZE>>2); i++)
	{
		 *(s_frm_ram_bck_ptr+i) = OR1200_READ_REG(FRAME_ADDR_TABLE_BASE_ADDR+(i<<2),"restore glb reg.");
	}	
	for(i= 0; i <(DCT_RAM_BCK_SIZE>>2); i++)
	{
		 *(s_dct_ram_bck_ptr+i) = OR1200_READ_REG(DCT_IQW_TABLE_BASE_ADDR+(i<<2),"restore dct reg.");
	}		

	// Generate interrupt to arm.
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");


	while(1);	
}










































