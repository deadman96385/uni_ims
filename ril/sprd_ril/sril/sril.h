/**
 * COPYRIGHT (C)  SAMSUNG Electronics CO., LTD (Suwon, Korea). 2009
 * All rights are reserved. Reproduction and redistiribution in whole or
 * in part is prohibited without the written consent of the copyright owner.
 */

/**
 * @file	sril.h
 *
 * @author	Sungdeuk Park (seungdeuk.park @samsung.com)
 *
 * @brief	Request from RIL deamon
 */

#ifndef _SRIL_H
#define _SRIL_H

#include <telephony/sprd_ril.h>

void sril_OemHookRaw(int request, void *data, size_t datalen, RIL_Token t);

void sril_SvcModeHander(int request, void *data, size_t datalen, RIL_Token t);
void sril_SysDumpHander(int request, void *data, size_t datalen, RIL_Token t);
void sril_FactoryTestHander(int request, void *data, size_t datalen, RIL_Token t);
void sril_IMEIHandler(int request, void *data, size_t datalen, RIL_Token t);
void sril_TotalCallTimeHandler(int request, void *data, size_t datalen, RIL_Token t);

/*
 * OEM function IDs
 */
#define OEM_FUNCTION_ID_SVC_MODE			0x01
#define OEM_FUNCTION_ID_NETWORK				0x02
#define OEM_FUNCTION_ID_SS					0x03
#define OEM_FUNCTION_ID_PERSONALIZATION		0x04
#define OEM_FUNCTION_ID_POWER				0x05
#define OEM_FUNCTION_ID_IMEI				0x06
#define OEM_FUNCTION_ID_SYSDUMP				0x07
#define OEM_FUNCTION_ID_SOUND				0x08
#define OEM_FUNCTION_ID_GPRS				0x09
#define OEM_FUNCTION_ID_OMADM				0x0A
#define OEM_FUNCTION_ID_CALL				0x0B
#define OEM_FUNCTION_ID_CONFIGURATION		0x0C
#define OEM_FUNCTION_ID_DATA				0x0D
#define OEM_FUNCTION_ID_GPS					0x0E
#define OEM_FUNCTION_ID_PHONE				0x10
#define OEM_FUNCTION_ID_MISC				0x11
#define OEM_FUNCTION_ID_FACTORY				0x12
#define OEM_FUNCTION_ID_RFS					0x13
#define OEM_FUNCTION_ID_SAP					0x14
#define OEM_FUNCTION_ID_AUTH				0x15
#define OEM_FUNCTION_ID_DATAROUTER          0x16

/* KEY_CODE  field */
/* key value which is used in PDA keypad */

/*
 * OEM sub-function IDs
 */
// Service mode
#define	OEM_SVC_ENTER_MODE_MESSAGE			0x01
#define	OEM_SVC_END_MODE_MESSAGE			0x02
#define	OEM_SVC_PROCESS_KEY_MESSAGE			0x03
#define	OEM_SVC_GET_DISPLAY_DATA_MESSAGE	0x04
#define	OEM_SVC_QUERY_DISPLAY_DATA_MESSAGE	0x05
#define OEM_SVC_DEBUG_DUMP_MESSAGE			0x06
#define OEM_SVC_DEBUG_STRING_MESSAGE		0x07

// Network
#define OEM_NWK_GET_AVAILABLE_NETWORK		0x01
#define OEM_NWK_SET_MANUAL_SELECTION		0x02
#define OEM_NWK_CANCEL_AVAILABLE_NETWORK	0x03

// SS
#define OEM_SS_CHANGE_CB_PWD				0x01

// Personalization

// Power
#define OEM_PWK_SET_RESET					0x01
#define OEM_PWK_GET_LAST_PHONE_FATAL_RSN	0x02
#define OEM_PWK_SET_PHONE_STATE				0x03

// IMEI
//#define OEM_IMEI_START					0x01
#define OEM_IMEI_SET_PRECONFIGURAION		0x01
#define OEM_IMEI_SET_WRITE_ITEM				0X02
#define OEM_IMEI_GET_WRITE_ITEM				0x03
#define OEM_IMEI_RESP_FILE_NUM				0x04
#define OEM_IMEI_EVENT_START_IMEI			0x05
#define OEM_IMEI_EVENT_VERIFY_COMPARE		0x06
#define OEM_IMEI_SET_UPDATE_ITEM			0x07
#define OEM_IMEI_CFRM_UPDATE_ITEM			0x08
#define OEM_IMEI_GET_PRECONFIGURAION		0x09
#define OEM_IMEI_SIM_OUT				0x0A

