/*hvlc_tv.c*/
#include <stdio.h>
#include <assert.h>

FILE * g_hvlc_event_fp;

void HVlcTestVectorInit ()
{
	g_hvlc_event_fp = fopen ("..\\..\\trace\\hvlc_event.txt", "w");

	assert (g_hvlc_event_fp != NULL);
}

