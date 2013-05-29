/*h264dbk_trace.c*/

#include "sc6800x_video_header.h"

FILE * g_dbk_trace_fp;
//#define DBK_FPRINTF fprintf //weihu

void InitDbkTrace ()
{
	g_dbk_trace_fp = fopen ("..\\..\\trace\\hdbk_trace.txt", "w");
}

void DBK_TraceMBInfor (
					   int		cur_qp, 
					   int		left_qp, 
					   int		top_qp, 
					   int		chroma_qp_offset, 
					   int		alpha_offset, 
					   int		beta_offset
					   )
{
	if(g_trace_enable_flag&TRACE_ENABLE_DBK)
	{
		DBK_FPRINTF (g_dbk_trace_fp, "current_MB_QP: %d, left_MB_QP: %d, top_MB_QP: %d\n",
			cur_qp, left_qp, top_qp);
		
		DBK_FPRINTF (g_dbk_trace_fp, "chroma_qp_index_offset: %d, alpha_offset: %d, beta_offset: %d", 
			chroma_qp_offset, alpha_offset, beta_offset);
	}
}

void DBK_TraceMBBs (
					uint32	bs_h0,
					uint32	bs_h1,
					uint32	bs_v0,
					uint32	bs_v1
					)
{
	if(g_trace_enable_flag&TRACE_ENABLE_DBK)
	{
		DBK_FPRINTF (g_dbk_trace_fp, "vertical edge BS: \n");
		
		DBK_FPRINTF (g_dbk_trace_fp, "%d,   %d,   %d,   %d\n", (bs_h0>>0)  & 0xf, (bs_h0>>16) & 0xf, (bs_h1>>0)  & 0xf, (bs_h1>>16) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d,   %d,   %d,   %d\n", (bs_h0>>4)  & 0xf, (bs_h0>>20) & 0xf, (bs_h1>>4)  & 0xf, (bs_h1>>20) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d,   %d,   %d,   %d\n", (bs_h0>>8)  & 0xf, (bs_h0>>24) & 0xf, (bs_h1>>8)  & 0xf, (bs_h1>>24) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d,   %d,   %d,   %d\n", (bs_h0>>12) & 0xf, (bs_h0>>28) & 0xf, (bs_h1>>12) & 0xf, (bs_h1>>28) & 0xf);
		
		DBK_FPRINTF (g_dbk_trace_fp, "horizontal edge BS: \n");
		DBK_FPRINTF (g_dbk_trace_fp, "%d, %d, %d, %d\n\n", (bs_v0>>0)  & 0xf, (bs_v0>>4)  & 0xf, (bs_v0>>8)  & 0xf, (bs_v0>>12) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d, %d, %d, %d\n\n", (bs_v0>>16) & 0xf, (bs_v0>>20) & 0xf, (bs_v0>>24) & 0xf, (bs_v0>>28) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d, %d, %d, %d\n\n", (bs_v1>>0)  & 0xf, (bs_v1>>4)  & 0xf, (bs_v1>>8)  & 0xf, (bs_v1>>12) & 0xf);
		DBK_FPRINTF (g_dbk_trace_fp, "%d, %d, %d, %d\n",   (bs_v1>>16) & 0xf, (bs_v1>>20) & 0xf, (bs_v1>>24) & 0xf, (bs_v1>>28) & 0xf);
	}
}

void DBK_PrintOneMB (int mb_y, int mb_x)
{
if(g_trace_enable_flag&TRACE_ENABLE_DBK)
{
	int		i;
	int		j;
	uint8 * pix_ptr;
	uint8	pix_data;

	/*Y component*/
	/*top 4 line*/
	DBK_FPRINTF (g_dbk_trace_fp, "Y component:\n");
	pix_ptr = (uint8 *)(g_dbk_line_buf + mb_x* 4);
	for (i = 0; i < 4; i++)
	{
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		for (j = 0; j < 16; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}

		DBK_FPRINTF (g_dbk_trace_fp, "\n");

		pix_ptr += LINE_WIDTH_Y*4;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + 24);
	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 24; j++)
		{
			if (j < 8)//weihu//j < 8
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
		
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 24;
	}


	/*U component*/
	DBK_FPRINTF (g_dbk_trace_fp, "U component:\n");
	pix_ptr = (uint8 *)(g_dbk_line_buf + U_OFFSET_LINE_BUF + mb_x*2 );//+ U_OFFSET_LINE_BUF + mb_x*2 + 2*64);//weihu
	for (i = 0; i < 4; i++)//weihu//i < 2
	{
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");//DBK_FPRINTF (g_dbk_trace_fp,"            ");

		for (j = 0; j < 8; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += LINE_WIDTH_C*4;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + U_OFFSET_BUF_C + 16) ;//+ U_OFFSET_BUF_C + 12) + 2;//weihu
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 16; j++)//weihu//j < 10
		{
			if (j < 8)//weihu//j < 2
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;//weihu//12
	}


	/*V component*/
	DBK_FPRINTF (g_dbk_trace_fp, "V component:\n");
	pix_ptr = (uint8 *)(g_dbk_line_buf + V_OFFSET_LINE_BUF + mb_x*2 );
	for (i = 0; i < 4; i++)
	{
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");//DBK_FPRINTF (g_dbk_trace_fp,"            ");
		for (j = 0; j < 8; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += LINE_WIDTH_C*4;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + V_OFFSET_BUF_C + 16);
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 16; j++)
		{
			if (j < 8)
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;
	}
}
}

