/******************************************************************************
**
 *  Generic SEC Library API
 *
 *  Copyright (C) 2014 Spreadtrum Communications Inc.
**
******************************************************************************/
#ifndef TKS_LIB_H
#define TKS_LIB_H
/**--------------------------------------------------------------------------*
 **                         Include Files                                    *
 **--------------------------------------------------------------------------*/
#include <stdint.h>
#include <time.h>
/**--------------------------------------------------------------------------*
 **                         MACRO DEFINITION                                 *
 **--------------------------------------------------------------------------*/
#define PACKDATA_MAX_LENGTH  256
#define DEVCERT_NAME         "DEVICEKEY"  // 设备密钥名称
#define MAX_FRAME_LENGTH     512

#define KERNEL_TEST
/**--------------------------------------------------------------------------*
 **                         TYPE AND CONSTANT                                *
 **--------------------------------------------------------------------------*/
typedef unsigned char    byte;
//typedef unsigned char    uint8_t;
//typedef unsigned short   uint16;

// For SEC Lib
typedef enum
{
    SEC_ERROR_SUCCESS = 0x0,       // 0: 成功
    SEC_ERROR_KEY_NOTFOUND,        // 1: name不存在
    SEC_ERROR_KEYTYPE,             // 2: 密钥类型错误
    SEC_ERROR_USERPASS_NOTFOUND,   // 3: userpass不存在
    SEC_ERROR_ENCRYPTKEY_NOTFOUND, // 4: encrytoname不存在
    SEC_ERROR_KEY_EXIST,           // 5: name已存在
    SEC_ERROR_DATALEN_INVALID,     // 6: 数据长度不匹配
    SEC_ERROR_DEC_USERPASS_FAIL,   // 7: 解密userpass失败
    SEC_ERROR_GEN_KEYPAIR_FAIL,    // 8: 创建密钥对失败
    SEC_ERROR_SAVE_KEY_FAIL,       // 9: 保存密钥失败
    SEC_ERROR_DEC_CERTI_FAIL,      // 10: 解密certi失败
    SEC_ERROR_OPTION_VERIFY_FAIL,  // 11: 标志位验证未通过
    SEC_ERROR_USERPASS_VERIFY_FAIL,// 12: userpass验证失败
    SEC_ERROR_DEL_KEY_FAIL,        // 13: 删除key失败
    SEC_ERROR_OPEN_SEC_DRIVER_FAIL,// 14: 打开sec设备失败
    SEC_ERROR_INPUT_PARAM_INVALID, // 15: 输入参数无效
    SEC_ERROR_GEN_DHKEY_FAIL,      // 16: 生成dhkey失败
    SEC_ERROR_SIGN_FAIL,           // 17: 签名失败
    SEC_ERROR_VERIFY_FAIL,         // 18: 签名验证失败
    SEC_ERROR_ENCRYPT_FAIL,        // 19: 加密失败
    SEC_ERROR_DECRYPT_FAIL,        // 20: 解密失败
    SEC_ERROR_SAVE_DEVCERT_FAIL,   // 21: 保存设备公钥证书失败
    SEC_ERROR_READ_SYSINFO_FAIL,   // 22: 读取密钥系统信息失败
    SEC_ERROR_KEY_INVALID,         // 23: 密钥已失效
    SEC_ERROR_RESET_UP_DENIED      // 24: 不允许重设user_pass
}SEC_ERR_NUM_E;

enum tks_keytype{
    KEY_AES_128,
    KEY_AES_256,
    KEY_RSA_1024_PUB,
    KEY_RSA_1024_PRV,
    KEY_RSA_2048_PUB,
    KEY_RSA_2048_PRV,
    KEY_NAME
};

enum crypto_op{
    SIGN,
    VERIFY,
    ENC,
    DEC
};

enum key_operate{
    ACCESS,
    WRITE,
    DEL
};

typedef struct rsa_pub_st{
    int exp;//4 bytes
    byte mod[256];// bitlen/8
} rsa_pub;//1024 RSA:132B,2048  RSA:260B

typedef struct rsa_prv_st{
   int pubexp;       // 4 bytes
   byte pubmod[256]; // bitlen/8
   byte primep[128]; // bitlen/16
   byte primeq[128]; // bitlen/16
   byte expp[128];   // bitlen/16
   byte expq[128];   // bitlen/16
   byte coef[128];   // bitlen/16
   byte prvexp[256]; // bitlen/8
} rsa_prv;//1024 RSA:580B,2048 RSA:1156B

typedef struct  tks_key_st{
   enum tks_keytype  key_type;      // 4bytes
   int               keylen;        // keydata中有效数据的长度
   byte              keydata[1280]; // 可能是明文，可能是密文，密文需先解密，然后
                                    // 根据keytype的类型转换为aes/rsa_pub/rsa_prv类型。
} tks_key;

