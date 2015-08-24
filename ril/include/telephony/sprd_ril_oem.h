#if defined (RIL_SUPPORT_CALL_BLACKLIST)


#ifndef SPRD_RIL_OEM_H
#define SPRD_RIL_OEM_H

/* OEM_HOOK_STRINGS request*/
typedef struct _OemRequest
{
    char * funcId;
    char * subFuncId;
    char * len;
    char * payload;
}OemRequest;

/* OEM_HOOK_RAW response*/
typedef struct {
    int oemFuncId;
    int oemSubFuncId;
    char* data;
} RIL_OEM_NOTIFY;

/* OEM function IDs */
#define OEM_FUNCTION_ID_CALL_BLACKLIST 1

/* OEM sub function IDs */
#define OEM_SUBFUNC_ID_MINMATCH 0
#define OEM_SUBFUNC_ID_BLACKLIST 1 //represent OEM_HOOK_STRINGS with blacklist
#define OEM_SUBFUNC_ID_BLACKCALL 2 ////represent OEM_HOOK_RAW with blacknumber

#endif

#endif
