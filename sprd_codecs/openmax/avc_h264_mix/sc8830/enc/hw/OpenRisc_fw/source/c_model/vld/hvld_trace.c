/*hvld_trace.c*/
#include <stdio.h>

#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"



void InitVldTrace ()
{
if(g_trace_enable_flag&TRACE_ENABLE_VLD)
{
	g_hvld_trace_fp = fopen ("..\\seq\\hvld_trace.txt", "w");
}
}
