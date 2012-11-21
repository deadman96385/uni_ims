/******************************************************************************
 ** File Name:    sc6600l_isp_reg.h                                           *
 ** Author:       Jianping.Wang                                               *
 ** DATE:         06/10/2008                                                  *
 ** Copyright:    2008 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/1/2006    Jianping.Wang   Create.                                     *10
 ******************************************************************************/
#ifndef _SC6600L_REG_ISP_
#define _SC6600L_REG_ISP_
/*----------------------------------------------------------------------------*
 **                          Dependencies                                     *
 **---------------------------------------------------------------------------*/
#define DCAM_ISP_REG_BASE   0x20c00000
//top module 
#define DCAM_CFG					(DCAM_ISP_REG_BASE + 0x0000)
#define CAP_SHADOW_CNTL			(DCAM_ISP_REG_BASE + 0x0004)
#define YUV2RGB_CFG					(DCAM_ISP_REG_BASE + 0x0008)
#define DCAM_SRC_SIZE      			(DCAM_ISP_REG_BASE + 0x000C)
#define ISP_DIS_SIZE					(DCAM_ISP_REG_BASE + 0x0010)
#define DCAM_INT_STS       			(DCAM_ISP_REG_BASE + 0x0020)
#define DCAM_INT_MASK				(DCAM_ISP_REG_BASE + 0x0024)
#define DCAM_INT_CLR				(DCAM_ISP_REG_BASE + 0x0028)
#define DCAM_INT_RAW				(DCAM_ISP_REG_BASE + 0x002C)

//ahb master
#define FRM_ADDR0					(DCAM_ISP_REG_BASE + 0x0040)
#define FRM_ADDR1					(DCAM_ISP_REG_BASE + 0x0044)
#define FRM_ADDR2					(DCAM_ISP_REG_BASE + 0x0048)
#define FRM_ADDR3					(DCAM_ISP_REG_BASE + 0x004C)
#define FRM_ADDR4					(DCAM_ISP_REG_BASE + 0x0050)
#define FRM_ADDR5 					(DCAM_ISP_REG_BASE + 0x0054)
#define FRM_ADDR6					(DCAM_ISP_REG_BASE + 0x0058)
#define FRM_ADDR7					(DCAM_ISP_REG_BASE + 0x005c)
#define BURST_GAP					(DCAM_ISP_REG_BASE + 0x0060)

//cap
#define CAP_CTRL					(DCAM_ISP_REG_BASE + 0x0100)
#define CAP_FRM_CNT				(DCAM_ISP_REG_BASE + 0x0104)
#define CAP_START					(DCAM_ISP_REG_BASE + 0x0108)
#define CAP_END						(DCAM_ISP_REG_BASE + 0x010C)
#define CAP_IMAGE_DECI				(DCAM_ISP_REG_BASE + 0x0110)
#define CAP_TRANS					(DCAM_ISP_REG_BASE + 0x0114)
#define CAP_OBSERVE					(DCAM_ISP_REG_BASE + 0x0118)
#define CAP_JPEG_FRM_CTRL			(DCAM_ISP_REG_BASE + 0x011C)
#define CAP_JPEG_LAST_FRM_SIZE	(DCAM_ISP_REG_BASE + 0x0120)

//trim&scale
#define SCALING_CFG					(DCAM_ISP_REG_BASE + 0x0124)
#define TRIM_START					(DCAM_ISP_REG_BASE + 0x0128)
#define TRIM_SIZE					(DCAM_ISP_REG_BASE + 0x012C)
#define VID_LUMA_HCOEFF01			(DCAM_ISP_REG_BASE + 0x0200)
#define VID_LUMA_HCOEFF02			(DCAM_ISP_REG_BASE + 0x0204)
#define VID_LUMA_HCOEFF03			(DCAM_ISP_REG_BASE + 0x0208)
#define VID_LUMA_HCOEFF04			(DCAM_ISP_REG_BASE + 0x020C)
#define VID_LUMA_HCOEFF11			(DCAM_ISP_REG_BASE + 0x0210)
#define VID_LUMA_HCOEFF12			(DCAM_ISP_REG_BASE + 0x0214)
#define VID_LUMA_HCOEFF13			(DCAM_ISP_REG_BASE + 0x0218)
#define VID_LUMA_HCOEFF14			(DCAM_ISP_REG_BASE + 0x021C)
#define VID_LUMA_HCOEFF21			(DCAM_ISP_REG_BASE + 0x0220)
#define VID_LUMA_HCOEFF22			(DCAM_ISP_REG_BASE + 0x0224)
#define VID_LUMA_HCOEFF23			(DCAM_ISP_REG_BASE + 0x0228)
#define VID_LUMA_HCOEFF24			(DCAM_ISP_REG_BASE + 0x022C)

