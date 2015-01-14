/******************************************************************************
**
 *  Generic SEC Library API
 *
 *  Copyright (C) 2014 Spreadtrum Communications Inc.
**
******************************************************************************/
#define LOG_TAG "tks_lib"
/**--------------------------------------------------------------------------*
 **                         INCLUED FILES                                    *
 **--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include "tks_lib.h"
#ifndef KERNEL_TEST
#include <cutils/properties.h>
#include <utils/Log.h>
#include "AtChannel.h"
#endif
#include "rpmb_api.h"
/**--------------------------------------------------------------------------*
 **                         MACRO DEFINITION                                 *
 **--------------------------------------------------------------------------*/
#define LOCAL           static
#define SEC_DRIVER_DEV  "/dev/sprdtks"

#define SEC_IOC_GEN_KEYPAIR        _IOWR('S', 1, request_gen_keypair)
#define SEC_IOC_DEC_SIMPLE         _IOWR('S', 2, request_dec_simple)
#define SEC_IOC_DEC_PACK_USERPASS  _IOWR('S', 3, request_dec_pack_userpass)
#define SEC_IOC_CRYPTO             _IOWR('S', 4, request_crypto)
#define SEC_IOC_GEN_DHKEY          _IOWR('S', 5, request_gen_dhkey)
#define SEC_IOC_HMAC               _IOWR('S', 6, request_hmac)

#ifdef KERNEL_TEST
#define ALOGD    printf
#define ALOGE    printf
#endif

/**--------------------------------------------------------------------------*
 **                         TYPE AND CONSTANT                                *
 **--------------------------------------------------------------------------*/

/**--------------------------------------------------------------------------*
 **                         STATIC DEFINITION                                *
 **--------------------------------------------------------------------------*/
// frame length of tks_keytype
static int s_framelen[6] = {16, 16, 128, 128, 256, 256};

#ifdef KERNEL_TEST
int s_is_need_break = 0;
#endif

/**--------------------------------------------------------------------------*
 **                         EXTERNAL DECLARE                                 *
 **--------------------------------------------------------------------------*/

/**--------------------------------------------------------------------------*
 **                         LOCAL FUNCTION DECLARE                           *
 **--------------------------------------------------------------------------*/
LOCAL int tks_del_key_by_name(char *name, tks_key user_pass, int sec_fd);
int tks_dec_userpass(char *encryptoname, tks_key in_pass, int  fd,
                     request_dec_pack_userpass *out_pass, tks_key *encrytkey);
LOCAL int compare_key(tks_key  key1, tks_key  key2);
LOCAL void init_dec_simple(request_dec_simple  *p);
LOCAL void copy_key2packdata(tks_key  src_key, packdata  *p_data);
LOCAL void release_dec_simple(request_dec_simple  *p);
LOCAL void init_dhkey(request_gen_dhkey  *p);
LOCAL void copy_bigint(big_int  src_int, big_int  *p_dst);
LOCAL void copy_dhparam(dh_param  src_param, dh_param  *p_dst);
LOCAL void copy_bigint2key(big_int  src_data, tks_key  *p_dst);
LOCAL void release_dhkey(request_gen_dhkey  *p);
LOCAL void get_tm(struct tm  *p_result);
LOCAL int tks_save_devcert(char  *name, tks_key  *key);
LOCAL int tks_write_key(char  *name, rpmb_keycontainer  key);
LOCAL int tks_read_key(char  *name, tks_key  *key);
LOCAL void update_keyinfo_store(char  *name);
LOCAL void update_keyinfo_access(char  *name);
LOCAL void update_keyinfo_fail(char  *name);
LOCAL void update_sysinfo(char  *name, int  apino, int  retcode);
LOCAL int tks_check_key_valid(char  *name, uint8_t  operation);
LOCAL void tks_err_notify(int  fd);

void hexdump(const char *title, const unsigned char *s, int l)
{
    int n=0;

    ALOGD("%s",title);
    for( ; n < l ; ++n)
    {
        if((n%16) == 0)
            ALOGD("\n%04x",n);
        ALOGD(" %02x",s[n]);
    }
    ALOGD("\n");
}

void print_key(const char  *name, tks_key  key)
{
    ALOGD("key name: %s  type = %d, len = %d\n", name, key.key_type, key.keylen);
    hexdump("key data:", key.keydata, key.keylen);
}

void print_userpass(const char  *name, tks_key  up)
{
    ALOGD("up name: %s  type = %d, len = %d\n", name, up.key_type, up.keylen);
    hexdump("up data:", up.keydata, up.keylen);
}

void print_packdata(const char  *title, packdata  data)
{
    ALOGD("packdata len = %d\n", data.len);
    hexdump(title, data.data, data.len);
    ALOGD("\n");
}

void print_time(struct tm  time)
{
    ALOGD(" %02d-%02d-%02d %02d:%02d:%02d \n",
          time.tm_year % 100,
          time.tm_mon + 1,
          time.tm_mday,
          time.tm_hour,
          time.tm_min,
          time.tm_sec);
}

void print_crypto(const char  *title, request_crypto  crypto)
{
    ALOGD("%s  op = %d\n", title, crypto.op);
    print_key("crypto.encryptkey: ", crypto.encryptkey);
    print_key("crypto.userpass: ",   crypto.userpass);
    print_packdata("crypto.data: ", crypto.data);
}

/**--------------------------------------------------------------------------*
 **                         FUNCTION DEFINITION                              *
 **--------------------------------------------------------------------------*/
