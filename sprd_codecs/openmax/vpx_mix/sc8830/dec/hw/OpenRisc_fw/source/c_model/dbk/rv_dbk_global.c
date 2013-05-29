#include "rdbk_global.h"
#include <stdio.h>

FILE	*fp_rdbk_bf_frame;
FILE	*cmd_file;
FILE	*fp_rdbk_af_frame;

// VSP_RDBK_REG_T * g_rdbk_reg_ptr;
// VSP_DBK_REG_T  * g_dbk_reg_ptr;
uint32	*vsp_rdbk_lbuf; //Line buffer
uint32	*vsp_rdbk_obuf; //Out buffer
uint32	*vsp_rdbk_tbuf; //Trans buffer

uint32	*vsp_ripred_lbuf; // Line buffer for Ipred to store undeblocked pixels of last line of above MB
uint8	vsp_ripred_left[35]; // register to store undeblocked pixels of right most colum of left MB and three topleft pixel
uint8	vsp_ripred_topleft_pix[3];// register to store undeblocked pixels of topleft of MB


//const uint8 g_strength [32] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5};