#define VID_LUMA_HCOEFF31			(DCAM_ISP_REG_BASE + 0x0230)
#define VID_LUMA_HCOEFF32			(DCAM_ISP_REG_BASE + 0x0234)
#define VID_LUMA_HCOEFF33			(DCAM_ISP_REG_BASE + 0x0238)
#define VID_LUMA_HCOEFF34			(DCAM_ISP_REG_BASE + 0x023C)
#define VID_LUMA_HCOEFF41			(DCAM_ISP_REG_BASE + 0x0240)
#define VID_LUMA_HCOEFF42			(DCAM_ISP_REG_BASE + 0x0244)
#define VID_LUMA_HCOEFF43			(DCAM_ISP_REG_BASE + 0x0248)
#define VID_LUMA_HCOEFF44			(DCAM_ISP_REG_BASE + 0x024C)
#define VID_LUMA_HCOEFF51			(DCAM_ISP_REG_BASE + 0x0250)
#define VID_LUMA_HCOEFF52			(DCAM_ISP_REG_BASE + 0x0254)
#define VID_LUMA_HCOEFF53			(DCAM_ISP_REG_BASE + 0x0258)
#define VID_LUMA_HCOEFF54			(DCAM_ISP_REG_BASE + 0x025C)

#define VID_LUMA_HCOEFF61			(DCAM_ISP_REG_BASE + 0x0260)
#define VID_LUMA_HCOEFF62			(DCAM_ISP_REG_BASE + 0x0264)
#define VID_LUMA_HCOEFF63			(DCAM_ISP_REG_BASE + 0x0268)
#define VID_LUMA_HCOEFF64			(DCAM_ISP_REG_BASE + 0x026C)
#define VID_LUMA_HCOEFF71			(DCAM_ISP_REG_BASE + 0x0270)
#define VID_LUMA_HCOEFF72			(DCAM_ISP_REG_BASE + 0x0274)
#define VID_LUMA_HCOEFF73			(DCAM_ISP_REG_BASE + 0x0278)
#define VID_LUMA_HCOEFF74			(DCAM_ISP_REG_BASE + 0x027C)

#define VID_CHROMA_HCOEFF_LOW0		(DCAM_ISP_REG_BASE + 0x0280)
#define VID_CHROMA_HCOEFF_HIGH0		(DCAM_ISP_REG_BASE + 0x0284)
#define VID_CHROMA_HCOEFF_LOW1		(DCAM_ISP_REG_BASE + 0x0288)
#define VID_CHROMA_HCOEFF_HIGH1		(DCAM_ISP_REG_BASE + 0x028C)

#define VID_CHROMA_HCOEFF_LOW2		(DCAM_ISP_REG_BASE + 0x0290)
#define VID_CHROMA_HCOEFF_HIGH2		(DCAM_ISP_REG_BASE + 0x0294)
#define VID_CHROMA_HCOEFF_LOW3		(DCAM_ISP_REG_BASE + 0x0298)
#define VID_CHROMA_HCOEFF_HIGH3		(DCAM_ISP_REG_BASE + 0x029C)

#define VID_CHROMA_HCOEFF_LOW4		(DCAM_ISP_REG_BASE + 0x02A0)
#define VID_CHROMA_HCOEFF_HIGH4		(DCAM_ISP_REG_BASE + 0x02A4)
#define VID_CHROMA_HCOEFF_LOW5		(DCAM_ISP_REG_BASE + 0x02A8)
#define VID_CHROMA_HCOEFF_HIGH5		(DCAM_ISP_REG_BASE + 0x02AC)

