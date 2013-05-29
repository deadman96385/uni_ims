
#ifndef _VP8DEC_REG_H_
#define _VP8DEC_REG_H_

#include "video_common.h"
#include "vp8dec_mode.h"
#include "vsp_frame_header.h"

extern VSP_FH_REG_T   * g_fh_reg_ptr;

#define AHB_BASE_ADDR		0x20900000
#define AHB_SHARERAM_OFF	AHB_BASE_ADDR+0x200
#define AHB_VSP_GLB_OFF		AHB_BASE_ADDR+0x1000
#define AHB_BSM_CTRL_OFF	AHB_BASE_ADDR+0x8000

#ifdef ORSC_SIM
	#define ORSC_BASE_ADDR		0x00500000
#else
	#define ORSC_BASE_ADDR		0x80000000
#endif
#define ORSC_SHARERAM_OFF	ORSC_BASE_ADDR+0x200
#define ORSC_VSP_GLB_OFF	ORSC_BASE_ADDR+0x1000
#define ORSC_PPA_SINFO_OFF	ORSC_BASE_ADDR+0x1200		// ListX_POC[X][31:0], MCA weighted & offset table
#define ORSC_IQW_TBL_OFF	ORSC_BASE_ADDR+0x1400
#define ORSC_FMADD_TBL_OFF	ORSC_BASE_ADDR+0x1800
#define ORSC_VLC0_TBL_OFF	ORSC_BASE_ADDR+0x2000
#define ORSC_VLC1_TBL_OFF	ORSC_BASE_ADDR+0x3000
#define ORSC_PPA_PARAM_OFF	ORSC_BASE_ADDR+0x7400
#define ORSC_BSM_CTRL_OFF	ORSC_BASE_ADDR+0x8000


#define	VSP_REG_BASE_ADDR 0x80000000
#define	SHARE_RAM_BASE_ADDR (VSP_REG_BASE_ADDR+0x200)
#define	GLB_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x1000)
#define	PPA_SLICE_INFO_BASE_ADDR (VSP_REG_BASE_ADDR+0x1200)
#define	DCT_IQW_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1400)
#define	FRAME_ADDR_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1800)
#define	CABAC_CONTEXT_BASE_ADDR (VSP_REG_BASE_ADDR+0x1a00)
#define	VLC_TABLE0_BASE_ADDR (VSP_REG_BASE_ADDR+0x2000)
#define	VLC_TABLE1_BASE_ADDR (VSP_REG_BASE_ADDR+0x3000)
#define	BSM_CTRL_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x8000)


//glb reg
#define VSP_INT_SYS_OFF 0x0
#define VSP_INT_MASK_OFF 0x04
#define VSP_INT_CLR_OFF 0x08
#define VSP_INT_RAW_OFF 0x0c
#define AXIM_ENDIAN_SET_OFF 0x10
#define VSP_MODE_OFF 0x20
#define IMG_SIZE_OFF 0x24
#define RAM_ACC_SEL_OFF 0x28
#define VSP_INT_GEN_OFF 0x2c
#define VSP_START_OFF 0x30
#define VSP_SIZE_SET_OFF 0x34
#define VSP_CFG0_OFF 0x3c
#define VSP_CFG1_OFF 0x40
#define VSP_CFG2_OFF 0x44
#define VSP_CFG3_OFF 0x48
#define VSP_CFG4_OFF 0x4c
#define VSP_CFG5_OFF 0x50
#define VSP_CFG6_OFF 0x54
#define VSP_CFG7_OFF 0x58
#define BSM0_FRM_ADDR_OFF 0x60
//#define BSM1_FRM_ADDR_OFF 0x64
#define VSP_ADDRIDX0_OFF 0x68
#define VSP_ADDRIDX1_OFF 0x6c
#define VSP_ADDRIDX2_OFF 0x70
#define VSP_ADDRIDX3_OFF 0x74
#define VSP_ADDRIDX4_OFF 0x78
#define VSP_ADDRIDX5_OFF 0x7c
#define MCU_START_OFF 0x80//ro
#define CLR_START_OFF 0x84//wo
#define MCU_SLEEP_OFF 0x88//wo
#define VSP_DBG_STS0_OFF 0x100//mbx mby

//bsm reg

#define BSM_CFG0_OFF 0x0
#define BSM_CFG1_OFF 0x4
#define BSM_OP_OFF 0x8
#define BSM_WDATA_OFF 0xc
#define BSM_RDATA_OFF 0x10
#define TOTAL_BITS_OFF 0x14
#define BSM_DBG0_OFF 0x18
#define BSM_DBG1_OFF 0x1c
#define BSM_RDY_OFF 0x20
#define USEV_RD_OFF 0x24
#define USEV_RDATA_OFF 0x28
#define DSTUF_NUM_OFF 0x2c
#define BSM_NAL_LEN 0x34
#define BSM_NAL_DATA_LEN 0x38

/*#ifdef ORSC_SIM
	#define VLC_TABLE_BFR_ADDR	0x0600000
#else
	#define VLC_TABLE_BFR_ADDR	0x4500000
#endif*/
#define VLC_TABLE_BFR_SIZE	0x4000		// 16K

#ifdef SIM_IN_WIN
	#define OR1200_WRITE_REG(reg_addr, value, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"1_%08x_%08x \t\t//%s\n", reg_addr, value, pstring)
	#define OR1200_READ_REG(reg_addr, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"0_%08x_00000000 \t\t//%s\n", reg_addr, pstring)
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"2_%08x_%08x_%08x \t//%s\n", reg_addr, msk_data, msked_data, pstring)
#else
	#define OR1200_WRITE_REG(reg_addr, value, pstring) (*((volatile uint32*)(reg_addr)))=(value)
	#define OR1200_READ_REG(reg_addr, pstring) (*((volatile uint32*)(reg_addr)))
#ifdef ORSC_SIM
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring)
#else
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) \
	{\
	uint32 tmp;\
	tmp=(*((volatile uint32*)(reg_addr)))&(msk_data);\
	while(tmp != (msked_data))\
		{\
		tmp=(*((volatile uint32*)(reg_addr)))&(msk_data);\
		}\
	}
#endif
#endif

/*#define BITSTREAMFLUSHBITS(value, nbits) \
	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x20, V_BIT_0, 0x00000001,"ORSC: Polling BSM_RDY"); \
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, ((nbits)&0x3f)<<24, "ORSC: BSM_OPERATE: Set OPT_BITS"); \
#ifdef SIM_IN_WIN
	OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x10, "ORSC: BSM_RDATA"); \
#else
	value = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x10, "ORSC: BSM_RDATA"); \
#endif
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, (((nbits)&0x3f)<<24)|0x1, "ORSC: BSM_OPERATE: BSM_FLUSH n bits");*/


void SharedRAM_Init();
void ORSC_Init(VP8D_COMP *pbi);
void BSM_Init(unsigned long size);
uint32 BitstreamReadBits (uint32 nbits);
uint32 BITSTREAMSHOWBYTE(vp8_reader *stream, uint32 nbytes);
void Write_tbuf_Probs(VP8D_COMP *pbi);

#endif //VP8DEC_REG_H