//SYSDUMP
#define	OEM_LOGCAT_MAIN						0x01
#define OEM_LOGCAT_RADIO					0x02
#define OEM_DUMPSTATE						0x03
#define OEM_KERNEL_LOG						0x04
#define OEM_LOGCAT_CLEAR					0x05
#define OEM_SYSDUMP_DBG_STATE_GET			0x06
#define OEM_SYSDUMP_ENABLE_LOG				0x07
#define OEM_IPC_DUMP_LOG					0x08
#define OEM_IPC_DUMP_BIN					0x09
#define OEM_RAMDUMP_MODE					0x0A
#define OEM_RAMDUMP_STATE_GET				0x0B
#define OEM_START_RIL_LOG					0x0C
#define OEM_DEL_RIL_LOG						0x0D

#define OEM_TCPDUMP_START				0x15

#define OEM_TCPDUMP_STOP					0x16



//SOUND
#define OEM_SOUND_SET_MINUTE_ALERT			0x01
#define OEM_SOUND_GET_MINUTE_ALERT			0x02
#define OEM_SOUND_SET_VOLUME_CTRL			0x03
#define OEM_SOUND_GET_VOLUME_CTRL			0x04
#define OEM_SOUND_SET_AUDIO_PATH_CTRL		0x05
#define OEM_SOUND_GET_AUDIO_PATH_CTRL		0x06
#define OEM_SOUND_SET_VIDEO_CALL_CTRL		0x07
#define OEM_SOUND_SET_LOOPBACK_CTRL         0x08
#define OEM_SOUND_SET_VOICE_RECORDING_CTRL  0x09
#define OEM_SOUND_SET_CLOCK_CTRL            0x0A

// Hidden Menu for cdma
#define OEM_HIDDEN_AKEY_VERIFY				0x01
#define OEM_HIDDEN_GET_MSL					0x02
#define OEM_HIDDEN_GET_ESNMEID				0x03
#define OEM_HIDDEN_SET_TEST_CALL			0x04
#define OEM_HIDDEN_END_TEST_CALL			0x09
#define OEM_HIDDEN_GET_RC_DATA				0x05
#define OEM_HIDDEN_SET_RC_DATA				0x06
#define OEM_HIDDEN_GET_HYBRID_MODE			0x07
#define OEM_HIDDEN_SET_HYBRID_MODE			0x08
#define OEM_HIDDEN_GET_LIFETIMECALL			0x0A
#define OEM_HIDDEN_GET_LIFEBYTE				0x0B
#define OEM_HIDDEN_GET_RECONDITIONED		0x0C
#define OEM_HIDDEN_GET_ACTIVATIONDATE 		0x0D
#define OEM_HIDDEN_GET_MOBILEIPNAI	 		0x0E
#define OEM_HIDDEN_SET_MOBILEIPNAI	 		0x0F
#define OEM_HIDDEN_GET_MODEM_NAI			0x10
#define OEM_HIDDEN_SET_MODEM_NAI			0x11
#define OEM_HIDDEN_GET_KOREA_MODE	 		0x12
#define OEM_HIDDEN_SET_KOREA_MODE 			0x13

//OMADM
#define OEM_OMADM_START_CIDC				0x01
#define OEM_OMADM_START_CIFUMO				0x02
#define OEM_OMADM_START_CIPRL				0x03
#define OEM_OMADM_START_HFA					0x04
#define OEM_OMADM_START_REG_HFA				0x05
#define OEM_OMADM_SETUP_SESSION				0x06
#define OEM_OMADM_SERVER_START_SESSION		0x07
#define OEM_OMADM_CLIENT_START_SESSION		0x08
#define OEM_OMADM_SEND_DATA					0x09
#define OEM_OMADM_EANBLE_HFA				0x19

// GPRS
#define OEM_GPRS_SET_DORMANCY				0x01
#define OEM_GPRS_EXEC_DUN_PIN_CTRL			0x02
#define OEM_GPRS_DISCONNECT_DUN				0x03 // ActiveSync for internet sharing

//CALL
#define OEM_CALL_SEND_DTMF_STRING			0x01
#define OEM_CALL_E911CB_MODE				0x02
#define OEM_CALL_SET_DTMFLENGTH				0x07
#define OEM_CALL_GET_DTMFLENGTH				0x08
#define OEM_CALL_GET_LIFETIMECALL			0x0A
#define OEM_CALL_SET_LIFETIMECALL			0x0B // jkjo@latinMW:2010.06.08 - implement_TotallCallTime

