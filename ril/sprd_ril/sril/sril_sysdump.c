#ifndef _SRIL_SYSDUMP_C
#define _SRIL_SYSDUMP_C

#include <telephony/sprd_ril.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <pcap.h>

#include <cutils/properties.h>
#include <sys/time.h>
#include <stdarg.h>

#define LOG_TAG "SRIL_SYSDUMP"

#include <utils/Log.h>
#include "sprd_ril_api.h"
#include "sril.h"
#include "sril_sysdump.h"
#include "sril_svcmode.h"
#include "samsung_nv_flash.h"

// [ipc_debug.c@secril_multi/ipc]
unsigned char bdbg_enable = 1;
// [secril.h@secril_multi]
#define DBG_FLAG_FILE_NAME            "/data/data/com.android.providers.telephony/databases/dbg.conf"
#define LOG_ENABLE_FILE_NAME    "/mnt/.lfs/log_enable.inf"
// [secril_misc.c@secril_multi]
#define RAMDUMP_MODE_FILE_NAME        "/sys/bus/platform/drivers/s3c-keypad/auto_ramdump"

void srilOemLogcatMain(int request, void *data, size_t datalen, RIL_Token t);
void srilOemLogcatRadio(int request, void *data, size_t datalen, RIL_Token t);
void srilOemDumpState(int request, void *data, size_t datalen, RIL_Token t);
void srilOemKernelLog(int request, void *data, size_t datalen, RIL_Token t);
void srilOemLogcatClear(int request, void *data, size_t datalen, RIL_Token t);
void srilOemDbgStateGet(int request, void *data, size_t datalen, RIL_Token t);
void srilOemSysDumpEnable(int request, void *data, size_t datalen, RIL_Token t);
void srilOemRamdumpMode(int request, void *data, size_t datalen, RIL_Token t);
void srilOemRamdumpStateGet(int request, void *data, size_t datalen, RIL_Token t);
void srilOemStartRilLog(int request, void *data, size_t datalen, RIL_Token t);
void srilOemDelRilLog(int request, void *data, size_t datalen, RIL_Token t);
void srilOemStartModemLog(int request, void *data, size_t datalen, RIL_Token t);
void srilOemSetTCPDumpStart(int request, void *data, size_t datalen, RIL_Token t);
void srilOemSetTCPDumpStop(int request, void *data, size_t datalen, RIL_Token t);

void sril_SysDumpHander(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsg msgReq = *((OemReqMsg *)data);

    msgReq.payload = (char*)data + sizeof(OemReqMsgHdr);

    ALOGD("%s: subfId[%x], len[%d]\n", __FUNCTION__, msgReq.hdr.subfId, msgReq.hdr.len);

    switch(msgReq.hdr.subfId) {
        case OEM_LOGCAT_MAIN:
            return srilOemLogcatMain(request, data, datalen, t);

        case OEM_LOGCAT_RADIO:
            return srilOemLogcatRadio(request, data, datalen, t);

        case OEM_DUMPSTATE:
            return srilOemDumpState(request, data, datalen, t);

        case OEM_KERNEL_LOG:
            return srilOemKernelLog(request, data, datalen, t);

        case OEM_LOGCAT_CLEAR:
            return srilOemLogcatClear(request, data, datalen, t);

        case OEM_SYSDUMP_DBG_STATE_GET:
            return srilOemDbgStateGet(request, data, datalen, t);

        case OEM_SYSDUMP_ENABLE_LOG:
            return srilOemSysDumpEnable(request, data, datalen, t);
#if 1
        case OEM_RAMDUMP_MODE:
            return srilOemRamdumpMode(request, data, datalen, t);

        case OEM_RAMDUMP_STATE_GET:
            return srilOemRamdumpStateGet(request, data, datalen, t);
#endif
        case OEM_START_RIL_LOG:
            return srilOemStartRilLog(request, data, datalen, t);

        case OEM_DEL_RIL_LOG:
            return srilOemDelRilLog(request, data, datalen, t);

        case OEM_MODEM_LOG:
               return srilOemStartModemLog(request, data, datalen, t);

        case OEM_TCPDUMP_START:
            return srilOemSetTCPDumpStart(request, data, datalen, t);

        case OEM_TCPDUMP_STOP:
            return srilOemSetTCPDumpStop(request, data, datalen, t);

        default: {
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            break;
        }
    }
}

