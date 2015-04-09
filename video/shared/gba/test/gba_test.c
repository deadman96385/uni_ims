/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#include <osal.h>
#include <gba.h>
#include <osal_crypto.h>
#include <gaa.h>
#include <iemu.h>


#define _MY_MAX_BODY_SZ     (4096)
#define _MY_MAX_HEADER_SZ   1024
#define _MY_STRING_SZ       (64)

typedef struct {
    char bsf[_MY_STRING_SZ];
    char naf[_MY_STRING_SZ];
    char nafFqdn[_MY_STRING_SZ];
    char impi[_MY_STRING_SZ];
    char simki[_MY_STRING_SZ]; /*  should be 32 chars */
} MyAppObj;

MyAppObj myAppObj={
    "http://bsf.labs.ericsson.net:8080/bsfv2/bsf",
    "http://naf.labs.ericsson.net:8080/gbanafv2",
    "naf.labs.ericsson.net",
    "tomyjwu@labs.ericsson.net",
    "658e91bd575fc190b983b178ef7eef65"
};

GBA_NetAppObj myServiceApp = {
    "gbanafv2",
    "http://naf.labs.ericsson.net:8080/gbanafv2",
    GBA_NET_APP_AUTHTYPE_GAA_DIGEST,
    "admin", /*  not used */
    "passwd" /*  not used */
};

GBA_NetAppObj myAutoNetApp = {
    "gbanafv2",
    "http://naf.labs.ericsson.net:8080/gbanafv2",
    GBA_NET_APP_AUTHTYPE_AUTO,
    "admin", /*  not used */
    "passwd" /*  not used */
};

void showUsage(void)
{
    OSAL_logMsg("Testing GBA bootstrapping \n");
    OSAL_logMsg("Usage   : gba_test [Options]\n");
    OSAL_logMsg("Options :\n");
    OSAL_logMsg(
            " --bsf=bsfurl          bsf url in http://xxx:0000/yyy format\n");
    OSAL_logMsg(
            " --naf=nafurl          naf url in http://xxx:0000/yyy format\n");
    OSAL_logMsg(
            " --simki=key-hexstr    sim card secrete key for AKA\n");
    OSAL_logMsg(
            " --impi=impi-str       bsf registered user impi\n");
    OSAL_logMsg(
            " --help                print this help.\n");
    OSAL_logMsg("\n");
    return;
}

static int myAppInit()
{
    /*  init gba */

    return 0;
}

int gba_test(int argc, char **argv)
{
    int     OptionIndex;
    GAA_NafSession *appSession_ptr;

    myAppInit(&myAppObj);

    OptionIndex = 0;
    if (argc > 1) {

        /*  parsing options */
        while (OptionIndex < (argc)) {
            if (strncmp(argv[OptionIndex], "--bsf=", 6) == 0) {
                OSAL_snprintf(myAppObj.bsf, _MY_STRING_SZ, "%s", &argv[OptionIndex][6]);
            }
            if (strncmp(argv[OptionIndex], "--nsf=", 6) == 0) {
                OSAL_snprintf(myAppObj.naf, _MY_STRING_SZ, "%s", &argv[OptionIndex][6]);
                OSAL_strcpy(myServiceApp.appUri, myAppObj.naf);

            }
            if (strncmp(argv[OptionIndex], "--impi=", 7) == 0) {
                OSAL_snprintf(myAppObj.impi, _MY_STRING_SZ, "%s", &argv[OptionIndex][7]);
            }
            if (strncmp(argv[OptionIndex], "--simki=", 8) == 0) {
                OSAL_snprintf(myAppObj.simki, _MY_STRING_SZ, "%s", &argv[OptionIndex][8]);
            }
            if ((strcmp(argv[OptionIndex], "--help") == 0) ||
              (strcmp(argv[OptionIndex], "/?") == 0)) {
                showUsage();
                return 0;
            }
            OptionIndex++;
        }
    }

    IEMU_init();
    IEMU_credSetup(myAppObj.impi, myAppObj.simki);

    GBA_init();
    GBA_setup(myAppObj.bsf, myAppObj.impi);

    /*  do it */
    GBA_bootstrape();
    OSAL_logMsg("\nDone bootstrapping\n");
    GBA_registerNetApp(&myServiceApp);
    appSession_ptr = GAA_newNafSession(myServiceApp.appName);

    /*  do the action now */
    GAA_startNafSession(appSession_ptr);

    /* 
     * in the app, could reuse the http obj to do other http requests 
     * using the http lib api
     */

    /*  done with it. */
    GAA_stopNafSession(appSession_ptr);
    GAA_freeNafSession(appSession_ptr);

    return 0;
}


