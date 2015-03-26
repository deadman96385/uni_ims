/******************************************************************************
 ** File Name:    mp4dec_deblock.c                                 	      *
 ** Author:       Simon.Wang                                                  *
 ** DATE:         08/31/2012                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 08/31/2012    Simon.Wang     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
const uint32 s_mp4_dbk_qp_tbl[32] =
{
    0, 2, 3, 5, 7, 8, 10, 11, 13, 15, 16, 18, 20, 21,
    23, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42, 44, 46, 47, 49, 51
};
const uint8 QP_SCALE_CR[52] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
    37, 38, 38, 38, 39, 39, 39, 39
};
const uint8 ALPHA_TABLE[52] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,5,6,
    7,8,9,10,12,13,15,17,20,22,25,28,32,36,40,45,
    50,56,63,71,80,90,101,113,127,144,162,182,203,226,255,255
};
const uint8 BETA_TABLE[52] = {
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,2,2,2,3, 3,3,3, 4, 4, 4, 6, 6,
    7, 7, 8, 8, 9, 9, 10,10, 11, 11, 12, 12, 13, 13, 14, 14,
    15, 15, 16, 16, 17, 17, 18, 18
};
const uint8 CLIP_TAB[52][5] =
{
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 0, 0
    }
    ,
    {
        0, 0, 0, 1, 1
    }
    ,
    {
        0, 0, 0, 1, 1
    }
    ,
    {
        0, 0, 0, 1, 1
    }
    ,
    {
        0, 0, 0, 1, 1
    }
    ,
    {
        0, 0, 1, 1, 1
    }
    ,
    {
        0, 0, 1, 1, 1
    }
    ,
    {
        0, 1, 1, 1, 1
    }
    ,
    {
        0, 1, 1, 1, 1
    }
    ,
    {
        0, 1, 1, 1, 1
    }
    ,
    {
        0, 1, 1, 1, 1
    }
    ,
    {
        0, 1, 1, 2, 2
    }
    ,
    {
        0, 1, 1, 2, 2
    }
    ,
    {
        0, 1, 1, 2, 2
    }
    ,
    {
        0, 1, 1, 2, 2
    }
    ,
    {
        0, 1, 2, 3, 3
    }
    ,
    {
        0, 1, 2, 3, 3
    }
    ,
    {
        0, 2, 2, 3, 3
    }
    ,
    {
        0, 2, 2, 4, 4
    }
    ,
    {
        0, 2, 3, 4, 4
    }
    ,
    {
        0, 2, 3, 4, 4
    }
    ,
    {
        0, 3, 3, 5, 5
    }
    ,
    {
        0, 3, 4, 6, 6
    }
    ,
    {
        0, 3, 4, 6, 6
    }
    ,
    {
        0, 4, 5, 7, 7
    }
    ,
    {
        0, 4, 5, 8, 8
    }
    ,
    {
        0, 4, 6, 9, 9
    }
    ,
    {
        0, 5, 7, 10, 10
    }
    ,
    {
        0, 6, 8, 11, 11
    }
    ,
    {
        0, 6, 8, 13, 13
    }
    ,
    {
        0, 7, 10, 14, 14
    }
    ,
    {
        0, 8, 11, 16, 16
    }
    ,
    {
        0, 9, 12, 18, 18
    }
    ,
    {
        0, 10, 13, 20, 20
    }
    ,
    {
        0, 11, 15, 23, 23
    }
    ,
    {
        0, 13, 17, 25, 25
    }
};

PUBLIC void Mp4Dec_Deblock_vop(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    int32 plane_idx;
    int32 line_cnt, edge_cnt;
    DEC_MB_MODE_T * mb_mode_ptr = vop_mode_ptr->pMbMode;
    DEC_MB_BFR_T * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    int32 edge_num;
    int32 line_num;
    int32 block_line_offset;
    int32 StratInTransY, StratInTransUV;
    uint8 qp = s_mp4_dbk_qp_tbl[vop_mode_ptr->StepSize];
    DBK_PARA_T * dbk_para = vop_mode_ptr->dbk_para;
    uint8 * ppxlcRecGob;
    uint8 * dst_DBK_Gob_ptr;
    uint8 * above_edge_src_ptr, * below_edge_src_ptr;
    uint8 * dst_block_ptr;

    SPRD_CODEC_LOGD("%s, vo->g_dbk_tmp_frm_ptr: 0x%p", __FUNCTION__, vo->g_dbk_tmp_frm_ptr);

    dbk_para->alpha = ALPHA_TABLE[IClip(0, 51, qp + 12)];
    dbk_para->beta  = BETA_TABLE[IClip(0, 51, qp + 12)];
    StratInTransY = vop_mode_ptr->FrameExtendHeight * YEXTENTION_SIZE + YEXTENTION_SIZE;
    StratInTransUV= (vop_mode_ptr->FrameExtendHeight >>1) * UVEXTENTION_SIZE + UVEXTENTION_SIZE;
    for(plane_idx = 0; plane_idx < 3; plane_idx ++)
    {
        //Horizontal 8x8 block edge.
        dbk_para->plane_idx = plane_idx;
        dbk_para->direction = 0;
        block_line_offset = 0;
        if(plane_idx == 0)
        {
            dbk_para->src_line_width = vop_mode_ptr->FrameExtendWidth;
            dbk_para->dst_line_width = vop_mode_ptr->FrameExtendHeight;
            ppxlcRecGob = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY;
            dst_DBK_Gob_ptr = vo->g_dbk_tmp_frm_ptr + StratInTransY - (BLOCK_SIZE/2);
            edge_num = (vop_mode_ptr->MBNumX << 1);
            line_num = (vop_mode_ptr->MBNumY <<1) + 1;
        } else
        {
            dbk_para->src_line_width = vop_mode_ptr->FrameExtendWidth >>1;
            dbk_para->dst_line_width = vop_mode_ptr->FrameExtendHeight>>1;
            ppxlcRecGob = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[plane_idx] + vop_mode_ptr->iStartInFrameUV;
            dst_DBK_Gob_ptr = vo->g_dbk_tmp_frm_ptr + StratInTransUV - (BLOCK_SIZE/2);
            edge_num = (vop_mode_ptr->MBNumX);
            line_num = (vop_mode_ptr->MBNumY) + 1;
        }

        for(line_cnt = 0; line_cnt < line_num; line_cnt ++)
        {
            below_edge_src_ptr = ppxlcRecGob + block_line_offset;
            above_edge_src_ptr = below_edge_src_ptr -dbk_para->src_line_width*(BLOCK_SIZE/2);
            dst_block_ptr = dst_DBK_Gob_ptr + line_cnt * BLOCK_SIZE;

            if(line_cnt ==0 || line_cnt == (line_num -1))
            {
                dbk_para->bs = 0;
            } else
            {
                dbk_para->bs = (line_cnt & 0x1)? 3: 4;
            }
            dbk_para->clip = CLIP_TAB[IClip(0,51,qp+12)][dbk_para->bs];
            for(edge_cnt = 0; edge_cnt < edge_num; edge_cnt ++)
            {
                //arm_Mp4Dec_Deblock_Process_Block(dbk_para, above_edge_src_ptr, below_edge_src_ptr, dst_block_ptr);
                above_edge_src_ptr += BLOCK_SIZE;
                below_edge_src_ptr += BLOCK_SIZE;
                dst_block_ptr += dbk_para->dst_line_width * BLOCK_SIZE;
            }
            block_line_offset += BLOCK_SIZE * dbk_para->src_line_width;
        }

        // Vertical 8x8 block edges.
        dbk_para->direction = 1;
        block_line_offset = 0;
        if(plane_idx == 0)
        {
            dbk_para->src_line_width = vop_mode_ptr->FrameExtendHeight;
            dbk_para->dst_line_width = vop_mode_ptr->FrameExtendWidth;
            ppxlcRecGob =  vo->g_dbk_tmp_frm_ptr + StratInTransY;
            dst_DBK_Gob_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0] + vop_mode_ptr->iStartInFrameY- (BLOCK_SIZE/2);
            edge_num = (vop_mode_ptr->MBNumY << 1);
            line_num = (vop_mode_ptr->MBNumX <<1) + 1;
        } else
        {
            dbk_para->src_line_width = vop_mode_ptr->FrameExtendHeight >>1;
            dbk_para->dst_line_width = vop_mode_ptr->FrameExtendWidth>>1;
            ppxlcRecGob =  vo->g_dbk_tmp_frm_ptr + StratInTransUV;
            dst_DBK_Gob_ptr =  vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[plane_idx] + vop_mode_ptr->iStartInFrameUV - (BLOCK_SIZE/2);
            edge_num = (vop_mode_ptr->MBNumY);
            line_num = (vop_mode_ptr->MBNumX) + 1;
        }

        for(line_cnt = 0; line_cnt < line_num; line_cnt ++)
        {
            below_edge_src_ptr = ppxlcRecGob -BLOCK_SIZE * dbk_para->src_line_width + block_line_offset;
            above_edge_src_ptr = below_edge_src_ptr+(2*BLOCK_SIZE - 4)*dbk_para->src_line_width;
            dst_block_ptr = dst_DBK_Gob_ptr + line_cnt * BLOCK_SIZE;

            if(line_cnt ==0 || line_cnt == (line_num -1))
            {
                dbk_para->bs = 0;
            } else
            {
                dbk_para->bs = (line_cnt & 0x1)? 3: 4;
            }
            dbk_para->clip = CLIP_TAB[IClip(0,51,qp+12)][dbk_para->bs];
            for(edge_cnt = 0; edge_cnt < edge_num; edge_cnt ++)
            {
                //arm_Mp4Dec_Deblock_Process_Block(dbk_para, above_edge_src_ptr, below_edge_src_ptr, dst_block_ptr);
                above_edge_src_ptr += BLOCK_SIZE;
                below_edge_src_ptr += BLOCK_SIZE;
                dst_block_ptr += dbk_para->dst_line_width * BLOCK_SIZE;
            }
            block_line_offset += BLOCK_SIZE * dbk_para->src_line_width;
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
