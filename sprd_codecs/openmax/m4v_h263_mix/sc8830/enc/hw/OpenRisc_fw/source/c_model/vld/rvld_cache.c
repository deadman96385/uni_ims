/*rvld_cache.c*/
#include "common_global.h"
#include "rvld_mode.h"
#include "rvld_global.h"

/************************************************************************/
/* dsc4x4_tab_idx
	0: dsc4x4 for Luma intra mb and not intra16x16
	1: dsc4x4 for Luma dc_removed intra16x16 
	2: dc_y for Luma intra16x16 or inter16x16
	3: dsc4x4 for Luma inter mb and not inter16x16
	4: dsc4x4 for Chroma intra
	5: dsc4x4 for chroma inter    
	
	[0, 1, 2, 3, 4, 5]
	[1, 2, 3, 4, 5, 0]
	[2, 3, 4, 5, 0, 1]
	[3, 4, 5, 0, 1, 2]
	[4, 5, 0, 1, 2, 3]
	[5, 0, 1, 2, 3, 4]

  segmentation0: line 0  ~ 8
  segmentation1: line 9  ~ 17
  segmentation2: line 18 ~ 26
  segmentation3: line 27 ~ 35
  segmentation4: line 36 ~ 44
  segmentation5: line 45 ~ 53
                                   */
/************************************************************************/

int g_acc_times = 0;
int g_hit_times = 0;

uint32 GetDsc4x4Code (int dsc4x4_tab_idx, int dsc4x4_addr)
{
	int		cnt;
	int		part_idx_tab;		//segmentation number in one table, one table is partitioned into 6 segmentation
	int		part_idx_cache;		//cache is partitioned into 6 part, and the line is belong to which part
	int		line_start_part;	//first line number of this part
	int		line_idx_cache;		//line number in the cache
	int		line_num;			//line in one dsc4x4 table
	int		line_id_part;		//line_idx in one part
	int		line_offset;		//word offset in one cache line
	int		vdb_addr;
	uint32	vdb_rdata;
	int		tag_addr;			//tag address in huffman table
	int		tag_pos;			//tag position in one word, one word contain 4 tags, one byte for one tag
	int		tab_id_tag;			//table index of the tag
	int		huff_addr;
	int		huff_rdata;
	int		cache_addr;
	uint32	cache_rdata;
	uint32	tab_base_addr;
	int		tab_line_base;
	int		is_intra;
	uint32	offset_vdb;
	int		is_hit;

	g_acc_times++;


	/*localize the address in which segmentation and which cache line*/
	line_num	 =  dsc4x4_addr >> 3;
	part_idx_tab =  (line_num < 9) ?  0 :
					(line_num < 18) ? 1 :
					(line_num < 27) ? 2 :
					(line_num < 36) ? 3 :
					(line_num < 45) ? 4 : 5;

	line_start_part = part_idx_tab * 9;

	part_idx_cache  = (dsc4x4_tab_idx == 0)			   ? part_idx_tab : 
					  (part_idx_tab >= dsc4x4_tab_idx) ? (part_idx_tab - dsc4x4_tab_idx) : 
														 (part_idx_tab + 6 - dsc4x4_tab_idx);	

	/****************************************************************************************
	1. get the line_idx in one segmentation
	2. get the offset in the cache line, 
	3. convert to the address in cache buffer
	*****************************************************************************************/
	line_offset		= dsc4x4_addr & 0x7;
	line_id_part	= line_num - line_start_part;
	line_idx_cache	= part_idx_cache * 9 + line_id_part;	

	if ((dsc4x4_tab_idx == 2) && (((dsc4x4_addr >> 3) << 3) == 0x80))
		printf ("");
	if (line_idx_cache == 52)
		printf ("");

	/*1th cycle, read the tag memory, get the tag to indicate which table the contant is belong to*/
	tag_addr    = line_idx_cache >> 2;

	tag_pos     = line_idx_cache & 0x3;

	huff_addr   = TAG_BASE_ADDR + tag_addr;

	huff_rdata  = g_rvld_huff_tab[huff_addr];

	tab_id_tag  = (tag_pos == 0) ? huff_rdata & 0xff :
				  (tag_pos == 1) ? (huff_rdata >>  8) & 0xff :
				  (tag_pos == 2) ? (huff_rdata >> 16) & 0xff : (huff_rdata >> 24) & 0xff;

	/*1th cycle, read the cache buffer*/
	cache_addr  = line_idx_cache * 8 + line_offset;
	cache_rdata = g_rvld_cache_buf[cache_addr];

	is_hit = (tab_id_tag == dsc4x4_tab_idx) ? 1 : 0;
	
	g_hit_times = g_hit_times + is_hit;

	FPRINTF (g_cache_sta_fp, "%08x\n", (is_hit<<31) | (line_idx_cache << 3) | line_offset);

	/*2th cycle, judge the tag is effective, if be, reture the data*/
	if (tab_id_tag != dsc4x4_tab_idx)
	{
		/*if not, read one line data into cache line*/
		is_intra   = ((dsc4x4_tab_idx == 3) || (dsc4x4_tab_idx == 5)) ? 0 : 1;

		offset_vdb = (dsc4x4_tab_idx == 0) ? 0		:
					 (dsc4x4_tab_idx == 1) ? 432	:
					 (dsc4x4_tab_idx == 2) ? 432*2	:
					 (dsc4x4_tab_idx == 3) ? 0		:
					 (dsc4x4_tab_idx == 4) ? 432*3	: 432;

		tab_base_addr = (is_intra ? g_rvld_reg_ptr->intra_dsc4x4_addr : g_rvld_reg_ptr->inter_dsc4x4_addr) + offset_vdb*4;
				
		tab_line_base = (dsc4x4_addr >> 3) << 3;
	
		for (cnt = 0; cnt < 8; cnt++)
		{
			cache_addr		= line_idx_cache * 8 + cnt;

			vdb_addr		= tab_base_addr + (tab_line_base + cnt)*4;

			vdb_rdata		= *((uint32 *)vdb_addr);

			g_rvld_cache_buf [cache_addr] = vdb_rdata;

			if (cnt == line_offset)
			{
				cache_rdata = vdb_rdata;
			}
		}

		if (((huff_addr*4+tag_pos) == 0x287*4) && (dsc4x4_tab_idx == 4))
			printf ("");

		 /*update the cache tag memory*/
		((uint8 *)g_rvld_huff_tab)[huff_addr*4+tag_pos] = dsc4x4_tab_idx;
	}

	return cache_rdata;
}