//CONFIGURATION
#define OEM_CFG_EXEC_DEFAULT					0x01
#define OEM_CFG_CFRM_WIFI_TEST					0x02
#define OEM_CFG_CFRM_BT_FACTORY_TEST			0x03
#define OEM_CFG_SET_AUDIO_LOOPBACK_TEST			0x04
#define OEM_CFG_GET_DGS_UNIQUENUMBER			0x05
#define OEM_CFG_CFRM_ACCELERATION_SENSOR_TEST	0x06
#define OEM_CFG_SIO_MODE_SETTING				0X08
#define OEM_CFG_CDMA_TEST_SYS					0X10
#define OEM_CFG_SET_1X_EVDO_DIVERSITY_CONFIG	0x22
#define OEM_CFG_SET_USER_LOCK_CODE_STATUS		0x24
#define OEM_CFG_SET_DEVICE_MAC_ADDRESS			0x30

// DATA
#define OEM_DATA_TE2_STATUS					0x01
#define OEM_DATA_RESTORE_NAI				0x0d

//PHONE
#define OEM_PHONE_RAM_DUMP					0x01
#define OEM_PHONE_RESET						0x02
#define OEM_PHONE_MAKE_CRASH				0x03
#define OEM_PHONE_DUMP_MODE					0x04

#ifdef CONFIG_RTC_CP_BACKUP_FEATURE
// MISC
#define OEM_MISC_GET_TIME					0x01
#define OEM_MISC_SET_TIME					0x02
#endif
#define OEM_MISC_GET_SERIALNUMBER			0x11
#define OEM_MISC_GET_MANUFACTUREDATE        0x12
#define OEM_MISC_GET_BARCODE                0x13

//FACTORY
#if 1//FEATURE_SAMSUNG_IPC41_NEW_FACTORY
//#define OEM_FACTORY_DEVICE_TEST				0x01
//#define OEM_FACTORY_OMISSION_AVOIDANCE_TEST	0x02
//#define OEM_FACTORY_DFT_TEST				0x03
//#define OEM_FACTORY_MISCELLANEOUS_TEST		0x04

#define OEM_FACTORY_EVENT					0x01
#define OEM_FACTORY_CFRM					0x02
#define OEM_OMISSION_GET					0x03
#define OEM_OMISSION_SET					0x04
#define OEM_DFT_EVENT						0x05
#define OEM_DFT_CFRM						0x06
#define OEM_MISCELLANEOUS_EVENT				0x07
#define OEM_MISCELLANEOUS_CFRM				0x08

#else
#define OEM_FACTORY_CFRM_SD_CHECK			0x01
#define OEM_FACTORY_CFRM_SHIPMENT_TEST		0x02
#define OEM_FACTORY_CFRM_G_SENSOR_TEST		0x03 //Acceleration , 3AXIS
#define OEM_FACTORY_SET_PROCESS_TEST		0x04
#define OEM_FACTORY_GET_PROCESS_TEST		0x05
#define OEM_FACTORY_CFRM_WIFI_TEST			0x06
#define OEM_FACTORY_CFRM_CAMERA_TEST		0x07
#endif

// AUTH
#define OEM_AUTH_GSM_CONTEXT				0x06
#define OEM_AUTH_3G_CONTEXT					0x07

//RFS
#define OEM_RFS_NV_MOBILE_TRACKER 	0x01
typedef struct _OemReqMsgHdr
{
	unsigned char funcId;
	unsigned char subfId;
	unsigned short len;
}OemReqMsgHdr;

typedef struct _OemReqMsg
{
	OemReqMsgHdr hdr;
	char * payload;
}OemReqMsg;

#if 1
typedef struct {
	char pdu[180];
}RIL_SMS_from_SIM;

typedef struct {
	int sim_status[2];
}RIL_Get_Stored_Msg_Count;

/*
typedef struct {
	int 	size;		  // @field structure size in bytes
	int     dataLen;	  // @field data size in bytes
	int 	params;    // @field indicates valid parameters
	int 	status; 	// @field additional status for message
	char    *data;	 // @field message itself
}RIL_SS_Release_Comp_Msg;
*/

