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
#include <time.h>
#include "tks_lib.h"
#ifndef KERNEL_TEST
#include <cutils/properties.h>
#include <utils/Log.h>
#endif
#include "rpmb_api.h"

#include <pthread.h>
#include <sched.h>

#ifdef KERNEL_TEST
#define ALOGD    printf
#define ALOGE    printf
#endif

char *name[] = {"testkey0","testkey1", "testkey2", "testkey3", "testkey4", "testkey5"};
char *encryptoname = "encryptkey";
// test master key
char *s_name_master = "master_key";
#define MASTER_KEY_TEXT "000102030405060708090A0B0C0D0E0F"
tks_key s_master_key = {KEY_AES_256, 32,{0}};
// test data key names
char *s_name_aes = "aes_256";
char* s_name_deletable = "aes_256_deletable";
char *s_name_aes_del_denied = "aes_256_del_denied";
char *s_name_aes_upaes = "aes_256_upaes";
char *s_name_aes_uprsa = "aes_256_uprsa";
char *s_name_rsa = "rsa_2048";
char *s_name_rsa_upaes = "rsa_2048_upaes";
char *s_name_rsa_uprsa = "rsa_2048_uprsa";
char *s_name_rsa_4 = "rsa_2048_4";
char *s_name_rsa_5 = "rsa_2048_5";
char *s_name_rsa_6 = "rsa_2048_6";
char *s_name_for_rst = "rst_aes_256_up";
char *s_name_for_rst_aes = "rst_aes_256_upaes";
char *s_name_for_cannot_rst = "cannot_rst_aes_256_up";
char *s_name_keyname = "key_keyname";
char *s_name_option = "test_option";
// test data userpass
tks_key s_up_null = {KEY_AES_128, 0, {0}};
tks_key s_up_aes = {KEY_AES_256, 32, {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38}};
tks_key s_up_aes_keyaes = {0};
tks_key s_up_aes_keyrsa = {0};
tks_key s_up_rsaprv = {KEY_RSA_2048_PRV, 1156, {0}};
tks_key s_new_pass = {KEY_AES_256, 32,
                      {0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
                       0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
                       0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
                       0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39}};
// test data for add key
tks_key s_certi_aes = {KEY_AES_256, 32,
                       {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                        0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                        0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                        0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38}};
tks_key s_certi_aes_encaes = {0};
tks_key s_certi_keyname = {KEY_NAME, 0, {0}};
opt s_option = {0b1111111111,3,{0}};

byte  s_data[] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                  0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                  0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                  0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38};
//packdata s_sign_data = {32, "12345678123456781234567812345678"};
packdata s_sign_data = {32, s_data};
packdata s_plaintext_32 = {32, "12345678123456781234567812345678"};
byte s_bigdata[] =
{
    #include "plaintext.txt"
};
packdata s_plaintext_3200 = {3200, s_bigdata};
packdata s_sign_big = {245, s_bigdata};

byte s_rsa1[] =
{
    #include "rsakey1.txt"
};
byte s_rsa2[] =
{
    #include "rsakey2.txt"
};
byte s_rsa3[] =
{
    #include "rsakey3.txt"
};
tks_key s_rsa_key1 = {KEY_RSA_2048_PRV, 516, {0}};
tks_key s_rsa_key2 = {KEY_RSA_2048_PRV, 516, {0}};
tks_key s_rsa_key3 = {KEY_RSA_2048_PRV, 516, {0}};


extern void hexdump(const char *title, const unsigned char *s, int l);
extern void print_key(const char  *name, tks_key  key);
extern void print_userpass(const char  *name, tks_key  up);
extern void print_packdata(const char  *title, packdata  data);
extern void print_time(struct tm  time);
extern void test_dec_simple(char *name, tks_key  *in_out_key);
extern int test_encrypt_by_key(char *name, tks_key  *in_out_key);
extern int test_encrypt_by_upAndkey(char *name, tks_key  *in_out_key);
extern void test_set_break();

const char *keytypeToString(int type) {
    switch(type) {
        case KEY_AES_128:return"KEY_AES_128";
        case KEY_AES_256:return"KEY_AES_256";
        case KEY_RSA_1024_PUB:return"KEY_RSA_1024_PUB";
        case KEY_RSA_1024_PRV:return"KEY_RSA_1024_PRV";
        case KEY_RSA_2048_PUB:return"KEY_RSA_2048_PUB";
        case KEY_RSA_2048_PRV:return"KEY_RSA_2048_PRV";
        case KEY_NAME:return"KEY_NAME";
        default: return "unknown key type";
    }
}

void bin2hex(byte *bin_ptr, int length, char *hex_ptr)
{
    int i;
    byte tmp;

    if (bin_ptr == NULL || hex_ptr == NULL) {
        return;
    }
    for (i=0; i<length; i++) {
        tmp = ((bin_ptr[i] & 0xf0)>>4);
        if (tmp <= 9) {
            *hex_ptr = (tmp + '0');
        } else {
            *hex_ptr = (tmp + 'A' - 10);
        }
        hex_ptr++;
        tmp = (bin_ptr[i] & 0x0f);
        if (tmp <= 9) {
            *hex_ptr = (tmp + '0');
        } else {
            *hex_ptr = (tmp + 'A' - 10);
        }
        hex_ptr++;
    }
}