#define VID_CHROMA_HCOEFF_LOW6		(DCAM_ISP_REG_BASE + 0x02B0)
#define VID_CHROMA_HCOEFF_HIGH6		(DCAM_ISP_REG_BASE + 0x02B4)
#define VID_CHROMA_HCOEFF_LOW7		(DCAM_ISP_REG_BASE + 0x02B8)
#define VID_CHROMA_HCOEFF_HIGH7		(DCAM_ISP_REG_BASE + 0x02BC)

#define VID_VCOEFF0					(DCAM_ISP_REG_BASE + 0x0300)
#define VID_VCOEFF1 				(DCAM_ISP_REG_BASE + 0x0304)
#define VID_VCOEFF2					(DCAM_ISP_REG_BASE + 0x0308)
#define VID_VCOEFF3 				(DCAM_ISP_REG_BASE + 0x030C)

#define VID_VCOEFF4					(DCAM_ISP_REG_BASE + 0x0310)
#define VID_VCOEFF5 				(DCAM_ISP_REG_BASE + 0x0314)
#define VID_VCOEFF6					(DCAM_ISP_REG_BASE + 0x0318)
#define VID_VCOEFF7 				(DCAM_ISP_REG_BASE + 0x031C)

#define VID_VCOEFF8					(DCAM_ISP_REG_BASE + 0x0320)
#define VID_VCOEFF9 				(DCAM_ISP_REG_BASE + 0x0324)
#define VID_VCOEFF10				(DCAM_ISP_REG_BASE + 0x0328)
#define VID_VCOEFF11 				(DCAM_ISP_REG_BASE + 0x032C)

#define VID_VCOEFF12				(DCAM_ISP_REG_BASE + 0x0330)
#define VID_VCOEFF13				(DCAM_ISP_REG_BASE + 0x0334)
#define VID_VCOEFF14				(DCAM_ISP_REG_BASE + 0x0338)
#define VID_VCOEFF15				(DCAM_ISP_REG_BASE + 0x033C)

#define VID_VCOEFF16				(DCAM_ISP_REG_BASE + 0x0340)
#define VID_VCOEFF17 				(DCAM_ISP_REG_BASE + 0x0344)
#define VID_VCOEFF18				(DCAM_ISP_REG_BASE + 0x0348)
#define VID_VCOEFF19 				(DCAM_ISP_REG_BASE + 0x034C)

#define VID_VCOEFF20				(DCAM_ISP_REG_BASE + 0x0350)
#define VID_VCOEFF21 				(DCAM_ISP_REG_BASE + 0x0354)
#define VID_VCOEFF22				(DCAM_ISP_REG_BASE + 0x0358)
#define VID_VCOEFF23 				(DCAM_ISP_REG_BASE + 0x035C)

#define VID_VCOEFF24				(DCAM_ISP_REG_BASE + 0x0360)
#define VID_VCOEFF25 				(DCAM_ISP_REG_BASE + 0x0364)
#define VID_VCOEFF26				(DCAM_ISP_REG_BASE + 0x0368)
#define VID_VCOEFF27 				(DCAM_ISP_REG_BASE + 0x036C)

#define VID_VCOEFF28				(DCAM_ISP_REG_BASE + 0x0370)
#define VID_VCOEFF29 				(DCAM_ISP_REG_BASE + 0x0374)
#define VID_VCOEFF30				(DCAM_ISP_REG_BASE + 0x0378)
#define VID_VCOEFF31 				(DCAM_ISP_REG_BASE + 0x037C)

#define VID_VCOEFF32				(DCAM_ISP_REG_BASE + 0x0380)
#define VID_VCOEFF33 				(DCAM_ISP_REG_BASE + 0x0384)
#define VID_VCOEFF34				(DCAM_ISP_REG_BASE + 0x0388)
#define VID_VCOEFF35 				(DCAM_ISP_REG_BASE + 0x038C)



#define DCAM_ISP_MODE					0x0
#define DCAM_VSP_MODE					0x1
#define DCAM_INT_MASK_VALUE				0x0000FFFF 
//#define DCAM_INT_MASK_VALUE				0x0000001f