typedef enum{
	SRIL_MN_SMS_NO_ERROR = 0x0000,/* MN_SMS_NO_ERROR in msnu.h*/
	SRIL_MN_SMS_GENERIC_ERROR = 0x0001,/* all the error except above three in msnu.h*/
	SRIL_MN_SMS_MEMORY_CAPACITY_EXCEEDED = 0x8016, /* MN_SMS_MEMORY_CAPACITY_EXCEEDED =  3 T_SMS_PP_CAUSE in msnu.h*/
	SRIL_MN_SMS_NETWORK_OUT_OF_ORDER = 0x802A,/* MN_SMS_NETWORK_OUT_OF_ORDER in msnu.h , when retry has failed*/
}RIL_SMS_Result;


typedef struct {
    char call_type;
    char control_result;
    char alpha_id_present;
    char alpha_id_len;
    char alpha_id[64];
    char call_id;
    char old_call_type;
    char modadd_ton;
    char modadd_npi;
    char modadd_len;
    char modadd[200];
}RIL_Stk_CallCtrl_Result;


typedef enum
{
	SRIL_LOCK_READY = 0x00,
	SRIL_LOCK_PH_SIM = 0x01, // SIM LOCK (Personalisation)
	SRIL_LOCK_PH_FSIM = 0x02, //SIM LOCK (Personalisation)
	SRIL_LOCK_SIM = 0x03, // PIN LOCK
	SRIL_LOCK_FD = 0x04, // Fixed Dialing Memeory feature
	SRIL_LOCK_NETWORK_PERS = 0x05,
	SRIL_LOCK_NETWORK_SUBSET_PERS = 0x06,
	SRIL_LOCK_SP_PERS = 0x07,
	SRIL_LOCK_CORP_PERS = 0x08,
	SRIL_LOCK_PIN2 = 0x09,
	SRIL_LOCK_PUK2 = 0x0A,
	SRIL_LOCK_ACL = 0x0B,
	SRIL_LOCK_NO_SIM = 0x80
}SRIL_Lock_Type_t;

typedef enum
{
	SRIL_PIN_NOT_NEED = 0x00,
	SRIL_PIN = 0x01,
	SRIL_PUK = 0x02,
	SRIL_PIN2 = 0x03,
	SRIL_PUK2 = 0x04,
	SRIL_PERM_BLOCKED = 0x05,
	SRIL_PIN2_DISABLE = 0x06
}SRIL_LOCK_STATUS;

typedef struct {
/*
   response[0] - Total slot count.
                 The number of the total phonebook memory.
                 according to Phone book storage.
   response[1] - Used slot count.
                 The number of the used phonebook memory.
   response[2] - First index.
                 Minimum index of phonebook entries.
   response[3] - Maximum number of phonebook entry text field.
   resopnse[4] - Maximum number of phonebook entry number field.
*/

  int total;
  int used;
  int first_id;
  int max_text;
  int max_num;

}RIL_Phonebk_Storage_Info;

#if 0
#define MAX_3GPP_TYPE 13
#define MAX_DATA_LEN 4

