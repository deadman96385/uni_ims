#ifndef _H264ENC_MALLOC_H_
#define _H264ENC_MALLOC_H_

PUBLIC void *h264enc_extra_mem_alloc (uint32 mem_size);
PUBLIC void *h264enc_inter_mem_alloc (uint32 mem_size);
PUBLIC void h264enc_mem_free (void);
PUBLIC 	void h264enc_mem_init (MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr);
PUBLIC void *H264Enc_ExtraMemAlloc_64WordAlign(uint32 mem_size);


#endif //_H264ENC_MALLOC_H_