/*
功    能：在tks中产生一个密钥对，存储私钥到密钥数据库，输出公钥。
参数定义：[in]  name：要存储在tks中的密钥名称
          [in]  encryptoname：在tks中已经存在的一个密钥名称，如果其为null，则
                              user_pass为明文，否则user_pass的keydata成员被
                              encryptoname加密。
          [in]  user_pass;
          [in]  option：一些选项和标志位，例如是否允许重设user_pass
          [out] rsapub：输出rsa公钥
返 回 值：SEC_ERR_NUM_E
*/
int tks_gen_keypair(char *name, char *encryptoname, tks_key user_pass, opt option,
                    tks_key *rsapub)
{
    int                        ret = 0;
    int                        sec_fd = 0;
    int                        retcode = SEC_ERROR_SUCCESS;
    tks_key                    encrytkey = {0};
    request_dec_pack_userpass  userpass_data;
    request_gen_keypair        keypair;
    rpmb_keycontainer          keyprv;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    if (rsapub == NULL) {
        ALOGD("%s: rsapub is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    ret = tks_read_key(name, &encrytkey);
    if (ret == 0) {
        ALOGD("%s: name already exists!\n", __FUNCTION__);
        return SEC_ERROR_KEY_EXIST;
    }
    // 初始化
    memset(&userpass_data, 0, sizeof(request_dec_pack_userpass));
    memset(&keypair, 0, sizeof(request_gen_keypair));
    memset(&keyprv, 0, sizeof(rpmb_keycontainer));
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 解密userpass
    retcode = tks_dec_userpass(encryptoname, user_pass, sec_fd,
                               &userpass_data, &encrytkey);
    if (retcode != SEC_ERROR_SUCCESS) {
        ALOGD("%s: tks_dec_userpass failed!\n", __FUNCTION__);
        goto ERR_EXIT;
    }
    // 生成密钥对
    if (ioctl(sec_fd, SEC_IOC_GEN_KEYPAIR, &keypair) < 0) {
        ALOGD("%s: ioctl SEC_IOC_GEN_KEYPAIR failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_DEC_USERPASS_FAIL;
        goto ERR_EXIT;
    }
    print_key("rsapub: ", keypair.out_rsapub);
    print_key("rsaprv: ", keypair.rsaprv);
    // 存储私钥
    keyprv.option = option;
    get_tm(&keyprv.keyinfo.timestored);
    keyprv.key = keypair.rsaprv;
    ret = tks_write_key(name, keyprv);
    if (ret != 0) {
        ALOGD("%s: write key failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 存储user_pass
    ret = rpmb_write_userpass(name, userpass_data.userpass);
    if (ret != 0) {
        ALOGD("%s: write user_pass failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 返回公钥给用户
    *rsapub = keypair.out_rsapub;
    // 结束处理
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(name, 1, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能: 在tks中增加一个密钥。
参数定义：[in] name：存储在tks中的密钥名称
          [in] encryptoname: 在tks中已经存在的一个密钥名称, 如果其为null，
                             则user_pass, certi为明文，否则，tks需要通过其
                             解密user_pass,certi才可以使用。
          [in] user_pass: 和密钥一起存储，为访问此密钥的验证凭据。
          [in] certi ：要存储的密钥，如果certi是已存密钥名称（密钥类型是名称），
                       则需要使用其对应的userpass对此参数做进一步加密，以示授权。
          [in] opt option：同上
返 回 值：SEC_ERR_NUM_E
*/
int tks_add_key(char *name, char *encryptoname, tks_key user_pass, tks_key certi,
                opt option)
{
    int                        ret = 0;
    int                        sec_fd = 0;
    int                        retcode = SEC_ERROR_SUCCESS;
    tks_key                    encrytkey = {0};
    tks_key                    certi_key = {0};
    tks_key                    certi_userpass = {0};
    tks_key                    userpass = {0};
    request_dec_pack_userpass  dec_key;
    request_dec_pack_userpass  dec_up;
    request_dec_simple         certi_data;
    rpmb_keycontainer          rpmb_key;
    char                       keyname[KEYNAME_LEN] = {0};

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    ret = tks_read_key(name, &encrytkey);
    if (ret == 0) {
        ALOGD("%s: name already exists!\n", __FUNCTION__);
        return SEC_ERROR_KEY_EXIST;
    }
    if (!strcmp(name, DEVCERT_NAME)) {
        // 检查密钥类型
        if (certi.key_type != KEY_RSA_1024_PRV &&
            certi.key_type != KEY_RSA_2048_PRV) {
            ALOGD("%s: for devcert, key type mismatch!\n", __FUNCTION__);
            return SEC_ERROR_KEYTYPE;
        }
    }
    // 初始化
    memset(&dec_key, 0, sizeof(request_dec_pack_userpass));
    memset(&dec_up, 0, sizeof(request_dec_pack_userpass));
    memset(&rpmb_key, 0, sizeof(rpmb_keycontainer));
    init_dec_simple(&certi_data);
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 解密userpass
    retcode = tks_dec_userpass(encryptoname, user_pass, sec_fd,
                               &dec_up, &encrytkey);
    if (retcode != SEC_ERROR_SUCCESS) {
        ALOGD("%s: tks_dec_userpass failed!\n", __FUNCTION__);
        goto ERR_EXIT;
    }
    // 根据encryptoname获取certi
    if (encryptoname != NULL) {
        if (certi.key_type == KEY_NAME) {
            // 使用encrytkey解密certi
            certi_data.encryptkey = encrytkey;
            copy_key2packdata(certi, &certi_data.data);
            if (ioctl(sec_fd, SEC_IOC_DEC_SIMPLE, &certi_data) < 0) {
                ALOGD("%s: ioctl decryption certi failed!\n", __FUNCTION__);
                ALOGD("%s: error = %d\n", __FUNCTION__, errno);
                retcode = SEC_ERROR_DEC_CERTI_FAIL;
                goto ERR_EXIT;
            }
            // 使用userpass解密certi
            memset(&certi_data.encryptkey, 0, sizeof(tks_key));
            certi_data.encryptkey = dec_up.userpass;
            if (ioctl(sec_fd, SEC_IOC_DEC_SIMPLE, &certi_data) < 0) {
                ALOGD("%s: ioctl decryption certi failed!\n", __FUNCTION__);
                ALOGD("%s: error = %d\n", __FUNCTION__, errno);
                retcode = SEC_ERROR_DEC_CERTI_FAIL;
                goto ERR_EXIT;
            }
            if (certi_data.data.len > KEYNAME_LEN) {
                ALOGD("%s: key name length is invalid!\n", __FUNCTION__);
                retcode = SEC_ERROR_DATALEN_INVALID;
                goto ERR_EXIT;
            }
            memcpy(keyname, certi_data.data.data, certi_data.data.len);
            ALOGD("%s: keyname is: %s\n", __FUNCTION__, keyname);
            // 获取certi所对应的userpass
            ret = rpmb_read_userpass(keyname, &certi_userpass);
            if (ret != 0) {
                ALOGD("%s: certi key userpass not found!\n", __FUNCTION__);
                retcode = SEC_ERROR_USERPASS_NOTFOUND;
                goto ERR_EXIT;
            }
            // 比较userpass
            ret = compare_key(certi_userpass, dec_up.userpass);
            ALOGD("%s: compare_key ret = %d\n", __FUNCTION__, ret);
            if (ret == 0) {
                // 获取密钥
                ret = tks_read_key(keyname, &certi_key);
                if (ret != 0) {
                    ALOGD("%s: read key failed!\n", __FUNCTION__);
                    retcode = SEC_ERROR_KEY_NOTFOUND;
                    goto ERR_EXIT;
                }
            } else {
                ALOGD("%s: userpass verify fail!\n", __FUNCTION__);
                retcode = SEC_ERROR_USERPASS_VERIFY_FAIL;
                goto ERR_EXIT;
            }
        } else {
            // 加密certi
            dec_key.encryptkey = encrytkey;
            dec_key.userpass   = certi;
            if (ioctl(sec_fd, SEC_IOC_DEC_PACK_USERPASS, &dec_key) < 0) {
                ALOGD("%s: ioctl decryption certi failed!\n", __FUNCTION__);
                ALOGD("%s: error = %d\n", __FUNCTION__, errno);
                retcode = SEC_ERROR_DEC_USERPASS_FAIL;
                goto ERR_EXIT;
            }
            certi_key = dec_key.userpass;
        }
    } else {
        ALOGD("%s: encryptoname is NULL,certi is in clear text.\n", __FUNCTION__);
        // 加密certi
        dec_key.encryptkey = encrytkey;
        dec_key.userpass   = certi;
        if (ioctl(sec_fd, SEC_IOC_DEC_PACK_USERPASS, &dec_key) < 0) {
            ALOGD("%s: ioctl decryption certi failed!\n", __FUNCTION__);
            ALOGD("%s: error = %d\n", __FUNCTION__, errno);
            retcode = SEC_ERROR_DEC_USERPASS_FAIL;
            goto ERR_EXIT;
        }
        certi_key = dec_key.userpass;
    }
    // 存储user_pass
    print_key("up is:", dec_up.userpass);
    ret = rpmb_write_userpass(name, dec_up.userpass);
    if (ret != 0) {
        ALOGD("%s: write user_pass failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 存储密钥
    rpmb_key.option = option;
    get_tm(&rpmb_key.keyinfo.timestored);
    ALOGD("%s: timestored:\n", __FUNCTION__);
    print_time(rpmb_key.keyinfo.timestored);
    rpmb_key.key = certi_key;
    print_key("key is:", rpmb_key.key);
    ret = tks_write_key(name, rpmb_key);
    if (ret != 0) {
        ALOGD("%s: write key failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 保存设备公钥证书
    if (!strcmp(name, DEVCERT_NAME)) {
        ret = tks_save_devcert(name, &certi_key);
        if (ret != 0) {
            ALOGD("%s: save devcert key failed!\n", __FUNCTION__);
            retcode = ret;
            goto ERR_EXIT;
        }
    }
    retcode = SEC_ERROR_SUCCESS;
    // 结束处理
ERR_EXIT:
    close(sec_fd);
    release_dec_simple(&certi_data);
    update_sysinfo(name, 2, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：根据名称删除密钥。
参数定义：[in] name：密钥名称，如果名称为空，则删除所有密钥；删除所有密钥时，某
                     个密钥也受标志位控制，删除所有密钥时，密钥不符合的也无法删
                     除；具体情况受加密钥时的flags控制。
          [in] encryptoname：在tks中已经存在的一个密钥名称，如果其为null，则
                             user_pass为明文，否则，tks需要通过其解密user_pass
                             才可以使用。
          [in] user_pass：与name指定的密钥关联的user_pass，根据name的标志位判断
                          是否需要验证user_pass。
返 回 值：SEC_ERR_NUM_E
*/
int tks_del_key(char *name, char *encryptoname, tks_key user_pass)
{
    int               ret = 0;
    int               key_num = 0;
    int               i = 0;
    int               sec_fd = 0;
    keyname_list_t    name_list = NULL;
    int               retcode = SEC_ERROR_SUCCESS;
    request_dec_pack_userpass  userpass_data;
    tks_key                    encrytkey = {0};

    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    // 先解密userpass，避免encryptoname被删除
    memset(&userpass_data, 0, sizeof(request_dec_pack_userpass));
    retcode = tks_dec_userpass(encryptoname, user_pass, sec_fd,
                               &userpass_data, &encrytkey);
    if (retcode != SEC_ERROR_SUCCESS) {
        ALOGD("%s: tks_dec_userpass failed!\n", __FUNCTION__);
        goto ERR_EXIT;
    }
    if (name != NULL) {
        // 删除指定密钥
        ALOGD("%s: del key by name = %s\n", __FUNCTION__, name);
        retcode = tks_del_key_by_name(name, userpass_data.userpass, sec_fd);
    } else {
        // 删除所有密钥
        ALOGD("%s: del all key\n", __FUNCTION__);
        ret = rpmb_getall_keyname(&name_list, &key_num);
        ALOGD("%s: rpmb_getall_keyname ret = %d key_num = %d\n",
              __FUNCTION__, ret, key_num);
        if (ret != 0 || key_num <= 0) {
            ALOGD("%s: read key name list failed!\n", __FUNCTION__);
            retcode = SEC_ERROR_KEY_NOTFOUND;
            goto ERR_EXIT;
        }
        for(i = 0;i < key_num;i ++)
        {
            ALOGD("%s: del all key by name = %s\n", __FUNCTION__, name_list[i]);
            retcode = tks_del_key_by_name(name_list[i], userpass_data.userpass,
                                          sec_fd);
        }
        retcode = SEC_ERROR_SUCCESS;
    }
ERR_EXIT:
    free(name_list);
    close(sec_fd);
    update_sysinfo(name, 3, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

LOCAL int tks_del_key_by_name(char *name, tks_key user_pass, int sec_fd)
{
    int                        ret = 0;
    int                        retcode = SEC_ERROR_SUCCESS;
    tks_key                    key = {0};
    tks_key                    key_userpass = {0};
    opt                        option;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) return SEC_ERROR_INPUT_PARAM_INVALID;
    // 初始化
    memset(&option, 0, sizeof(opt));
    // 读取控制参数
    ret = rpmb_read_opt(name, &option);
    if (ret != 0) {
        ALOGD("%s: read key opt failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_KEY_NOTFOUND;
        goto ERR_EXIT;
    }
    // 检查删除标志位
    if (!(option.flags & 0x4)) {
        ALOGD("%s: key deletion not allowed\n", __FUNCTION__);
        retcode = SEC_ERROR_OPTION_VERIFY_FAIL;
        goto ERR_EXIT;
    }
    // 读取密钥
    ret = tks_read_key(name, &key);
    if (ret != 0) {
        ALOGD("%s: deletion key not found!\n", __FUNCTION__);
        retcode = SEC_ERROR_KEY_NOTFOUND;
        goto ERR_EXIT;
    }
    // 检查userpass标志位
    if (option.flags & 0x8) {
        // 需要校验userpass
        ALOGD("%s: need check userpass\n", __FUNCTION__);
        // 读取保存的userpass
        ret = rpmb_read_userpass(name, &key_userpass);
        if (ret != 0) {
            ALOGD("%s: deletion key userpass not found!\n", __FUNCTION__);
            retcode = SEC_ERROR_USERPASS_NOTFOUND;
            goto ERR_EXIT;
        }
        // 比较userpass
        ret = compare_key(key_userpass, user_pass);
        ALOGD("%s: compare_key ret = %d\n", __FUNCTION__, ret);
        if (ret == 0) {
            // 删除密钥
            ret = rpmb_del_key(name);
            if (ret != 0) {
                ALOGD("%s: del key failed!\n", __FUNCTION__);
                retcode = SEC_ERROR_DEL_KEY_FAIL;
                goto ERR_EXIT;
            }
        } else {
            ALOGD("%s: userpass verify fail!\n", __FUNCTION__);
            retcode = SEC_ERROR_USERPASS_VERIFY_FAIL;
            update_keyinfo_fail(name);
            goto ERR_EXIT;
        }
    } else {
        // 无需校验userpass，直接删除密钥
        ALOGD("%s: not be checked userpass\n", __FUNCTION__);
        ret = rpmb_del_key(name);
        if (ret != 0) {
            ALOGD("%s: del key failed!\n", __FUNCTION__);
            retcode = SEC_ERROR_DEL_KEY_FAIL;
            goto ERR_EXIT;
        }
    }
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：根据name查询密钥是否存在，或者提供存入时间等。
          注意info中不包含密钥本身。如果密钥不存在，返回值做提示
参数定义：[in]  name:要查询的密钥名称
          [out] info:返回要查询的密钥信息
返 回 值：SEC_ERR_NUM_E
*/
int tks_get_keyinfo(char *name, key_info *info)
{
    int    ret = 0;
    int    retcode = 0;
    opt    option;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL || info == NULL) {
        ALOGD("%s: input paramater invalid!\n", __FUNCTION__);
        retcode = SEC_ERROR_INPUT_PARAM_INVALID;
        goto ERR_EXIT;
    }
    ALOGD("%s: name: %s\n", __FUNCTION__, name);
    // 读取控制参数
    memset(&option, 0, sizeof(opt));
    ret = rpmb_read_opt(name, &option);
    if (ret != 0) {
        ALOGD("%s: read key opt failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_KEY_NOTFOUND;
        goto ERR_EXIT;
    }
    // 检测是否允许读取
    if (!(option.flags & 0x2)) {
        ALOGD("%s: get keyinfo not allowed!\n", __FUNCTION__);
        retcode = SEC_ERROR_OPTION_VERIFY_FAIL;
        goto ERR_EXIT;
    }
    // 获取key info
    memset(info, 0, sizeof(key_info));
    ret = rpmb_read_keyinfo(name, info);
    if (ret != 0) {
        ALOGD("%s: read key info fail!\n", __FUNCTION__);
        retcode = SEC_ERROR_KEY_NOTFOUND;
        goto ERR_EXIT;
    }
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    update_sysinfo(name, 4, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能: 获取密钥系统的整体信息。
参数定义：[in]  flags：用于控制需要输出那些信息：bit0:timeinit,bit1:timeaccessed,...
                       无详细定义，目前不处理；
          [out] einfo
返 回 值：SEC_ERR_NUM_E
*/
int tks_get_sysinfo(int flags, sys_info *einfo)
{
    int         ret = 0;
    int         retcode = 0;
    sys_info    info;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (einfo == NULL) {
        ALOGD("%s: einfo is NULL!\n", __FUNCTION__);
        retcode = SEC_ERROR_INPUT_PARAM_INVALID;
        goto ERR_EXIT;
    }
    // 获取sys info
    memset(&info, 0, sizeof(sys_info));
    ret = rpmb_read_sysinfo(&info);
    if (ret != 0) {
        ALOGD("%s: read sys info fail!\n", __FUNCTION__);
        retcode = SEC_ERROR_READ_SYSINFO_FAIL;
        goto ERR_EXIT;
    }
    memset(einfo, 0, sizeof(sys_info));
    *einfo = info;
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    update_sysinfo(NULL, 5, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：重设名称为name的密钥的user_pass。
参数定义：[in] name: 相关的密钥名称
          [in] encryptoname: 在tks中已经存在的一个密钥名称，如果其为null，则
                             old_pass为明文，否则，tks需要通过其解密才可以使用。
          [in] old_pass: 当和内部的user_pass符合才修改
          [in] new_pass: 新的凭据，其被old_pass加密
返 回 值：SEC_ERR_NUM_E
*/
int tks_rst_userpass(char *name, char *encryptoname, tks_key old_pass, tks_key new_pass)
{
    int                        ret = 0;
    int                        sec_fd = 0;
    int                        retcode = SEC_ERROR_SUCCESS;
    tks_key                    key_userpass = {0};
    tks_key                    encrytkey = {0};
    request_dec_pack_userpass  dec_pack_data;
    opt                        option;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 初始化
    memset(&dec_pack_data, 0, sizeof(request_dec_pack_userpass));
    memset(&option, 0, sizeof(opt));
    // 检测是否允许重设user_pass
    ret = rpmb_read_opt(name, &option);
    if (ret != 0) {
        ALOGD("%s: read key opt failed!\n", __FUNCTION__);
        return SEC_ERROR_OPTION_VERIFY_FAIL;
    }
    if (!(option.flags & 0x1)) {
        ALOGD("%s: reset user_pass not allowed!\n", __FUNCTION__);
        return SEC_ERROR_RESET_UP_DENIED;
    }
    // 读取保存的userpass
    ret = rpmb_read_userpass(name, &key_userpass);
    if (ret != 0) {
        ALOGD("%s: userpass not found!\n", __FUNCTION__);
        retcode = SEC_ERROR_USERPASS_NOTFOUND;
        goto ERR_EXIT;
    }
    // 检查密钥类型
    if (key_userpass.key_type != KEY_AES_128 &&
        key_userpass.key_type != KEY_AES_256) {
        ALOGD("%s: only aes key can be reset!\n", __FUNCTION__);
        retcode = SEC_ERROR_KEYTYPE;
        goto ERR_EXIT;
    }
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 解密old_pass
    retcode = tks_dec_userpass(encryptoname, old_pass, sec_fd,
                               &dec_pack_data, &encrytkey);
    if (retcode != SEC_ERROR_SUCCESS) {
        ALOGD("%s: tks_dec_userpass failed!\n", __FUNCTION__);
        goto ERR_EXIT;
    }
    // 比较old_pass
    ret = compare_key(key_userpass, dec_pack_data.userpass);
    ALOGD("%s: compare_key ret = %d\n", __FUNCTION__, ret);
    if (ret != 0) {
        ALOGD("%s: old_pass verify fail!\n", __FUNCTION__);
        retcode = SEC_ERROR_USERPASS_VERIFY_FAIL;
        update_keyinfo_fail(name);
        goto ERR_EXIT;
    }
    // 使用old_pass解密new_pass
    dec_pack_data.encryptkey = dec_pack_data.userpass;
    dec_pack_data.userpass   = new_pass;
    if (ioctl(sec_fd, SEC_IOC_DEC_PACK_USERPASS, &dec_pack_data) < 0) {
        ALOGD("%s: ioctl decryption new_pass failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_DEC_USERPASS_FAIL;
        goto ERR_EXIT;
    }
    // 保存new_pass
    ret = rpmb_write_userpass(name, dec_pack_data.userpass);
    if (ret != 0) {
        ALOGD("%s: write new_pass failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(name, 6, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：签名。
          在应用送数据到tks前，需要把in的数据通过user_pass加密，在tks收到in数据
          后，通过user_pass进行解密，然后再执行。在tks输出out数据时，需要通过
          user_pass进行加密，然后再输出。应用在使用out数据前，需要先通过user_pass解密。
参数定义：[in]  name：用来签名的密钥名称
          [in]  data：需要签名的明文
          [out] signature：签名结果
返 回 值：SEC_ERR_NUM_E
*/
int tks_sign(char *name, packdata data, packdata *signature)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    tks_key           enckey = {0};
    tks_key           userpass = {0};
    request_crypto    cryptreq;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    if (signature == NULL || signature->data == NULL) {
        ALOGD("%s: signature is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    // 检查数据长度
    if (data.len > MAX_FRAME_LENGTH) {
        ALOGD("%s: data length invalid!\n", __FUNCTION__);
        return SEC_ERROR_DATALEN_INVALID;
    }
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    // 获取密钥
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read name failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 检查密钥是否有效
    ret = tks_check_key_valid(name, SIGN);
    if (ret != 0) {
        ALOGD("%s: invalid key!\n", __FUNCTION__);
        return SEC_ERROR_KEY_INVALID;
    }
    // 获取userpass
    ret = rpmb_read_userpass(name, &userpass);
    if (ret != 0) {
        ALOGD("%s: read userpass failed!\n", __FUNCTION__);
        return SEC_ERROR_USERPASS_NOTFOUND;
    }
    // 检查密钥类型
    if (enckey.key_type != KEY_RSA_1024_PRV &&
        enckey.key_type != KEY_RSA_2048_PRV) {
        ALOGD("%s: encrypt key type mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 签名
    cryptreq.op         = SIGN;
    cryptreq.encryptkey = enckey;
    cryptreq.userpass   = userpass;
    memcpy(signature->data, data.data, data.len);
    cryptreq.data.data = signature->data;
    cryptreq.data.len  = data.len;
//print_crypto("Before sign, cryptreq is: ", cryptreq);
    if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_SIGN_FAIL;
        goto ERR_EXIT;
    }
    signature->len = cryptreq.data.len;
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(NULL, 7, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：验证。
          在应用送数据到tks前，需要把in的数据通过user_pass加密，在tks收到in数据
          后，通过user_pass进行解密，然后再执行。
参数定义：[in] name：用来验证签名的密钥名称
          [in] data：需要验证的明文
          [in] signature：需要验证的签名
返 回 值：SEC_ERR_NUM_E
*/
int tks_verify(char *name, packdata data, packdata signature)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    tks_key           enckey = {0};
    tks_key           userpass = {0};
    request_crypto    cryptreq;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    // 获取密钥
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read name failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 检查密钥是否有效
    ret = tks_check_key_valid(name, VERIFY);
    if (ret != 0) {
        ALOGD("%s: invalid key!\n", __FUNCTION__);
        return SEC_ERROR_KEY_INVALID;
    }
    // 获取userpass
    ret = rpmb_read_userpass(name, &userpass);
    if (ret != 0) {
        ALOGD("%s: read userpass failed!\n", __FUNCTION__);
        return SEC_ERROR_USERPASS_NOTFOUND;
    }
    // 检查密钥类型
    if (enckey.key_type != KEY_RSA_1024_PUB &&
        enckey.key_type != KEY_RSA_2048_PUB &&
        enckey.key_type != KEY_RSA_1024_PRV &&
        enckey.key_type != KEY_RSA_2048_PRV) {
        ALOGD("%s: encrypt key type mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
#if 0
    if (userpass.key_type != KEY_RSA_1024_PUB &&
        userpass.key_type != KEY_RSA_2048_PUB) {
        ALOGD("%s: userpass keytype mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
#endif
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 验证
    cryptreq.op         = VERIFY;
    cryptreq.encryptkey = enckey;
    cryptreq.userpass   = userpass;
    cryptreq.data       = data;
    cryptreq.signature  = signature;
//print_crypto("Before verify, cryptreq is: ", cryptreq);
    if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_VERIFY_FAIL;
        goto ERR_EXIT;
    }
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(NULL, 8, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：加密。
          在应用送数据到tks前，需要把in的数据通过user_pass加密，在tks收到in数据
          后，通过user_pass进行解密，然后再执行。在tks输出out数据时，需要通过
          user_pass进行加密，然后再输出。应用在使用out数据前，需要先通过user_pass解密。
参数定义：[in]  name：用来验证签名的密钥名称，密钥可以是对称也可以是非对称
          [in]  data：需要加密的明文
          [out] dataencrypted：加密结果
返 回 值：SEC_ERR_NUM_E
*/
int tks_encrypt(char *name, packdata data, packdata *dataencypted)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    int               inputlen = 0;
    int               inpos = 0, outpos = 0;
    int               frame_len = 0;
    tks_key           enckey = {0};
    tks_key           userpass = {0};
    request_crypto    cryptreq;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    ALOGD("%s: key = %s\n", __FUNCTION__, name);
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    // 获取密钥
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read name failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 检查密钥是否有效
    ret = tks_check_key_valid(name, ENC);
    if (ret != 0) {
        ALOGD("%s: invalid key!\n", __FUNCTION__);
        return SEC_ERROR_KEY_INVALID;
    }
    // 获取userpass
    ret = rpmb_read_userpass(name, &userpass);
    if (ret != 0) {
        ALOGD("%s: read userpass failed!\n", __FUNCTION__);
        return SEC_ERROR_USERPASS_NOTFOUND;
    }
    // 检查密钥类型
    if (userpass.key_type == KEY_RSA_1024_PUB ||
        userpass.key_type == KEY_RSA_2048_PUB) {
        ALOGD("%s: userpass keytype mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 加密
    cryptreq.op         = ENC;
    cryptreq.encryptkey = enckey;
    cryptreq.userpass   = userpass;
    inputlen            = data.len;
    ALOGD("%s: ready to encrypt: inputlen = %d\n", __FUNCTION__, inputlen);
    while (inputlen > 0) {
        frame_len = (inputlen <= MAX_FRAME_LENGTH) ? inputlen : MAX_FRAME_LENGTH;
        ALOGD("%s: frame_len = %d\n", __FUNCTION__, frame_len);
        memcpy(dataencypted->data+outpos, data.data+inpos, frame_len);
        cryptreq.data.data = dataencypted->data + outpos;
        cryptreq.data.len  = frame_len;
        if (inputlen <= MAX_FRAME_LENGTH) {
            cryptreq.op |= 0x10;
        }
        if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
            ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
            ALOGD("%s: error = %d\n", __FUNCTION__, errno);
            retcode = SEC_ERROR_ENCRYPT_FAIL;
            // 通知sec world做清除的动作
            if (errno < 1000) {
                tks_err_notify(sec_fd);
            }
            goto ERR_EXIT;
        }
#ifdef KERNEL_TEST
        if (s_is_need_break) {
            tks_err_notify(sec_fd);
            s_is_need_break = 0;
            goto ERR_EXIT;
        }
#endif
        inpos    += frame_len;
        outpos   += cryptreq.data.len;
        inputlen -= frame_len;
        ALOGD("%s: inputlen = %d\n", __FUNCTION__, inputlen);
    }
    dataencypted->len = outpos;
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(NULL, 9, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：解密。
          在应用送数据到tks前，需要把in的数据通过user_pass加密，在tks收到in数据
          后，通过user_pass进行解密，然后再执行。在tks输出out数据时，需要通过
          user_pass进行加密，然后再输出。应用在使用out数据前，需要先通过user_pass解密。
参数定义：[in]  name：用来验证签名的密钥名称，密钥可以是对称也可以是非对称
          [in]  dataencrypted：需要解密的密文
          [out] data：解密结果（明文被userpass加密）
返 回 值：SEC_ERR_NUM_E
*/
int tks_decrypt(char *name, packdata dataencrypted, packdata *data)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    int               inputlen = 0;
    int               inpos = 0, outpos = 0;
    int               frame_len = 0;
    tks_key           deckey = {0};
    tks_key           userpass = {0};
    request_crypto    cryptreq;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    ALOGD("%s: key = %s\n", __FUNCTION__, name);
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    // 获取密钥
    ret = tks_read_key(name, &deckey);
    if (ret != 0) {
        ALOGD("%s: read name failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    // 检查密钥是否有效
    ret = tks_check_key_valid(name, DEC);
    if (ret != 0) {
        ALOGD("%s: invalid key!\n", __FUNCTION__);
        return SEC_ERROR_KEY_INVALID;
    }
    // 获取userpass
    ret = rpmb_read_userpass(name, &userpass);
    if (ret != 0) {
        ALOGD("%s: read userpass failed!\n", __FUNCTION__);
        return SEC_ERROR_USERPASS_NOTFOUND;
    }
    // 检查密钥类型
    if (deckey.key_type == KEY_RSA_1024_PUB ||
        deckey.key_type == KEY_RSA_2048_PUB) {
        ALOGD("%s: deckey keytype mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
    if (userpass.key_type == KEY_RSA_1024_PUB ||
        userpass.key_type == KEY_RSA_2048_PUB) {
        ALOGD("%s: userpass keytype mismatch!\n", __FUNCTION__);
        return SEC_ERROR_KEYTYPE;
    }
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 解密
    cryptreq.op         = DEC;
    cryptreq.encryptkey = deckey;
    cryptreq.userpass   = userpass;
    inputlen            = dataencrypted.len;
    while (inputlen > 0) {
        frame_len = (inputlen <= MAX_FRAME_LENGTH) ? inputlen : MAX_FRAME_LENGTH;
        ALOGD("%s: frame_len = %d\n", __FUNCTION__, frame_len);
        memcpy(data->data+outpos, dataencrypted.data+inpos, frame_len);
        cryptreq.data.data = data->data+outpos;
        cryptreq.data.len  = frame_len;
        if (inputlen <= MAX_FRAME_LENGTH) {
            cryptreq.op |= 0x10;
        }
        if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
            ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
            ALOGD("%s: error = %d\n", __FUNCTION__, errno);
            retcode = SEC_ERROR_DECRYPT_FAIL;
            // 通知sec world做清除的动作
            if (errno < 1000) {
                tks_err_notify(sec_fd);
            }
            goto ERR_EXIT;
        }
#ifdef KERNEL_TEST
        if (s_is_need_break) {
            tks_err_notify(sec_fd);
            s_is_need_break = 0;
            goto ERR_EXIT;
        }
#endif
        inpos    += frame_len;
        outpos   += cryptreq.data.len;
        inputlen -= frame_len;
        ALOGD("%s: inputlen = %d\n", __FUNCTION__, inputlen);
    }
    data->len = outpos;
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    update_sysinfo(NULL, 10, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/*
功    能：添加dh交换密钥。
          在应用送数据到tks前，需要把in的数据通过user_pass加密，在tks收到in数据
          后，通过user_pass进行解密，然后再执行。在tks输出out数据时，需要通过
          user_pass进行加密，然后再输出。应用在使用out数据前，需要先通过user_pass解密。
参数定义：[in] name: 要存储在tks中的密钥名称
          [in] encryptoname: 在tks中已经存在的一个密钥名称，如果其为null，则
                             user_pass, param，b为明文，否则，tks需要通过其解
                             密user_pass, param才可以使用。
          [in] user_pass: 和密钥一起存储，为访问此密钥的验证凭据
          [in] param: dh算法参数（a，g，p）
          [in] option: 参考2.1.1“产生密钥对”
          [out] b: tks输出，用于调用者产生密钥
返 回 值：SEC_ERR_NUM_E
*/
int tks_add_dhkey(char *name, char *encryptoname, tks_key user_pass,
                  dh_param param, opt option, big_int *b)
{
    int                        ret = 0;
    int                        sec_fd = 0;
    int                        retcode = SEC_ERROR_SUCCESS;
    tks_key                    encrytkey = {0};
    request_dec_pack_userpass  userpass_data;
    request_gen_dhkey          dhkey;
    rpmb_keycontainer          keyprv;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (name == NULL) {
        ALOGD("%s: name is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    if (b == NULL) {
        ALOGD("%s: b is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    ret = tks_read_key(name, &encrytkey);
    if (ret == 0) {
        ALOGD("%s: name already exists!\n", __FUNCTION__);
        return SEC_ERROR_KEY_EXIST;
    }
    // 初始化
    memset(&userpass_data, 0, sizeof(request_dec_pack_userpass));
    memset(&keyprv, 0, sizeof(rpmb_keycontainer));
    init_dhkey(&dhkey);
    // 打开sec设备节点
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 解密userpass
    retcode = tks_dec_userpass(encryptoname, user_pass, sec_fd,
                               &userpass_data, &encrytkey);
    if (retcode != SEC_ERROR_SUCCESS) {
        ALOGD("%s: tks_dec_userpass failed!\n", __FUNCTION__);
        goto ERR_EXIT;
    }
    // 解密dh_param
    // TBD??

    // 生成DHKEY
    dhkey.encryptkey = userpass_data.userpass;
    copy_dhparam(param, &dhkey.param);
    if (ioctl(sec_fd, SEC_IOC_GEN_DHKEY, &dhkey) < 0) {
        ALOGD("%s: ioctl SEC_IOC_GEN_DHKEY failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_GEN_DHKEY_FAIL;
        goto ERR_EXIT;
    }
    // 保存userpass
    ret = rpmb_write_userpass(name, userpass_data.userpass);
    if (ret != 0) {
        ALOGD("%s: write user_pass failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 保存dhkey
    keyprv.option = option;
    get_tm(&keyprv.keyinfo.timestored);
    copy_bigint2key(dhkey.k, &keyprv.key);
    ret = tks_write_key(name, keyprv);
    if (ret != 0) {
        ALOGD("%s: write dhkey failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_KEY_FAIL;
        goto ERR_EXIT;
    }
    // 输出dhkey
    copy_bigint(dhkey.b, b);
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    release_dhkey(&dhkey);
    update_sysinfo(NULL, 11, retcode);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/**---------------------------------------------------------------------------*
 **                         LOCAL FUNCTION DEFINITION                         *
 **---------------------------------------------------------------------------*/
int tks_dec_userpass(char *encryptoname, tks_key in_pass, int  fd,
                     request_dec_pack_userpass *out_pass, tks_key *encrytkey)
{
    int        ret = 0;
    int        retcode = SEC_ERROR_SUCCESS;

    // 入口参数检查
    ALOGD("%s: enter\n", __FUNCTION__);
    if (in_pass.keylen == 0) {
        ALOGD("%s: in_pass is empty, return succ\n", __FUNCTION__);
        return SEC_ERROR_SUCCESS;
    }
    if (out_pass == NULL || encrytkey == NULL) {
        ALOGD("%s: out_pass is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    // 初始化
    memset(out_pass, 0, sizeof(request_dec_pack_userpass));
    memset(encrytkey, 0, sizeof(tks_key));
    // 读取encryptkey
    if (encryptoname != NULL) {
        ret = tks_read_key(encryptoname, encrytkey);
        if (ret != 0) {
            ALOGD("%s: read encryptoname failed!\n", __FUNCTION__);
            retcode = SEC_ERROR_ENCRYPTKEY_NOTFOUND;
            goto ERR_EXIT;
        }
        // 拷贝encrytkey到IO数据结构中
        out_pass->encryptkey = *encrytkey;
    } else {
        ALOGD("%s: encryptoname is NULL\n", __FUNCTION__);
    }
    // 解密userpass
    out_pass->userpass = in_pass;
    if (ioctl(fd, SEC_IOC_DEC_PACK_USERPASS, out_pass) < 0) {
        ALOGD("%s: ioctl decryption userpass failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_DEC_USERPASS_FAIL;
        goto ERR_EXIT;
    }
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

/* 返回值：0: 相等；   */
/*       非0: 不相等； */
LOCAL int compare_key(tks_key  key1, tks_key  key2)
{
    if (key1.key_type != key2.key_type) return 1;
    if (key1.keylen   != key2.keylen) return 1;
    return memcmp(key1.keydata, key2.keydata, key1.keylen);
}

LOCAL void init_dec_simple(request_dec_simple  *p)
{
    if (p == NULL) {
        ALOGE("%s: p is NULL!\n", __FUNCTION__);
        return;
    }
    memset(p, 0, sizeof(request_dec_simple));
    p->data.len = 0;
    p->data.data = malloc(PACKDATA_MAX_LENGTH);
    memset(p->data.data, 0, PACKDATA_MAX_LENGTH);
    return;
}

LOCAL void copy_key2packdata(tks_key  src_key, packdata  *p_data)
{
    if (p_data == NULL || p_data->data == NULL) {
        ALOGE("%s: p_data or p_data->data is NULL!\n", __FUNCTION__);
        return;
    }
    p_data->len  = src_key.keylen;
    memset(p_data->data, 0, PACKDATA_MAX_LENGTH);
    memcpy(p_data->data, src_key.keydata, src_key.keylen);
    return;
}

LOCAL void release_dec_simple(request_dec_simple  *p)
{
    if (p == NULL) {
        ALOGE("%s: p is NULL!\n", __FUNCTION__);
        return;
    }
    memset(&p->encryptkey, 0, sizeof(tks_key));
    p->data.len = 0;
    free(p->data.data);
    return;
}

LOCAL void init_dhkey(request_gen_dhkey  *p)
{
    if (p == NULL) {
        ALOGE("%s: p is NULL!\n", __FUNCTION__);
        return;
    }
    memset(p, 0, sizeof(request_gen_dhkey));
    p->b.len = 0;
    p->k.len = 0;
    p->b.data = malloc(PACKDATA_MAX_LENGTH);
    p->k.data = malloc(PACKDATA_MAX_LENGTH);
    memset(p->b.data, 0, PACKDATA_MAX_LENGTH);
    memset(p->k.data, 0, PACKDATA_MAX_LENGTH);
    p->param.a.len = 0;
    p->param.g.len = 0;
    p->param.p.len = 0;
    p->param.a.data = malloc(PACKDATA_MAX_LENGTH);
    p->param.g.data = malloc(PACKDATA_MAX_LENGTH);
    p->param.p.data = malloc(PACKDATA_MAX_LENGTH);
    memset(p->param.a.data, 0, PACKDATA_MAX_LENGTH);
    memset(p->param.g.data, 0, PACKDATA_MAX_LENGTH);
    memset(p->param.p.data, 0, PACKDATA_MAX_LENGTH);
    return;
}

LOCAL void copy_bigint(big_int  src_int, big_int  *p_dst)
{
    if (p_dst == NULL || p_dst->data == NULL) {
        ALOGE("%s: p_dst or p_dst->data is NULL!\n", __FUNCTION__);
        return;
    }
    p_dst->len  = src_int.len;
    memset(p_dst->data, 0, PACKDATA_MAX_LENGTH);
    memcpy(p_dst->data, src_int.data, src_int.len);
    return;
}

LOCAL void copy_dhparam(dh_param  src_param, dh_param  *p_dst)
{
    if (p_dst == NULL) {
        ALOGE("%s: p_dst is NULL!\n", __FUNCTION__);
        return;
    }
    copy_bigint(src_param.a, &p_dst->a);
    copy_bigint(src_param.g, &p_dst->g);
    copy_bigint(src_param.p, &p_dst->p);
    return;
}

LOCAL void copy_bigint2key(big_int  src_data, tks_key  *p_dst)
{
    if (p_dst == NULL) {
        ALOGE("%s: p_dst is NULL!\n", __FUNCTION__);
        return;
    }
    p_dst->keylen = src_data.len;
    p_dst->key_type = KEY_RSA_1024_PRV; // TBD ??
    memcpy(p_dst->keydata, src_data.data, src_data.len);
    return;
}

LOCAL void release_dhkey(request_gen_dhkey  *p)
{
    if (p == NULL) {
        ALOGE("%s: p is NULL!\n", __FUNCTION__);
        return;
    }
    memset(&p->encryptkey, 0, sizeof(tks_key));
    p->param.a.len = 0;
    p->param.g.len = 0;
    p->param.p.len = 0;
    free(p->param.a.data);
    free(p->param.g.data);
    free(p->param.p.data);
    p->b.len = 0;
    p->k.len = 0;
    free(p->b.data);
    free(p->k.data);
    return;
}

LOCAL void get_tm(struct tm  *p_result)
{
    time_t        now;
    struct tm    *timenow;

    if (p_result == NULL) return;
    time(&now);
    timenow = localtime(&now);
    memcpy(p_result, timenow, sizeof(struct tm));
    return;
}

LOCAL int tks_save_devcert(char  *name, tks_key  *key)
{
    const char  *atrsp;
    int          ret = 0;
    int          retcode = SEC_ERROR_SUCCESS;
    packdata     signature;
    packdata     data;
    tks_cert     cert;
    sys_info     sysinfo;

    // 入口参数检查
    if (name == NULL || key == NULL) {
        ALOGE("%s: name or key is NULL!\n", __FUNCTION__);
        return SEC_ERROR_INPUT_PARAM_INVALID;
    }
    // 初始化
    ALOGD("%s: enter keyname = %s\n", __FUNCTION__, name);
    memset(&cert, 0, sizeof(tks_cert));
    memset(&sysinfo, 0, sizeof(sys_info));
    // 获取imei
#ifndef KERNEL_TEST
    atrsp = sendAt(0, 0, "AT+CGSN\r");
    if (!strcmp("OK", atrsp)) {
        ALOGD("%s: get imei succ: %s", __FUNCTION__, atrsp);
    } else {
        ALOGD("%s: get imei failed", __FUNCTION__);
        goto ERR_EXIT;
    }
#else
    atrsp = "123456789012345";
#endif
    memcpy(cert.devid, atrsp, strlen(atrsp));
    // 对imei做签名
    data.len  = sizeof(cert.devid);
    data.data = cert.devid;
    signature.len  = 0;
    signature.data = cert.signature;
    print_packdata("sign data is: ", data);
    ret = tks_sign(name, data, &signature);
    if (ret != SEC_ERROR_SUCCESS) {
        ALOGD("%s: sign imei failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_DEVCERT_FAIL;
        goto ERR_EXIT;
    }
    // 从key中读取公钥信息到pubkey中
    memcpy(&cert.pubkey.exp, key->keydata, 4);
    memcpy(cert.pubkey.mod, key->keydata+4, sizeof(cert.pubkey.mod));
    // 读取密钥系统信息
    ret = rpmb_read_sysinfo(&sysinfo);
    if (ret != 0) {
        ALOGD("%s: read sys info fail!\n", __FUNCTION__);
        retcode = SEC_ERROR_READ_SYSINFO_FAIL;
        goto ERR_EXIT;
    }
    // 保存设备公钥
    sysinfo.devcert = cert;
    ret = rpmb_write_sysinfo(sysinfo);
    if (ret != 0) {
        ALOGD("%s: write sys_info failed!\n", __FUNCTION__);
        retcode = SEC_ERROR_SAVE_DEVCERT_FAIL;
        goto ERR_EXIT;
    }
    // 结束处理
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

LOCAL int tks_write_key(char  *name, rpmb_keycontainer  key)
{
    int    ret = 0;

    ret = rpmb_write_key(name, key);
    if (ret == 0) {
        update_keyinfo_store(name);
    } else {
        update_keyinfo_fail(name);
    }
    return ret;
}

LOCAL int tks_read_key(char  *name, tks_key  *key)
{
    int    ret = 0;

    ret = rpmb_read_key(name, key);
    if (ret == 0) {
        update_keyinfo_access(name);
    } else {
        update_keyinfo_fail(name);
    }
    return ret;
}

LOCAL void update_keyinfo_store(char  *name)
{
    int         ret = 0;
    key_info    keyinfo;

    ALOGD("%s: enter, keyname = %s\n", __FUNCTION__, name);
    memset(&keyinfo, 0, sizeof(key_info));
    ret = rpmb_read_keyinfo(name, &keyinfo);
    if (ret != 0) {
        ALOGD("%s: read old keyinfo failed, do nothing\n", __FUNCTION__);
        return;
    }
    get_tm(&keyinfo.timestored);
    keyinfo.timesfailed = 0;
    rpmb_write_keyinfo(name, keyinfo);
    return;
}

LOCAL void update_keyinfo_access(char  *name)
{
    int         ret = 0;
    key_info    keyinfo;

    ALOGD("%s: enter, keyname = %s\n", __FUNCTION__, name);
    memset(&keyinfo, 0, sizeof(key_info));
    ret = rpmb_read_keyinfo(name, &keyinfo);
    if (ret != 0) {
        ALOGD("%s: read old keyinfo failed, do nothing\n", __FUNCTION__);
        return;
    }
    get_tm(&keyinfo.timeaccessed);
    rpmb_write_keyinfo(name, keyinfo);
    return;
}

LOCAL void update_keyinfo_fail(char  *name)
{
    int         ret = 0;
    key_info    keyinfo;
    opt         option;

    ALOGD("%s: enter, keyname = %s\n", __FUNCTION__, name);
    memset(&keyinfo, 0, sizeof(key_info));
    memset(&option, 0, sizeof(opt));
    ret = rpmb_read_keyinfo(name, &keyinfo);
    if (ret != 0) {
        ALOGD("%s: read old keyinfo failed, do nothing\n", __FUNCTION__);
        return;
    }
    get_tm(&keyinfo.timefailed);
    keyinfo.timesfailed ++;
    ALOGD("%s: timesfailed = %d\n", __FUNCTION__, keyinfo.timesfailed);
    ret = rpmb_read_opt(name, &option);
    if (ret == 0) {
        if ((option.flags & 0x10) &&
            (keyinfo.timesfailed > option.locklimit)) {
            ALOGD("%s: key failure reach locklimit !\n", __FUNCTION__);
            keyinfo.flags = 1;
        }
    } else {
        ALOGD("%s: read key opt failed, do nothing\n", __FUNCTION__);
    }
    rpmb_write_keyinfo(name, keyinfo);
    return;
}

LOCAL void update_sysinfo(char  *name, int  apino, int  retcode)
{
    int         ret = 0;
    int         len = 0;
    int         keylen = 0;
    int         copylen = 0;
    sys_info    sysinfo;

    ALOGD("%s: enter. apino = %d, retcode = %d\n", __FUNCTION__, apino, retcode);
    memset(&sysinfo, 0, sizeof(sys_info));
    ret = rpmb_read_sysinfo(&sysinfo);
    if (ret != 0) {
        ALOGD("%s: read sys info fail, do nothing\n", __FUNCTION__);
        return;
    }
    if (retcode == SEC_ERROR_SUCCESS) {
        get_tm(&sysinfo.timeaccessed);
        sysinfo.timesfailed = 0;
    } else {
        get_tm(&sysinfo.timefailed);
        sysinfo.timesfailed ++;
        sysinfo.codefailed  = apino;
        sysinfo.codefailed |= retcode << 8;
        if (name != NULL) {
            len = strlen(name);
            keylen = sizeof(sysinfo.keyname);
            copylen = (len <= keylen) ? len : keylen;
            memcpy(sysinfo.keyname, name, copylen);
        }
    }
    rpmb_write_sysinfo(sysinfo);
    return;
}

/* 返回值：0: key有效；   */
/*       非0: key无效；   */
LOCAL int tks_check_key_valid(char  *name, uint8_t  operation)
{
    int         ret = 0;
    key_info    keyinfo;
    opt         option;
    time_t      now;
    time_t      limit;

    // 读取keyinfo
    memset(&keyinfo, 0, sizeof(key_info));
    ret = rpmb_read_keyinfo(name, &keyinfo);
    if (ret != 0) {
        ALOGD("%s: read keyinfo failed, do nothing\n", __FUNCTION__);
        return 1;
    }
    // 读取option
    memset(&option, 0, sizeof(opt));
    ret = rpmb_read_opt(name, &option);
    if (ret != 0) {
        ALOGD("%s: read key opt failed!\n", __FUNCTION__);
        return 1;
    }
    // 检查是否允许对应的操作
    switch(operation) {
      case SIGN:
        if (!(option.flags & 0x200)) {
            ALOGD("%s: the key cannot do SIGN operation!\n", __FUNCTION__);
            return 1;
        }
        break;
      case VERIFY:
        if (!(option.flags & 0x200)) {
            ALOGD("%s: the key cannot do VERIFY operation!\n", __FUNCTION__);
            return 1;
        }
        break;
      case ENC:
        if (!(option.flags & 0x100)) {
            ALOGD("%s: the key cannot do ENC operation!\n", __FUNCTION__);
            return 1;
        }
        break;
      case DEC:
        if (!(option.flags & 0x100)) {
            ALOGD("%s: the key cannot do DEC operation!\n", __FUNCTION__);
            return 1;
        }
        break;
      default:
        ALOGD("%s: operation mismatch!\n", __FUNCTION__);
        return 1;
    }
    // 检查密钥是否被锁定
    if (keyinfo.flags == 0x01) {
        ALOGD("%s: key is locked!\n", __FUNCTION__);
        return 1;
    }
    // 检查密钥是否过期
    time(&now);
    limit = mktime(&option.timelimit);
    if (now > limit) {
        ALOGD("%s: the key is overdue!\n", __FUNCTION__);
        return 1;
    }
    ALOGD("%s: key check passed\n", __FUNCTION__);
    return 0;
}

LOCAL void tks_err_notify(int  fd)
{
    request_crypto    cryptreq;

    ALOGD("%s: enter\n", __FUNCTION__);
    memset(&cryptreq, 0, sizeof(request_crypto));
    cryptreq.op = 4;
    if (ioctl(fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed again!\n", __FUNCTION__);
        ALOGD("%s: error = %d, do nothing!\n", __FUNCTION__, errno);
        return;
    }
    ALOGD("%s: leave\n", __FUNCTION__);
}

/*****************************************************/
/* The following functions are used only for testing */
/*****************************************************/
void test_dec_simple(char *name, tks_key  *in_out_key)
{
    request_dec_simple    data;
    int sec_fd = -1;
    tks_key    enckey = {0};
    int ret = 0;

    ALOGD("%s: enter\n", __FUNCTION__);
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read key failed!\n", __FUNCTION__);
        return;
    }
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return;
    }
    init_dec_simple(&data);
    data.encryptkey = enckey;
    copy_key2packdata(*in_out_key, &data.data);
    if (ioctl(sec_fd, SEC_IOC_DEC_SIMPLE, &data) < 0) {
        ALOGD("%s: SEC_IOC_DEC_SIMPLE failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        return;
    }
    ALOGD("%s: dec data.len = %d\n", __FUNCTION__, data.data.len);
    in_out_key->keylen = data.data.len;
    memcpy(in_out_key->keydata, data.data.data, data.data.len);
    release_dec_simple(&data);
    return;
}

int test_encrypt_by_key(char *name, tks_key  *in_out_key)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    int               frame_len = 0;
    request_crypto    cryptreq;
    byte              data_arr[10240] = {0};
    tks_key           enckey = {0};

    ALOGD("%s: enter\n", __FUNCTION__);
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read key failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 加密
    cryptreq.op         = ENC;
    cryptreq.op        |= 0x10;
    cryptreq.encryptkey = enckey;
    cryptreq.data.data  = data_arr;
    copy_key2packdata(*in_out_key, &cryptreq.data);
    ALOGD("%s: before SEC_IOC_CRYPTO\n", __FUNCTION__);
    print_key("encryptkey", cryptreq.encryptkey);
    print_packdata("data is: ", cryptreq.data);
    if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_ENCRYPT_FAIL;
        goto ERR_EXIT;
    }
    in_out_key->keylen = cryptreq.data.len;
    memcpy(in_out_key->keydata, cryptreq.data.data, cryptreq.data.len);
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

int test_encrypt_by_upAndkey(char *name, tks_key  *in_out_key)
{
    int               ret = 0;
    int               sec_fd = 0;
    int               retcode = SEC_ERROR_SUCCESS;
    int               frame_len = 0;
    request_crypto    cryptreq;
    byte              data_arr[10240] = {0};
    tks_key           enckey = {0};
    tks_key           userpass = {0};

    ALOGD("%s: enter\n", __FUNCTION__);
    // 初始化
    memset(&cryptreq, 0, sizeof(request_crypto));
    ret = tks_read_key(name, &enckey);
    if (ret != 0) {
        ALOGD("%s: read key failed!\n", __FUNCTION__);
        return SEC_ERROR_KEY_NOTFOUND;
    }
     // 获取userpass
    ret = rpmb_read_userpass(name, &userpass);
    if (ret != 0) {
        ALOGD("%s: read userpass failed!\n", __FUNCTION__);
        return SEC_ERROR_USERPASS_NOTFOUND;
    }
   sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return SEC_ERROR_OPEN_SEC_DRIVER_FAIL;
    }
    // 使用userpass加密
    cryptreq.op         = ENC;
    cryptreq.op        |= 0x10;
    cryptreq.encryptkey = userpass;
    cryptreq.data.data  = data_arr;
    copy_key2packdata(*in_out_key, &cryptreq.data);
    ALOGD("%s: before SEC_IOC_CRYPTO\n", __FUNCTION__);
    print_key("encryptkey is up", cryptreq.encryptkey);
    print_packdata("data is: ", cryptreq.data);
    if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_ENCRYPT_FAIL;
        goto ERR_EXIT;
    }
    // 使用enckey加密
    cryptreq.op         = ENC;
    cryptreq.op        |= 0x10;
    cryptreq.encryptkey = enckey;
    ALOGD("%s: before SEC_IOC_CRYPTO\n", __FUNCTION__);
    print_key("encryptkey", cryptreq.encryptkey);
    print_packdata("data is: ", cryptreq.data);
    if (ioctl(sec_fd, SEC_IOC_CRYPTO, &cryptreq) < 0) {
        ALOGD("%s: ioctl crypto failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        retcode = SEC_ERROR_ENCRYPT_FAIL;
        goto ERR_EXIT;
    }
    // Done
    in_out_key->keylen = cryptreq.data.len;
    memcpy(in_out_key->keydata, cryptreq.data.data, cryptreq.data.len);
    retcode = SEC_ERROR_SUCCESS;
ERR_EXIT:
    close(sec_fd);
    ALOGD("%s: leave ret = %d\n", __FUNCTION__, retcode);
    return retcode;
}

#ifdef KERNEL_TEST
void test_set_break()
{
    ALOGD("%s: enter\n", __FUNCTION__);
    s_is_need_break = 1;
}
#endif