#define SCALE_COEFF_V_NUM					48
#define SCALE_COEFF_H_NUM					36

#define ISP_HCLK_DOMAIN					1
#define ISP_CLK_DCAM_DOMAIN				0
#define ISP_JPEG_MEM_SIZE_1             1//512K BYTE
#define ISP_JPEG_MEM_SIZE_2             2//640K BYTE
#define ISP_JPEG_MEM_SIZE_3             3//768K
#define ISP_JPEG_MEM_SIZE_4             4//1024 BYTE
#define ISP_JPEG_MEM_SIZE_5             5//1.25M
#define ISP_JPEG_MEM_SIZE_6             6//1.5M
#define ISP_JPEG_MEM_SIZE_7             7//2M BYTE

/*
#define ISP_JPEG_MEM_SIZE_512K			512*1024
#define ISP_JPEG_MEM_SIZE_640K			640*1024
#define ISP_JPEG_MEM_SIZE_768K			768*1024
#define ISP_JPEG_MEM_SIZE_1024K			1024*1024
#define ISP_JPEG_MEM_SIZE_1280K			1280*1024
#define ISP_JPEG_MEM_SIZE_1536K			1536*1024
#define ISP_JPEG_MEM_SIZE_2048K			2048*1024
*/


/**---------------------------------------------------------------------------*
 **                          Compiler Flag                                    *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**---------------------------------------------------------------------------*
**                               Micro Define                                **
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                               Data Prototype                              **
**----------------------------------------------------------------------------*/
//top module reg define being
typedef union _dcam_cfg_tag {
	struct _dcam_cfg_map {
		volatile unsigned int reserved       	:24;//Reserved ;		    	
		volatile unsigned int clk_status    	:1;	//0:enable clock gating;1:disable clock gating
		volatile unsigned int cap_input_format	:1;	//0:YUV ; 1:jpeg
		volatile unsigned int cap_eb    		:1; //0:disable CAP module;1:enable CAP module
		volatile unsigned int clk_switch        :1; //0:clk_dcam_domain;1:HCLK
		volatile unsigned int vsp_eb  			:1; //0:dcam clock domain;1: enbale VSP module
		volatile unsigned int isp_mode			:2; //00:idle;01:capture;10:preview;11:review
		volatile unsigned int dcam_mode			:1; //0:ISP mode;1:VSP mode
	}mBits ;
	volatile unsigned int dwValue ;
}DCAM_CFG_U;

typedef union _cap_shadow_cntl_tag {
	struct _cap_shadow_cntl_map {
		volatile unsigned int reserved		:30;
		volatile unsigned int auto_copy_cap :1;
		volatile unsigned int frc_copy_cap	:1;
	}mBits;
	volatile unsigned int dwValue;
}CAP_SHADOW_CNTL_U;

typedef union _yuv2rgb_cfg_tag {
	struct _yuv2rgb_cfg_map {
		volatile unsigned int b0			:8;
		volatile unsigned int g0			:8;
		volatile unsigned int r0			:8;
		volatile unsigned int alpha			:6;
		volatile unsigned int dither_eb		:1;		
		volatile unsigned int yuv2rgb_eb	:1;
	}mBits;
	volatile unsigned int dwValue;
}YUV2RGB_CFG_U;

typedef union _dcam_src_size_tag {
	struct _dcam_src_size_map {
		volatile unsigned int reserved0		:4;
		volatile unsigned int src_size_y	:12;
		volatile unsigned int reserved1		:4;
		volatile unsigned int src_size_x	:12;
	}mBits;
	volatile unsigned int dwValue;
}DCAM_SRC_SIZE_U;

typedef union _isp_dis_size_tag {
	struct _isp_dis_size_map {
		volatile unsigned int reserved0		:7;
		volatile unsigned int dis_size_y	:9;
		volatile unsigned int reserved1		:7;
		volatile unsigned int dis_size_x    :9;
	}mBits;
	volatile unsigned int dwValue;
}ISP_DIS_SIZE_U;