typedef struct {


	int response[MAX_3GPP_TYPE][MAX_DATA_LEN];

/*
	"response" is int[MAX_3GPP_TYPE(13) MAX_DATA_LEN(4)] type.

   response[0] PB_FIELD_TYPE
   response[1] MAX_INDEX     Maximum index of phonebook field entries
   response[2] MAX_ENTRY     Maximum number of phonebook field entry field
   response[3] USED RECORD   Record in use of phoebook field (except 3GPP_PBC )



   PB_FIELD_TYPE has following values:

   FIELD_3GPP_NAME   = 0x01 : Name of current storageg
   FIELD_3GPP_NUMBER = 0x02 : Number of current storageg
   FIELD_3GPP_ANR    = 0x03 : First additional numberg
   FIELD_3GPP_EMAIL  = 0x04 : First email addressg
   FIELD_3GPP_SNE    = 0x05 : Second name entryg
   FIELD_3GPP_GRP    = 0x06 : Grouping fileg
   FIELD_3GPP_PBC    = 0x07 : Phonebook controlg
   FIELD_3GPP_ANRA   = 0x08 : Second additional numberg
   FIELD_3GPP_ANRB   = 0x09 : Third additional numberg
   FIELD_3GPP_ANRC   = 0x0a : Fourth additional numberg
   FIELD_3GPP_EMAILA = 0x0b : Second email addressg
   FIELD_3GPP_EMAILB = 0x0c : Third email addressg
   FIELD_3GPP_EMAILC = 0x0d : Fourth email addressg
 */
}RIL_Usim_PB_Capa;
#endif
/* RIL_REQUEST_GET_PHONEBOOK_ENTRY response*/
typedef struct {

   int lengthAlphas[NUM_OF_ALPHA];	/* length of each alphaTags[i] */

   int dataTypeAlphas[NUM_OF_ALPHA];	/*

     character set of each of <alphaTags[i]>.

     Now, following values are available:

       GSM 7BIT = 0x02: <alphaTag> is converted a string into
                        an 8-bit unpacked GSM alphabet byte array.

       UCS2     = 0x03: <alphaTag> is converted a string into
                        an 16-bit universal multiple-octet coded
                        character set (ISO/IEC10646)

                        it will be read by UTF-16.

       others are not supported.

     dataTypeAlphas[NAME  = 0];	character set of name that corresponds to
                                <text> field of +CPBR.

     dataTypeAlphas[SNE   = 1]: character set of secondary name entry that corresponds to
                                <secondtext> field of +CPBR.

                                it is not used parameter now, because SNE field is not used.

     dataTypeAlphas[EMAIL = 2]: character set of first email address that corresopnds to
                                <email> field of +CPBR.
                                */

   char* alphaTags[NUM_OF_ALPHA ];	/*hex string to specify following fields

     alphaTags[NAME  = 0]: name that corresponds to <text> field of +CPBR.

     alphaTags[SNE   = 1]: second name entry that corresponnds to
                           <secondtext> field of +CPBR.

                           it is not used parameter now.

     alphaTags[EMAIL = 2]: first email address that corresponds to
                           <email> field of +CPBR.	*/


   int lengthNumbers[NUM_OF_NUMBER ];	/* length of each numbers[i].	*/

   int dataTypeNumbers[NUM_OF_NUMBER];	/* data type or format of each numbers[i].

                                           it is not used parameter now.	*/

   char* numbers[NUM_OF_NUMBER ];	/*

     numbers[INDEX_NUMBER = 0] number that corresponds to <number> field of +CPBR.

     numbers[INDEX_ANR    = 1] second additional number that corresponds to
                                <adnumber> field of +CPBR.

                                it is not used parameter now.

     numbers[INDEX_ANRA   = 2]: third additional number.

                                it is not used parameter now.

     numbers[INDEX_ANRB   = 3]: fourth additional number.

                                it is not used parameter now. */

   int recordIndex ;	/* location which the numbers are stored
                                          in the storage.
                                          it can be corresponded to <index> field of +CPBR.*/

   int nextIndex ;	/* location of the next storage from current number.
                                          it can be corresponded to next <index> field
                                          of +CPBR.		*/


}RIL_Phonebk_Entry_rp;

/*typedef enum{
	MO_VOICE=0x00,
    MO_SMS=0x01,
    MO_SS=0x02,
    MO_USSD=0x03,
    PDP_CTXT=0x04,
}RIL_CallType;


typedef enum {
    CALL_CONTROL_NO_CONTROL=0,
    CALL_CONTROL_ALLOWED_NO_MOD=1,
    CALL_CONTROL_NOT_ALLOWED=2,
    CALL_CONTROL_ALLOWED_WITH_MOD=3,
}RIL_CallControlResultCode;
*/

