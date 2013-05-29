#include "sc8810_video_header.h"


#include "spr-defs.h"
#include "or1k-support.h"

void __attribute__ ((section ("MAIN_FUNC")))main();
void foo_bar()
{

}



#define GLB_REG_BCK_SIZE (0x200)
#define FRM_RAM_BCK_SIZE (0x200)


PUBLIC uint32 or_addr_offset;

PUBLIC uint32 * s_glb_reg_bck_ptr;
PUBLIC uint32 * s_frm_ram_bck_ptr;

/*Function Type for H264 Decode*/
#define MP4ENC_ENCODE 		0
#define MP4ENC_INIT			1
#define MP4ENC_HEADER		2
#define MP4ENC_SETCONF		3
#define MP4ENC_GETCONF		4


void main(void)
{
	uint32 function_type;
	uint32 ret;
	uint32 i;

	function_type = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x58, "SHARE_RAM_BASE_ADDR 0x58: function_type.");
	if((function_type == MP4ENC_ENCODE) ||(function_type ==  MP4ENC_HEADER))
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
		case MP4ENC_ENCODE:
			{
				MMEncIn EncInput ;
				MMEncOut EncOutput ;


				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
	
				EncInput.p_src_y		 =  (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x2c,"ORSC_SHARE: Frame_Y_ADDR");
				EncInput.p_src_u 		= (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x30,"ORSC_SHARE: Frame_UV_ADDR");
				EncInput.time_stamp	= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x3c,"SHARE_RAM_BASE_ADDR 0x3c:time_stamp ");
				EncInput.vopType		= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x70, "SHARE_RAM_BASE_ADDR 0x70: mp4enc_vopType");

				or1200_print = 1;
			
				MP4EncStrmEncode(&EncInput, &EncOutput);

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x20, EncOutput.strmSize, "SHARE_RAM_BASE_ADDR 0x20: bs_used_len as encoded bs length.");

			}
			
			break;
			
		case MP4ENC_INIT:
			{
				MMCodecBuffer Enc_Inter_malloc_Bfr;
				MMCodecBuffer Enc_Extra_malloc_Bfr;
				MMCodecBuffer Enc_Bitstream_Bfr;
				MMEncVideoInfo video_info;
				uint32 tmp;

				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, STREAM_ID_MPEG4|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");

				// Inter and extra buffer start addr and size.
				Enc_Inter_malloc_Bfr.common_buffer_ptr= (uint8 *)(OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x8, "ORSC_SHARE: VSP_MEM0_ST_ADDR"));	//ddr_start_addr+code size
				Enc_Inter_malloc_Bfr.size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0xc, "ORSC_SHARE: VSP_MEM0_SIZE: CODE_RUN_SIZE");    //heap+stack
				Enc_Extra_malloc_Bfr.common_buffer_ptr= (uint8 *)(OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x10, "ORSC_SHARE: VSP_MEM1_ST_ADDR"));
				Enc_Extra_malloc_Bfr.size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x14, "ORSC_SHARE: VSP_MEM1_SIZE");

				//Bitstream buffer start addr and size
				Enc_Bitstream_Bfr.common_buffer_ptr= (uint8 *)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x18,"ORSC_SHARE: STREAM_BUF_ADDR");
				Enc_Bitstream_Bfr.size = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x1c,"ORSC_SHARE: STREAM_BUF_SIZE");	//byte

				// OR start addr.
				or_addr_offset=OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x54,"shareRAM 0x54 or_addr_offset");

				// Encoding config.
				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x0, "SHARE_RAM_BASE_ADDR 0x0: config.");
				video_info.is_h263 = ((tmp &  0xf) == STREAM_ID_H263);	// [3:0] video standard
				g_anti_shake.enable_anti_shake = (tmp >>17) & 0x1;	// [17] enable_anti_shake

				video_info.time_scale = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x40, "SHARE_RAM_BASE_ADDR 0x40: time_scale.");

				video_info.uv_interleaved = 1;//g_uv_interleaved;
				or1200_print = 1;//for or1200 cmd

				// Image size.
				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x04, "read img_size");
				video_info.frame_width = (tmp&0xfff);
				video_info.frame_height  = ((tmp>>12)&0xfff);

				// anti shake config.
				if(g_anti_shake.enable_anti_shake)
				{					
					tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x44, "ORSC_SHARE: Size_crop");
					g_anti_shake.shift_x= ((tmp>>8)&0xff);// Left
					g_anti_shake.shift_y = (tmp & 0xff);	// Top

					tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x50, "ORSC_SHARE:actual buffer height & width");
					g_anti_shake.input_width = tmp & 0xfff;
					g_anti_shake.input_height = (tmp >> 12) & 0xfff;
				}else
				{
					g_anti_shake.shift_x = 0;
					g_anti_shake.shift_y = 0;

					g_anti_shake.input_width = video_info.frame_width;
					g_anti_shake.input_height= video_info.frame_height;
				}
		
	
				// Init encoder.
				ret = MP4EncInit(&Enc_Inter_malloc_Bfr, &Enc_Extra_malloc_Bfr,&Enc_Bitstream_Bfr, &video_info);

				// Set back mem for ram and glb registers.
				s_frm_ram_bck_ptr = (uint32 *)(Enc_Inter_malloc_Bfr.common_buffer_ptr - FRM_RAM_BCK_SIZE);
				s_glb_reg_bck_ptr =  (uint32 *)(Enc_Inter_malloc_Bfr.common_buffer_ptr - FRM_RAM_BCK_SIZE - GLB_REG_BCK_SIZE);		

				
					{
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xf0, 0x77777777,"");

						}
				
			}
			break;
			
		case MP4ENC_HEADER:
			{
				MMEncOut EncOutput ;	
				VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
				uint8 video_type =vol_mode_ptr->short_video_header ?STREAM_ID_H263: STREAM_ID_MPEG4 ;
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF,( 1 << 4) |video_type, "ORSC: VSP_MODE: Set standard, work mode and manual mode");
				
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	

				BSM_Init();

				MP4EncGenHeader(&EncOutput);			

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x20, EncOutput.strmSize, "SHARE_RAM_BASE_ADDR 0x20: bs_used_len as encoded bs length.");

			}
			
			break;

			

		case MP4ENC_SETCONF:
		/*
			// Offset 0x60~0x6c. Regs for MPEG4 ENC config.
			// Offset 0x60
			uint8 mp4enc_short_video_header;		// [0]
			uint8 mp4enc_RateCtrlEnable;				// [8]
			uint8 mp4enc_StepI;						// [23:16]
			uint8 mp4enc_StepP;						// [31:24]

			// Offset 0x64
			uint32 mp4enc_FrameRate;

			//Offset 0x68 
			uint32 mp4enc_targetBitRate;

			// Offset 0x6c
			uint32 mp4enc_ProfileAndLevel;
		*/				
			{

				VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
				ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
				uint32 tmp;
				
				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x60, "SHARE_RAM_BASE_ADDR 0x60");
				vol_mode_ptr->short_video_header	= tmp & 0x1;
				vop_mode_ptr->RateCtrlEnable		= (tmp>>8) & 0x1;
				vop_mode_ptr->StepI				= (tmp >>16) & 0xff;
				vop_mode_ptr->StepP				= (tmp >>24) & 0xff;	

				vop_mode_ptr->FrameRate			=  OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x64, "SHARE_RAM_BASE_ADDR 0x64: mp4enc_FrameRate");	

				vop_mode_ptr->targetBitRate		= OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x68, "SHARE_RAM_BASE_ADDR 0x68: mp4enc_targetBitRate");	
				
				vol_mode_ptr->ProfileAndLevel		= OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x6c, "SHARE_RAM_BASE_ADDR 0x6c: mp4enc_ProfileAndLevel");			
					
			}
			break;

		case MP4ENC_GETCONF:
			{

				VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
				ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
				uint32 tmp;

				tmp = (vop_mode_ptr->StepP << 24) | (vop_mode_ptr->StepI <<16) | (vop_mode_ptr->RateCtrlEnable <<8) | (vol_mode_ptr->short_video_header);

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x60,tmp ,"SHARE_RAM_BASE_ADDR 0x60");	

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x64, vop_mode_ptr->FrameRate,"SHARE_RAM_BASE_ADDR 0x64: mp4enc_FrameRate");	

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x68, vop_mode_ptr->targetBitRate,"SHARE_RAM_BASE_ADDR 0x68: mp4enc_targetBitRate");	
				
				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x6c,vol_mode_ptr->ProfileAndLevel, "SHARE_RAM_BASE_ADDR 0x6c: mp4enc_ProfileAndLevel");			
					
			}
			

			break;


		default :
			break;
	}

	
	// Store registers.

	if(function_type < MP4ENC_SETCONF) 
	{
		for(i= 0; i <(GLB_REG_BCK_SIZE>>2); i++)
		{
			 *(s_glb_reg_bck_ptr +i) = OR1200_READ_REG(GLB_REG_BASE_ADDR+(i<<2),"restore glb reg.");
		}
		for(i= 0; i <(FRM_RAM_BCK_SIZE>>2); i++)
		{
			 *(s_frm_ram_bck_ptr+i) = OR1200_READ_REG(FRAME_ADDR_TABLE_BASE_ADDR+(i<<2),"restore glb reg.");
		}	
	}

	// Generate interrupt to arm.
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");


	while(1);

}
