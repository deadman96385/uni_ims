
#include "vp8dec_global.h"
#include "common_global.h"
#include "vp8dec_mode.h"

void MCASplitInit();
void MCASplitDeinit();

void Print_Stream_Offset(int offset, uint8* comment);
void Print_Frame_Header_CFG();
void Print_PPA_CFG(MACROBLOCKD *xd, VP8_COMMON *pc);
void Print_tbuf_Probs(VP8D_COMP *pbi);
void Print_Partition_Buf(MACROBLOCKD* xd);
void Print_VLD_Out(MACROBLOCKD *xd);
void Print_REF_Frame(YV12_BUFFER_CONFIG *frame_to_show, FILE *fp);
void Print_MCA_In(MACROBLOCKD *xd, uint32 filter_type);
void Print_IQIT_Out(MACROBLOCKD *xd, int idct_zero);
void Print_MCA_HOR(int *FDtata, uint32 output_height, uint32 output_width);
void Print_PRED_Out(MACROBLOCKD *xd, int idct_zero);
void Print_REC_Out(MACROBLOCKD *xd);
void Print_DBK_Out();
void Print_DBK_Frame(YV12_BUFFER_CONFIG *frame_to_show);
void Print_ISYN_Buf();
void Print_PPA_Line_Buf();
void Print_DBK_Buf();
void Print_MBC_Buf();
void Print_MCA_Buf(MACROBLOCKD *xd);
void Print_DCT_Buf(MACROBLOCKD *xd, VP8_COMMON *pc);
void Write_BUF_REG(int mb_row, int mb_col, MACROBLOCKD* xd, VP8_COMMON *pc);