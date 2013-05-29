
#include "sc6800x_video_header.h"

void h264enc_reference_update( ENC_IMAGE_PARAMS_T *img_ptr )
{
	H264EncStorablePic *pTmp = PNULL;

	pTmp = img_ptr->pYUVRefFrame;
	img_ptr->pYUVRefFrame = img_ptr->pYUVRecFrame;
	img_ptr->pYUVRecFrame = pTmp;

	return;
}

void h264enc_reference_build_list( ENC_IMAGE_PARAMS_T *img_ptr, int i_poc, int i_slice_type )
{
//     int i;

    /* build ref list 0/1 */
    img_ptr->i_ref0 = 0;

	if( i_slice_type == SLICE_TYPE_P )
	{
		img_ptr->i_ref0++;
	}
}