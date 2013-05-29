#ifndef _RV_DBK_TRACE_H_
#define _RV_DBK_TRACE_H_
//#include "rvdec_basic.h"
#include "rdbk_mode.h"
#include "rdbk_global.h"
//#include "rvdec_mode.h"

#define RDBK_TRACE_FPRINTF //fprintf
#define RDBK_TRACE_ON 0//1
#define RDBK_TRACE //fprintf

extern FILE	*fp_rdbk_bf_frame;
extern FILE	*fp_rdbk_cmd_file;
extern FILE	*fp_rdbk_af_frame;
extern FILE	*fp_rdbk_trace_file;
extern FILE *fp_rdbk_config_trace;
extern FILE *fp_rdbk_input_data_trace;
extern FILE *fp_rdbk_blk_para_trace;
extern FILE *fp_rdbk_filter_para_trace;
extern FILE *fp_rdbk_filter_data_trace;
extern FILE *fp_rdbk_output_data_trace;


void RDbkTraceInit();
void PrintFrmBfDBK();
void PrintDBKCmd();
void PrintMBTrace(int width, uint8 * y_ptr, uint8 * u_ptr, uint8 *v_ptr,int mb_num_x,int mb_num_y,int g_nFrame);
//void PrintMBBeforeDBK(RV_DECODER_T * pDecoder, uint8 * y_ptr,uint8 *u_ptr,uint8 *v_ptr);
void Print_input_data(int mb_num);
void Print_config(int mb_num);
void Print_output_data(int mb_num,int last_row,int last_col);


#endif