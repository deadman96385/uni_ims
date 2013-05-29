/*rdbk_mode.h*/
#ifndef _RDBK_MODE_H_
#define _RDBK_MODE_H_


#include "hdbk_mode.h"

//#define BLK_COMPONET_Y		0
//#define BLK_COMPONET_U		1
//#define BLK_COMPONET_V		2


#define Y_OFFSET_OBUF_C		0
#define U_OFFSET_OBUF_C		120
#define V_OFFSET_OBUF_C		168

#define Y_OFFSET_TBUF_C		0
#define U_OFFSET_TBUF_C		24
#define V_OFFSET_TBUF_C		40

#define Y_LINE_BUF_WIDTH	256
#define UV_LINE_BUF_WIDTH	(Y_LINE_BUF_WIDTH/2)

//#define Y_OFFSET_LINE_BUF	0
//#define U_OFFSET_LINE_BUF	(Y_LINE_BUF_WIDTH*4)
//#define V_OFFSET_LINE_BUF	(Y_LINE_BUF_WIDTH*4 + UV_LINE_BUF_WIDTH * 4)




#endif