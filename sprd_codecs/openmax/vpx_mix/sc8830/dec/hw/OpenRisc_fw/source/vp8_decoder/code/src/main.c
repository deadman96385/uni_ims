#include "sc8810_video_header.h"


#include "spr-defs.h"
#include "or1k-support.h"

void __attribute__ ((section ("MAIN_FUNC")))main();
void foo_bar()
{

}

VP8D_PTR optr;

#define GLB_REG_BCK_SIZE (0x200)

PUBLIC uint32 * s_glb_reg_bck_ptr;

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


/*Function Type for VP8 Decode*/
#define VP8DEC_DECODE				0
#define VP8DEC_INIT					1
#define VP8DEC_HEADER				2




void main(void)
{
   	uint32 function_type;
	uint32 ret;
	uint32 i;
	function_type = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x58, "SHARE_RAM_BASE_ADDR 0x58: function_type.");
	if(function_type != VP8DEC_INIT)
	{
		// Restore registers.
		for(i= 0; i <(GLB_REG_BCK_SIZE >>2); i++)
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+(i<<2), *(s_glb_reg_bck_ptr +i),"restore glb reg.");
		}
	
	}	

	switch(function_type)
	{
		case VP8DEC_DECODE:
			{
				uint32 FrameY, FrameY_phy,BufferHeader;
			        VP8D_COMP *pbi = (VP8D_COMP *) optr;
    				VP8_COMMON *cm = &pbi->common;
				 YV12_BUFFER_CONFIG *rec_frame = &cm->new_frame;

				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF,V_BIT_6 | STREAM_ID_VP8,"VSP_MODE");
				
				// Rec Buffer.
				FrameY = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x60,"shareRame 0x60: rec buffer virtual address.");
				FrameY_phy = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x64,"shareRame 0x64: rec buffer physical address.");
				BufferHeader = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x68,"shareRame 0x68: rec buffer header.");

				//rec_frame->y_width = cm->Height;
				//rec_frame->y_height = cm->Width;

				VP8Dec_SetCurRecPic((uint8 *)FrameY, (uint8 *)FrameY_phy, (void *)BufferHeader, rec_frame);

			
				g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
				bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
				bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .


				OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
				//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(0x3fffff)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

					
				  ret = vp8dx_receive_compressed_data(optr, bs_buffer_length,(uint8 *)(g_stream_offset +bs_start_addr - or_addr_offset ) , 0);


				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, (cm->Height<<12)|(cm->Width),"shareRAM 0x4 IMAGE_SIZE");		

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70,((s_bind_cnt <<16)| s_unbind_cnt),"unbind_buffer_number");

				s_unbind_cnt = 0;
				s_bind_cnt = 0;		

				//if(cm->show_frame)
				{ 				

					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x5c,ret," Write output of OR function");
							
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c, (uint32)cm->frame_to_show->y_buffer_virtual,"display frame_Y_addr");
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30,  (uint32)cm->frame_to_show->u_buffer_virtual,"display frame_UV_addr");
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4c, cm->show_frame,"display en");		

                                        OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xb4,(uint32)cm->frame_to_show->pBufferHeader,"pBufferHeader");

				}
				
			}

			break;

		case VP8DEC_INIT:
			{
				MMCodecBuffer dec_malloc_bfr;
				uint32 frame4_buffer_addr;
				VP8D_COMP *pbi;
    				VP8_COMMON *cm;
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF,V_BIT_6 | STREAM_ID_VP8,"VSP_MODE");
				or_addr_offset = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x54, "or_addr_offset");//OPENRISC text+heap+stack

				dec_malloc_bfr.int_buffer_ptr = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+8, "shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
				dec_malloc_bfr.int_size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0xc,"shareRAM c CODE_RUN_SIZE"); //OPENRISC text+heap+stack

				s_glb_reg_bck_ptr =  (uint32 *)(dec_malloc_bfr.int_buffer_ptr  - GLB_REG_BCK_SIZE);

				vp8dec_InitInterMem (&dec_malloc_bfr);

				g_fh_reg_ptr = (VSP_FH_REG_T *)vp8dec_InterMemAlloc(sizeof(VSP_FH_REG_T));
				
				optr = vp8dx_create_decompressor(/*&oxcf*/NULL);


			}
			
			break;

		case VP8DEC_HEADER:
			{
				VP8D_COMP *pbi = (VP8D_COMP *) optr;
    				VP8_COMMON *cm = &pbi->common;
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
				
				g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
				bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
				bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .


				OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

				if( BitstreamReadBits(32) == 0x444b4946) // DKIF, byte0-3, big-endian
				{
					uint32 val;
					uint32 fourcc;
					
					//file_is_ivf = 1;
					
					val = BitstreamReadBits(16); // byte4-5
					if (val != 0)
					{
						//printf("Error: Unrecognized IVF version!");
					}
					
					BitstreamReadBits(16); // byte6-7
					
					fourcc = BitstreamReadBits(32); // byte8-11
					fourcc &= 0xffffff00;
					
					if (fourcc != 0x56503800)
					{
						//printf("Error: not vp8 bitstream!\n");
					}
					
					val = (BitstreamReadBits(16) & 0xff3f); // byte12-13
					cm->Width= ((val&0xff00)>>8) | ((val&0xff)<<8);
					val = (BitstreamReadBits(16) & 0xff3f); // byte14-15
					cm->Height= ((val&0xff00)>>8) | ((val&0xff)<<8);
					
					BitstreamReadBits(32); // byte16-31
					BitstreamReadBits(32);
					BitstreamReadBits(32);
					BitstreamReadBits(32);
				//	g_stream_offset += 32; // OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS")/8
					ret =  MMDEC_OK;
				}
				else
				{
					ret =  MMDEC_STREAM_ERROR;					
				}
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x5c,ret," Write output of OR function");

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4, (cm->Height<<12)|(cm->Width),"shareRAM 0x4 IMAGE_SIZE");	

				//vp8_init_frame_buffers(cm);
							
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

	// Generate interrupt to arm.
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");


	while(1);	

}