int compare_packdata(packdata  s1, packdata  s2)
{
    if (s1.len == 0 || s2.len == 0) {
        ALOGD("[test] compare_packdata:  length is 0\n");
        return 1;
    }
    if (s1.len != s2.len) {
        ALOGD("[test] compare_packdata:  length is not equal!\n");
        return 1;
    }
    return memcmp(s1.data, s2.data, s1.len);
}

void test_read_key(char *name)
{
    tks_key  key4print = {0};
    tks_key  up = {0};
    int      ret = 0;

    rpmb_read_key(name, &key4print);
    print_key(name, key4print);

    rpmb_read_userpass(name, &up);
    if (ret == 0) {
        print_key("userpass", up);
    }
}

void add_key(char *name, char *encryptoname, tks_key user_pass, tks_key certi,
                  opt option)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    // Add KEY_AES_256
    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ALOGD("type is: %s\n", keytypeToString(certi.key_type));
    ret = tks_add_key(name, encryptoname, user_pass, certi, option);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("---------------------------\n");
        ALOGD(" Add key: %s SUCC\n", name);
        ALOGD("---------------------------\n");
#if 0
        memset(&key4print, 0, sizeof(tks_key));
        ret = rpmb_read_key(name, &key4print);
        if (ret == 0) {
            print_key(name, key4print);
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read key fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
        ret = rpmb_read_userpass(name, &key4print);
        if (ret == 0) {
            print_userpass(name, key4print);
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read userpass fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
#endif
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test]2. Add key FAIL !\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
ERROR:
    return;
}

