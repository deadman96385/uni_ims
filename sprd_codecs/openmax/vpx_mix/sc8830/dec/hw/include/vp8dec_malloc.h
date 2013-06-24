
#ifndef VP8DEC_MALLOC_H
#define VP8DEC_MALLOC_H

#include "mmcodec.h"
#include "sci_types.h"

PUBLIC void *vp8dec_ExtraMemAlloc(uint32 mem_size);
PUBLIC void *vp8dec_ExtraMemAlloc_64WordAlign(uint32 mem_size);
PUBLIC void *vp8dec_InterMemAlloc(uint32 mem_size);
PUBLIC void vp8dec_ExtraMemFree(uint32 mem_size);
PUBLIC void vp8dec_InterMemFree(uint32 mem_size);
PUBLIC void vp8dec_FreeMem(void) ;
PUBLIC void vp8dec_InitInterMem(MMCodecBuffer *dec_buffer_ptr);
MMDecRet VP8DecMemInit(MMCodecBuffer *pBuffer);
PUBLIC uint32 VP8Dec_GetPhyAddr(void * vitual_ptr);

#endif //VP8DEC_MALLOC_H