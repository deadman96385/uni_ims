/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef __ABNFCORE_H__
#define __ABNFCORE_H__ 

#define ABNF_ISEOL_CRLF   1
#define ABNF_EOL          (ABNF_ISEOL_CRLF ? "\r\n": "\n")   

/* ABNF core set of rules */
#define ABNF_ISALPHA(c)    ( (((c)>='A')&&((c)<='Z')) || (((c)>='a')&&((c)<='z')) )
#define ABNF_ISALPHA_LC(c) ( ((c)>='a')&&((c)<='z') )
#define ABNF_ISBIT(c)      ( ((c)=='0')||((c)=='1') )
#define ABNF_ISCHAR(c)     ( ((c)>=0x01)&&((c)<=0x7F) )
#define ABNF_ISCR(c)       ( (c)==0x0D )
#define ABNF_ISCTL(c)      ( (((c)>=0x00)&&((c)<=0x1F)) || ((c)==0x7F) )
#define ABNF_ISDIGIT(c)    ( ((c)>='0')&&((c)<='9') )
#define ABNF_ISDQUOTE(c)   ( (c)=='\"' )
#define ABNF_ISHEXDIG(c)   ( ABNF_ISDIGIT(c) || (((c)>='A')&&((c)<='F')) || (((c)>='a')&&((c)<='f')) )
#define ABNF_ISHTAB(c)     ( (c)==0x09 )
#define ABNF_ISLF(c)       ( (c)==0x0A )
#define ABNF_ISOCTET(c)    ( ((c)>=0x00)&&((c)<=0xFF) )
#define ABNF_ISSP(c)       ( (c)==' ' ) 
#define ABNF_ISVCHAR(c)    ( ((c)>=0x21)&&((c)<=0x7E) )
#define ABNF_ISWSP(c)      ( ABNF_ISSP(c)||ABNF_ISHTAB(c) )
#define ABNF_ISLBRKT(c)    ( (c)=='{' )
#define ABNF_ISRBRKT(c)    ( (c)=='}' )
#define ABNF_ISALPHADIG(c) ( ABNF_ISALPHA(c)||ABNF_ISDIGIT(c) )
#define ABNF_ISDTMFLTR(c)  ( ABNF_ISALPHA(c)||ABNF_ISDIGIT(c)||((c)=='*')||((c)=='#')||((c)=='T')||((c)=='t') )
#define ABNF_ISCRLF(s)     ( ABNF_ISCR(*((char *)(s))) && ABNF_ISLF(*(((char *)(s))+1)) )
#define ABNF_ISEOL(s)      ( ABNF_ISLF(*((char *)(s))) ? 1: (ABNF_ISCRLF(s)? 2: 0) )
#define ABNF_ISPERIOD(s)    ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='2')&&(LOW2UP_CASE((*((char *)(s+2))))=='E') )
#define ABNF_ISEQUAL(s)     ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='3')&&(LOW2UP_CASE((*((char *)(s+2))))=='D') )
#define ABNF_ISSEMICOLON(s) ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='3')&&(LOW2UP_CASE((*((char *)(s+2))))=='B') )
#define ABNF_ISATSIGN(s)    ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='4')&&(LOW2UP_CASE((*((char *)(s+2))))=='0') )
#define ABNF_ISSPACE(s)     ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='2')&&(LOW2UP_CASE((*((char *)(s+2))))=='0') )
#ifndef SIP_D2_MOD
#define ABNF_ISPOUND(s)     ( (*((char *)(s))=='%')&&(*((char *)(s+1))=='2')&&(LOW2UP_CASE((*((char *)(s+2))))=='3') )
#endif
#define ABNF_ISPGBCK(s)     ( (*((char *)(s))=='.')&&(ABNF_ISEOL(((char *)(s)+1)))&&(ABNF_ISLF(*((char *)(s)-1))) )
#define ABNF_HEXCHAR2NUM(c)  ( ((c)>='a' && (c)<='f')? ((c)-'a'+10): (((c)>='A' && (c)<='F')? ((c)-'A'+10): ((c)-'0')) )
#define ABNF_HEXNUM2CHAR(n)  ( ((n)>9)? ('A'-10+(n)): ('0'+(n)) )
#define ABNF_ISHEXALPHAUP(c) ( ((c)>='A')&&((c)<='F') )
#define ABNF_ISHEXALPHALW(c) ( ((c)>='a')&&((c)<='f') )
#define LOW2UP_CASE(c)     ( ABNF_ISALPHA_LC(c)? ('A' + ((c) - 'a')): (c) )

#endif
