#include "sc8810_video_header.h"


#include "spr-defs.h"
#include "or1k-support.h"

void __attribute__ ((section ("MAIN_FUNC")))main();
/* Loops/exits simulation */
void	exit (int i)
	   {
	     asm("l.add r3,r0,%0": : "r" (i));
	     asm("l.nop %0": :"K" (NOP_EXIT));
	     while (1);
	   }

#define GLB_REG_BCK_SIZE (0x200)
#define FRM_RAM_BCK_SIZE (0x200)


PUBLIC INPUT_PARA_T *g_input;
PUBLIC int32 g_stream_type;

PUBLIC int32 bs_start_addr;
PUBLIC int32 bs_buffer_length;
PUBLIC uint32 vld_table_addr;
PUBLIC int32 extra_malloc_mem_start_addr;
PUBLIC int32 inter_malloc_mem_start_addr;
PUBLIC int32 frame_buf_size;
PUBLIC int32 total_inter_malloc_size;
PUBLIC int32 total_extra_malloc_size;
PUBLIC uint32 or_addr_offset;

PUBLIC uint32 * s_glb_reg_bck_ptr;
PUBLIC uint32 * s_frm_ram_bck_ptr;








volatile uint32 video_size_get;
volatile uint32 video_buffer_malloced;


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
		

/*Function Type for H264 Decode*/
#define H264DEC_DECODE				0
#define H264DEC_INIT					1
#define H264DEC_RELEASE				2
#define H264DEC_GETLAST				3
#define H264DEC_CROP_PARAM                    4

void main()
{
	

    	uint32 function_type;
	uint32 ret;
	uint32 i;

	function_type = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x58, "SHARE_RAM_BASE_ADDR 0x58: function_type.");
	if(function_type != H264DEC_INIT)
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
	
	}
	
	switch(function_type)
	{
		case H264DEC_DECODE:
			{
				MMDecInput dec_input; 
				MMDecOutput  dec_output;
				uint32 FrameY, FrameY_phy,BufferHeader;



				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
				g_image_ptr->is_need_init_vsp_hufftab = TRUE;				
				

				//set default value

				video_buffer_malloced =( (OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x0, "SHARE_RAM_BASE_ADDR 0x0: bit16 :video_buffer_malloced")) >>16) & 0x1;
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

				H264Dec_SetCurRecPic((uint8 *)FrameY, (uint8 *)FrameY_phy, (void *)BufferHeader);

				if(video_buffer_malloced && video_size_get)
				{
					// Set to buffer to decoder.
					for(i=0; i< (MAX_REF_FRAME_NUMBER + 1); i++)
					{
						
						direct_mb_info_addr[i] = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x70 + i*4, "Read direct mb info buffer address.");
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x70 + i*4, 0, "clean register");
					}
					video_size_get = 0;
				}
						
	            		// Find the first start code. 
				g_stream_offset =OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x20, "shareRAM 0x20 stream_len");		// bitstream offset
				bs_start_addr=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"shareRAM 0x18 STREAM_BUF_ADDR");	// bistream start address		
				bs_buffer_length= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"shareRAM 0x1c STREAM_BUF_SIZE");//bitstream length .

				OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

#if 1				
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset|0xc0000000),"BSM_cfg1 check startcode");//byte align
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (0x80000000|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
			    	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, 0x00000004,0x00000004,"startcode found");//check bsm is idle	


				//Get start code length of first NALU.
				g_slice_datalen=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");			
				g_stream_offset+=g_slice_datalen;
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
//				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xfc, g_slice_datalen, "FPGA test");	










				while(g_stream_offset<bs_buffer_length)
				{
					// Find the next start code and get length of NALU.
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
					OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset|0xc0000000),"BSM_cfg1 check startcode");//byte align
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (0x80000000|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, 0x00000004,0x00000004,"startcode found");//check bsm is idle	

					// Get length of NALU and net bitstream length.
					g_slice_datalen=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
					g_nalu_ptr->len=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_DATA_LEN,"get NAL_DATA_LEN");
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
        

					// Configure BSM for decoding.
					OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x80000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//point to the start of NALU.
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.
						

					ret = H264DecDecode(&dec_input, &dec_output);
					
					g_stream_offset += g_slice_datalen;//dec_input_ptr->dataLen;




					if(!video_buffer_malloced && !video_size_get)
					{
						if(g_sps_ptr->pic_height_in_map_units_minus1) // sps has been parsed.
						{
							uint32 buffer_size;
							uint32 buffer_num = 0;
							
							buffer_size  = ((g_sps_ptr->pic_height_in_map_units_minus1+1) * (g_sps_ptr->pic_width_in_mbs_minus1+1))  * 80;
							if(g_sps_ptr->profile_idc != 0x42)
							{
								buffer_num = 17;
							}
							OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+ 0x10,buffer_num ,"resuse for buffer num.");
							OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+ 0x14,buffer_size,"reuse for buffer size.");
							
							
							video_size_get = 1;
							
						}
					}
						
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4,(video_size_get <<31)| (((g_sps_ptr->pic_height_in_map_units_minus1+1)&0xff)<<16)|(((g_sps_ptr->pic_width_in_mbs_minus1+1)&0xff)<<4),"shareRAM 0x4 IMAGE_SIZE");


					
          OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xb4, frame_dec_finish,"dec a frame");

					if( (MMDEC_ERROR == ret) ||frame_dec_finish)//dec_output.frameEffective
					{ 
						frame_dec_finish=0;
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70,((s_bind_cnt <<16)| s_unbind_cnt),"unbind_buffer_number");

						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x5c,ret," Write output of OR function");
							
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c, dec_output.pOutFrameY,"display frame_Y_addr");
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30, dec_output.pOutFrameU,"display frame_UV_addr");
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4c, dec_output.frameEffective,"display en");

						s_unbind_cnt = 0;
						s_bind_cnt = 0;
						

						break;	//break loop.					
					}
					

				}
				
