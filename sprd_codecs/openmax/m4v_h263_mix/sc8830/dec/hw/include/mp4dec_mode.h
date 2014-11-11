/******************************************************************************
 ** File Name:      mp4dec_mode.h		                                      *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    basic coding modes for VO, VOL, VOP and MB.			      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_MODE_H_
#define _MP4DEC_MODE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mpeg4dec.h"
#include "video_common.h"
#include "mp4dec_basic.h"
#include "mmcodec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

typedef struct cbpy_table_code_len_tag
{
    uint8 code;	/* right justified *///the value of the variable length code
    uint8 len;	// the length of the variable length code
} CBPY_TABLE_CODE_LEN_T;

typedef struct dc_table_code_len_tag
{
    int8 code;	/* right justified *///the value of the variable length code, modified by xiaowei
    uint8 len;	// the length of the variable length code
} DC_TABLE_CODE_LEN_T;

typedef enum {NOT_DECODED, DECODED_IN_ERR_PKT, DECODED_NOT_IN_ERR_PKT, DECODED_OK, ERR_CONCEALED} MB_DEC_STATUS_E;

typedef struct SLICEINFO_
{
//	unsigned int8 VideoStard:4;
    /*	unsigned char VOPCodingType:2;
        unsigned char ShortHeader:1;
    	unsigned char DataPartition:1;
        unsigned char QuantType:1;
        unsigned char :0;

    	unsigned char VopQuant:5;
    	unsigned char IntraDCThr:3;
        unsigned char :0;

    	unsigned char IsRvlc:1;
    	unsigned char  VopRoundingType:1;
    	unsigned char VOPFcodeFwd:3;
    	unsigned char VOPFcodeBck:3;*/

    unsigned char VOPCodingType;
    unsigned char ShortHeader;
    unsigned char DataPartition;
    unsigned char QuantType;

    unsigned char VopQuant;
    unsigned char IntraDCThr;


    unsigned char IsRvlc;
    unsigned char  VopRoundingType;
    unsigned char VOPFcodeFwd;
    unsigned char VOPFcodeBck;


    int8 video_std;
    int16 PicWidth ;
    int16 PicHeight;
    int16 NumMbsInGob;
    int16 GobNum;
    int16 NumMbLineInGob;
    int8  Max_MBX;
    int8  Max_MBy;
    int8  FirstMBx;
    int8  FirstMBy;
    int16  FirstMBNum;
    int8 SliceNum;
} SLICEINFO;



typedef struct dec_buffer_seq_info_tag
{
    uint8 *imgY;		//should be 64 word alignment
    uint8 *imgU;
    uint8 *imgV;

    uint_32or64 imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
    uint_32or64 imgUAddr;	//imgUVAddr = ((uint32)imgU>>8)
    uint_32or64 imgVAddr;	//imgUVAddr = ((uint32)imgV>>8)

    uint8	id;    	// buffer number
    BOOLEAN bRef;   	// FALSE£¬not to be  ref frame
    // TRUE, to  the ref frame
    BOOLEAN bDisp; 	// FALSE, not to be display frame
    // TRUE, to be the display frame

    uint8 *rec_imgY;
    uint8 *rec_info;
    uint_32or64 rec_infoAddr;

    void *pBufferHeader;
} DEC_FRM_BFR;

typedef struct Mp4Dec_storable_pic
{
    int		time;		//display time of the frame
    int		bfrId;		//yuv buffer index

    DEC_FRM_BFR *pDecFrame;

} Mp4DecStorablePic;