typedef union _dcam_int_sts_tag {
	struct _dcam_int_sts_map {
		volatile unsigned int reserved		 :18;
		volatile unsigned int vsp_time_out	 :1;
		volatile unsigned int jpeg_buf_ovf   :1;
		volatile unsigned int cap_fifo_ovf	 :1;
		volatile unsigned int isp_buffer_ovf :1;
		volatile unsigned int vsp_mbio_done	 :1;
		volatile unsigned int vsp_vlc_done	 :1;
		volatile unsigned int vsp_bsm_done   :1;
		volatile unsigned int sc_done		 :1;
		volatile unsigned int rgb_done		 :1;
		volatile unsigned int capture_done	 :1;
		volatile unsigned int cap_eof		 :1;
		volatile unsigned int cap_sof		 :1;
		volatile unsigned int sensor_eof     :1;
		volatile unsigned int sensor_sof	 :1;
	}mBits;
	volatile unsigned int dwValue;
}DCAM_INT_STS_U;

typedef union _dcam_int_maks_tag {
	struct _dcam_int_mask_map {
		volatile unsigned int reserved		 :18;
		volatile unsigned int vsp_time_out	 :1;
		volatile unsigned int jpeg_buf_ovf   :1;
		volatile unsigned int cap_fifo_ovf	 :1;
		volatile unsigned int isp_buffer_ovf :1;
		volatile unsigned int vsp_mbio_done	 :1;
		volatile unsigned int vsp_vlc_done	 :1;
		volatile unsigned int vsp_bsm_done   :1;
		volatile unsigned int sc_done		 :1;
		volatile unsigned int rgb_done		 :1;
		volatile unsigned int capture_done	 :1;
		volatile unsigned int cap_eof		 :1;
		volatile unsigned int cap_sof		 :1;
		volatile unsigned int sensor_eof     :1;
		volatile unsigned int sensor_sof	 :1;
	}mBits;
	volatile unsigned int dwValue;
}DCAM_INT_MASK_U;

typedef union _dcam_int_clr_tag {
	struct _dcam_int_clr_map {
		volatile unsigned int reserved		 :18;
		volatile unsigned int vsp_time_out	 :1;
		volatile unsigned int jpeg_buf_ovf   :1;
		volatile unsigned int cap_fifo_ovf	 :1;
		volatile unsigned int isp_buffer_ovf :1;
		volatile unsigned int vsp_mbio_done	 :1;
		volatile unsigned int vsp_vlc_done	 :1;
		volatile unsigned int vsp_bsm_done   :1;
		volatile unsigned int sc_done		 :1;
		volatile unsigned int rgb_done		 :1;
		volatile unsigned int capture_done	 :1;
		volatile unsigned int cap_eof		 :1;
		volatile unsigned int cap_sof		 :1;
		volatile unsigned int sensor_eof     :1;
		volatile unsigned int sensor_sof	 :1;
	}mBits;
	volatile unsigned int dwValue;
}DCAM_INT_CLR_U;

typedef union _dcam_int_raw_tag {
	struct _dcam_int_raw_map {
		volatile unsigned int reserved		 :18;
		volatile unsigned int vsp_time_out	 :1;
		volatile unsigned int jpeg_buf_ovf   :1;
		volatile unsigned int cap_fifo_ovf	 :1;
		volatile unsigned int isp_buffer_ovf :1;
		volatile unsigned int vsp_mbio_done	 :1;
		volatile unsigned int vsp_vlc_done	 :1;
		volatile unsigned int vsp_bsm_done   :1;
		volatile unsigned int sc_done		 :1;
		volatile unsigned int rgb_done		 :1;
		volatile unsigned int capture_done	 :1;
		volatile unsigned int cap_eof		 :1;
		volatile unsigned int cap_sof		 :1;
		volatile unsigned int sensor_eof     :1;
		volatile unsigned int sensor_sof	 :1;
	}mBits;
	volatile unsigned int dwValue;
}DCAM_INT_RAW_U;

//top module reg define end