int gba_autoDetectionTest(int argc, char **argv)
{
    int     OptionIndex;
    GAA_NafSession *appSession_ptr;

    myAppInit(&myAppObj);

    OptionIndex = 0;
    if (argc > 1) {

        /*  parsing options */
        while (OptionIndex < (argc)) {
            if (strncmp(argv[OptionIndex], "--bsf=", 6) == 0) {
                OSAL_snprintf(myAppObj.bsf, _MY_STRING_SZ, "%s", &argv[OptionIndex][6]);
            }
            if (strncmp(argv[OptionIndex], "--nsf=", 6) == 0) {
                OSAL_snprintf(myAppObj.naf, _MY_STRING_SZ, "%s", &argv[OptionIndex][6]);
                OSAL_strcpy(myAutoNetApp.appUri, myAppObj.naf);

            }
            if (strncmp(argv[OptionIndex], "--impi=", 7) == 0) {
                OSAL_snprintf(myAppObj.impi, _MY_STRING_SZ, "%s", &argv[OptionIndex][7]);
            }
            if (strncmp(argv[OptionIndex], "--simki=", 8) == 0) {
                OSAL_snprintf(myAppObj.simki, _MY_STRING_SZ, "%s", &argv[OptionIndex][8]);
            }
            if ((strcmp(argv[OptionIndex], "--help") == 0) ||
              (strcmp(argv[OptionIndex], "/?") == 0)) {
                showUsage();
                return 0;
            }
            OptionIndex++;
        }
    }

    IEMU_init();
    IEMU_credSetup(myAppObj.impi, myAppObj.simki);

    GBA_init();
    GBA_setup(myAppObj.bsf, myAppObj.impi);

    /*  test auto detect */
    GBA_registerNetApp(&myAutoNetApp);

    appSession_ptr = GAA_newNafSession(myAutoNetApp.appName);

    /*  do the action now */
    /*  GAA_detectAuthType(); will be called because we set GBA_NET_APP_AUTHTYPE_AUTO */
    GAA_startNafSession(appSession_ptr);

    /* 
     * in the app, could reuse the http obj to do other http requests 
     * using the http lib api
     */
    
    /*  done with it. */
    GAA_stopNafSession(appSession_ptr);
    GAA_freeNafSession(appSession_ptr);

    return 0;
}


void
hexdump(buf, len)
    const void *buf;
    int len;
{
    int i;

    for (i = 0; i < len; i++) {
        if (i != 0 && i % 32 == 0) OSAL_logMsg("\n");
        if (i % 4 == 0) OSAL_logMsg(" ");
        OSAL_logMsg("%02x", ((const unsigned char *)buf)[i]);
    }
#if 0
    if (i % 32 != 0) OSAL_logMsg("\n");
#endif

    return;
}

char *sha256(const char *str, size_t len)
{
    unsigned char hash[OSAL_CRYPTO_SHA256_DIGEST_LENGTH];

    OSAL_CryptoCtxId ctxId;

    ctxId = OSAL_cryptoAllocCtx();
    OSAL_cryptoSha256Init(ctxId);
    OSAL_cryptoSha256Update(ctxId, str, len);
    OSAL_cryptoSha256Final(ctxId, hash);
    OSAL_cryptoFreeCtx(ctxId);
    OSAL_logMsg("\nsha256 of %s\n", str);
    hexdump(hash, OSAL_CRYPTO_SHA256_DIGEST_LENGTH);

    return ((char *)str);
}


int crypto_test(int argc, char **argv)
{
    /*
    9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08
    60303ae22b998861bce3b28f33eec1be758a213c86c93c076dbe9f558c11c752
    */
    sha256("test", 4);
    sha256("test2", 5);

    OSAL_logMsg("\nsha256 of test\n");
    hexdump(OSAL_cryptoSha256((unsigned const char *)"test", 4, NULL),
            OSAL_CRYPTO_SHA256_DIGEST_LENGTH);

    OSAL_logMsg("\nsha256 of test2\n");
    hexdump(OSAL_cryptoSha256((unsigned const char *)"test2", 5, NULL),
            OSAL_CRYPTO_SHA256_DIGEST_LENGTH);

    return 0;
}

