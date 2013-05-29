#include "vp8dbk_global.h"
#include "vp8dbk_trace.h"
FILE *  g_dbk_trace_fp;
VSP_DBK_REG_T  * g_dbk_reg_ptr;
VSP_GLB_REG_T  * g_glb_reg_ptr;
uint32 * vsp_dbk_out_bfr;		//172x32
#define MBC_OUT_BFR_SIZE			(216)
uint32			g_dbk_line_buf[4096];

void InitVp8DBKTrace()
{
	g_dbk_trace_fp = fopen("..\\seq\\dbk_para_trace.txt","w");
	g_dbk_reg_ptr	= (VSP_DBK_REG_T  *)malloc(sizeof(VSP_DBK_REG_T));
	g_glb_reg_ptr	= (VSP_GLB_REG_T  *)malloc(sizeof(VSP_GLB_REG_T));
	vsp_dbk_out_bfr		= (uint32  *)malloc(sizeof(uint32) * MBC_OUT_BFR_SIZE);
}