//cap reg define being
typedef union _cap_ctrl_tag {
	struct _cap_ctrl_map {
		volatile unsigned int reserved0		 :19;
		volatile unsigned int cap_ccir_pd	 :1;
		volatile unsigned int cap_ccir_rst   :1;
		volatile unsigned int fifo_data_rate :2;
		volatile unsigned int yuv_type		 :2;
		volatile unsigned int cut_first_y0   :1;
		volatile unsigned int reserved1		 :1;
		volatile unsigned int vsync_pol		 :1;
		volatile unsigned int hsync_pol		 :1;
		volatile unsigned int tv_frame_sel	 :2;
		volatile unsigned int ccir_656		 :1;
	}mBits;
	volatile unsigned int dwValue;
}CAP_CTRL_U;

typedef union _cap_frm_cnt_tag {
	struct _cap_frm_cnt_map {
		volatile unsigned int reserved0		:9;
		volatile unsigned int cap_frm_clr	:1;
		volatile unsigned int cap_frm_cnt	:6;
		volatile unsigned int reserved1		:4;
		volatile unsigned int cap_frm_deci	:4;
		volatile unsigned int reserved2		:2;
		volatile unsigned int pre_skip_cnt	:6;
	}mBits;
	volatile unsigned int dwValue;
}CAP_FRM_CNT_U;

typedef union _cap_start_tag {
	struct _cap_start_map {
		volatile unsigned int reserved0		:4;
		volatile unsigned int start_y       :12;
		volatile unsigned int reserved1		:4;
		volatile unsigned int start_x		:12;
	}mBits;
	volatile unsigned int dwValue;
}CAP_START_U;

typedef union _cap_end_tag {
	struct _cap_end_map {
		volatile unsigned int reserved0		:4;
		volatile unsigned int end_y			:12;
		volatile unsigned int reserved1     :4;
		volatile unsigned int end_x			:12;
	}mBits;
	volatile unsigned int dwValue;
}CAP_END_U;

typedef union _cap_image_deci_tag {
	struct _cap_image_deci_map {
		volatile unsigned int reserved		:23;
		volatile unsigned int cap_deci_mode :1;
		volatile unsigned int cap_deci_y	:4;
		volatile unsigned int cap_deci_x	:4;
	}mBits;
	volatile unsigned int dwValue;
}CAP_IMAGE_DECI_U;

typedef union _cap_trans_tag {
	struct _cap_trans_map {
		volatile unsigned int reserved		   :29;
		volatile unsigned int uv_pre_shift     :1;
		volatile unsigned int y_pre_shift	   :1;
		volatile unsigned int trans_eb		   :1;
	}mBits;
	volatile unsigned int dwValue;
}CAP_TRANS_U;

typedef union _cap_jpeg_frm_ctrl_tag {
	struct _cap_jpeg_frm_ctrl_map {
		volatile unsigned int reserved0		:21;
		volatile unsigned int mem_size		:3;
		volatile unsigned int jpeg_drop_num :4;
		volatile unsigned int reserved1     :1;
		volatile unsigned int get_num		:3;
	}mBits;
	volatile unsigned int dwValue;
}CAP_JPEG_FRM_CTRL_U;

//trim&scale
typedef union _scaling_cfg_tag {
	struct _scaling_cfg_map {
		volatile unsigned int reserved		:24;
		volatile unsigned int review_start  :1;
		volatile unsigned int input_format  :2;
		volatile unsigned int trim_eb		:1;
		volatile unsigned int ver_down_tap  :3;
		volatile unsigned int scale_bypass  :1;
	}mBits;
	volatile unsigned int dwValue;
}SCALING_CFG_U;

typedef union _trim_start_tag {
	struct _trim_start_map {
		volatile unsigned int reserved0		:4;
		volatile unsigned int start_y		:12;
		volatile unsigned int reserved1		:4;
		volatile unsigned int start_x       :12;
	}mBits;
	volatile unsigned int dwValue;
}TRIM_START_U;

typedef union _trim_size_tag {
	struct _trim_size_map {
		volatile unsigned int reserved0		:5;
		volatile unsigned int size_y 		:11;
		volatile unsigned int reserved1		:5;
		volatile unsigned int size_x		:11;
	}mBits;
	volatile unsigned int dwValue;
}TRIM_SIZE_U;
/*
typedef union _tag {
	struct _map {
		volatile unsigned int reserved		:
		volatile unsigned int 
		volatile unsigned int 
	}mBits;
	volatile unsigned int dwValue;
}_U;
*/



/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End 