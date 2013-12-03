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
   �ú�������AAC�������������ʵ��һ�����룬�ӿڲ������£�
       buffer_ptr��     ����������ֻ����һ����������
       buffer_size��    ��ǰ�������Ĵ�С
       pcm_out_l_ptr��  ���������PCM
       pcm_out_r_ptr��  ���������PCM
       frm_pcm_len_ptr�������ǰ������
       aac_dec_mem_ptr��������Ҫ��memory��ַ
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
   �ú�������AAC��ʼ���������ӿڲ������£�
       headerstream_ptr�� �������ͷ��Ϣ��������
       head_length��      ͷ��Ϣ��������
       sample_rate��      ������
       sign��             ����������Ч�����������ǲ����ʣ�����1��ʾ��������Ч��������������������Ϣ������
       frm_pcm_len_ptr��  �����ǰ������
       aac_buf_ptr    ��  ������Ҫ��memory��ַ
*/						
int16 AAC_DecInit(int8  *headerstream_ptr,  // input header info stream
                    int16  head_length,       // header stream length
                    int32  sample_rate,       // sample rate
                    int16  sign,              // 1: input sample rate, other: inout header stream 
					void     *aac_buf_ptr);     // allocate memory
/* 
   �ú������ڹ���demux��Ҫ�Ŀռ����Ϣ
   aac_mem_ptr����Ҫ������DEMUX
   �ɹ�����0��ʧ�ܷ��� 1
*/
int16 AAC_MemoryAlloc(void ** const aac_mem_ptr);    // memory constrct

/*
    �ú�����������demuxʹ�õĿռ����Ϣ��
    aac_mem_ptr����Ҫ������DEMUX
	�ɹ�����0��ʧ�ܷ��� 1
*/
int16 AAC_MemoryFree(void ** const aac_mem_ptr);    // memory deconstrct
/*
    �ú�������֪ͨ������seek��Ϣ��
    seek_sign��
        1�� seek ��־
        other�����������
*/
int16 AAC_DecStreamBufferUpdate(
                    int16  update_sign,         // 1: update mode, 
					void     *aac_buf_ptr);     // allocate memory




#ifdef __cplusplus
}
#endif
#endif