#else
				OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (0x80000000|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data

				//Test
				uint8 tmp_read = READ_FLC(NULL, 8);
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xec, tmp_read, "FPGA test");
				tmp_read = READ_FLC(NULL, 8);
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xe8, tmp_read, "FPGA test");
				tmp_read = READ_FLC(NULL, 8);
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xe4, tmp_read, "FPGA test");
				tmp_read = READ_FLC(NULL, 8);
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xe0, tmp_read, "FPGA test");

				// Test end
#endif			
				
			}
			
			break;
			
		case H264DEC_INIT:
			{
				MMDecVideoFormat video_format;
				MMCodecBuffer dec_malloc_bfr;

				video_size_get = 0;
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264,"VSP_MODE");
				or_addr_offset = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x54, "or_addr_offset");//OPENRISC text+heap+stack

				dec_malloc_bfr.int_buffer_ptr = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+8, "shareRAM 8 VSP_MEM0_ST_ADDR");//OPENRISC ddr_start_addr+code size
				dec_malloc_bfr.int_size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0xc,"shareRAM c CODE_RUN_SIZE"); //OPENRISC text+heap+stack

				s_frm_ram_bck_ptr = (uint32 *)(dec_malloc_bfr.int_buffer_ptr - FRM_RAM_BCK_SIZE);
				s_glb_reg_bck_ptr =  (uint32 *)(dec_malloc_bfr.int_buffer_ptr - FRM_RAM_BCK_SIZE - GLB_REG_BCK_SIZE);
				
				g_nFrame_dec_h264 = 0;
 	      g_dispFrmNum = 0;
	      display_array_len=0;
				
				ret = H264DecInit(&dec_malloc_bfr,& video_format);			
			}


			break;

		case H264DEC_RELEASE:
			break;

		case H264DEC_GETLAST:
					{
				MMDecOutput  dec_output;
				
				//unbind
				H264Dec_flush_dpb(g_curr_slice_ptr->p_Dpb);
				
				if(g_dpb_layer[0]->used_size != 0)
					H264Dec_flush_dpb(g_dpb_layer[0]);
				else if(g_dpb_layer[1]->used_size != 0)
					H264Dec_flush_dpb(g_dpb_layer[1]);
				
				
				
				s_unbind_cnt = 0;
				s_bind_cnt = 0;
						 
				//display   
				if(display_array_len>0)
				{	
					dec_output.frameEffective = TRUE;
					dec_output.pOutFrameY = display_array_Y[0];//g_dec_picture_ptr->imgY;
					dec_output.pOutFrameU = display_array_UV[0];//g_dec_picture_ptr->imgU;
					//dec_output.pOutFrameV = display_array_UV[0];//g_dec_picture_ptr->imgV;
					OR_VSP_UNBIND(display_array_BH[0]);
					display_array_len--;
					for(i=0;i<display_array_len;i++)//weihu for display
					{
						display_array_BH[i]=display_array_BH[i+1];
						display_array_Y[i]=display_array_Y[i+1];
						display_array_UV[i]=display_array_UV[i+1];
					}
				}
				else
				{
				  dec_output.frameEffective = FALSE;
				  
				 
				}
				
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70,((s_bind_cnt <<16)| s_unbind_cnt),"unbind_buffer_number");   

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x4c, dec_output.frameEffective,"display en");
				//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x5c,ret," Write output of OR function");
				if(dec_output.frameEffective)
				{ 								
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x2c, dec_output.pOutFrameY,"display frame_Y_addr");
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x30, dec_output.pOutFrameU,"display frame_UV_addr");											
				}

			}

			break;

                case H264DEC_CROP_PARAM:
                        {
                                uint32 croppingFlag, leftOffset, width, topOffset, height;
                            
                                H264DecCroppingParams(&croppingFlag,  &leftOffset, &width, &topOffset, &height);

                                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xb4, croppingFlag,"croppingFlag");
                                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xb8, leftOffset,"leftOffset");
                                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xbc, width,"width");
                                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc0, topOffset,"topOffset");
                                OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0xc4, height,"height");
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

	// Generate interrupt to arm.
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");


	while(1);

}










































