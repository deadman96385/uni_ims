
/*
 *
 * config.h: configuration  implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#ifndef config_H
#include <errno.h>

#define config_H

/*The switch to connect gprs through
 * pppd or virtual enthernet
 * ifdef CONFIG_VETH -->virtual enternet
 * ifndef CONFIG_VETH -->pppd*/

//#define CONFIG_VETH

#if defined CONFIG_SINGLE_SIM
#define CHN_NUM 						18	//  number of channel buffer
#elif defined CONFIG_DUAL_SIM
#define CHN_NUM 						27	//  number of channel buffer
#endif
#define SERIAL_BUFFSIZE   					(8*1024)	// channel buffer size
#define MAX_AT_RESPONSE 					(8 * 1024)

#define MUX_NUM  						15	//CSMUX_NUM+GSMUX_NUM+PSMUX_NUM+STK+PBK+SS+SIM+STM+NW+4PPP+1VT+IND

#define CSMUX_NUM  						1
#define PSMUX_NUM  						1
#define GSMUX_NUM  						1

#define MISCMUX_NUM  						1


#define RESMUX_NUM  						0	//other mux out of channel manager

#if defined CONFIG_SINGLE_SIM
#define GSM_WAIT_NUM						4
#define CSM_WAIT_NUM						4
#define PSM_WAIT_NUM						4
#define STMM_WAIT_NUM                            		4

#define PTY_CHN_NUM  						6	//send thread number
#define INDPTY_NUM						1
#define PHS_MUX_NUM  						11	//Receive thread number

typedef enum mux_type_t { CSM, PSM, GSM,NWM,SIMM,SSM,PBKM,STKM, SMSM,SMSTM,INDM, STMM, AT,
	STMAT, AUDAT, IND, RESERVE,
} mux_type;
#elif defined CONFIG_DUAL_SIM

#define SIM1_WAIT_NUM						4
#define SIM2_WAIT_NUM						4
#define SIM3_WAIT_NUM						4
#define SIM4_WAIT_NUM						4

#define PTY_CHN_NUM  						15	//send thread number
#define INDPTY_NUM						4
#define PHS_MUX_NUM  						12	//Receive thread number


typedef enum mux_type_t { ATM1_SIM1,ATM2_SIM1,ATM1_SIM2,ATM2_SIM2,
	ATM1_SIM3,ATM2_SIM3,ATM1_SIM4,ATM2_SIM4,
	INDM_SIM1, INDM_SIM2, INDM_SIM3, INDM_SIM4,
	VTM_SIM1, VTM_SIM2, VTM_SIM3, VTM_SIM4,
	AT_SIM1,AT_SIM2,AT_SIM3,AT_SIM4,
	IND_SIM1, IND_SIM2, IND_SIM3, IND_SIM4,
	STMAT,RESERVE,
} mux_type;
#endif
typedef struct channel_config channel_config;
struct channel_config {

	/***    User explicit entries    ***/
	char *dev_str;		/*## attribute dev_str , device node name */
	int index;
	mux_type type;		/*## attribute type */
	int prority;		/*## channel thread's prority */
};
struct chns_config_t {

	/***    User explicit entries    ***/
	channel_config mux[MUX_NUM];
	channel_config pty[PTY_CHN_NUM];
	channel_config itsMngPty;
};

/*cmux0-9:
  cmux0  indicator,misc at; (P)
  cmux1  vt data
  cmux2-5 ppp

  cmux6  cs's AT (P)
  cmux7  ps's AT (P)
  cmux8  general (P)
  cmux9  cmmb
  cmux10  cp log

*/

#endif /*  */
