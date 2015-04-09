/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: $ $Date: $
 *
 * Name:        MD5 calulator  
 *
 * File:        auth_digcalc.c
 *
 * Description: helper functions to aid in calulating values when performing Md5 hash
 *
 * Author:      
 */

#include "auth_port.h"
#include "auth_md5.h"
#include "auth_digcalc.h"

/* ABNF core set of rules */
#define ABNF_ISALPHA_LC(c) ( ((c)>='a')&&((c)<='z') )
#define LOW2UP_CASE(c)     ( ABNF_ISALPHA_LC(c)? ('A' + ((c) - 'a')): (c) )

int auth_stricmp(const char *a, const char *b)
{
    while (*a)
    {
        if (LOW2UP_CASE(*a) != LOW2UP_CASE(*b))
        {
            return ((int)(*a) - (int)(*b));
        } 
        else
        {
            a++;
            b++;
        } 
    }

    return (*b ? -1 : 0);
}


static void CvtHex(
    HASH Bin,
    HASHHEX Hex
    )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
};

/* calculate H(A1) as per spec */
void DigestCalcHA1(
    char * pszAlg,
    char * pszUserName,
    char * pszRealm,
    char * pszPassword,
    int    pwdLen,
    char * pszNonce,
    char * pszCNonce,
    HASHHEX SessionKey
    )
{
      MD5_CTX Md5Ctx;
      HASH HA1;

      MD5Init(&Md5Ctx);
      MD5Update(&Md5Ctx, (uint8*)pszUserName, strlen(pszUserName));
      MD5Update(&Md5Ctx, (uint8*)":", 1);
      MD5Update(&Md5Ctx, (uint8*)pszRealm, strlen(pszRealm));
      MD5Update(&Md5Ctx, (uint8*)":", 1);
      MD5Update(&Md5Ctx, (uint8*)pszPassword, pwdLen);
      MD5Final((uint8*)HA1, &Md5Ctx);
      if (auth_stricmp(pszAlg, "md5-sess") == 0) {
            MD5Init(&Md5Ctx);
            MD5Update(&Md5Ctx, (uint8*)HA1, HASHLEN);
            MD5Update(&Md5Ctx, (uint8*)":", 1);
            MD5Update(&Md5Ctx, (uint8*)pszNonce, strlen(pszNonce));
            MD5Update(&Md5Ctx, (uint8*)":", 1);
            MD5Update(&Md5Ctx, (uint8*)pszCNonce, strlen(pszCNonce));
            MD5Final((uint8*)HA1, &Md5Ctx);
      };
      CvtHex(HA1, SessionKey);
};

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    HASHHEX HA1,           /* H(A1) */
    char * pszNonce,       /* nonce from server */
    char * pszNonceCount,  /* 8 hex digits */
    char * pszCNonce,      /* client nonce */
    char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    char * pszMethod,      /* method from the request */
    char * pszDigestUri,   /* requested URL */
    HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    HASHHEX Response      /* request-digest or response-digest */
    )
{
      MD5_CTX Md5Ctx;
      HASH HA2;
      HASH RespHash;
      HASHHEX HA2Hex;

      /* calculate H(A2) */
      MD5Init(&Md5Ctx);
      MD5Update(&Md5Ctx, (uint8*)pszMethod, strlen(pszMethod));
      MD5Update(&Md5Ctx, (uint8*)":", 1);
      MD5Update(&Md5Ctx, (uint8*)pszDigestUri, strlen(pszDigestUri));
      if (auth_stricmp(pszQop, "auth-int") == 0) {
            MD5Update(&Md5Ctx, (uint8*)":", 1);
            MD5Update(&Md5Ctx, (uint8*)HEntity, HASHHEXLEN);
      };
      MD5Final((uint8*)HA2, &Md5Ctx);
      CvtHex(HA2, HA2Hex);

      /* calculate response */
      MD5Init(&Md5Ctx);
      MD5Update(&Md5Ctx, (uint8*)HA1, HASHHEXLEN);
      MD5Update(&Md5Ctx, (uint8*)":", 1);
      MD5Update(&Md5Ctx, (uint8*)pszNonce, strlen(pszNonce));
      MD5Update(&Md5Ctx, (uint8*)":", 1);
      if (*pszQop) {
          MD5Update(&Md5Ctx, (uint8*)pszNonceCount, strlen(pszNonceCount));
          MD5Update(&Md5Ctx, (uint8*)":", 1);
          MD5Update(&Md5Ctx, (uint8*)pszCNonce, strlen(pszCNonce));
          MD5Update(&Md5Ctx, (uint8*)":", 1);
          MD5Update(&Md5Ctx, (uint8*)pszQop, strlen(pszQop));
          MD5Update(&Md5Ctx, (uint8*)":", 1);
      };
      MD5Update(&Md5Ctx, (uint8*)HA2Hex, HASHHEXLEN);
      MD5Final((uint8*)RespHash, &Md5Ctx);
      CvtHex(RespHash, Response);
};