int HMAC_test(int argc, char **argv)
{
    unsigned char* key = (unsigned char*) "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b";
    unsigned char* data = (unsigned char*) "\x48\x69\x20\x54\x68\x65\x72\x65";
    unsigned char* expected = (unsigned char*) "\x49\x2c\xe0\x20\xfe\x25\x34\xa5\x78\x9d\xc3\x84\x88\x06\xc7\x8f\x4f\x67\x11\x39\x7f\x08\xe7\xe7\xa1\x2c\xa5\xa4\x48\x3c\x8a\xa6";
    unsigned char* result;
    unsigned int result_len = 32;
    unsigned int i;
    OSAL_CryptoCtxId ctxId;

    result = (unsigned char*) malloc(sizeof(char) * result_len);
    ctxId = OSAL_cryptoAllocCtx();
    OSAL_cryptoHmacInit(ctxId, key,
            16,
            OSAL_CRYPTO_MD_ALGO_SHA256);
    OSAL_cryptoHmacUpdate(ctxId, data, 8);
    OSAL_cryptoHmacFinal(ctxId, result, NULL);
    OSAL_cryptoFreeCtx(ctxId);

    for (i=0; i!=result_len; i++)
    {
        if (expected[i]!=result[i])
        {
            OSAL_logMsg("Got %02X instead of %02X at byte %d!\n", result[i], expected[i], i);
            break;
        }
    }
    if (i==result_len)
    {
        OSAL_logMsg("Test ok!\n");
    }
    return 0;
}

int ksnaf_test(int argc, char **argv)
{
    myAppInit(&myAppObj);

    IEMU_init();
    IEMU_credSetup(myAppObj.impi, myAppObj.simki);

    GBA_init();
    GBA_setup(myAppObj.bsf, myAppObj.impi);

    /*  do it */
    GBA_bootstrape();

    OSAL_logMsg("\nDone bootstrapping\n");

    GBA_registerNetApp(&myServiceApp);

    GAA_newNafSession(myServiceApp.appName);

    /*  print ksnaf calculation */
    return 0;
}

int b64_test(int argc, char **argv)
{
    /* User-ID:Password string  = "Aladdin:open sesame"                          */
    /* Base64 encoded   string  = "QWxhZGRpbjpvcGVuIHNlc2FtZQ=="                 */
    char srcStr[] = "Aladdin:open sesame";
    char dstStr[32];
    char expectedStr[] = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
    int resLen;

    resLen = OSAL_cryptoB64Encode(srcStr, dstStr, OSAL_strlen(srcStr)); /* Encodes a buffer to Base64. */
    /* hack the trailing new line. we dont' need that for testing */
    if ('\n' == dstStr[resLen-1]) {
        dstStr[--resLen] = '\0';
    }
    if ( (0 != OSAL_memCmp(dstStr, expectedStr, OSAL_strlen(expectedStr))) ||
            (resLen != OSAL_strlen(expectedStr))) {
        OSAL_logMsg("Test Failed Base64 encode :src:%s dst:%s, resLen=%d\n", srcStr, dstStr, resLen);
    }
    else {
        OSAL_logMsg("Test ok Base64 encode!\n");
    }

    resLen = OSAL_cryptoB64Decode(expectedStr, OSAL_strlen(expectedStr), dstStr);
    if ( (0 != OSAL_memCmp(dstStr, srcStr, OSAL_strlen(srcStr))) ||
            (resLen != OSAL_strlen(srcStr))) {
        OSAL_logMsg("Test Failed Base64 decode :from:%s decoded:%s resLen=%d\n", expectedStr, dstStr, resLen);
    }
    else {
        OSAL_logMsg("Test ok Base64 decode!\n");
    }

    return 0;
}

int main(int argc, char **argv)
{
    /*  gba_test(argc, argv); */
    /*  gba_autoDetectionTest(argc, argv); */
    /*  crypto_test(argc, argv); */
    /*  ksnaf_test(argc, argv); */
    /*  HMAC_test(argc, argv); */
    b64_test(argc, argv);

    return 0;
}


