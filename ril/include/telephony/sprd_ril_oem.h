#ifndef SPRD_RIL_OEM_H
#define SPRD_RIL_OEM_H

/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * request data struct
 */
typedef struct _OemRequest
{
    unsigned char funcId;
    unsigned char subFuncId;
    int len;
    char * payload;
}OemRequest;

/*
 * RIL_UNSOL_OEM_HOOK_RAW
 * response data struct
 */
typedef struct {
    unsigned char oemFuncId;
    char* data;
} RIL_OEM_NOTIFY;


/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * function IDs
 */
//blacklist
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_REQ_FUNCTION_ID_CALL_BLACKLIST 0x01
#endif


/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * sub-function IDs
 */
//blacklist
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_REQ_SUBFUNC_ID_MINMATCH 0x01
#define OEM_REQ_SUBFUNC_ID_BLACKLIST 0x02
#endif


/*
 * RIL_REQUEST_OEM_HOOK_STRINGS
 * function IDs
 */


/*
 * RIL_REQUEST_OEM_HOOK_STRINGS
 * sub-function IDs
 */


/*
 * RIL_UNSOL_OEM_HOOK_RAW
 * function IDs
 */
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_UNSOL_FUNCTION_ID_BLACKCALL 0x01
#endif


/*
 * RIL_UNSOL_OEM_HOOK_STRINGS
 * function IDs
 */



#endif