void DBK_PrintFilterOneMB (int mb_y, int mb_x)
{
if(g_trace_enable_flag&TRACE_ENABLE_DBK)
{
	int		i;
	int		j;
	uint8 * pix_ptr;
	uint8	pix_data;


	/*Y component*/
	/*top 4 line*/
	DBK_FPRINTF (g_dbk_trace_fp, "Y component:\n");
	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + 2);
	for (i = 0; i < 4; i++)
	{
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		for (j = 0; j < 16; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}

		DBK_FPRINTF (g_dbk_trace_fp, "\n");

		pix_ptr += 24;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + 24);
	for (i = 0; i < 12; i++)
	{
		for (j = 0; j < 24; j++)
		{
			if (j < 4)
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
		
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 24;
	}

	pix_ptr = (uint8 *)(g_dbk_line_buf + mb_x*4 - 2);
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 24; j++)
		{
			if (j < 8)//weihu
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
		
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += LINE_WIDTH_Y*4;
	}


	/*U component*/
	DBK_FPRINTF (g_dbk_trace_fp, "U component:\n");
	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + U_OFFSET_BUF_C  + 2);//weihu// + 4*2 + 2);
	for (i = 0; i < 4; i++)//weihu//<2
	{
		//DBK_FPRINTF (g_dbk_trace_fp,"            ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		for (j = 0; j < 8; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + U_OFFSET_BUF_C + 16); //weihu//+ 2;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 16; j++)//weihu//10
		{
			if (j < 8)//weihu
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;
	}

	pix_ptr = (uint8 *)(g_dbk_line_buf + U_OFFSET_LINE_BUF + mb_x*2 - 2); //+ 2;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 16; j++)//10
		{
			if (j < 8)
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += LINE_WIDTH_C*4;
	}

	/*V component*/
	DBK_FPRINTF (g_dbk_trace_fp, "V component:\n");
	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + V_OFFSET_BUF_C + 2);//weihu// 3 *2 + 1);
	for (i = 0; i < 4; i++)
	{
		//DBK_FPRINTF (g_dbk_trace_fp,"            ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		DBK_FPRINTF (g_dbk_trace_fp,"                        ");
		for (j = 0; j < 8; j++)
		{
			if (mb_y > 0)
			{
				pix_data = pix_ptr[j];
			}
			else
			{
				pix_data = 0;
			}
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;
	}

	pix_ptr = (uint8 *)(vsp_dbk_out_bfr + V_OFFSET_BUF_C + 16); //+ 2;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 16; j++)
		{
			if (j < 8)
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += 16;
	}

	pix_ptr = (uint8 *)(g_dbk_line_buf + V_OFFSET_LINE_BUF + mb_x * 2 - 2);//mb_x * 2 - 1) + 2;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 16; j++)
		{
			if (j < 8)
			{
				if (mb_x > 0)
				{
					pix_data = pix_ptr[j];
				}
				else
				{
					pix_data = 0;
				}
			}
			else
			{
				pix_data = pix_ptr[j];
			}
			
			DBK_FPRINTF (g_dbk_trace_fp, "0x%02x, ", pix_data);
		}
		
		DBK_FPRINTF (g_dbk_trace_fp, "\n");
		
		pix_ptr += LINE_WIDTH_C*4;
	}
}
}
