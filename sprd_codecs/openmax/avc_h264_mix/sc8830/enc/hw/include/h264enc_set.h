#ifndef _H264ENC_SET_H_
#define	_H264ENC_SET_H_

void h264enc_sps_init (ENC_IMAGE_PARAMS_T *img_ptr);
void h264enc_pps_init (ENC_IMAGE_PARAMS_T *img_ptr);
void h264enc_sps_write (ENC_SPS_T *sps);
void h264enc_pps_write (ENC_PPS_T *pps);
void h264enc_sei_version_write(ENC_IMAGE_PARAMS_T *img_ptr);

#endif //_H264ENC_SET_H_