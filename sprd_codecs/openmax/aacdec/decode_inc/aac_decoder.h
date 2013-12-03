/*************************************************************************
** File Name:      decoder.h                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_structs.h"





/* 
   该函数用于AAC幀级解码操作，实现一幀解码，接口参数如下：
       buffer_ptr：     输入码流，只能是一幀的码流。
       buffer_size：    当前幀码流的大小
       pcm_out_l_ptr：  输出左声道PCM
       pcm_out_r_ptr：  输出右声道PCM
       frm_pcm_len_ptr：输出当前幀幀长
       aac_dec_mem_ptr：解码需要的memory地址
*/
int16 AAC_FrameDecode(uint8           *buffer_ptr,         // the input stream
						uint32           buffer_size,    // input stream size
						uint16          *pcm_out_l_ptr,      // output left channel pcm
						uint16          *pcm_out_r_ptr,      // output right channel pcm
						uint16          *frm_pcm_len_ptr,     // frmae length 
						void            *aac_dec_mem_ptr,
						int16            aacplus_decode_flag,//1,
						int16	     *decoded_bytes
						);

uint32 AAC_RetrieveSampleRate(void     *aac_buf_ptr);

/* 
   该函数用于AAC初始化操作，接口参数如下：
       headerstream_ptr： 输入包含头信息的码流。
       head_length：      头信息码流长度
       sample_rate：      采样率
       sign：             区分输入有效値是码流还是采样率，输入1表示采样率有效，其他値，表明输入信息是码流
       frm_pcm_len_ptr：  输出当前幀幀长
       aac_buf_ptr    ：  解码需要的memory地址
*/						
int16 AAC_DecInit(int8  *headerstream_ptr,  // input header info stream
                    int16  head_length,       // header stream length
                    int32  sample_rate,       // sample rate
                    int16  sign,              // 1: input sample rate, other: inout header stream 
					void     *aac_buf_ptr);     // allocate memory
/* 
   该函数用于构造demux需要的空间等信息
   aac_mem_ptr：所要操作的DEMUX
   成功返回0，失败返回 1
*/
int16 AAC_MemoryAlloc(void ** const aac_mem_ptr);    // memory constrct

/*
    该函数用于析构demux使用的空间等信息。
    aac_mem_ptr：所要操作的DEMUX
	成功返回0，失败返回 1
*/
int16 AAC_MemoryFree(void ** const aac_mem_ptr);    // memory deconstrct
/*
    该函数用于通知解码器seek信息。
    seek_sign：
        1： seek 标志
        other：正常情况。
*/
int16 AAC_DecStreamBufferUpdate(
                    int16  update_sign,         // 1: update mode, 
					void     *aac_buf_ptr);     // allocate memory




#ifdef __cplusplus
}
#endif
#endif
