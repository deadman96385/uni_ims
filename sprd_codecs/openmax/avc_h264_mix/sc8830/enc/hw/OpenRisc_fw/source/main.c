#include "sc6800x_video_header.h"

#include "spr-defs.h"
#include "or1k-support.h"

void __attribute__ ((section ("MAIN_FUNC")))main();



PUBLIC uint32 or_addr_offset;


/*Function Type for H264 Encode*/
#define H264ENC_ENCODE 		0
#define H264ENC_INIT			1
#define H264ENC_SETCONF		2
#define H264ENC_GETCONF		3
#define H264ENC_GEN_SPS             4
#define H264ENC_GEN_PPS             5


void main(void)
{
	uint32 function_type;
	uint32 ret;
	uint32 i;

	function_type = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x58, "SHARE_RAM_BASE_ADDR 0x58: function_type.");


	switch(function_type)
	{
		case H264ENC_ENCODE:
			{
				MMEncIn EncInput ;
				MMEncOut EncOutput ;


				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
				
	
				EncInput.p_src_y		=  (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x2c,"ORSC_SHARE: Frame_Y_ADDR");
				EncInput.p_src_u 		=  (uint8*)OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x30,"ORSC_SHARE: Frame_UV_ADDR");
				EncInput.vopType		= OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x68, "SHARE_RAM_BASE_ADDR 0x68: h264enc_vopType");

				EncOutput.strmSize 	= 0;

				while(1)
				{
					OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");					


					H264EncStrmEncode(&EncInput, &EncOutput);

					if(EncOutput.strmSize)
					{
						OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x20, EncOutput.strmSize, "SHARE_RAM_BASE_ADDR 0x20: bs_used_len as encoded bs length.");
						break;
					}
					
				}				
								
			}
			
			break;
			
		case H264ENC_INIT:
			{
				MMCodecBuffer Enc_Inter_malloc_Bfr;
				MMCodecBuffer Enc_Extra_malloc_Bfr;
				MMCodecBuffer Enc_Bitstream_Bfr;
				MMEncVideoInfo video_info;
				uint32 tmp;

				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

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


				// Image size.
				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x04, "read img_size");
				video_info.frame_width = (tmp&0xfff);
				video_info.frame_height  = ((tmp>>12)&0xfff);

		
	
				// Init encoder.				
				ret = H264EncInit(&Enc_Inter_malloc_Bfr, &Enc_Extra_malloc_Bfr,&Enc_Bitstream_Bfr, &video_info);
				


									
				
			}
			break;
			
		case H264ENC_SETCONF:	
			{
				uint32 tmp;
				// Encoding config.

				//g_input->qp_ISLICE = 25;

				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR+0x38, "ORSC_SHARE: RATE_CONTROL");
				g_h264_enc_config->targetBitRate	= (tmp&0x7fffffff); // TARGET_BITRATE
				g_h264_enc_config->RateCtrlEnable= ((tmp>>31)&0x1);


				tmp = OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x60, "SHARE_RAM_BASE_ADDR 0x60");
				g_h264_enc_config->qp_ISLICE		= (tmp >>16) & 0xff;
				g_h264_enc_config->qp_PSLICE		= (tmp >>24) & 0xff;	

				g_h264_enc_config->FrameRate	=  OR1200_READ_REG(SHARE_RAM_BASE_ADDR + 0x64, "SHARE_RAM_BASE_ADDR 0x64: mp4enc_FrameRate");	
					
			}
			break;

		case H264ENC_GETCONF:
			{

				uint32 tmp;

				tmp = (g_h264_enc_config->RateCtrlEnable << 31) |(g_h264_enc_config->targetBitRate);

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x38,tmp ,"SHARE_RAM_BASE_ADDR 0x38");

				tmp = (g_h264_enc_config->qp_PSLICE << 24) | (g_h264_enc_config->qp_ISLICE <<16);

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x60,tmp ,"SHARE_RAM_BASE_ADDR 0x60");	

				OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x64, g_h264_enc_config->FrameRate,"SHARE_RAM_BASE_ADDR 0x64: mp4enc_FrameRate");				
					
			}
			

			break;

		case H264ENC_GEN_SPS:
			{
				MMEncOut EncOutput ;

				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");

                                EncOutput.strmSize 	= 0;

                                OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");					

                                H264EncGenHeader(&EncOutput, 1);

                                if(EncOutput.strmSize)
				{
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x20, EncOutput.strmSize, "SHARE_RAM_BASE_ADDR 0x20: bs_used_len as encoded bs length.");
                                }
			}			

			break;

		case H264ENC_GEN_PPS:
			{
				MMEncOut EncOutput ;

				OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");

                                EncOutput.strmSize 	= 0;

                                OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");					

                                H264EncGenHeader(&EncOutput, 0);

                                if(EncOutput.strmSize)
				{
					OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0x20, EncOutput.strmSize, "SHARE_RAM_BASE_ADDR 0x20: bs_used_len as encoded bs length.");
                                }
			}			

			break;

		default :
			break;
	}

	
	// Generate interrupt to arm.
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 1,"arm done int");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_GEN_OFF, 0,"arm done int");


	while(1);

}

