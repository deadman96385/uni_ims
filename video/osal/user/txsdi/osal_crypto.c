/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal_crypto.h>

#ifdef OSAL_CRYPTO_ENABLE

#include <osal_mem.h>
#include <cryptoApi.h>

/*
 * to wrap around many popular crypto functions in openssl or isc lib (DNS BIND)
 * sample sha256 functions in openssl
 *
int SHA256_Init(SHA256_CTX *c);
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA256_Final(unsigned char *md, SHA256_CTX *c);
unsigned char *SHA256(const unsigned char *d, size_t n,unsigned char *md);

 * and similar functions in isc lib
 *
void
isc_hmacsha256_init(isc_hmacsha256_t *ctx, const unsigned char *key,
            unsigned int len);
void
isc_hmacsha256_invalidate(isc_hmacsha256_t *ctx);
void
isc_hmacsha256_update(isc_hmacsha256_t *ctx, const unsigned char *buf,
              unsigned int len);
void
isc_hmacsha256_sign(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len);
isc_boolean_t
isc_hmacsha256_verify(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len);
 *
 */

/* OSAL_CryptoCtxId would be just an opaque pointer to the OSAL_CryptoCtx */
typedef struct {
    OSAL_CryptoMdAlgo hmac_alg;
    union {
        psDigestContext_t sha256;
        psHmacContext_t   hmac;
    } u;
} OSAL_CryptoCtx;

#define _CRYPTO_SHA256_CTX(ctxId)  (((OSAL_CryptoCtx *)ctxId)->u.sha256)
#define _CRYPTO_SHA256_CTX_PTR(ctxId)  (&((OSAL_CryptoCtx *)ctxId)->u.sha256)

#define _CRYPTO_HMAC_CTX(ctxId)  (((OSAL_CryptoCtx *)ctxId)->u.hmac)
#define _CRYPTO_HMAC_CTX_PTR(ctxId)  (&((OSAL_CryptoCtx *)ctxId)->u.hmac)

#define _CRYPTO_HMAC_ALG(ctxId) (((OSAL_CryptoCtx *)ctxId)->hmac_alg)

/*
 * ======== OSAL_cryptoAllocCtx() ========
 * allocate the context buffer and return as generic ctxId
 *
 * Returns:
 * ctxId: An opaque context id point to the context buffer.
 * NULL: The allocation failed.
 */
OSAL_CryptoCtxId OSAL_cryptoAllocCtx(void)
{
    OSAL_CryptoCtx *ctx_ptr;

    ctx_ptr = OSAL_memAlloc(sizeof(OSAL_CryptoCtx), 0);
    return ((OSAL_CryptoCtxId)ctx_ptr);
}

/*
 * ======== OSAL_cryptoFreeCtx() ========
 * Free the context buffer pointed by the ctx id
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoFreeCtx(
        OSAL_CryptoCtxId ctxId)
{
    return OSAL_memFree(ctxId, 0);
}

/*
 * ======== OSAL_cryptoSha256Init() ========
 * Init the context buffer pointed by the ctx id
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoSha256Init(
        OSAL_CryptoCtxId ctx)
{
    if (NULL == ctx) {
        return (OSAL_FAIL);
    }
    psSha256Init(_CRYPTO_SHA256_CTX_PTR(ctx));
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoSha256Update() ========
 * update the digest in the context with the passed in data
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoSha256Update(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t           len)
{
    if ((NULL == ctx) || (NULL == data)) {
        return (OSAL_FAIL);
    }
    psSha256Update(_CRYPTO_SHA256_CTX_PTR(ctx), data, len);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoSha256Final() ========
 * finalize the digest in the context and put result in the passed in buffer md
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoSha256Final(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md)
{
    int result;

    if ((NULL == ctx) || (NULL == md)) {
        return (OSAL_FAIL);
    }
    result = psSha256Final(_CRYPTO_SHA256_CTX_PTR(ctx), md);
    if (SHA256_HASH_SIZE == result) {
        return (OSAL_SUCCESS);
    } else {
        return (OSAL_FAIL);
    }
}

/*
 * ======== OSAL_cryptoSha256() ========
 * convenient function to do a quick sha256 digest.
 *
 * Returns:
 * buffer pointer: the digest result, either the passed in buffer or the shared
 * buffer
 */
