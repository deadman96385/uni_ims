/******************************************************************************
 ** File Name:      common_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _COMMON_GLOBAL_H_
#define _COMMON_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "vsp_drv_sc8800g.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern int g_tv_cmd_num;

extern VSP_DCAM_REG_T * g_dcam_reg_ptr;
extern VSP_GLB_REG_T  * g_glb_reg_ptr;
extern VSP_AHBM_REG_T * g_ahbm_reg_ptr;
extern VSP_VLD_REG_T  * g_vld_reg_ptr;
extern VSP_HVLD_REG_T * g_hvld_reg_ptr;
extern VSP_BSM_REG_T  * g_bsm_reg_ptr;
extern VSP_MBC_REG_T  * g_mbc_reg_ptr;
extern VSP_DBK_REG_T  * g_dbk_reg_ptr;
extern VSP_MEA_REG_T  * g_mea_reg_ptr;
extern VSP_HMEA_REG_T * g_hmea_reg_ptr; 
extern VSP_DCT_REG_T  * g_dct_reg_ptr;
extern VSP_MCA_REG_T  * g_mca_reg_ptr;
extern VSP_VLC_REG_T  * g_vlc_reg_ptr;
extern VSP_ISYN_BUF_T * g_isyn_buf_ptr;
extern VSP_PPA_LINE_BUF_T * g_ppal_buf_ptr;

extern uint8 g_block_num_in_one_mcu;

extern int	g_stream_type;
//////////////////////////////////////////////////////////////////////////
//  TestVector FILES
//////////////////////////////////////////////////////////////////////////
extern FILE *g_fw_tv_cmd;
extern FILE *g_fp_vld_tv_run;
extern FILE *g_fp_vld_tv_lev;
extern FILE *g_fp_dbk_tv;
extern FILE *g_fp_before_flt_tv;
extern FILE *g_fp_dbk_mb_tv;
extern FILE *g_fp_dbk_frame_tv;
extern FILE *g_fp_mca_tv;
extern FILE *g_fp_vld_coeffPos;
extern FILE *g_fp_dct_tv;
extern FILE *g_fp_vlc_tv;
extern FILE *g_fp_mea_tv;
extern FILE *g_fp_mea_src_mb_tv;
extern FILE *g_fp_mea_prefilter_mb_tv;
extern FILE *g_fp_mea_ref_mb_tv;
extern FILE *g_fp_mea_src_frm_y_tv;
extern FILE *g_fp_mea_src_frm_u_tv;
extern FILE *g_fp_mea_src_frm_v_tv;
extern FILE *g_fp_mbc_tv;
extern FILE *g_fp_bsm_tv;
extern FILE *g_fp_isqt_tv;
extern FILE *g_fp_ipred_tv;
extern FILE *g_fp_rec_frm_tv;
extern FILE *g_fp_vld_tv;
extern FILE *g_fp_idct_tv;
	
//////////////////////////////////////////////////////////////////////////
//  TRACE FILES
//////////////////////////////////////////////////////////////////////////
extern FILE *g_fp_trace_isqt;
extern FILE *g_fp_trace_ipred;
extern FILE *g_fp_trace_dbk;
extern FILE *g_fp_trace_mbc;
extern FILE *g_ipred_log_fp;
extern FILE *g_fp_trace_mca;
extern FILE *g_fp_trace_mea;
extern FILE *g_fp_trace_mea_src_mb;
extern FILE *g_fp_trace_mea_ref_mb;
extern FILE *g_fp_trace_mea_sad;
extern FILE *g_fp_trace_vlc;
extern FILE *g_fp_trace_vld;
extern FILE *g_fp_trace_fw;
extern FILE *g_fp_trace_fw_bs;
extern FILE *g_fp_dbp;
extern FILE *g_pf_before_iqidct;
extern FILE *g_pf_after_iq;
extern FILE *g_pf_after_iqidct;
extern int g_fw_trace_mb_type;
extern FILE *pFmoFile;
extern FILE *g_fp_trace_mb_info;

extern uint8 g_luma_top[17+4];
extern uint8 g_luma_left[16];
extern uint8 g_chroma_top[2][9];
extern uint8 g_chroma_left[2][8];
extern uint8 g_top_left[3];

#define VLD_FPRINTF		//fprintf
#define VLC_FPRINTF		//fprintf
#define DBK_FPRINTF		//fprintf

#define SAFE_FREE(bfr) {\
	assert(bfr != PNULL);\
	free(bfr);\
	bfr = PNULL;\
}

void * safe_malloc(uint32 mem_size);

#define PRINTF_TV_NOTES	 fprintf

#define OUTPUT_FW_CMD_VECTOR(CmdType, RegAddr, value, pstring)	\
{\
	if(g_vector_enable_flag&VECTOR_ENABLE_FW)\
	{\
		FPRINTF(g_fw_tv_cmd,"%x,%08x,%08x \t  ",CmdType, RegAddr, value);\
		PRINTF_TV_NOTES(g_fw_tv_cmd,"//%s", pstring);\
		FPRINTF(g_fw_tv_cmd,"\n");\
		FFLUSH(g_fw_tv_cmd);\
	}\
	g_tv_cmd_num++;\
}
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void VSP_Init_CModel (void);
PUBLIC void VSP_Delete_CModel(void);
PUBLIC void AllocMCUBuf(void);
PUBLIC void VSP_WRITE_REG(uint32 reg_addr, int32 value, char *pstring);
PUBLIC uint32 VSP_READ_REG (uint32 reg_addr, int8 *pString);
PUBLIC int32 READ_REG_POLL(uint32 addr, uint32 mask,uint32 exp_value, uint32 time, char *pstring);

void fprintf_oneWord_hex (FILE * pf_txt, int val);
void FormatPrintHexNum(int num, FILE *fp);
void FomatPrint4Pix(uint8 *pLine, FILE *fp);

int InitVectorFiles();
int DenitVectorFiles(void);
int InitTraceFiles();
int DenitTraceFiles();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_COMMON_GLOBAL_H_