typedef struct opt_st {
   uint16_t   flags; // 0x1:   可以被重设user_pass
                     // 0x2:   可以获取密钥信息
                     // 0x4:   可以被删除
                     // 0x8:   删除时是否需要userpass验证
                     // 0x10:  是否使用lock limit功能
                     // 0x20:  是否需要userpass授权使用
                     // 0x40:  是否通知密钥被锁定(暂不实现)
                     // 0x80:  是否为临时密钥(临时密钥仅在内存中使用，不保存)
                     // 0x100: 是否可以加解密
                     // 0x200: 是否可以签名验签
   int       locklimit;// 如果连续user_pass错误达到上限，则锁定密钥
   struct tm timelimit;// 密钥过期后，密钥不可用
} opt;//64bit system:14B,32bit system:10B

typedef struct key_info_st{
   struct tm  timestored;   // 存入时间
   struct tm  timeaccessed; // 最近访问时间
   struct tm  timefailed;   // 最近失败时间
   int        timesfailed;  // 最近失败次数，成功后清零
   int        flags;        // 0x01=密钥被锁定
// int rev[27];             // 保留字段
} key_info;//64bit system:32B,32bit system:20B

typedef struct tks_cert_st{
   rsa_pub    pubkey;
   byte       devid[15];      // device unique uuid,IMEI号15B
   byte       signature[256]; // signature of devid
} tks_cert;//RSA 1024:275B,RSA 2048:531B

typedef struct  sys_info_st {
   struct tm  timeinit;     // 出厂后首次启动的时间
   struct tm  timeaccessed; // 最近访问时间
   struct tm  timefailed;   // 最近失败时间
   int        timesfailed;  // 最近失败次数，成功后清零
   int        codefailed;   // 最近失败的代号（包含api）,
                            // api num=codefailed&0xff,api按此文档顺序编号。
                            // 失败代号= codefailed>>8;
   char       keyname[30];  // 最近失败的密钥名称 30B
   tks_cert   devcert;      // 设备公钥证书
} sys_info;//64bit system+RSA2048:62+531B,32bit system+RSA 1024=50+275

typedef struct pack_data_st{
   int    len;  // data中的有效数据长度
   byte  *data; // 明文经user_pass加密之后的密文
} packdata;

typedef struct  big_int_st{
    int    len;
    byte  *data;
} big_int;

typedef struct  dh_param_st{
   big_int a;
   big_int g;
   big_int p;
} dh_param;

// For SEC Driver
typedef struct request_gen_keypair_st{
    tks_key rsaprv; //[out]
    tks_key out_rsapub; //[out]
} request_gen_keypair;

typedef struct request_dec_simple_st{
    tks_key encryptkey; //[in]
    packdata data; //[in/out]
} request_dec_simple;

typedef struct request_dec_pack_userpass_st{
    tks_key encryptkey;//[in]
    tks_key userpass; //[in/out]
} request_dec_pack_userpass;

typedef struct request_crypto_st {
    uint8_t   op;          /* 0=SIGN,1=VERIFY,2=ENC,3=DEC,4=ERR NOTIFY; [in] */
    tks_key   encryptkey;  /* [in] */
    tks_key   userpass;    /* encryptkey userpass;[in] */
    packdata  data;        /* [in/out] */
    packdata  signature;   /* [in] only used for op==VERIFY */
} request_crypto;

typedef struct request_gen_dhkey_st {
    tks_key  encryptkey; //[in]
    dh_param  param; //dh算法参数（a，g，p）被encryptkey加密的密文//[in]
    big_int b; //tks 输出，用于调用者产生密钥//[out]
    big_int k;//tks生成的DH key，被master key加密过,保存至RPMB;//[out]
} request_gen_dhkey;

typedef struct request_hmac_st {
    packdata message; //[in(4090-4)/out(32)]
} request_hmac;

// For RPMB Driver
typedef struct rpmb_keycontainer_st{
    opt option;
    key_info keyinfo;
    tks_key key;
} rpmb_keycontainer;

/**--------------------------------------------------------------------------*
 **                         FUNCTION DEFINITION                              *
 **--------------------------------------------------------------------------*/
int tks_gen_keypair(char *name, char *encryptoname, tks_key user_pass, opt option,
                    tks_key *rsapub);

int tks_add_key(char *name, char *encryptoname, tks_key user_pass, tks_key certi,
                opt option);

int tks_del_key(char *name, char *encryptoname, tks_key user_pass);

int tks_get_keyinfo(char *name, key_info *info);

int tks_get_sysinfo(int flags, sys_info *einfo);

int tks_rst_userpass(char *name, char *encryptoname, tks_key old_pass, tks_key new_pass);

int tks_sign(char *name, packdata data, packdata *signature);

int tks_verify(char *name, packdata data, packdata signature);

int tks_encrypt(char *name, packdata data, packdata *dataencypted);

int tks_decrypt(char *name, packdata dataencrypted, packdata *data);

int tks_add_dhkey(char *name, char *encryptoname, tks_key user_pass,
                  dh_param param, opt option, big_int *b);

#endif