void generate_key1(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa, NULL, s_up_null, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
        memset(&key4print, 0, sizeof(tks_key));
        ret = rpmb_read_key(s_name_rsa, &key4print);
        if (ret == 0) {
            s_up_rsaprv = key4print;
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read rsaprv fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void generate_key2(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa_upaes, NULL, s_up_aes, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
        memset(&key4print, 0, sizeof(tks_key));
        ret = rpmb_read_key(s_name_rsa_upaes, &key4print);
        if (ret == 0) {
            print_key(s_name_rsa_upaes, key4print);
//            s_up_rsaprv = key4print;
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read rsaprv fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void generate_key3(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa_uprsa, NULL, s_up_rsaprv, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void generate_key4(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa_4, s_name_aes, s_up_aes_keyaes, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        print_key(s_name_rsa_4, rsapub);
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void generate_key5(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa_5, s_name_rsa, s_up_aes_keyrsa, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        print_key(s_name_rsa_uprsa, rsapub);
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void generate_key6(void)
{
    int        ret = 0;
    tks_key    rsapub;
    tks_key    key4print;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_gen_keypair(s_name_rsa_6, s_name_aes, s_up_null, s_option, &rsapub);
    if (ret == SEC_ERROR_SUCCESS) {
        print_key(s_name_rsa_6, rsapub);
        ALOGD("-----------------------\n");
        ALOGD(" Generate rsa key SUCC \n");
        ALOGD("-----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] Generate rsa key FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void test_generate_key(void)
{
    int  ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
#if 0
    generate_key1();
    generate_key2();
    generate_key3();
//    generate_key4();
//    generate_key5();
//    generate_key6();
#else
    ALOGD("Genrate key from test data\n");
    add_key(s_name_rsa,       NULL, s_up_null,  s_rsa_key1, s_option);
    add_key(s_name_rsa_upaes, NULL, s_up_aes,   s_rsa_key2, s_option);
    add_key(s_name_rsa_uprsa, NULL, s_rsa_key1, s_rsa_key3, s_option);
    ret = rpmb_read_key(s_name_rsa, &s_up_rsaprv);
    if (ret != 0) {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("test_generate_key FAIL! EXIT!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        exit(0);
    }
//    test_read_key(s_name_rsa);
//    test_read_key(s_name_rsa_upaes);
//    test_read_key(s_name_rsa_uprsa);
#endif
}

void test_key_option(void)
{
    int         ret = 0;
    opt         option = {0b1111111111,3,{0}};
    key_info    info = {0};
    byte        enc_arr[10] = {0};
    packdata    enc_data = {0, enc_arr};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");

    ALOGD("****************\n");
    ALOGD("test flags = 0x2\n");
    ALOGD("****************\n");
    /* 0x2: 可以获取密钥信息 */
    option.flags = 0b1111111111;
    add_key(s_name_option, NULL, s_up_null, s_certi_aes, option);
    ret = tks_get_keyinfo(s_name_option, &info);
    ALOGD("tks_get_keyinfo ret = %d\n", ret);
    ret = tks_del_key(s_name_option, NULL, s_up_null);

    option.flags = 0b1111111101;
    add_key(s_name_option, NULL, s_up_null, s_certi_aes, option);
    ret = tks_get_keyinfo(s_name_option, &info);
    ALOGD("tks_get_keyinfo ret = %d\n", ret);
    ret = tks_del_key(s_name_option, NULL, s_up_null);
    ALOGD("*****************\n");
    ALOGD("test flags = 0x10\n");
    ALOGD("*****************\n");
    /* 0x10:  是否使用lock limit功能 */
    /* locklimit = 2 */
    option.flags = 0b1111111111;
    option.locklimit = 2;
    add_key(s_name_option, NULL, s_rsa_key1, s_certi_aes, option);
    ret = tks_del_key(s_name_option, s_name_aes, s_up_aes_keyaes);
    ret = tks_del_key(s_name_option, s_name_aes, s_up_aes_keyaes);
    ret = tks_encrypt(s_name_option, s_plaintext_32, &enc_data);
    ALOGD("test locklimit: tks_encrypt ret = %d\n", ret);

    /* 0x1:   可以被重设user_pass */
    /* test in test_add_key()     */

    /* 0x4:   可以被删除      */
    /* test in test_del_key() */

    /* 0x8:   删除时是否需要userpass验证 */
    /* test in test_del_key()            */

    /* 0x20:  是否需要userpass授权使用 */
    /* 默认都需要，不再做为测试项      */

    /* 0x80:  是否为临时密钥(临时密钥仅在内存中使用，不保存) */
    /* 对tks lib透明，需在rpmb层验证                         */

    /* 0x100: 是否可以加解密  */
    /* test in test_del_key() */

    /* 0x200: 是否可以签名验签                */
    /* test in test_encrypt(),test_decrypt()  */

    /* timelimit = 2014-xx-xx             */
    /* test in test_init(),test_add_key() */
}

void test_encrypt(char *name, char *typestr, packdata in_data, packdata *out_data)
{
    int    ret = 0;

    ALOGD("%s: enter: %s\n", __FUNCTION__, typestr);
    //print_packdata("[test]plain text:", in_data);
    ret = tks_encrypt(name, in_data, out_data);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("---------------------\n");
        ALOGD(" tks_encrypt %s SUCC\n", typestr);
        ALOGD("---------------------\n");
        //print_packdata("[test]encrypt text:", *out_data);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("tks_encrypt %s FAIL!\n", typestr);
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void test_decrypt(char *name, char *typestr, packdata in_data, packdata comp_data,
                  packdata *out_data)
{
    int         ret = 0;

    ALOGD("%s: enter: %s\n", __FUNCTION__, typestr);
    //print_packdata("[test]before dec text:", in_data);
    ret = tks_decrypt(name, in_data, out_data);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("---------------------\n");
        ALOGD(" tks_decrypt %s SUCC\n", typestr);
        ALOGD("---------------------\n");
        //print_packdata("[test]after dec text:", *out_data);
//        ret = compare_packdata(comp_data, *out_data);
//        ALOGD("[test]tks_decrypt compare_packdata ret = %d\n", ret);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("tks_decrypt %s FAIL!\n", typestr);
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void test_decrypt_up(char *name, char *typestr, tks_key in, tks_key *out)
{
    packdata    in_data = {0};
    packdata    enc_data = {0};
    packdata    comp_data = {0};
    byte        data_arr[1024] = {0};
    packdata    out_data = {0, data_arr};

    enc_data.len = in.keylen;
    enc_data.data = in.keydata;
    out_data.data = out->keydata;
    test_decrypt(name, typestr, enc_data, comp_data, &out_data);
    out->key_type = KEY_AES_256;
    out->keylen = out_data.len;
}

void test_add_key(void)
{
    s_option.flags = 0b1111110111;
    add_key(s_name_deletable, NULL, s_up_null, s_certi_aes, s_option);

    s_option.flags = 0b1111111011;
    add_key(s_name_aes_del_denied, NULL, s_up_null, s_certi_aes, s_option);

    s_option.flags = 0b1111111111;
//    print_key("s_up_aes_keyaes", s_up_aes_keyaes);
//    print_key("s_certi_aes_encaes", s_certi_aes_encaes);
    add_key(s_name_aes_upaes, s_name_aes, s_up_aes_keyaes, s_certi_aes_encaes,
            s_option);
    add_key(s_name_aes_uprsa, NULL, s_rsa_key1, s_certi_aes, s_option);

    add_key(s_name_for_rst, NULL, s_up_aes, s_certi_aes, s_option);
    add_key(s_name_for_rst_aes, s_name_aes, s_up_aes_keyaes, s_certi_aes_encaes,
            s_option);
}

void test_add_key_name(void)
{
    int         len = 0;
    char       *keyname_key = "keyname_enckey";
    byte        in_arr[128] = {0};
    byte        enc_arr[128] = {0};
    byte        out_arr[128] = {0};
    packdata    in_data = {0, in_arr};
    packdata    enc_data = {0, enc_arr};
    packdata    out_data = {0, out_arr};

    len = strlen(s_name_aes_upaes);
    memcpy(in_data.data, s_name_aes_upaes, len);
    in_data.len = len;
    test_encrypt(s_name_aes, "keyname,enc by key", in_data, &enc_data);
    print_packdata("after enc by key, s_certi_keyname:", enc_data);

    test_encrypt(s_name_aes, "keyname,enc by up", enc_data, &out_data);
    s_certi_keyname.keylen = out_data.len;
    memcpy(s_certi_keyname.keydata, out_data.data, out_data.len);
    print_key("after enc by up, s_certi_keyname:", s_certi_keyname);
    add_key(s_name_keyname, s_name_aes, s_up_aes_keyaes, s_certi_keyname,
            s_option);
}

void test_add_key_devcert(void)
{
    add_key(DEVCERT_NAME, NULL, s_up_null, s_rsa_key1, s_option);
}

void test_encANDdec_AES(void)
{
    byte        enc_arr[10240]   = {0};
    byte        enc_arr1[10240]  = {0};
    byte        dec_arr[10240]   = {0};
    byte        plain_arr[10240] = {0};
    packdata    enc_data   = {0, enc_arr};
    packdata    enc_data1  = {0, enc_arr1};
    packdata    dec_data   = {0, dec_arr};
    packdata    plain_data = {0, plain_arr};

    ALOGD("*****************************\n");
    ALOGD("Enckey: AES;  Userpass: NULL \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "aes 32 bytes", s_plaintext_32, &enc_data);
    test_decrypt(s_name_aes, "aes 32 bytes", enc_data, s_plaintext_32, &dec_data);
    print_packdata("after dec:aes upnull 32bytes", dec_data);

//    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
//    test_decrypt(s_name_aes, "aes 3200 bytes", enc_data, s_plaintext_3200, &dec_data);
//    print_packdata("after dec:aes upnull 3200bytes", dec_data);
    ALOGD("*****************************\n");
    ALOGD("Enckey: AES;  Userpass: AES  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "aes 32 bytes", s_plaintext_32, &enc_data);
    test_encrypt(s_name_aes_upaes, "aes 32 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_aes_upaes, "aes 32 bytes", enc_data1, s_plaintext_32, &dec_data);
    test_decrypt(s_name_aes, "aes 32 bytes", dec_data, s_plaintext_32, &plain_data);
    print_packdata("after dec:aes upaes 32bytes", plain_data);

//    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
//    test_encrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data, &enc_data1);
//    test_decrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
//    test_decrypt(s_name_aes, "aes 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
//    print_packdata("after dec:aes upaes 3200bytes", plain_data);
    ALOGD("*****************************\n");
    ALOGD("Enckey: AES;  Userpass: RSA  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_rsa, "aes 32 byte", s_plaintext_32, &enc_data);
    test_encrypt(s_name_aes_uprsa, "aes 32 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_aes_uprsa, "aes 32 bytes", enc_data1, s_plaintext_32, &dec_data);
    test_decrypt(s_name_rsa, "aes 32 bytes", dec_data, s_plaintext_32, &plain_data);
    print_packdata("after dec:aes uprsa 32bytes", plain_data);

//    test_encrypt(s_name_rsa, "aes 3200 bytes", s_plaintext_3200, &enc_data);
//    test_encrypt(s_name_aes_uprsa, "aes 3200 bytes", enc_data, &enc_data1);
//    test_decrypt(s_name_aes_uprsa, "aes 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
//    test_decrypt(s_name_rsa, "aes 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
//    print_packdata("after dec:aes uprsa 3200bytes", plain_data);
}

void test_encANDdec_RSA(void)
{
    byte        enc_arr[10240]   = {0};
    byte        enc_arr1[10240]  = {0};
    byte        dec_arr[10240]   = {0};
    byte        plain_arr[10240] = {0};
    packdata    enc_data   = {0, enc_arr};
    packdata    enc_data1  = {0, enc_arr1};
    packdata    dec_data   = {0, dec_arr};
    packdata    plain_data = {0, plain_arr};

    ALOGD("*****************************\n");
    ALOGD("Enckey: RSA;  Userpass: NULL \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_rsa, "rsa 32 bytes", s_plaintext_32, &enc_data);
    test_decrypt(s_name_rsa, "rsa 32 bytes", enc_data, s_plaintext_32, &dec_data);
    print_packdata("after dec:rsa upnull 32bytes", dec_data);

//    test_encrypt(s_name_rsa, "rsa 3200 bytes", s_plaintext_3200, &enc_data);
//    test_decrypt(s_name_rsa, "rsa 3200 bytes", enc_data, s_plaintext_3200, &dec_data);
//    print_packdata("after dec:rsa upnull 3200bytes", dec_data);
    ALOGD("*****************************\n");
    ALOGD("Enckey: RSA;  Userpass: AES  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "rsa 32 bytes", s_plaintext_32, &enc_data);
    test_encrypt(s_name_rsa_upaes, "rsa 32 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_rsa_upaes, "rsa 32 bytes", enc_data1, s_plaintext_32, &dec_data);
    test_decrypt(s_name_aes, "rsa 32 bytes", dec_data, s_plaintext_32, &plain_data);
    print_packdata("after dec:rsa upaes 32bytes", plain_data);

//    test_encrypt(s_name_aes, "rsa 3200 bytes", s_plaintext_3200, &enc_data);
//    test_encrypt(s_name_rsa_upaes, "rsa 3200 bytes", enc_data, &enc_data1);
//    test_decrypt(s_name_rsa_upaes, "rsa 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
//    test_decrypt(s_name_aes, "rsa 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
//    print_packdata("after dec:rsa upaes 3200bytes", plain_data);
    ALOGD("*****************************\n");
    ALOGD("Enckey: RSA;  Userpass: RSA  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_rsa, "rsa 32 bytes", s_plaintext_32, &enc_data);
    test_encrypt(s_name_aes_uprsa, "rsa 32 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_aes_uprsa, "rsa 32 bytes", enc_data1, s_plaintext_32, &dec_data);
    test_decrypt(s_name_rsa, "rsa 32 bytes", dec_data, s_plaintext_32, &plain_data);
    print_packdata("after dec:rsa uprsa 32bytes", plain_data);

//    test_encrypt(s_name_rsa, "rsa 3200 bytes", s_plaintext_3200, &enc_data);
//    test_encrypt(s_name_rsa_uprsa, "rsa 3200 bytes", enc_data, &enc_data1);
//    test_decrypt(s_name_rsa_uprsa, "rsa 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
//    test_decrypt(s_name_rsa, "rsa 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
//    print_packdata("after dec:rsa uprsa 3200bytes", plain_data);
}

void test_encANDdec_abnormal(void)
{
    byte        enc_arr[10240]   = {0};
    byte        enc_arr1[10240]  = {0};
    byte        dec_arr[10240]   = {0};
    byte        plain_arr[10240] = {0};
    packdata    enc_data   = {0, enc_arr};
    packdata    enc_data1  = {0, enc_arr1};
    packdata    dec_data   = {0, dec_arr};
    packdata    plain_data = {0, plain_arr};

    ALOGD("*****************************\n");
    ALOGD("Encrypt abnormal case        \n");
    ALOGD("*****************************\n");
#ifdef KERNEL_TEST
    test_set_break();
#endif
    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
    ALOGD("*****************************\n");
    ALOGD("After abnormal do enc again  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
    test_encrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
    test_decrypt(s_name_aes, "aes 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
    print_packdata("after dec:aes upaes 3200bytes", plain_data);

    ALOGD("*****************************\n");
    ALOGD("Decrypt abnormal case        \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
#ifdef KERNEL_TEST
    test_set_break();
#endif
    test_decrypt(s_name_aes, "aes 3200 bytes", enc_data, s_plaintext_3200, &dec_data);
    ALOGD("*****************************\n");
    ALOGD("After abnormal do dec again  \n");
    ALOGD("*****************************\n");
    test_encrypt(s_name_aes, "aes 3200 bytes", s_plaintext_3200, &enc_data);
    test_encrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data, &enc_data1);
    test_decrypt(s_name_aes_upaes, "aes 3200 bytes", enc_data1, s_plaintext_3200, &dec_data);
    test_decrypt(s_name_aes, "aes 3200 bytes", dec_data, s_plaintext_3200, &plain_data);
    print_packdata("after dec:aes upaes 3200bytes", plain_data);
}

void test_sign_verify_upnull(void)
{
    packdata    signature = {0};
    byte        sig_data[1024] = {0};
    int         ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    signature.data = sig_data;
    print_packdata("[test]packdata is: ", s_sign_data);
    ret = tks_sign(s_name_rsa, s_sign_data, &signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("--------------\n");
        ALOGD(" tks_sign SUCC\n");
        ALOGD("--------------\n");
        print_packdata("[test]signature is: ", signature);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_sign FAIL  !\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ret = tks_verify(s_name_rsa, s_sign_data, signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("----------------\n");
        ALOGD(" tks_verify SUCC\n");
        ALOGD("----------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_verify FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ALOGD("[test]test_sign_verify exit\n");
    return;
ERROR:
    exit(0);
}

void test_sign_verify_upaes(void)
{
    packdata    signature = {0};
    byte        sig_data[1024] = {0};
    byte        data_arr[1024] = {0};
    packdata    enc_data = {0, data_arr};
    int         ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    signature.data = sig_data;
    print_packdata("s_sign_data is: ", s_sign_data);
    test_encrypt(s_name_aes, "encrypt s_sign_data", s_sign_data, &enc_data);
    print_packdata("after enc,s_sign_data is: ", enc_data);
    ret = tks_sign(s_name_rsa_upaes, enc_data, &signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("--------------\n");
        ALOGD(" tks_sign SUCC\n");
        ALOGD("--------------\n");
        print_packdata("[test]signature is: ", signature);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_sign FAIL  !\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ret = tks_verify(s_name_rsa_upaes, enc_data, signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("----------------\n");
        ALOGD(" tks_verify SUCC\n");
        ALOGD("----------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_verify FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ALOGD("[test]test_sign_verify exit\n");
    return;
ERROR:
    exit(0);
}

void test_sign_verify_uprsa(void)
{
    packdata    signature = {0};
    byte        sig_data[1024] = {0};
    byte        data_arr[1024] = {0};
    packdata    enc_data = {0, data_arr};
    int         ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    signature.data = sig_data;
    print_packdata("s_sign_data is: ", s_sign_data);
    test_encrypt(s_name_rsa, "encrypt s_sign_data", s_sign_data, &enc_data);
    print_packdata("after enc,s_sign_data is: ", enc_data);
    ret = tks_sign(s_name_rsa_uprsa, enc_data, &signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("--------------\n");
        ALOGD(" tks_sign SUCC\n");
        ALOGD("--------------\n");
        print_packdata("[test]signature is: ", signature);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_sign FAIL  !\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ret = tks_verify(s_name_rsa_uprsa, enc_data, signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("----------------\n");
        ALOGD(" tks_verify SUCC\n");
        ALOGD("----------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_verify FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ALOGD("[test]test_sign_verify exit\n");
    return;
ERROR:
    exit(0);
}

void test_sign_verify_bigdata(void)
{
    packdata    signature = {0};
    byte        sig_data[1024] = {0};
    int         ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    signature.data = sig_data;
    print_packdata("[test]packdata is: ", s_sign_big);
    ret = tks_sign(s_name_rsa, s_sign_big, &signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("--------------\n");
        ALOGD(" tks_sign SUCC\n");
        ALOGD("--------------\n");
        print_packdata("[test]signature is: ", signature);
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_sign FAIL  !\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ret = tks_verify(s_name_rsa, s_sign_big, signature);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("----------------\n");
        ALOGD(" tks_verify SUCC\n");
        ALOGD("----------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("[test] tks_verify FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    ALOGD("[test]test_sign_verify exit\n");
    return;
ERROR:
    exit(0);
}

void test_sign_verify_abnormal(void)
{
    packdata    signature = {0};
    byte        sig_data[1024] = {0};
    int         ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    signature.data = sig_data;
    ret = tks_sign("aaa", s_sign_data, &signature);
    ALOGD("tks_sign ret = %d\n", ret);

    ret = tks_verify("aaa", s_sign_data, signature);
    ALOGD("tks_verify ret = %d\n", ret);

    s_sign_big.len = 1200;
    ret = tks_sign(s_name_rsa, s_sign_big, &signature);
    ALOGD("tks_sign ret = %d\n", ret);
    return;
}

void test_sign_verify(void)
{
    test_sign_verify_upnull();
    test_sign_verify_upaes();
    test_sign_verify_uprsa();
    test_sign_verify_bigdata();
    test_sign_verify_abnormal();
}

void del_1key(char  *name, char *encryptoname, tks_key user_pass)
{
    int         ret = 0;
    key_info    info = {0};

    ALOGD("******************************\n");
    ALOGD("%s: enter, name = %s\n", __FUNCTION__, name);
    ALOGD("******************************\n");
    ret = tks_del_key(name, encryptoname, user_pass);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("Del ok,read again\n");
        ret = tks_get_keyinfo(name, &info);
        ALOGD("tks_get_keyinfo ret = %d\n", ret);
        if (ret != 0) {
            ALOGD("------------------\n");
            ALOGD(" del_1key  SUCC\n");
            ALOGD("------------------\n");
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!\n");
            ALOGD("del_1key  FAIL!\n");
            ALOGD("!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
    } else if (ret == SEC_ERROR_OPTION_VERIFY_FAIL) {
        ALOGD("KEY OPTION VERIFY FAIL\n");
        ALOGD("------------------\n");
        ALOGD(" del_1key  SUCC\n");
        ALOGD("------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!\n");
        ALOGD("del_1key  FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }
    return;
ERROR:
    exit(0);
}

void del_allkey(char *encryptoname, tks_key user_pass)
{
    int         ret = 0;
    key_info    info = {0};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_del_key(NULL, encryptoname, user_pass);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("------------------\n");
        ALOGD(" del_allkey  SUCC\n");
        ALOGD("------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!\n");
        ALOGD("del_allkey  FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!\n");
        goto ERROR;
    }

    return;
ERROR:
    exit(0);
}

void test_del_key(void)
{
    ALOGD("============================================================\n");
    ALOGD("del_1key\n");
    ALOGD("============================================================\n");
    del_1key(s_name_deletable, NULL, s_up_null);
    del_1key(s_name_aes_del_denied, NULL, s_up_null);
    del_1key(s_name_aes_upaes, s_name_aes, s_up_aes_keyaes);
    ALOGD("============================================================\n");
    ALOGD("del_allkey\n");
    ALOGD("============================================================\n");
    ALOGD("run test_add_key again\n");
    test_add_key();
    del_allkey(s_name_aes, s_up_aes_keyaes);
}

void test_get_keyinfo(void)
{
    int         ret = 0;
    key_info    info;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    memset(&info, 0, sizeof(key_info));

    ret = tks_get_keyinfo(NULL, &info);
    ALOGD("tks_get_keyinfo ret = %d\n", ret);

    ret = tks_get_keyinfo(s_name_aes, &info);
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("timestored:\n");
        print_time(info.timestored);
        ALOGD("timeaccessed:\n");
        print_time(info.timeaccessed);
        ALOGD("timefailed:\n");
        print_time(info.timefailed);
        ALOGD("timesfailed = %d flages = %d\n", info.timesfailed, info.flags);
        ALOGD("----------------------\n");
        ALOGD(" test_get_keyinfo SUCC\n");
        ALOGD("----------------------\n");
    } else {
        ALOGD("tks_get_keyinfo ret = %d\n", ret);
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("test_get_keyinfo  FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
}

void test_get_sysinfo(void)
{
    int         ret = 0;
    sys_info    einfo = {0};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_get_sysinfo(0, &einfo);
#if 0
   struct tm  timeinit;     // 出厂后首次启动的时间
   struct tm  timeaccessed; // 最近访问时间
   struct tm  timefailed;   // 最近失败时间
   int        timesfailed;  // 最近失败次数，成功后清零
   int        codefailed;   // 最近失败的代号（包含api）,
                            // api num=codefailed&0xff,api按此文档顺序编号。
                            // 失败代号= codefailed>>8;
   char       keyname[30];  // 最近失败的密钥名称 30B
   tks_cert   devcert;      // 设备公钥证书

typedef struct tks_cert_st{
   rsa_pub    pubkey;
   byte       devid[15];      // device unique uuid,IMEI号15B
   byte       signature[256]; // signature of devid
} tks_cert;//RSA 1024:275B,RSA 2048:531B
#endif
    if (ret == SEC_ERROR_SUCCESS) {
        ALOGD("[test]timeinit:\n");
        print_time(einfo.timeinit);
        ALOGD("[test]timeaccessed:\n");
        print_time(einfo.timeaccessed);
        ALOGD("[test]timefailed:\n");
        print_time(einfo.timefailed);
        ALOGD("[test]timesfailed = %d codefailed = %d\n", einfo.timesfailed, einfo.codefailed);
        ALOGD("[test]keyname = %s\n", einfo.keyname);
        ALOGD("[test]devcert rsapub exp = %d\n", einfo.devcert.pubkey.exp);
        hexdump("devcert rsapub:", einfo.devcert.pubkey.mod, sizeof(einfo.devcert.pubkey.mod));
        hexdump("devid:", einfo.devcert.devid, sizeof(einfo.devcert.devid));
        hexdump("signature:", einfo.devcert.signature, sizeof(einfo.devcert.signature));
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("tks_get_sysinfo FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
    }
}

void rst_userpass1(void)
{
    int    ret = 0;
    tks_key key4print = {0};
    tks_key out = {0};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    print_key("new pass", s_new_pass);
    ret = tks_rst_userpass(s_name_for_rst, NULL, s_up_aes, s_new_pass);
    ALOGD("tks_rst_userpass ret = %d\n", ret);
    if (ret == SEC_ERROR_SUCCESS) {
        ret = rpmb_read_userpass(s_name_for_rst, &key4print);
        if (ret == 0) {
            print_userpass(s_name_for_rst, key4print);
            test_dec_simple(s_name_master, &key4print);
            print_key("dec new up:", key4print);
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read userpass fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
        ALOGD("----------------------\n");
        ALOGD(" tks_rst_userpass SUCC\n");
        ALOGD("----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("tks_rst_userpass FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
    }
    return;
ERROR:
    exit(0);
}

void rst_userpass2(void)
{
    int    ret = 0;
    tks_key key4print = {0};
    tks_key out = {0};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    print_key("new pass", s_new_pass);
    ret = tks_rst_userpass(s_name_for_rst_aes, s_name_aes, s_up_aes_keyaes, s_new_pass);
    ALOGD("tks_rst_userpass ret = %d\n", ret);
    if (ret == SEC_ERROR_SUCCESS) {
        ret = rpmb_read_userpass(s_name_for_rst_aes, &key4print);
        if (ret == 0) {
            print_userpass(s_name_for_rst_aes, key4print);
            test_dec_simple(s_name_master, &key4print);
            print_key("dec new up:", key4print);
        } else {
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            ALOGD("[test]read userpass fail\n");
            ALOGD("!!!!!!!!!!!!!!!!!!!!!!!!\n");
            goto ERROR;
        }
        ALOGD("----------------------\n");
        ALOGD(" tks_rst_userpass SUCC\n");
        ALOGD("----------------------\n");
    } else {
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
        ALOGD("tks_rst_userpass FAIL!\n");
        ALOGD("!!!!!!!!!!!!!!!!!!!!!\n");
    }
    return;
ERROR:
    exit(0);
}

void rst_userpass3(void)
{
    int    ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_rst_userpass(s_name_for_rst, NULL, s_up_null, s_new_pass);
    ALOGD("tks_rst_userpass ret = %d\n", ret);
    return;
}

void rst_userpass4(void)
{
    int    ret = 0;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    ret = tks_rst_userpass(s_name_rsa_uprsa, NULL, s_up_rsaprv, s_new_pass);
    ALOGD("tks_rst_userpass ret = %d\n", ret);
    return;
}

void rst_userpass5(void)
{
    int    ret = 0;
    opt    option;

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    option = s_option;
    option.flags = 0b1111111110;
    add_key(s_name_for_cannot_rst, NULL, s_up_aes, s_certi_aes, option);
    ret = tks_rst_userpass(s_name_for_cannot_rst, NULL, s_up_rsaprv, s_new_pass);
    ALOGD("tks_rst_userpass ret = %d\n", ret);
    return;
}

void test_rst_userpass(void)
{
    test_encrypt_by_key(s_name_aes, &s_new_pass);
    print_key("after enc by s_up_aes, new pass:", s_new_pass);
    rst_userpass1();
    rst_userpass2();
    rst_userpass3();
    rst_userpass4();
    rst_userpass5();
}

void test_init(void)
{
    int len = 0;

    ALOGD("[test]test_init enter\n");
    s_option.timelimit.tm_year = 2020;
    s_option.flags = 0b1111111111;
    add_key(s_name_aes, NULL, s_up_null, s_certi_aes, s_option);

    s_master_key.keylen = 32;
    s_master_key.key_type = KEY_AES_256;
    memcpy(s_master_key.keydata, MASTER_KEY_TEXT, 32);
    add_key(s_name_master, NULL, s_up_null, s_master_key, s_option);
    //print_key("s_master_key", s_master_key);

    memcpy(s_rsa_key1.keydata, s_rsa1, s_rsa_key1.keylen);
    memcpy(s_rsa_key2.keydata, s_rsa2, s_rsa_key2.keylen);
    memcpy(s_rsa_key3.keydata, s_rsa3, s_rsa_key3.keylen);
}

void test_data_generate(void)
{
    packdata    in_data = {0};
    packdata    enc_data = {0};
    packdata    comp_data = {0};
    byte        data_arr[1024] = {0};
    packdata    out_data = {0, data_arr};

    ALOGD("******************************\n");
    ALOGD("%s: enter\n", __FUNCTION__);
    ALOGD("******************************\n");
    in_data.len = s_up_aes.keylen;
    in_data.data = s_up_aes.keydata;
    enc_data.data = s_up_aes_keyaes.keydata;
    print_packdata("[test]in_data is: ", in_data);
    test_encrypt(s_name_aes, "s_up_aes_keyaes", in_data, &enc_data);
    s_up_aes_keyaes.key_type = KEY_AES_256;
    s_up_aes_keyaes.keylen = enc_data.len;
    print_key("aes userpass", s_up_aes_keyaes);
    comp_data.len  = s_up_aes.keylen;
    comp_data.data = s_up_aes.keydata;
    test_decrypt(s_name_aes, "aes", enc_data, comp_data, &out_data);
    //print_packdata("[test]out_data is: ", out_data);

    enc_data.data = s_certi_aes_encaes.keydata;
    print_packdata("[test]in_data is: ", s_plaintext_32);
    test_encrypt(s_name_aes, "aes 32 bytes", s_plaintext_32, &enc_data);
    s_certi_aes_encaes.key_type = KEY_AES_256;
    s_certi_aes_encaes.keylen = enc_data.len;
    print_key("aes key, enc by aes", s_certi_aes_encaes);
    memset(data_arr, 0, sizeof(data_arr));
    test_decrypt(s_name_aes, "aes", enc_data, s_plaintext_32, &out_data);
    //print_packdata("[test]out_data is: ", out_data);

    in_data.len = s_up_aes.keylen;
    in_data.data = s_up_aes.keydata;
    enc_data.data = s_up_aes_keyrsa.keydata;
    print_packdata("[test]in_data is: ", in_data);
    test_encrypt(s_name_rsa, "s_up_aes_keyrsa", in_data, &enc_data);
    s_up_aes_keyrsa.key_type = KEY_AES_256;
    s_up_aes_keyrsa.keylen = enc_data.len;
}

int set_cpu_affinity(unsigned int cpu)
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    ALOGD("set affinity to cpu #%u\n", cpu);
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset) < 0) {
        perror("sched_setaffinity");
        fprintf(stderr, "warning: unable to set cpu affinity\n");
        ALOGD("set_cpu_affinity return fail\n");
        return -1;
    }
    ALOGD("set_cpu_affinity return OK\n");
    return 0;
}

void main(int argc, char *argv[])
{
    ALOGD("\n\n");
    ALOGD("test_init, set_cpu_affinity() to cpu0 \n");
    set_cpu_affinity(0);
    ALOGD("============================================================\n");
    ALOGD("test_init\n");
    ALOGD("============================================================\n");
    test_init();
    ALOGD("============================================================\n");
    ALOGD("test_generate_key\n");
    ALOGD("============================================================\n");
    test_generate_key();
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_data_generate\n");
    ALOGD("============================================================\n");
    test_data_generate();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_add_key\n");
    ALOGD("============================================================\n");
    test_add_key();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_add_key_name\n");
    ALOGD("============================================================\n");
    test_add_key_name();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_add_key_devcert\n");
    ALOGD("============================================================\n");
    test_add_key_devcert();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_key_option\n");
    ALOGD("============================================================\n");
    test_key_option();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_sign_verify\n");
    ALOGD("============================================================\n");
    test_sign_verify();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_encrypt & test_decrypt AES\n");
    ALOGD("============================================================\n");
    test_encANDdec_AES();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_encrypt & test_decrypt RSA\n");
    ALOGD("============================================================\n");
    test_encANDdec_RSA();
#endif
#if 0
    ALOGD("============================================================\n");
    ALOGD("test_encrypt & test_decrypt abnormal\n");
    ALOGD("============================================================\n");
    test_encANDdec_abnormal();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_get_keyinfo\n");
    ALOGD("============================================================\n");
    test_get_keyinfo();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_get_sysinfo\n");
    ALOGD("============================================================\n");
    test_get_sysinfo();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_rst_userpass\n");
    ALOGD("============================================================\n");
    test_rst_userpass();
#endif
#if 1
    ALOGD("============================================================\n");
    ALOGD("test_del_key\n");
    ALOGD("============================================================\n");
    test_del_key();
#endif

    ALOGD("[test]Test done!\n");
    return;
}