unsigned char *OSAL_cryptoSha256(
        const unsigned char *d,
        size_t               n,
        unsigned char       *md)
{
    psDigestContext_t c;
    if ((NULL == d) || (NULL == md)) {
        return (NULL);
    }
    psSha256Init(&c);
    psSha256Update(&c, d, n);
    psSha256Final(&c, md);
    return (md);
}

/* ========== HMAC functions using specified md algo ============== */

#if 0
/*
 * ======== EVP_MD *_OSAL_cryptoGetEvpMd ========
 * Init the context buffer pointed by the ctx id
 *
 * Returns:
 * open ssl EVP_MD algorithm pointer, null if no matched algo
 */
EVP_MD *_OSAL_cryptoGetEvpMd(
        OSAL_CryptoMdAlgo mdAlgo)
{
    switch (mdAlgo) {
    case OSAL_CRYPTO_MD_ALGO_SHA256:
        return ((EVP_MD *)EVP_sha256());
    case OSAL_CRYPTO_MD_ALGO_MD5:
        return ((EVP_MD *)EVP_md5());
    default:
        return NULL;
        break;
    }
}
#endif

/*
 * ======== OSAL_cryptoHmacInit() ========
 * Init the context buffer pointed by the ctx id
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoHmacInit(
        OSAL_CryptoCtxId  ctx,
        const void       *key,
        size_t            keyLen,
        OSAL_CryptoMdAlgo mdAlgo)
{

    if ((NULL == ctx) || (NULL == key)) {
        return (OSAL_FAIL);
    }
    /* save the algorithm for later use */
    _CRYPTO_HMAC_ALG(ctx) = mdAlgo;
    switch (mdAlgo) {
        case OSAL_CRYPTO_MD_ALGO_SHA256:
            psHmacSha2Init(_CRYPTO_HMAC_CTX_PTR(ctx), key, keyLen, SHA256_HASH_SIZE);
            break;
        case OSAL_CRYPTO_MD_ALGO_MD5:
            psHmacMd5Init(_CRYPTO_HMAC_CTX_PTR(ctx), key, keyLen);
            break;
        default:
            return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoHmacUpdate() ========
 * update the digest in the context with the passed in data
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoHmacUpdate(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t          len)
{
    if ((NULL == ctx) || (NULL == data)) {
        return (OSAL_FAIL);
    }
    switch (_CRYPTO_HMAC_ALG(ctx)) {
        case OSAL_CRYPTO_MD_ALGO_SHA256:
            psHmacSha2Update(_CRYPTO_HMAC_CTX_PTR(ctx), data, len, SHA256_HASH_SIZE);
            break;
        case OSAL_CRYPTO_MD_ALGO_MD5:
            psHmacMd5Update(_CRYPTO_HMAC_CTX_PTR(ctx), data, len);
            break;
        default:
            return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoHmacFinal() ========
 * finalize the digest in the context and put result in the passed in buffer md
 *
 * Returns:
 * OSAL_SUCCESS: The buffer was successfully free.
 * OSAL_FAIL: The buffer failed to free.
 */
OSAL_Status OSAL_cryptoHmacFinal(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md,
        size_t          *mdLen_ptr)
{
    if ((NULL == ctx) || (NULL == md) || (NULL == mdLen_ptr)) {
        return (OSAL_FAIL);
    }
    switch (_CRYPTO_HMAC_ALG(ctx)) {
        case OSAL_CRYPTO_MD_ALGO_SHA256:
            *mdLen_ptr = psHmacSha2Final(_CRYPTO_HMAC_CTX_PTR(ctx), md, SHA256_HASH_SIZE);
            break;
        case OSAL_CRYPTO_MD_ALGO_MD5:
            *mdLen_ptr = psHmacMd5Final(_CRYPTO_HMAC_CTX_PTR(ctx), md);
            break;
        default:
            return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoHmacCleanup() ========
 * Function to erase the key and other data and releases any associated
 * resources.
 *
 * Return:
 *  OSAL_SUCCESS: Exit normally.
 *  OSAL_FAIL: Invalid arguments.
 */
OSAL_Status OSAL_cryptoHmacCleanup(
        OSAL_CryptoCtxId ctx)
{
    if (NULL == ctx) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_cryptoHmac() ========
 * convenient function to do a quick sha256 digest.
 *
 * Returns:
 * buffer pointer: the digest result, either the passed in buffer or the shared
 * buffer
 */
unsigned char *OSAL_cryptoHmac(
        const void          *key,
        size_t               keyLen,
        OSAL_CryptoMdAlgo    mdAlgo,
        const unsigned char *d,
        size_t               n,
        unsigned char       *md,
        size_t              *mdLen_ptr)
{
    unsigned char       hmacKey[SHA256_HASH_SIZE];
    uint32              hmacKeyLen;

    if ((NULL == key) || (NULL == d) || (NULL == md) || (NULL == mdLen_ptr)) {
        return (NULL);
    }

    switch (mdAlgo) {
        case OSAL_CRYPTO_MD_ALGO_SHA256:
            *mdLen_ptr = psHmacSha2(key, keyLen, d, n, md, hmacKey, &hmacKeyLen, SHA256_HASH_SIZE);
            break;
        case OSAL_CRYPTO_MD_ALGO_MD5:
            *mdLen_ptr = psHmacMd5(key, keyLen, d, n, md, hmacKey, &hmacKeyLen);
            break;
        default:
            return (NULL);
    }
    return (md);
}

/*
 * ======== OSAL_cryptoB64Encode() ========
 * Encodes a buffer to Base64.
 * target buffer should be large enough to hold the result, safe guess=size*2
 * exact length is size*4/3 + padding
 *
 * Returns:
 * resulted length of the encoded string
 */
int OSAL_cryptoB64Encode(
        char *s,
        char *t,
        int   size)
{
    int l;
    l = psEVP_EncodeBlock((unsigned char *)t, (const unsigned char *)s, size);
    return (l);
}

/*
 * ======== OSAL_cryptoB64Decode() ========
 * Decodes a Base64 string to a buffer.
 * target buffer should be large enough to hold the result, safe guess=size
 * exact length is about 3/4 of original buffer minus the padding.
 * also assumed input string won't have line longer than 64
 *
 * Returns:
 * resulted length of the decoded buffer
 */
int OSAL_cryptoB64Decode(
        char *s,
        int   size,
        char *t)
{
    int l;
    l = psEVP_DecodeBlock((unsigned char *)t, (const unsigned char *)s, size);
    /*
     * The return length includes processing of '=',
     * need to minutes the length of '='.
     */
    while ('=' == s[--size]) {
        l--;
    }
    return (l);
}

#else // not define OSAL_CRYPTO_ENABLE

/* OSAL_ENABLE_CRYPTO is not defined, put empty routines */
OSAL_CryptoCtxId OSAL_cryptoAllocCtx(void)
{
    return (NULL);
}

OSAL_Status OSAL_cryptoFreeCtx(
        OSAL_CryptoCtxId ctxId)
{
    return (OSAL_FAIL);
}

/* SHA256 algo */
OSAL_Status OSAL_cryptoSha256Init(
        OSAL_CryptoCtxId ctx)
{
    return (OSAL_FAIL);
}

OSAL_Status OSAL_cryptoSha256Update(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t           len)
{
    return (OSAL_FAIL);
}

OSAL_Status OSAL_cryptoSha256Final(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md)
{
    return (OSAL_FAIL);
}

unsigned char *OSAL_cryptoSha256(
        const unsigned char *d,
        size_t               n,
        unsigned char       *md)
{
    return (NULL);
}

/* HMAC functions using specified md algo */
OSAL_Status OSAL_cryptoHmacInit(
        OSAL_CryptoCtxId  ctx,
        const void       *key,
        size_t            keyLen,
        OSAL_CryptoMdAlgo mdAlgo)
{
    return (OSAL_FAIL);
}

OSAL_Status OSAL_cryptoHmacUpdate(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t           len)
{
    return (OSAL_FAIL);
}

OSAL_Status OSAL_cryptoHmacFinal(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md,
        size_t          *mdLen_ptr)
{
    return (OSAL_FAIL);
}

OSAL_Status OSAL_cryptoHmacCleanup(
        OSAL_CryptoCtxId ctx)
{
    return (OSAL_FAIL);
}

unsigned char *OSAL_cryptoHmac(
        const void          *key,
        size_t               keyLen,
        OSAL_CryptoMdAlgo    mdAlgo,
        const unsigned char *d,
        size_t               n,
        unsigned char       *md,
        size_t              *mdLen_ptr)
{
    return (NULL);
}

/* base64 encode/decode */
int OSAL_cryptoB64Encode(
        char *s,
        char *t,
        int   size)
{
    return (0);
}

int OSAL_cryptoB64Decode(
        char *s,
        int   size,
        char *t)
{
    return (0);
}

#endif // OSAL_CRYPTO_ENABLE
