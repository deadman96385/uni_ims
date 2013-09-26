/******************************************************************************
 ** File Name:    h264dec_fmo.c		                                          *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 03/29/2010    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define FMO_TRACE	0//1

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 0)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType0MbMap(H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 iGroup;
    int16 j;
    int32 i = 0;

    do
    {
        for (iGroup = 0; (iGroup <= pps_ptr->num_slice_groups_minus1) && (i < mb_num); i+= pps_ptr->run_length_minus1[iGroup++]+1)
        {
            for (j = 0; j <= pps_ptr->run_length_minus1[iGroup] && ((i+j)<mb_num); j++)
            {
                img_ptr->g_MbToSliceGroupMap[i+j] = (int8)iGroup;
            }
        }
    } while(i < mb_num);

    {
        uint32 add_mb_slice = 0;

        for (iGroup = 0; iGroup <= pps_ptr->num_slice_groups_minus1; iGroup++)
        {
            add_mb_slice += pps_ptr->run_length_minus1[iGroup];
        }
    }

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 1)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType1MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 i;

    for (i = 0; i < mb_num; i++)
    {
        img_ptr->g_MbToSliceGroupMap[i] = (int8)((i%img_ptr->frame_width_in_mbs)+(((i/img_ptr->frame_width_in_mbs)*(pps_ptr->num_slice_groups_minus1+1))/2))
                                          %(pps_ptr->num_slice_groups_minus1+1);
    }

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 2)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType2MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 iGroup;
    int32 i, x, y;
    int32 yTopLeft, xTopLeft, yBottomRight, xBottomRight;

    for (i = 0; i < mb_num; i++)
    {
        img_ptr->g_MbToSliceGroupMap[i] = pps_ptr->num_slice_groups_minus1;
    }

    for (iGroup = pps_ptr->num_slice_groups_minus1-1; iGroup >= 0; iGroup--)
    {
        yTopLeft = pps_ptr->top_left[iGroup]/img_ptr->frame_width_in_mbs;
        xTopLeft = pps_ptr->top_left[iGroup]%img_ptr->frame_width_in_mbs;
        yBottomRight = pps_ptr->bottom_right[iGroup]/img_ptr->frame_width_in_mbs;
        xBottomRight = pps_ptr->bottom_right[iGroup]%img_ptr->frame_width_in_mbs;

        for (y = yTopLeft; y <= yBottomRight; y++)
        {
            for (x = xTopLeft; x <= xBottomRight; x++)
            {
                img_ptr->g_MbToSliceGroupMap[y*img_ptr->frame_width_in_mbs+x] = iGroup;
            }
        }
    }

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 3)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType3MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 i, k;
    int32 leftBound, topBound, rightBound, bottomBound;
    int32 x, y, xDir, yDir;
    int32 mapUnitVacant;
    int32 mapUnitsInSliceGroup0 = mmin((int32)(pps_ptr->slice_group_change_rate_minus1+1)*img_ptr->slice_group_change_cycle, mb_num);

    for (i = 0; i < mb_num; i++)
    {
        img_ptr->g_MbToSliceGroupMap[i] = 2;
    }

    x = (img_ptr->frame_width_in_mbs - pps_ptr->slice_group_change_direction_flag)/2;
    y = ((int32)img_ptr->pic_height_in_map_units - pps_ptr->slice_group_change_direction_flag)/2;

    leftBound = x;
    topBound = y;
    rightBound = x;
    bottomBound = y;

    xDir = pps_ptr->slice_group_change_direction_flag-1;
    yDir = pps_ptr->slice_group_change_direction_flag;

    for (k = 0; k < mb_num; k += mapUnitVacant)
    {
        mapUnitVacant = (img_ptr->g_MbToSliceGroupMap[y*img_ptr->frame_width_in_mbs+x] == 2) ? 1 : 0;

        if (mapUnitVacant)
        {
            img_ptr->g_MbToSliceGroupMap[y*img_ptr->frame_width_in_mbs+x] = (k >= mapUnitsInSliceGroup0) ? 1 : 0;
        }

        if ((xDir == -1) && (x == leftBound))
        {
            leftBound = mmax(leftBound-1, 0);
            x = leftBound;
            xDir = 0;
            yDir = 2 * pps_ptr->slice_group_change_direction_flag - 1;
        } else if ((xDir == 1) && (x == rightBound))
        {
            rightBound = mmin(rightBound+1, (int)(img_ptr->frame_width_in_mbs) - 1);
            x = rightBound;
            xDir = 0;
            yDir = 1 - 2* pps_ptr->slice_group_change_direction_flag;
        } else if ((yDir == -1) && (y == topBound))
        {
            topBound = mmax(topBound-1, 0);
            y = topBound;
            xDir = 1 - 2*pps_ptr->slice_group_change_direction_flag;
            yDir = 0;
        } else if ((yDir == 1) && (y == bottomBound))
        {
            bottomBound = mmin(bottomBound+1, (int)img_ptr->pic_height_in_map_units-1);
            y = bottomBound;
            xDir = 2*pps_ptr->slice_group_change_direction_flag-1;
            yDir = 0;
        } else
        {
            x = x+xDir;
            y = y+yDir;
        }
    }

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 4)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType4MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 mapUnitsInSliceGroup0 = mmin((int32)(pps_ptr->slice_group_change_rate_minus1+1)*img_ptr->slice_group_change_cycle, mb_num);
    uint32 sizeOfUpperLeftGroup = pps_ptr->slice_group_change_direction_flag ? (mb_num - mapUnitsInSliceGroup0) : mapUnitsInSliceGroup0;
    int32 i;

    for (i = 0; i < mb_num; i++)
    {
        if (i < (int32)sizeOfUpperLeftGroup)
        {
            img_ptr->g_MbToSliceGroupMap[i] = (int8)(pps_ptr->slice_group_change_direction_flag);
        } else
        {
            img_ptr->g_MbToSliceGroupMap[i] = (int8)(1-pps_ptr->slice_group_change_direction_flag);
        }
    }

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 5)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType5MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 mapUnitsInSliceGroup0 = mmin((int32)(pps_ptr->slice_group_change_rate_minus1+1)*img_ptr->slice_group_change_cycle, mb_num);
    int32 sizeOfUpperLeftGroup = pps_ptr->slice_group_change_direction_flag ? (mb_num - mapUnitsInSliceGroup0) : mapUnitsInSliceGroup0;
    int32 i, j, k = 0;

    for (j = 0; j < (int32)img_ptr->frame_width_in_mbs; j++)
    {
        for (i = 0; i < (int32)img_ptr->pic_height_in_map_units; i++)
        {
            if (k++ < sizeOfUpperLeftGroup)
            {
                img_ptr->g_MbToSliceGroupMap[i*img_ptr->frame_width_in_mbs+j] = (int8)pps_ptr->slice_group_change_direction_flag;
            } else
            {
                img_ptr->g_MbToSliceGroupMap[i*img_ptr->frame_width_in_mbs+j] = (int8)(1-pps_ptr->slice_group_change_direction_flag);
            }
        }
    }

#if FMO_TRACE
    FPRINTF(pFmoFile, "pps->slice_group_change_rate_minus1 = %d\n", pps_ptr->slice_group_change_rate_minus1);
    FPRINTF(pFmoFile, "pps->slice_group_change_direction_flag = %d\n", pps_ptr->slice_group_change_direction_flag);
    FPRINTF(pFmoFile, "img->slice_group_change_cycle = %d\n", img_ptr->slice_group_change_cycle);
#endif

    return;
}

/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 6)
*
************************************************************************
*/
LOCAL void H264Dec_FmoGenerateType6MbMap (H264DecContext *img_ptr, DEC_PPS_T *pps_ptr, DEC_SPS_T *sps_ptr, int32 mb_num)
{
    int32 i;

    for (i = 0; i < mb_num; i++)
    {
        img_ptr->g_MbToSliceGroupMap[i] = (int8)(pps_ptr->slice_group_id[i]);
    }

    return;
}

