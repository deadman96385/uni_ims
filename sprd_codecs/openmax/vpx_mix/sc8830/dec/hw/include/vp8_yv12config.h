#ifndef VP8_YV12CONFIG_H
#define VP8_YV12CONFIG_H

#define VP7BORDERINPIXELS       48
#define VP8BORDERINPIXELS       32

    /*************************************
     For INT_YUV:

     Y = (R+G*2+B)/4;
     U = (R-B)/2;
     V =  (G*2 - R - B)/4;
    And
     R = Y+U-V;
     G = Y+V;
     B = Y-U-V;
    ************************************/
    typedef enum
    {
        REG_YUV = 0,    // Regular yuv
        INT_YUV = 1     // The type of yuv that can be tranfer to and from RGB through integer transform
    }YUV_TYPE;

	typedef struct
    {
        int   y_width;
        int   y_height;
        int   y_stride;
//    int   yinternal_width;

        int   uv_width;
        int   uv_height;
        int   uv_stride;
//    int   uvinternal_width;

        unsigned char *y_buffer;
        unsigned char *u_buffer;
        unsigned char *v_buffer;

        unsigned char *buffer_alloc;
		int addr_idx;
        int border;
        int frame_size;


	unsigned int y_buffer_virtual;
	unsigned int u_buffer_virtual;
//        YUV_TYPE clrtype;

	void *pBufferHeader;
    } YV12_BUFFER_CONFIG;


int vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border_tmp, int addr_idx);
int vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf);
int vp8_yv12_init_frame_buffer(YV12_BUFFER_CONFIG *ybf, int width, int height, int border_tmp, int addr_idx);

#endif //VP8_YV12CONFIG_H