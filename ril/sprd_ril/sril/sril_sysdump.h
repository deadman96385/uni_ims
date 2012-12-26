#ifndef _SRIL_SYSDUMP_H
#define _SRIL_SYSDUMP_H

#define	OEM_LOGCAT_MAIN						0x01
#define OEM_LOGCAT_RADIO					0x02
#define OEM_DUMPSTATE						0x03
#define OEM_KERNEL_LOG						0x04
#define OEM_LOGCAT_CLEAR					0x05
#define OEM_SYSDUMP_DBG_STATE_GET			0x06
#define OEM_SYSDUMP_ENABLE_LOG				0x07
//#define OEM_IPC_DUMP_LOG					0x08
//#define OEM_IPC_DUMP_BIN					0x09
#define OEM_RAMDUMP_MODE					0x0A
#define OEM_RAMDUMP_STATE_GET				0x0B
#define OEM_START_RIL_LOG					0x0C
//#define OEM_DEL_RIL_LOG						0x0D
#define OEM_MODEM_LOG					    0x12
#define OEM_TCPDUMP_START				    0x15
#define OEM_TCPDUMP_STOP					0x16


#define OEM_FACTORY_EVENT_0				0x00
#define OEM_FACTORY_EVENT_1				0x01
#define OEM_FACTORY_EVENT_2				0x02
#define OEM_FACTORY_EVENT_3				0x03

#define TEST_ITEM_NUM		90	//80
#define HISTORY_ITEM_NUM	60	//40
#define DUMMY_NUM	        20

typedef struct _OemReqMsgFactoryTest {
	unsigned char first_cmd;
	unsigned char sec_cmd;
	unsigned short file_size;
	unsigned char Id_1;
	unsigned char Id_2;
	unsigned char Id_3;
	unsigned char itemId;
	unsigned char result;
} OemReqMsgFactoryTest;

typedef struct {
	unsigned char 		test_id;
	unsigned char  		result;
} nv_factory_item_type_sys;

typedef struct {
	nv_factory_item_type_sys		factory_item[TEST_ITEM_NUM];
	nv_factory_item_type_sys		dummy_item[DUMMY_NUM];
} nvi_factory_test_check_type_sys;

typedef struct {
	nv_factory_item_type_sys		factory_item[HISTORY_ITEM_NUM];
} nvi_factory_history_type_sys;

#endif //_SRIL_SYSDUMP_H