LOCAL BOOLEAN H264Dec_FmoGenerateMbToSliceGroupMap(H264DecContext *img_ptr)
{
    int32 mb_num;
    DEC_PPS_T *pps_ptr = img_ptr->g_active_pps_ptr;
    DEC_SPS_T *sps_ptr = img_ptr->g_active_sps_ptr;

    mb_num = (sps_ptr->pic_height_in_map_units_minus1+1)*(sps_ptr->pic_width_in_mbs_minus1+1);
    if (img_ptr->g_MbToSliceGroupMap == PNULL)
    {
        img_ptr->g_MbToSliceGroupMap = (int8 *)H264Dec_MemAlloc(img_ptr, (uint32)(mb_num)*sizeof(int8), 4, SW_CACHABLE);
    }
#if FMO_TRACE
    FPRINTF(pFmoFile, "Decode Frame Num: %d, Slice group number: %d\n", g_nFrame_dec, pps_ptr->num_slice_groups_minus1+1);
#endif

    if (pps_ptr->num_slice_groups_minus1 == 0) //only one slice group
    {
        memset(img_ptr->g_MbToSliceGroupMap, 0, (uint32)(mb_num)*sizeof(int8));
        return TRUE;
    }

    switch(pps_ptr->slice_group_map_type)
    {
    case 0:
        H264Dec_FmoGenerateType0MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 1:
        H264Dec_FmoGenerateType1MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 2:
        H264Dec_FmoGenerateType2MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 3:
        H264Dec_FmoGenerateType3MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 4:
        H264Dec_FmoGenerateType4MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 5:
        H264Dec_FmoGenerateType5MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;
    case 6:
        H264Dec_FmoGenerateType6MbMap(img_ptr, pps_ptr, sps_ptr, mb_num);
        break;

    default:
        PRINTF ("Illegal slice_group_map_type %d , exit \n", pps_ptr->slice_group_map_type);
        return FALSE;
    }
#if FMO_TRACE
    {
        int32 i;
        FPRINTF(pFmoFile, "Slice group map type: %d, slice group map is shown below:\n", pps_ptr->slice_group_map_type);
        for(i = 0; i < mb_num; i++)
        {
            FPRINTF(pFmoFile, "%d,", g_MbToSliceGroupMap[i]);
            if( (i+1)%(sps_ptr->pic_width_in_mbs_minus1+1)==0)
            {
                FPRINTF(pFmoFile, "\n");
            }
        }
    }
#endif

    return TRUE;
}

PUBLIC BOOLEAN H264Dec_FMO_init(H264DecContext *img_ptr)
{
    return H264Dec_FmoGenerateMbToSliceGroupMap (img_ptr);
}

PUBLIC int32 H264Dec_Fmo_get_next_mb_num (int32 curr_mb_nr, H264DecContext *img_ptr)
{
    int32 slice_group = img_ptr->g_MbToSliceGroupMap[curr_mb_nr];

    while ((++curr_mb_nr < (int32)img_ptr->frame_size_in_mbs) && img_ptr->g_MbToSliceGroupMap[curr_mb_nr] != slice_group)
        ;

    if (curr_mb_nr >= (int32)img_ptr->frame_size_in_mbs)
    {
        return -1;// No further MB in this slice (could be end of picture)
    } else
    {
        return curr_mb_nr;
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