//for VideoObjectPlane Mode
typedef struct dec_vop_mode_tag
{
    /*about the size and position*/
    int8 MBNumX;
    int8 MBNumY;
    int8 mb_x;
    int8 mb_y;

    int16 MBNum;
    int16 OrgFrameWidth;

    int16 OrgFrameHeight;
    int16 FrameWidth;

    int16 FrameHeight;
    uint16 NumMBInGob;

    int16 num_mbline_gob;
    int8 NumGobInVop;	/*for short video header*/
    int8 GobNum;

    int16 post_filter_en;
    int16 resv;
    int32 resv1;

    int32 uv_interleaved;

    int8 video_std;
    int8 intra_acdc_pred_disable;
    int8 StepSize;
    int8 QuantPrecision;

    int8 QuantizerType;
    int8 bQuarter_pel;
    BOOLEAN bCoded;
    BOOLEAN srv;

    uint8 MB_in_VOP_length;
    uint8 time_inc_resolution_in_vol_length;
    int8 resv2;
    int8 resv3;

    int32 frame_len;

    BOOLEAN bResyncMarkerDisable;
    uint8 NumBitsTimeIncr;
    BOOLEAN bInitSuceess;
    BOOLEAN bInterlace; //interlace coding flag

    //for packet resync decoding
    int32 mbnumDec;
    int32 sliceNumber;
    int32 stop_decoding;
    int32 FirstMb;

    int32 pre_vop_type;
    int32 VopPredType;//whether IVOP, PVOP, BVOP
    int32 IntraDcSwitchThr;	 //threshold to code intraDC as with AC VLCs
    int32 RoundingControl;	 //rounding control

    MV_INFO_T mvInfoForward; //motion search info
    MV_INFO_T mvInfoBckward; //motion search info

    BOOLEAN  bAlternateScan;    //Alternate Scan
    BOOLEAN sprite_enable;/*gmc*/
    BOOLEAN  bDataPartitioning;	/* data partitioning*/
    BOOLEAN  bReversibleVlc;		/* reversible VLC*/

    BOOLEAN bLoadIntraQMatrix;		/* flag indicating whether to load intra Q-matrix*/
    BOOLEAN bLoadInterQMatrix;		/* flag indicating whether to load inter Q-matrix*/\
    BOOLEAN stuffing0;
    BOOLEAN stuffing1;
    uint8 IntraQuantizerMatrix[BLOCK_SQUARE_SIZE]; /* Intra Q-Matrix*/
    uint8 InterQuantizerMatrix[BLOCK_SQUARE_SIZE]; /* Inter Q-Matrix*/


    Mp4DecStorablePic *pCurDispFrame;
    Mp4DecStorablePic *pCurRecFrame;
    Mp4DecStorablePic *pBckRefFrame;
    Mp4DecStorablePic *pFrdRefFrame;

    /*for huffman decoding*/


    int32 sw_vld_flag;

    /*******************************************************/
    /*for computing time stamp for every frame*/
    int32 time;				/* for record time */
    int32 time_base;
    int32 last_time_base;
    int32 last_non_b_time;
    int32 time_pp;
    int32 time_bp;
    int32 time_inc_resolution;

    /*gmc*/
//	int sprite_enable;
    int sprite_warping_points;
    int sprite_warping_accuracy;
    int sprite_brightness_change;

    //flv_h263
    int8 h263_flv;
    int8 h263_plus;
    int8 unrestricted_mv;
    int8 h263_long_vectors;

    int32 picture_number;

    /*error information*/
    int			err_num;
    int			err_left;
    ERR_POS_T *	err_pos_ptr;

    int32 err_MB_num;		//error MB number

    uint8 *mbdec_stat_ptr;	//indicate mb decoded status,
    //0: not decoded,
    //1: decoded, but in error packet,
    //2: decoded, not in error packet,
    //3: error concealed

    int32 top_grad[16];
    int32 top_dir[16];
    int32 bot_grad[16];
    int32 bot_dir[16];
    int32 top_avg_grad;
    int32 top_var_grad;
    int32 bot_avg_grad;
    int32 bot_var_grad;

    //mv range
    int32 mv_x_max;
    int32 mv_x_min;
    int32 mv_y_max;
    int32 mv_y_min;

    //dct config
    uint32 intra_dct_cfg;
    uint32 inter_dct_cfg;

    int current_addr;

    int32 find_vop_header;

    uint8 * data_partition_buffer_ptr;
    uint_32or64 data_partition_buffer_Addr;
} DEC_VOP_MODE_T;

typedef struct h263_plus_header_info_tag
{
    int32 UFEP;
    int32 source_format;
    int32 mv_outside_frame;
    int32 long_vectors;
    int32 syntax_arith_coding;
    int32 adv_pred_mode;
    int32 overlapping_MC;
    int32 use_4mv;
    int32 pb_frame;

    /* Following variables are used in H.263+ bitstream decoding */
    int32 plus_type;           /* indicates if extended PTYPE exists or not. */
    int32 optional_custom_PCF;
    int32 advanced_intra_coding;
    int32 deblocking_filter_mode;
    int32 slice_structured_mode;
    int32 reference_picture_selection_mode;
    int32 independently_segmented_decoding_mode;
    int32 alternative_inter_VLC_mode;
    int32 modified_quantization_mode;
    int32 reduced_resolution_update_mode;
    int32 reference_picture_resampling_mode;
    int32 rtype;
    int32 post_filter;
    int32 unlimited_unrestricted_motion_vectors;
    int32 concealment;

    int32 CP_clock_frequency;
    int32 temp_ref;
    int32 prev_non_disposable_temp_ref;
    int32 next_non_disposable_temp_ref;

    int32 trd;
    int32 trb;
    int32 bscan;
    int32 bquant;
    int32 true_b_trb;
} H263_PLUS_HEAD_INFO_T;

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint8* v_base;  //virtual address
    uint_32or64 p_base;  //physical address
} CODEC_BUF_T;

typedef struct tagMp4DecObject
{
    uint_32or64 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 vsp_freq_div;
    int32	error_flag;
    int32   vsp_capability;

    MP4Handle  *mp4Handle;

    CODEC_BUF_T mem[MAX_MEM_TYPE];

    SLICEINFO SliceInfo;

    DEC_VOP_MODE_T *g_dec_vop_mode_ptr;
    DEC_FRM_BFR g_FrmYUVBfr[DEC_YUV_BUFFER_NUM];
    DEC_FRM_BFR g_DispFrmYUVBfr[DISP_YUV_BUFFER_NUM];

    uint32 g_nFrame_dec_h264;
    BOOLEAN g_dec_is_first_frame;
    BOOLEAN g_dec_is_stop_decode_vol;
    BOOLEAN g_dec_is_changed_format;
    VOP_PRED_TYPE_E g_dec_pre_vop_format;
    H263_PLUS_HEAD_INFO_T *g_h263_plus_head_info_ptr;
    uint32 g_nFrame_dec;

    DEC_FRM_BFR g_rec_buf;
    DEC_FRM_BFR g_tmp_buf;

    uint32 is_need_init_vsp_quant_tab;

    uint32 * g_rvlc_tbl_ptr;
    uint32 * g_huff_tbl_ptr;
    int32 memory_error;
    int32 yuv_format;
} Mp4DecObject;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif	//_MP4DEC_MODE_H_

