#ifndef _SAMSUNG_NV_FLASH_H_
#define _SAMSUNG_NV_FLASH_H_

#define 	BACKUP_MAGIC_CODE			0xABCDEF
#define	HW_VERSION_LENGTH			16
#define	SW_VERSION_LENGTH			16
#define 	LINE_NUMBER_LENGTH			4
#define 	PC_NUMBER_LENGTH				4
#define 	CALIBRATION_DATE_LENGTH 		16
#define 	RTC_LENGTH						12
#define 	BLUETOOTH_ID_ARRAY			6
#define 	FACTORY_DUMMY_SIZE	200
#define 	DUMMY_SIZE			200

#define  	NETWORK_MODE_GSM850  			0x10
#define 	NETWORK_MODE_GSM900  			0x20
#define 	NETWORK_MODE_DCS1800  			0x40
#define 	NETWORK_MODE_PCS1900  			0x80
#define 	NETWORK_MODE_GSM900_1800  		0x60
#define 	NETWORK_MODE_GSM850_1900  		0x90
#define 	NETWORK_MODE_AUTO  				0x7FF8

// TOTAL 120
#if 0 // Change Test NV Size
#define TEST_ITEM_NUM		40	//80
#else
#define TEST_ITEM_NUM		90	//180
#define TEST_ITEM_OLD_NUM		40	//80
#endif
#define HISTORY_ITEM_NUM	60	//40
#define DUMMY_NUM	20

// Constants for FILE Personalization
#define  MAX_GID_FIELD_LENGTH		10

#define PERSO_STATES_ITEM_LENGTH      5
#define PERSO_TAG_ITEM_LENGTH         1

#define PERSO_CODE_ITEM_LENGTH        1024
#define PERSO_CKS_ITEM_LENGTH         32

#define PERSO_CKS_ATTEMPTS_ITEM_LENGTH  5
#define PERSO_MAX_ATTEMPTS_ITEM_LENGTH  5

#define PERSO_STATES_LENGTH PERSO_STATES_ITEM_LENGTH
#define PERSO_CODES_LENGTH PERSO_CODE_ITEM_LENGTH*5
#define PERSO_CKS_LENGTH PERSO_CKS_ITEM_LENGTH*4
#define PERSO_CKS_ATTEMPTS_LENGTH PERSO_CKS_ATTEMPTS_ITEM_LENGTH
#define PERSO_MAX_ATTEMPTS_LENGTH PERSO_MAX_ATTEMPTS_ITEM_LENGTH
#define PERSO_TAG_LENGTH PERSO_TAG_ITEM_LENGTH*5

// ---------- NV ITEM TYPE DEFINE -start--------------------------------------

typedef struct {
	unsigned	long backup_flag; 	// 4 Bytes
	unsigned	long active_flag[10];	// 4 * 10 Bytes
} nv_backup_header_t;


typedef struct
{
	unsigned	char blueTooth_Id[BLUETOOTH_ID_ARRAY];
	unsigned	short padding;
}nvi_bluetooth_ID_t;


typedef struct
{
	unsigned	char HW_Version[HW_VERSION_LENGTH];
}nvi_HW_version_t;


typedef struct
{
	unsigned	char Cal_Date[CALIBRATION_DATE_LENGTH];
}nvi_CalDate_t;


typedef struct
{
	unsigned	char SW_Version[SW_VERSION_LENGTH];
}nvi_SW_version_t;


typedef struct
{
	unsigned int band;
}nvi_Setband_t;


typedef struct
{
	unsigned char          product_code[20];
} nvi_product_code_type;


typedef struct
{
	unsigned short          product_info;
} nvi_product_info_type;


typedef struct
{
	unsigned short          PowerOnAttach;
}nvi_Auto_Attach_type;


typedef struct {
	unsigned char		mac_address[6];
} nvi_wlan_mac_address_type;


// js0809.kim 2009.12.08 Serial Number Test
typedef struct
{
	unsigned char serialNum[16];
}nvi_SerialNum_t;


typedef struct
{
	unsigned char CSCVer[40];
}nvi_CSCVer_t;


/* --------------------------------------------------------------------- */


typedef struct {
	unsigned char 		test_id;
	unsigned char  		result;
} nv_factory_item_type;


typedef struct {
	nv_factory_item_type		factory_item[TEST_ITEM_NUM];
	nv_factory_item_type		dummy_item[DUMMY_NUM];
} nvi_factory_test_check_type;


typedef struct {
	nv_factory_item_type		factory_item[HISTORY_ITEM_NUM];
} nvi_factory_history_type;


