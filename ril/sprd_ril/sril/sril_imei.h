#ifndef _SRIL_IMEI_H
#define _SRIL_IMEI_H

/* result of preconfig */
#define PRECONF_OK              "OK"
#define PRECONF_SET_FAIL	    "PRECONFIG FAIL"
#define PRECONF_RC1_FAIL 	    "RC1 FAIL"
#define PRECONF_PARAM_ERR 	    "NA"
#define PRECONF_GET_FAIL 	    "CSC FAIL"

#define MAGICNO_FAIL 		    "MAGICNO FAIL"
#define NWLOCK_FAIL 		    "NWLOCK FAIL"
#define SUBSETLOCK_FAIL 	    "SUBSETLOCK FAIL"
#define SPLOCK_FAIL 		    "SPLOCK FAIL"
#define CPLOCK_FAIL 		    "CPLOCK FAIL"
#define SIMLOCK_FAIL 		    "SIMLOCK FAIL"
#define MULTILOCK_FAIL 		    "MULTILOCK FAIL"
#define LEVELOFLOCK_FAIL 	    "LEVELOFLOCK FAIL"
#define APPCOUNT_FAIL 		    "APPCOUNT FAIL"
#define RINGTONE_FAIL 		    "RINGTONE FAIL"

/* fsbuild_check value */
#define FSBUILD_BUILDING 	    "BUILDING"
#define FSBUILD_COMPLETE 	    "COMPLETE"
#define FSBUILD_NONE 		    "NONE"

#define SALES_CODE_PATH_TMP     "/proc/LinuStoreIII/efs_info"
#define SALES_CODE_PATH 	    "/efs/imei/mps_code.dat"
#define FSBUILD_CHECK_PATH 	    "/proc/LinuStoreIII/fsbuild_check"
#define CSC_DIR_PATH		    "/system/csc/"

void requestSetPreConfiguration(char *ATCresult, char *data);
void requestGetPreConfiguration(char *ATCresult);

#endif	//	_SRIL_IMEI_H