int MakeLogFile(char* pCmd, char* pFile, char* pTime) {
    char Command[100];
    struct tm *current;
    char current_time[20];
    time_t nTime;
    char temp[3];
    int i = 0, err = 0;

    time(&nTime);
    current = localtime(&nTime);

    sprintf(current_time, "%04d", current->tm_year + 1900);

    if(pTime != NULL) {
        for(i = 0; i < 4; i++) {
            sprintf(temp, "%02d", pTime[i]);
            strcat(current_time, temp);
        }
    } else {
        sprintf(temp, "%02d", current->tm_mon + 1);
        strcat(current_time, temp);

        sprintf(temp, "%02d", current->tm_mday);
        strcat(current_time, temp);

        sprintf(temp, "%02d", current->tm_hour);
        strcat(current_time, temp);

        sprintf(temp, "%02d", current->tm_min);
        strcat(current_time, temp);

        sprintf(temp, "%02d", current->tm_sec);
        strcat(current_time, temp);
    }

    memset(Command, 0, 100);
    sprintf(Command, "%s /data/log/%s_%s.log", pCmd, pFile, current_time);

    ALOGD("%s", Command);

    err = system(Command);

    memset(Command, 0, 100);
    sprintf(Command, "chmod 777 /data/log/%s_%s.log", pFile, current_time);

    ALOGD("%s", Command);

    err = system(Command);

    sync();

    return err;
}

/*int MakeModemLogFile(void) {
    int err = RIL_E_SUCCESS;

    char Command[100];

    ALOGD("%s!()", __FUNCTION__);

    memset(Command, 0, 100);
    sprintf(Command, "echo x > proc/brcm_logcfg");

    ALOGD("%s", Command);

    err = system(Command);

    ALOGD("err0 = %x", err);

    if (err == 0) {
        memset(Command, 0, 100);
        sprintf(Command, "chmod 655 /data/log/log_mtt_*");

        ALOGD("%s", Command);
        err = system(Command);
        ALOGD("err1 = %x", err);
    }

    return err;
}
*/

void srilOemLogcatMain(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];
    struct tm *current;
    char current_time[20];
    time_t nTime;
    char temp[3];
    int i;

    ALOGD("%s()", __FUNCTION__);

    time(&nTime);
    current = localtime(&nTime);

    sprintf(current_time, "%04d", current->tm_year + 1900);

    for(i = 0; i < 4 ; i++) {
        ALOGD("[%d] %02d", i, ((char*)data)[i]);

        sprintf(temp, "%02d", ((char*)data)[i]);
        strcat(current_time, temp);
    }

    memset(Command, 0, 100);
    sprintf(Command, "logcat -b main -d -f /data/log/main_%s.log", current_time);

    ALOGD("%s", Command);
    err = system(Command);

    memset(Command, 0, 100);
    sprintf(Command, "chmod 777 /data/log/main_%s.log", current_time);

    ALOGD("%s", Command);
    err = system(Command);

#if 0
    if (ioctl(ph->trans_fd , HN_DPRAM_DUMP) < 0)
    {
        ALOGD("ioctl(HN_DPRAM_DUMP) error(errno: %d)", errno);
    }