typedef struct {
	unsigned char  		fail_record;
	unsigned char  		retry;
	unsigned char 		dummy1;
	unsigned char 		dummy2;
} nvi_factory_fail_record_type;


typedef struct
{
	unsigned	char dummy[FACTORY_DUMMY_SIZE];
}nvi_factory_dummy_t;


/* --------------------------------------------------------------------- */


typedef struct
{
	char CalText[8];
}nvi_SensorCalDone_type;


typedef struct
{
        int CalDataX;
        int CalDataY;
        int CalDataZ;
}nvi_SensorCalData_type;


/* --------------------------------------------------------------------- */


typedef struct
{
	unsigned short 			checksum;
	unsigned short			padding;
}nvi_sysparm_checksum_t;


typedef struct
{
	unsigned short 			checksum;
	unsigned short			padding;
}nvi_NVRAM8_checksum_t;


typedef struct
{
	char		Sysparm_dep[13976];
}nvi_Sysparm_dep_t;

typedef struct
{
	signed short	TxPAHighToMid ;
	signed short	TxPAMidToHigh ;
	signed short	TxPAMidToLow ;
	signed short	TxPALowToMid ;
	signed char	CalTXPower24dBmOffset;
} RfBandParams_t;


#if 1 // temp remove rf
typedef struct
{
	char		Hdr[20];
	char		depCalibMeas[3384];
	unsigned int 			backup_flag;
	RfBandParams_t		RfBandParams[3];
} nvi_NVRAM8_t;
#else
typedef struct
{
	sRouterHdr				Hdr;
	sDepCalibMeas			depCalibMeas;
#if 0
	sIndCalibMeas   		indCalibMeas;			// 3G RF ind Backup
#else
	unsigned int 			backup_flag;
	RfBandParams_t		RfBandParams[WCDMA_SUPPORTED_BANDS];
#endif
} nvi_NVRAM8_t;
#endif

typedef struct
{
        int     amr_codec;
}nvi_amr_codec_t;//wannatea
/* --------------------------------------------------------------------- */

typedef struct
{
	unsigned	char dummy[DUMMY_SIZE];
}nvi_dummy_t;


// ---------- NV ITEM TYPE DEFINE -end--------------------------------------

typedef enum
{
	NV_SWVERSION=0,
	NV_HWVERSION=1,
	NV_PRDINFO=2, // RF calibration Date.
	NV_BLUETOOTH=3,
	NV_WLAN=4,
	NV_SERIAL_NUMBER=5,
	NV_CSC_VER=6,
	NV_SETBAND=7,
	NV_AUTO_ATTACH_I=8,

	NV_SELLOUT_PRODUCT_CODE_I=9,
	NV_SELLOUT_PRODUCT_INFO_I=10,
	NV_SELLOUT_SMS_MODE_I=11,
	NV_SELLOUT_OPR_MODE_I=12,

	NV_FACTORY_TEST_CHECK_ITEM=13,
	NV_FACTORY_TEST_HISTORY=14,
	NV_FACTORY_TEST_FAIL_CHECK=15,
	NV_FACTORY_DUMMY = 16,

       NV_SENSOR_CAL_DONE= 17,
       NV_SENSOR_CAL_DATA=18,

	NV_SYSPARM_CHECKSUM=19,
	NV_SYSPARM_DEP_I=20,
	NV_NVRAM8_CHECKSUM_I = 21,
	NV_NVRAM8_I = 22,

       NV_AMR_CODEC = 23,//wannatea	NV_TOTAL_CALLTIME = 24,	NV_AR_TDF_PROD_FLAG = 25, // LTN_SW1_SYSTEM kwanguk.kim 2012.05.21 - TDF Call Block Flag

	NV_DUMMY = 26,

	NV_MAX_ITEM,
} nv_items_enum_t;


typedef struct {
	nv_items_enum_t item_num;     /* Item num */
	unsigned	short item_size;
	unsigned	short table_index;
	unsigned	short active_bit;               /* 0 : backup_active, 1 : backup_not_active */
} nv_backup_item_info_type;