typedef enum
{
    MN_SMS_UNASSIGNED_NUMBER = 1,
    MN_SMS_OP_DETERMINED_BARRING = 8,
    MN_SMS_CALL_BARRED = 10,
    MN_SMS_CP_NETWORK_FAILURE = 17,
    MN_SMS_TRANSFER_REJECTED = 21,
    MN_SMS_MEMORY_CAPACITY_EXCEEDED = 22,
    MN_SMS_DEST_OUT_OF_SERVICE = 27,
    MN_SMS_UNIDENTIFIED_SUBSCRIBER = 28,
    MN_SMS_FACILITY_REJECTED = 29,
    MN_SMS_UNKNOWN_SUBSCRIBER = 30,
    MN_SMS_NETWORK_OUT_OF_ORDER = 38,
    MN_SMS_TEMPORARY_FAILURE = 41,
    MN_SMS_CONGESTION = 42,
    MN_SMS_RESOURCES_UNAVAILABLE = 47,
    MN_SMS_FACILITY_NOT_SUBSCRIBED = 50,
    MN_SMS_REQ_FACILTY_NON_IMPL = 69,
    MN_SMS_INVALID_REFERENCE_VALUE = 81,
    MN_SMS_SEMANT_INCORRECT_MSG = 95,
    MN_SMS_INVALID_MANDATORY_INFO = 96,
    MN_SMS_MSG_TYPE_NON_EXISTENT = 97,
    MN_SMS_MSG_NOT_COMPATIBLE = 98,
    MN_SMS_IE_NON_EXISTENT = 99,
    MN_SMS_PROTOCOLL_ERROR = 111,
    MN_SMS_INTERWORKING = 127,

    TP_FCS_NO_ERROR = 0x00,
    TP_FCS_TELEMATIC_NOT_SUPPORTED = 0x80,
    TP_FCS_SM_TYPE_0_NOT_SUPPORTED = 0x81,
    TP_FCS_CANNOT_REPLACE_SM = 0x82,
    TP_FCS_UNSPEC_TP_PID_ERROR = 0x8F,
    TP_FCS_DCS_NOT_SUPPORTED = 0x90,
    TP_FCS_MSG_CLASS_NOT_SUPPORTED = 0x91,
    TP_FCS_UNSPEC_TP_DCS_ERROR = 0x9F,
    TP_FCS_CMD_NOT_ACTIONED = 0xA0,
    TP_FCS_CMD_NOT_SUPPORTED = 0xA1,
    TP_FCS_UNSPEC_TP_CMD_ERROR = 0xAF,
    TP_FCS_TPDU_NOT_SUPPORTED = 0xB0,
    TP_FCS_SC_BUSY = 0xC0,
    TP_FCS_NO_SC_SUBSCRIPTION = 0xC1,
    TP_FCS_SC_SYSTEM_FAILURE = 0xC2,
    TP_FCS_INVALID_SME_ADDRESS = 0xC3,
    TP_FCS_DEST_SME_BARRED = 0xC4,
    TP_FCS_SM_REJ_DUPL_SM = 0xC5,
    TP_FCS_SIM_SMS_STORE_FULL = 0xD0,
    TP_FCS_NO_SMS_ON_SIM = 0xD1,
    TP_FCS_ERROR_IN_MS = 0xD2,
    TP_FCS_MEM_CAP_EXCEEDED = 0xD3,
    TP_FCS_TOOLKIT_BUSY = 0xD4,
    TP_FCS_DATADOWNLOAD_ERROR = 0xD5,
    TP_FCS_APPL_ERR_START = 0xE0,
    TP_FCS_APPL_ERR_STOP = 0xFE,
    TP_FCS_UNSPECIFIED = 0xFF,

    MN_SMS_RP_ACK = 512,
    MN_SMS_TIMER_EXPIRED,
    MN_SMS_FORW_AVAIL_FAILED,
    MN_SMS_FORW_AVAIL_ABORTED,

    MN_TP_INVALID_MTI,
    MN_TP_SRF_NOT_IN_PHASE1,
    MN_TP_RDF_NOT_IN_PHASE1,
    MN_TP_RPF_NOT_IN_PHASE1,
    MN_TP_UDHF_NOT_IN_PHASE1,
    MN_TP_MISSING_VALIDITY_PERIOD,
    MN_TP_INVALID_TIME_STAMP,
    MN_TP_MISSING_DEST_ADDRESS,
    MN_TP_INVALID_DEST_ADDRESS,
    MN_TP_MISSING_SC_ADDRESS,
    MN_TP_INVALID_SC_ADDRESS,
    MN_TP_INVALID_ALPHABET,
    MN_TP_INVALID_USER_DATA_LENGTH,
    MN_TP_MISSING_USER_DATA,
    MN_TP_USER_DATA_TOO_LARGE,
    MN_TP_CMD_REQ_NOT_IN_PHASE1,
    MN_TP_INVALID_DEST_ADDR_SPEC_CMDS,
    MN_TP_INVALID_CMD_DATA_LENGTH,
    MN_TP_MISSING_CMD_DATA,
    MN_TP_INVALID_CMD_DATA_TYPE,
    MN_TP_CREATION_OF_MNR_FAILED,
    MN_TP_CREATION_OF_CMM_FAILED,
    MN_TP_MT_CONNECTION_LOST,
    MN_TP_PENDING_MO_SMS,
    MN_TP_CM_REJ_MSG_NOT_COMPAT,
    MN_SMS_REJ_BY_SMS_CONTROL,


    MN_SMS_NO_ERROR,
    MN_SMS_NO_ERROR_NO_ICON_DISPLAY,

    MN_SMS_FDN_FAILED,
    MN_SMS_BDN_FAILED,

    SMS_PP_UNSPECIFIED
} T_SMS_PP_CAUSE;
#endif

#endif	//_SRIL_H