#endif

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemLogcatRadio(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    ALOGD("%s()", __FUNCTION__);
    err = MakeLogFile("logcat -b radio -d -f", "ril", data);

#if 0
    err = system(Command);
    if (ioctl(ph->trans_fd , HN_DPRAM_DUMP) < 0)
    {
        ALOGD("ioctl(HN_DPRAM_DUMP) error(errno: %d)", errno);
    }
#endif

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemDumpState(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;
    ALOGD("%s()", __FUNCTION__);

    //err = MakeModemLogFile();
    ALOGD("err = %x", err);

    err = MakeLogFile("dumpstate >", "dumpState", data);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemKernelLog(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];

    ALOGD("%s()", __FUNCTION__);

    memset(Command, 0, 100);
    sprintf(Command, "chmod 777 /data/panic");

    ALOGD("%s", Command);

    err = system(Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemLogcatClear(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];

    ALOGD("%s()", __FUNCTION__);

    memset(Command, 0, 100);
    sprintf(Command, "logcat -b radio -c");

    ALOGD("%s", Command);

    err = system(Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemDbgStateGet(int request, void *data, size_t datalen, RIL_Token t) {
    FILE *fp = NULL;
    char debug_buf[4] = {0,};
    int err = RIL_E_SUCCESS;
    char *response = NULL;
    int resp_len = 5;
    int logenable_state = 0;

    ALOGD("%s()", __FUNCTION__);

    if ((fp = fopen(LOG_ENABLE_FILE_NAME, "r")) == NULL) {
        ALOGD("checkDbgFlag error. (from DBG_FLAG_FILENAME)");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    fread(debug_buf, sizeof(debug_buf), 4, fp);

    if(strncmp(debug_buf, "LOGD",4) == 0) {
        logenable_state = 0;
    } else {
        logenable_state = 1;
    }

    fclose(fp);
    response = (char *)calloc(resp_len, 1);
    if (response == NULL) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    response[0] = OEM_FUNCTION_ID_SYSDUMP;
    response[1] = OEM_SYSDUMP_DBG_STATE_GET;
    response[2] = 0x00;
    response[3] = resp_len;
    response[4] = logenable_state;

    if (err != -1) {
        ALOGD("DBG_FLAG_FILENAME open OK.");
        RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)response, resp_len);
    } else {
        ALOGD("DBG_FLAG_FILENAME FAIL .");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
    if (response)
        free(response);
}

void srilOemSysDumpEnable(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;
    char Command[100];

    ALOGD("%s()", __FUNCTION__);

    if (((char*)data)[0] == 0)
        sprintf(Command, "echo LOGD > %s", LOG_ENABLE_FILE_NAME); //LOGDisable , command line debuglevel 1
    else
        sprintf(Command, "echo LOGE > %s", LOG_ENABLE_FILE_NAME); //LOGENable , command line  debuglevel 7

    ALOGD("data[0]= %d, Command: %s", ((char*)data)[0] , Command);

    err = system(Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemRamdumpMode(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];
    ALOGD("%s()", __FUNCTION__);

    sprintf(Command, "> %s", RAMDUMP_MODE_FILE_NAME);
    err = system(Command);

    if (((char*)data)[0] == 0)
        sprintf(Command, "echo 0 > %s", RAMDUMP_MODE_FILE_NAME);
    else
        sprintf(Command, "echo 1 > %s", RAMDUMP_MODE_FILE_NAME);

    ALOGD("data[0]= %d, Command: %s", ((char*)data)[0] , Command);
    err = system(Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemRamdumpStateGet(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;
    char *response = NULL;
    int resp_len = 5;
    int bRamdump_state = 0;
    char Command[100];

    ALOGD("%s()", __FUNCTION__);
    response = (char *)calloc(resp_len, 1);

    if (response == NULL) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    FILE * fd;
    int ch;
    int i;

    fd = fopen(RAMDUMP_MODE_FILE_NAME, "r");
    if(fd == NULL) {
        ALOGD("file not found");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        if (response)
            free(response);
        return;
    }

    while(1) {
        ch = fgetc(fd);

        if(ch != EOF) {
            if(ch == '1')
                bRamdump_state =  1;
            else if(ch == '0')
                bRamdump_state =  0;
            else
                break;
        }
        else
            break;
    }

    fclose(fd);

    response[0] = OEM_FUNCTION_ID_SYSDUMP;
    response[1] = OEM_RAMDUMP_STATE_GET;
    response[2] = 0x00;
    response[3] = resp_len;
    response[4] = bRamdump_state;

    ALOGD("bRamdump_state:  %d",  bRamdump_state);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)response, resp_len);

    if (response)
        free(response);
}

void srilOemStartRilLog(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];

    ALOGD("%s!()", __FUNCTION__);

    memset(Command, 0, 100);
    sprintf(Command, "logcat -b radio -c -d");

    ALOGD("%s", Command);

    err = system(Command);

    memset(Command, 0, 100);
    sprintf(Command, "logcat -b radio -f /data/log/lucky_ril.log &");

    ALOGD("%s", Command);

    err = system(Command);

    memset(Command, 0, 100);
    sprintf(Command, "chmod 777 /data/log/lucky_ril.log");

    ALOGD("%s", Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemDelRilLog(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    char Command[100];

    ALOGD("%s()", __FUNCTION__);

    memset(Command, 0, 100);
    sprintf(Command, "rm /data/log/lucky_ril.log");

    ALOGD("%s", Command);

    err = system(Command);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemStartModemLog(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    ALOGD("%s!()", __FUNCTION__);

    //err = MakeModemLogFile();

    ALOGD("err = %x", err);

    if (err != -1) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

#define HISTORY_ITEM_CMD    246 //(60*4)+6    //40
#define CHECK_ITEM_CMD     166 //(40*4)+6    //40
typedef struct {
    char            string[HISTORY_ITEM_CMD];
} OemFactoryTestRsp_History;

typedef struct {
    char            string[CHECK_ITEM_CMD];
} OemFactoryTestRsp_Item;

void sril_CharStringToHexString_Item(nvi_factory_test_check_type_sys *charstring , OemFactoryTestRsp_Item *dest) {
    char temp1[10];
    int i=0;
    int byte1;
    char *text="999999";
    strcpy((char*)dest, text);

    ALOGD("%s\n",__FUNCTION__);
    for(i = 0; i < TEST_ITEM_NUM; i++) {
        sprintf(temp1,"%02X%02X",(char)charstring->factory_item[i].test_id,(char)charstring->factory_item[i].result);
        strcat(dest->string,temp1);
    }
}

void sril_CharStringToHexString_History(nvi_factory_history_type_sys *charstring, OemFactoryTestRsp_Item *dest) {
    char temp1[10];
    int i=0;
    int byte1;
    char *text="888888";
    strcpy((char*)dest, text);

    ALOGD("%s\n", __FUNCTION__);
    for(i = 0; i < HISTORY_ITEM_NUM; i++) {
        sprintf(temp1,"%02X%02X",(char)charstring->factory_item[i].test_id,(char)charstring->factory_item[i].result);
        strcat(dest->string,temp1);
    }
}

void sril_Factory_Test_Read_History(int request, void *data, size_t datalen, RIL_Token t) {
    OemFactoryTestRsp_History     cmd_HistoryData;
    nvi_factory_history_type_sys    HistoryData;
    char cmd_buf[640] = {0};

    Flash_Read_NV_Data(&HistoryData, NV_FACTORY_TEST_HISTORY);
    sril_CharStringToHexString_History(&HistoryData, &cmd_HistoryData);
    /* Send AT command to android */
    sprintf(cmd_buf,"broadcast -a com.android.samsungtest.RilOmissionCommand --es COMMAND %s",&cmd_HistoryData);
    ALOGD("cmd_buf :%s", cmd_buf);

    RIL_onUnsolicitedResponse(RIL_UNSOL_AM, (void*)cmd_buf, strlen(cmd_buf));
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void sril_Factory_Test_Read_Status(int request, void *data, size_t datalen, RIL_Token t) {
    OemFactoryTestRsp_Item cmd_TestData;
    nvi_factory_test_check_type_sys TestData;

    char cmd_buf[640] = {0};

    ALOGD("%s\n",__FUNCTION__);

    Flash_Read_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
    /* Send AT command to android */
    sril_CharStringToHexString_Item(&TestData,&cmd_TestData);

    sprintf(cmd_buf,"broadcast -a com.android.samsungtest.RilOmissionCommand --es COMMAND %s",&cmd_TestData);
    ALOGD("cmd_buf :%s", cmd_buf);

    RIL_onUnsolicitedResponse(RIL_UNSOL_AM, (void*)cmd_buf, strlen(cmd_buf));
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void sril_Factory_Test_Read(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgFactoryTest * pSvcCmd = (OemReqMsgFactoryTest *)data;

    ALOGD("%s: Factory[%x]\n",__FUNCTION__, pSvcCmd->Id_3);

    switch(pSvcCmd->Id_3) {
        ALOGD("%s: Factory[%x]\n",__FUNCTION__, pSvcCmd->Id_3);

        case OEM_FACTORY_EVENT_0:
        case OEM_FACTORY_EVENT_1:
            //break;
        case OEM_FACTORY_EVENT_2:
            sril_Factory_Test_Read_Status(request, data, datalen, t);
            break;
        case OEM_FACTORY_EVENT_3:
            sril_Factory_Test_Read_History(request, data, datalen, t);
            break;
        default:
            ALOGD("%s: Not supported svcMode [%x]\n",__FUNCTION__, pSvcCmd->Id_3);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void sril_Factory_SETTESTNV_Handler(const unsigned char _P1, const unsigned char _P2) {
    nvi_factory_test_check_type_sys TestData;
    nvi_factory_test_check_type_sys TestData11;
    nvi_factory_history_type_sys    HistoryData;
    char str[24];
    unsigned int i;
    unsigned char log_index;
    unsigned char TestId;
    unsigned char Result;
    char temp1[126]={'\0'};

    Flash_Read_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
    Flash_Read_NV_Data(&HistoryData,NV_FACTORY_TEST_HISTORY);
//    ALOGD("%s: Before1 W-R  %s %d %d \n",__FUNCTION__, temp1,_P1, _P2);
    TestId = _P1;
    Result = _P2;
//    ALOGD("%s: Before2 W-R  %s %d %d\n",__FUNCTION__, temp1,TestId, Result);
    if((TestId > 0 && TestId <= TEST_ITEM_NUM) || TestId==0x62 || TestId==0x63) {
            if(!( TestId==0x62 || TestId==0x63)) {
                TestData.factory_item[TestId-1].test_id = TestId ;
                TestData.factory_item[TestId-1].result = Result;
            }
            if(TestId == 0x62){    // PBA Repair
                for(i = 3; i < TEST_ITEM_NUM; i++)
                    TestData.factory_item[i].result = 'N';
            } else if(TestId == 0x63) {    // main repair
                for(i = 6; i < TEST_ITEM_NUM; i++)
                    TestData.factory_item[i].result = 'N';
            }
            Flash_Write_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
//            Flash_Read_NV_Data(&TestData11,NV_FACTORY_TEST_CHECK_ITEM);
//            ALOGD("%s: FFF Affter W-R  %d %d %d\n",__FUNCTION__, TestData11.factory_item[TestId-1].test_id, TestData11.factory_item[TestId-1].result,TestId, Result);
            log_index     = 0;
            while(HistoryData.factory_item[log_index].test_id != 0x00)    { // Find the lastest log index
                log_index++;
                if(log_index==HISTORY_ITEM_NUM)
                    break;
            }

            if(log_index < HISTORY_ITEM_NUM) {
                HistoryData.factory_item[log_index].test_id  = TestId;
                HistoryData.factory_item[log_index].result  = Result;
            } else {
                for(i = 1; i < HISTORY_ITEM_NUM; i++) {
                    HistoryData.factory_item[i-1].test_id  = HistoryData.factory_item[i].test_id;
                    HistoryData.factory_item[i-1].result  = HistoryData.factory_item[i].result;
                }
                HistoryData.factory_item[HISTORY_ITEM_NUM-1].test_id  = TestId;
                HistoryData.factory_item[HISTORY_ITEM_NUM-1].result  = Result;
            }
            Flash_Write_NV_Data(&HistoryData,NV_FACTORY_TEST_HISTORY);
//            Flash_Read_NV_Data(temp1,NV_FACTORY_TEST_HISTORY);
//            ALOGD("%s: Affter W-R  %s %d %d\n",__FUNCTION__, temp1,TestId, Result);
    }
}

void sril_Factory_Test_Write(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgFactoryTest * pSvcCmd = (OemReqMsgFactoryTest *)data;
    char temp[126]={'\0',};
    char temp1[126]={'\0',};
    int i=0;
    ALOGD("%s: Factory[%x]\n",__FUNCTION__, pSvcCmd->Id_3);
    switch(pSvcCmd->Id_3) {
        case OEM_FACTORY_EVENT_0:
        case OEM_FACTORY_EVENT_1:
//        ALOGD("%s: OEM_FACTORY_EVENT_1 %d %d \n",__FUNCTION__,pSvcCmd->itemId,pSvcCmd->result);
            sril_Factory_SETTESTNV_Handler(pSvcCmd->itemId, pSvcCmd->result);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case OEM_FACTORY_EVENT_2:
        case OEM_FACTORY_EVENT_3:

        default:
            ALOGD("%s: Not supported svcMode [%x]\n",__FUNCTION__, pSvcCmd->Id_3);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void sril_FactoryTestHander(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgFactoryTest * pSvcCmd = (OemReqMsgFactoryTest *)data;

    ALOGD("%s: Factory[%x]\n",__FUNCTION__, pSvcCmd->sec_cmd);

    switch(pSvcCmd->sec_cmd)
    {
        ALOGD("%s: Factory[%x]\n",__FUNCTION__, pSvcCmd->sec_cmd);

        case OEM_OMISSION_GET:
            sril_Factory_Test_Read(request, data, datalen, t);
            break;
        case OEM_OMISSION_SET:
            sril_Factory_Test_Write(request, data, datalen, t);
            break;

        case OEM_FACTORY_EVENT:
        case OEM_FACTORY_CFRM:
        case OEM_DFT_EVENT:
        case OEM_DFT_CFRM:
        case OEM_MISCELLANEOUS_EVENT:
        case OEM_MISCELLANEOUS_CFRM:
        default:
            ALOGD("%s: Not supported svcMode [%x]\n",__FUNCTION__, pSvcCmd->sec_cmd);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

pthread_t p_TCPDUMPthread;
pcap_t *pcap_handle;
int TcpdumpErr = -1;
pthread_cond_t p_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;

static void TcpdumpStop(void *arg) {
    ALOGD("<%s>", __FUNCTION__);
    if(pcap_handle != NULL) {
        pcap_breakloop(pcap_handle);
    }
}

static void dump_packet(u_char *user, const struct pcap_pkthdr *h, const u_char *sp) {
    //ALOGD("<%s>", __FUNCTION__);
    pcap_dump(user, h, sp);
    //pcap_dump_flush((pcap_dumper_t *)user);
}

static void * T_Tcpdump(void *data) {
    ALOGD("<%s>", __FUNCTION__);
    int sigreg = 0;
    struct sigaction actions;
    int status;

    char *devname;
    char errmsg[PCAP_ERRBUF_SIZE];
    char interface_name[128] ={0};
    char *prop_name = "ril.tcpdumping";

    pcap_dumper_t *pcap_dump;
    pcap_handler pcap_callback;
    u_char *pcap_userdata;

    struct tm *current;
    char filename[40];
    time_t nTime;

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = TcpdumpStop;
    sigreg = sigaction(SIGUSR1,&actions,NULL);

    strcpy(interface_name, (char *)data);
    if(strlen(interface_name) == 0)
        strcpy(interface_name, "any");

    pcap_handle = pcap_open_live(interface_name, BUFSIZ, 0, -1, errmsg);
    if(pcap_handle == NULL) {
        ALOGE("%s : Error open (%s)", __FUNCTION__, errmsg);
        TcpdumpErr = 1;
        goto Err;
    }

    ALOGD("%s : tcpdump open live OK ", __FUNCTION__);

    time(&nTime);
    current = localtime(&nTime);
    char filepath[128] ={0};
    struct stat sbuf;

    strcpy(filepath, "/data/log");

    sprintf(filename, "%s/tcpdump_%s_%04d%02d%02d%02d%02d%02d.pcap", filepath, interface_name,
    current->tm_year + 1900,
    current->tm_mon+1,
    current->tm_mday,
    current->tm_hour,
    current->tm_min,
    current->tm_sec);
    ALOGD("%s : path : %s", __FUNCTION__,filename);
    pcap_dump = pcap_dump_open(pcap_handle, filename);

    if(pcap_dump == NULL) {
        ALOGE("%s : Error dump (%s)", __FUNCTION__, errmsg);
        TcpdumpErr = 2;
        goto Err;
    }
    ALOGD("%s : tcpdump dump open  OK ", __FUNCTION__);

    pcap_callback =    dump_packet;
    pcap_userdata = (u_char *)pcap_dump;
    property_set(prop_name, "On");
    TcpdumpErr = 0;
    pthread_mutex_lock(&p_mutex);
    pthread_cond_signal(&p_cond);
    pthread_mutex_unlock(&p_mutex);

    status = pcap_loop(pcap_handle, -1, pcap_callback, pcap_userdata);
    ALOGD("%s : tcpdump pcap_loop OK (%d)", __FUNCTION__, status);

    pcap_dump_close(pcap_dump);
    pcap_close(pcap_handle);
    property_set(prop_name, "Off");

Err:
    if(TcpdumpErr!=0) {
        pthread_mutex_lock(&p_mutex);
        pthread_cond_signal(&p_cond);
        pthread_mutex_unlock(&p_mutex);
    }
    return NULL;
}

void srilOemSetTCPDumpStart(int request, void *data, size_t datalen, RIL_Token t) {
    ALOGD("<%s>", __FUNCTION__);

    char interface_name[128]={0};
    int thread_id;

    if(datalen < 128) {
        strncpy(interface_name, data, datalen);
    }

    ALOGD("<%s> Interface : %s", __FUNCTION__, interface_name);

    thread_id = pthread_create(&p_TCPDUMPthread, NULL, T_Tcpdump, (void *)interface_name);
    if(thread_id < 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    pthread_mutex_lock(&p_mutex);
    pthread_cond_wait(&p_cond, &p_mutex);
    pthread_mutex_unlock(&p_mutex);

    pthread_cond_destroy(&p_cond);

    ALOGD("<%s> TCPDumpErr : %d", __FUNCTION__, TcpdumpErr);
    if(TcpdumpErr==0) {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void srilOemSetTCPDumpStop(int request, void *data, size_t datalen, RIL_Token t) {
    ALOGD("<%s>", __FUNCTION__);
    int status = pthread_kill(p_TCPDUMPthread, SIGUSR1);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}


#endif //_SRIL_SYSDUMP_H