typedef struct {
	nv_backup_header_t				nv_backup_header;		// 44
	nvi_SW_version_t					SW_version;				// 16
	nvi_HW_version_t					HW_version;				// 16
	nvi_CalDate_t						CalDate;					// 16
	nvi_bluetooth_ID_t					bluetooth_ID;			// 8
	nvi_wlan_mac_address_type			wlan_MAC_Address;		// 6
	nvi_SerialNum_t					serial_NUM;				// 16
	nvi_CSCVer_t						cscver;					// 40
	nvi_Setband_t						Set_Band;				// 4
	nvi_Auto_Attach_type				Auto_Attach;			// 2

	nvi_product_code_type				nv_sellout_product_code;			// 20
	nvi_product_info_type				nv_sellout_product_info;			// 2
	unsigned int						nv_sellout_sms_mode;		// 4
	unsigned int						nv_sellout_opr_mode;		// 4

	nvi_factory_test_check_type 		factory_test_check_item;	// 220
	nvi_factory_history_type 			factory_test_history;			// 120
	nvi_factory_fail_record_type 		factory_test_fail_check;		// 4
	nvi_factory_dummy_t				factory_dummy;				// 200

	nvi_SensorCalDone_type                         nv_sensor_caldone;       //8
        nvi_SensorCalData_type                        nv_sensor_caldata;       //12

	nvi_sysparm_checksum_t			sysparm_checksum;		// 4
	nvi_Sysparm_dep_t				Backup_sysparm_dep;		// 8480
	nvi_NVRAM8_checksum_t			nvram8_checksum;		// 4
	nvi_NVRAM8_t						nvram8;					// 3404 + 36
	unsigned long                   nv_total_call_time;	unsigned char                   nv_ar_tdf_prod_flag;		// LTN_SW1_SYSTEM kwanguk.kim 2012.05.21 - TDF Call Block Flag
	nvi_dummy_t						nv_dummy;				// 200
} nv_backup_type_t;


/* --------------------------------------------------------------------- */

typedef enum
{
	SEC_IMEI=0,
	SEC_MSL=1,
	SEC_LOCK=2,
	SEC_MAX_ITEM,
}SEC_ITEM_enum_t;

typedef struct {
	nv_items_enum_t item_num;     /* Item num */
	unsigned	short item_size;
	unsigned	short table_index;
	unsigned	short active_bit;		/* 0 : backup_active, 1 : backup_not_active */
}SEC_ITEM_info_type;

typedef struct
{
	unsigned char IMEI_NUMBER[40];
	unsigned char IMEI_NUMBER_slave[40];
}SEC_IMEI_data_t;

typedef struct
{
	char a_SEC_ATC_MSL_Address[10];
	char a_Padding1[2];
	char a_SEC_ATC_MSL_Code[16];
	char v_SEC_ATC_MSL_Code_OTP_Flag;
	char v_Padding2[3];
}SEC_MSL16_Data_t;

typedef struct
{
    unsigned char state_inds;
    unsigned char nw_tag;
    int nw_len;
    unsigned char nw_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char ns_tag;
    int ns_len;
    unsigned char ns_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char sp_tag;
    int sp_len;
    unsigned char sp_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char cp_tag;
    int cp_len;
    unsigned char cp_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char sim_tag;
    unsigned char sim_lock_codes[PERSO_CODE_ITEM_LENGTH];
    unsigned char ms_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char nw_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char ns_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char sp_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char cp_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char sim_cks[PERSO_CKS_ITEM_LENGTH];
    unsigned char cks_attempts;
    unsigned char cks_max_attempts;
}SEC_LOCK_Data_t;

typedef struct{
	SEC_IMEI_data_t		s_SEC_ATC_IMEI_data;
	SEC_MSL16_Data_t	s_SEC_ATC_MSL16_Data;
	SEC_LOCK_Data_t		s_SEC_ATC_Lock_Data;
}SEC_NORMAL_t;

/* --------------------------------------------------------------------- */

#ifndef FEATURE_SAMSUNG_BOOT
int  Get_Sysparm_writen(void);
#endif
int  Remove_All_NV_Data(void);
int  GetMagicCodeWriten(void);
int  GetBackupFlagActive(int index);
int  Flash_Write_NV_Data(void *data_ptr, int index);
int  Flash_Read_NV_Data(void *data_ptr, int index);
int  Flash_Remove_NV_Data(int index);
void Factory_Process_Check(unsigned char TestId, unsigned char Result);
unsigned char Factory_Process_Check_Read(unsigned char TestId);
void InitFactoryTestNV(void);
void FullEreaseHistoryNV(void);

int  Flash_Write_Secure_Data(void *data_ptr, int index);
int  Flash_Read_Secure_Data(void *data_ptr, int index);
int  sec_atc3_040_MSLSECUR_EncodingField(char * pp_Incomming_Field, char * pp_EncodedField, int vp_size);
int  sec_atc3_040_MSLSECUR_DecodingField(char * pp_EncodedField, char * pp_DecodedField, int vp_size);
#endif
