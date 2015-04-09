/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#include "osal_ut.h"

 /*
 * ========= _OSALUT_cryptoCmp() ========
 * Function to dump string buf in hex
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_cryptoCmp(
    unsigned char *result,
    unsigned char *expected,
    int            len)
{
    int i;
    int error = 0;

    if ((result == NULL) || (expected== NULL)) {
        return (UT_FAIL);
    }

    for (i = 0; i < len; i++) {
        if (expected[i] != result[i]) {
            error++;
         }
    }

    OSAL_logMsg("Computed message digest: \n");
    for (i = 0; i < len; i++) {
        OSAL_logMsg("%02x", result[i]);
    }
    OSAL_logMsg("\n");

    if (error) {
        return (UT_FAIL);
    }
    else {
        return (UT_PASS);
    }
}

/*
 * ========= _OSALUT_cryptoSha256Test() ========
 * Function to do sha256 test
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_cryptoSha256Test(
    const void    *data,
    size_t         dataLen,
    unsigned char *md)
{
    OSAL_CryptoCtxId ctxId;

    ctxId = OSAL_cryptoAllocCtx();
    if (OSAL_SUCCESS != OSAL_cryptoSha256Init(ctxId)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_cryptoSha256Update(ctxId, data, dataLen)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_cryptoSha256Final(ctxId, md)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=OSAL_cryptoFreeCtx(ctxId)) {
        return (UT_FAIL);
    }
    OSAL_logMsg("SHA-256 of %s\n", (char *) data);

    return (UT_PASS);
}

/*
 * ========= _OSALUT_cryptoHmacTest() ========
 * Function to do hmac test
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_cryptoHmacTest(
    const void        *key,
    size_t             keyLen,
    OSAL_CryptoMdAlgo  mdAlgo,
    const void        *data,
    size_t             dataLen,
    unsigned char     *md,
    size_t            *mdLen_ptr)
{
    OSAL_CryptoCtxId ctxId;

    ctxId = OSAL_cryptoAllocCtx();
    if (OSAL_SUCCESS != OSAL_cryptoHmacInit(ctxId, key, keyLen, mdAlgo)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_cryptoHmacUpdate(ctxId, data, dataLen)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=  OSAL_cryptoHmacFinal(ctxId, md, mdLen_ptr)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_cryptoHmacCleanup(ctxId)) {
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=OSAL_cryptoFreeCtx(ctxId)) {
        return (UT_FAIL);
    }
    OSAL_logMsg("HMAC of %s\n", (char *) data);

    return (UT_PASS);
}

/*
 * ========= do_test_crypto() ========
 * Gen unit test vectors for each OSAL crypto function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_crypto(
    void)
{
    OSAL_CryptoCtxId ctxId;
    const char data[] = "crypto";
    unsigned char* sha256Hash = (unsigned char*) "\xda\x2f\x07\x3e\x06\xf7\x89"
            "\x38\x16\x6f\x24\x72\x73\x72\x9d\xfe\x46\x5b\xf7\xe4\x61\x05\xc1"
            "\x3c\xe7\xcc\x65\x10\x47\xbf\x0c\xa4";
    unsigned char output[OSAL_CRYPTO_SHA256_DIGEST_LENGTH];
    const char key[] = "secret-a";
    unsigned int len;
    unsigned char* hmacSha256Hash = (unsigned char*) "\xe3\x0b\x6e\x2d\x99\x4a\xe7"
            "\x6b\xf4\x5d\x38\xcb\x9f\x41\x6d\x20\x96\xba\x27\x44\xc0\xcb\x9a"
            "\x4f\x44\x4a\xea\x88\x51\x24\x4f\xc5";
    unsigned char* hmacMd5Hash = (unsigned char*) "\x49\x83\x3d\x7e\x0b\x00\x24"
            "\x1c\xee\x12\xba\x21\x8a\x69\xed\x91";

    char b64SrcStr[] ="Aladdin:open sesame";
    char dstStr[32];
    char b64EncodedStr[] = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";

    OSAL_logMsg("Crypto Unit Test Starting...\n");
    /* reset this before every test */
    osal_utErrorsFound = 0;

    /*
     * Crypto test 1:
     * Generate sha256 message digest.
     */
    OSAL_logMsg("\nCrypto test 1\n");
    if (UT_PASS == _OSALUT_cryptoSha256Test(data, OSAL_strlen(data), output)) {
        if (UT_PASS != _OSALUT_cryptoCmp(output, sha256Hash,
                OSAL_CRYPTO_SHA256_DIGEST_LENGTH)) {
            prError("The SHA-256 hash isn't be computed as expected. \n");
        }
    }
    else {
        prError("The error occured in the process of SHA-256 generation. \n");
    }

    /*
     * Crypto test 2:
     * Compute HMAC with  sha256 message digest.
     */
    OSAL_logMsg("\nCrypto test 2\n");
    if (UT_PASS == _OSALUT_cryptoHmacTest(key, OSAL_strlen(key),
            OSAL_CRYPTO_MD_ALGO_SHA256, data, OSAL_strlen(data), output,
            &len)) {
        if (UT_PASS != _OSALUT_cryptoCmp(output, hmacSha256Hash, len)) {
            prError("The HMAC hash isn't be computed as expected. \n");
        }
    }
    else {
        prError("The error occured in the process of HMAC generation. \n");
    }

    /*
     * Crypto test 3:
     * error case: OSAL_cryptoSha256Init with NULL id.
     */
    OSAL_logMsg("\nCrypto test 3\n");
    ctxId = OSAL_cryptoAllocCtx();
    if (OSAL_FAIL != OSAL_cryptoSha256Init(NULL)) {
        prError("OSAL_cryptoSha256Init() didn't fail as expected.\n");
    }
    if (OSAL_SUCCESS !=OSAL_cryptoFreeCtx(ctxId)) {
        prError("OSAL_cryptoFreeCtx() didn't success as expected.\n");
    }

    /*
     * Crypto test 4:
     * error case: Generate sha256 message digest with wrong data length.
     */
    OSAL_logMsg("\nCrypto test 4\n");
    if (UT_PASS == _OSALUT_cryptoSha256Test(data, 0, output)) {
        if (UT_PASS == _OSALUT_cryptoCmp(output, sha256Hash,
                OSAL_CRYPTO_SHA256_DIGEST_LENGTH)) {
            prError("The SHA-25 hash isn't be computed as expected.\n");
        }
    }
    else {
        prError("The error occured in the process of SHA-256 generation.\n");
    }

    /*
     * Crypto test 5:
     * error case: Generate sha256 message digest with NULL data pointer.
     */
    OSAL_logMsg("\nCrypto test 5\n");
    if (UT_PASS == _OSALUT_cryptoSha256Test(NULL, OSAL_strlen(data), output)) {
        prError("The sucessful process of SHA-25 generation isn't as "
        "expected.\n");
    }

    /*
     * Crypto test 6:
     * error case: OSAL_cryptoSha256Init with NULL id.
     */
    OSAL_logMsg("\nCrypto test 6\n");
    ctxId = OSAL_cryptoAllocCtx();
    if (OSAL_FAIL !=  OSAL_cryptoHmacInit(NULL, key, OSAL_strlen(key),
                OSAL_CRYPTO_MD_ALGO_SHA256)) {
        prError("OSAL_cryptoHmacInit() didn't fail as expected.\n");
    }
    if (OSAL_SUCCESS !=OSAL_cryptoFreeCtx(ctxId)) {
        prError("OSAL_cryptoFreeCtx() didn't success as expected.\n");
    }

    /*
     * Crypto test 7:
     * error case: OSAL_cryptoHmacInit with invalid md algorithms.
     */
    OSAL_logMsg("\nCrypto test 7\n");
    ctxId = OSAL_cryptoAllocCtx();

    if (OSAL_FAIL !=  OSAL_cryptoHmacInit(ctxId, key, OSAL_strlen(key),
            OSAL_CRYPTO_MD_ALGO_NONE)) {
        prError("OSAL_cryptoHmacInit() didn't fail as expected.\n");
    }
    if (OSAL_SUCCESS !=OSAL_cryptoFreeCtx(ctxId)) {
        prError("OSAL_cryptoFreeCtx() didn't success as expected.\n");
    }

    /*
     * Crypto test 8:
     * error case: Compute HMAC with  sha256 message digest with wrong
     * key length.
     */
    OSAL_logMsg("\nCrypto test 8\n");
    if (UT_PASS == _OSALUT_cryptoHmacTest(key, 0,
            OSAL_CRYPTO_MD_ALGO_SHA256, data, OSAL_strlen(data), output,
            &len)) {
        if (UT_PASS == _OSALUT_cryptoCmp(output, hmacSha256Hash, len)) {
            prError("The SHA-25 hash isn't be computed as expected. \n");
        }
    }
    else {
        prError("The error occured in the process of HMAC generation. \n");
    }

    /*
     * Crypto test 9:
     * Generate sha256 message digest.
     */
    OSAL_logMsg("\nCrypto test 9\n");
    OSAL_logMsg("SHA-256 of %s\n", data);
    OSAL_cryptoSha256((const unsigned char *) data, OSAL_strlen(data),
            output);
    if (UT_PASS != _OSALUT_cryptoCmp(output, sha256Hash,
            OSAL_CRYPTO_SHA256_DIGEST_LENGTH)) {
        prError("The SHA-256 hash isn't be computed as expected. \n");
    }

    /*
     * Crypto test 10:
     * Compute HMAC with sha256 message digest.
     */
    OSAL_logMsg("\nCrypto test 10\n");
    OSAL_logMsg("HMAC_SHA256 of %s\n", data);
    OSAL_cryptoHmac(key, OSAL_strlen(key),
            OSAL_CRYPTO_MD_ALGO_SHA256, (const unsigned char *) data,
            OSAL_strlen(data), output, &len);
    if (UT_PASS != _OSALUT_cryptoCmp(output, hmacSha256Hash, len)) {
        prError("The HMAC hash isn't be computed as expected. \n");
    }


    /*
     * Crypto test 11:
     * Compute HMAC with MD5 message digest.
     */
    OSAL_logMsg("\nCrypto test 11\n");
    OSAL_logMsg("HMAC_MD5 of %s\n", data);
    OSAL_cryptoHmac(key, OSAL_strlen(key),
            OSAL_CRYPTO_MD_ALGO_MD5, (const unsigned char *) data,
            OSAL_strlen(data), output, &len);
    if (UT_PASS != _OSALUT_cryptoCmp(output, hmacMd5Hash, len)) {
        prError("The HMAC hash isn't be computed as expected. \n");
    }


    /*
     * Crypto test 12
     * Base64 encode test.
     */
    OSAL_logMsg("\nCrypto test 12\n");
    OSAL_logMsg("Base64 encode of %s\n", b64SrcStr);
    /* Encodes a buffer to Base64. */
    len = OSAL_cryptoB64Encode(b64SrcStr, dstStr, OSAL_strlen(b64SrcStr));
    if ((0 != OSAL_memCmp(dstStr, b64EncodedStr, OSAL_strlen(b64EncodedStr))) ||
            (len != OSAL_strlen(b64EncodedStr))) {
        prError("The Base64 encode is not computed as expected.\n");
    }

    /*
     * Crypto test 13
     * Base64 decode test.
     */
    OSAL_logMsg("\nCrypto test 13\n");
    OSAL_logMsg("Base64 decode of %s\n", b64EncodedStr);
    /* Encodes a buffer to Base64. */
    len = OSAL_cryptoB64Decode(b64EncodedStr, OSAL_strlen(b64EncodedStr), dstStr);
    if ((0 != OSAL_memCmp(dstStr, b64SrcStr, OSAL_strlen(b64SrcStr))) ||
            (len != OSAL_strlen(b64SrcStr))) {
        prError("The Base64 decode is not computed as expected.\n");
    }

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Crypto Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Crypto Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }


}
