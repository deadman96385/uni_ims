/*hdbk_mode.h*/
#ifndef _HDBK_MODE_H_
#define _HDBK_MODE_H_

#include "video_common.h"

#define BLK_COMPONET_Y		0
#define BLK_COMPONET_U		1
#define BLK_COMPONET_V		2


#define Y_OFFSET_BUF_C		0
#define U_OFFSET_BUF_C		120
#define V_OFFSET_BUF_C		168

#define LINE_WIDTH_Y		512 //512 //weihu 480
#define LINE_WIDTH_C		256//256	//weihu 240
		
#define Y_OFFSET_LINE_BUF	0
#define U_OFFSET_LINE_BUF	LINE_WIDTH_Y*4
#define V_OFFSET_LINE_BUF	(LINE_WIDTH_Y*4 + LINE_WIDTH_C*4)


#endif