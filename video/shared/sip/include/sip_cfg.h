/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SIP_CFG_H_
#define _SIP_CFG_H_

/**************************************************************
 ******************** SYSTEM CONFIGURATION ********************
 * The following are default configuration settings for system
 * wide attributes.
 *************************************************************/
 
/* This defines the rate (in milliseconds) at which SIP 
 * applications call the "SIPTIMER_ISR()" interface
 * This value should be 250 ms or less.
 */
#define SIP_TIMER_DEC_RATE               (100)

/* Default IP Port values used when users do not specify 
 * values in API parameters.
 */
#define SIP_DEFAULT_IPADDR_PORT          (5060)
#define SIP_DEFAULT_TLS_IPADDR_PORT      (5061)

/* The max number of network related file descriptors we will service */
#define SIP_MAX_TRANSPORT_DESCRIPTORS    (10)


/* Value used when calculating SIP packet re-send expiries for
 * re-registrations and re-subscriptions that is used for 3GPP TS 24.229.
 * See SIP_RESEND_BACKOFF for additional information.
 */
#define SIP_RESEND_BACKOFF_TS_24_229     (600)

/* Value used when calculating SIP packet re-send expiries for
 * re-registrations and re-subscriptions.
 *
 * Case 1:
 * If this value is <= 4 then the
 * timer will fire at the expiry divided by this number.  For example, if this 
 * value is '2' and 3600 seconds was negotiated for the re-registration interval
 * then the timer would fire at 1800 seconds.
 *
 * Case 2:
 * if the value is > 4 then this value is the time is seconds that is
 * subtracted from the expiry. For example, if this value is '30' and 3600
 * seconds was negotiated for the re-registration interval then the timer would
 * fire at 3570 seconds.
 *
 * Case 3: (3GPP TS 24.229)
 * if the value is > 4 and the server's expiration value is < 1200, then re-registration
 * will occur at 1/2 the negotiated value (which is what would be done if this value
 * were set to 2).  By setting this to '600', and using 600000 in sapp.xml for the sip
 * keepalivesec, the requirements of 3GPP TS 24.229 will be met.  This is to offer
 * 600000, and upon accepted registration re-register at 600 sec. < the negotiated time
 * unless such time is 1200, in which case to re-register at 1/2 the interval.
 */
#define SIP_RESEND_BACKOFF               (SIP_RESEND_BACKOFF_TS_24_229)

/* Define this if you are using a BIG-Endian Processor */
/* #define SIP_IP_BIG_ENDIAN */

/* This is the largest possible external SIP message allowed */
#define SIP_MAX_LAYER_4_PACKET_SIZE      (0x0000FFFF)

/* The MAX length (in bytes) of any domain name */
#define MAX_DOMAIN_NAME_LEN              (128)

/* The MAX length of flag and serive for NAPTR */
#define MAX_FLAG_STR_LEN                 (8)
#define MAX_SERVICE_STR_LEN              (16)

/* The max length (in bytes) of a string containing an 
 * IPV4 address (i.e. xxx.xxx.xxx.xxx). DO NOT MODIFY.
 */
#define MAX_IPV4_STR_LEN                 (16)

/* The max length (in bytes) of a string containing an 
 * IPV6 address (i.e. xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx). DO NOT MODIFY.
 */
#define MAX_IPV6_STR_LEN                 (OSAL_NET_IPV6_STR_MAX)

/* Define the largest positive 32 bit integer.
 * This is used in generating random SDP session id's
 * and version numbers.  As well as the 'CSeq' value 
 * for OPTION requests.
 */
#define SIP_MAX_POSTIVE_INT (0x7fffffff)

/* The max size in bytes for text message payloads 
 * allowed in MESSAGE Requests.
 */
#define SIP_MAX_TEXT_MSG_SIZE            (4096)

/* The max number of IP addresses that can be returned from 
 * DNS 'A' record lookups. If the number of IP addresses returned 
 * from DNS lookups is greater than this, those extra IP addresses 
 * will simply be disregarded.
 */ 
#define MAX_DNS_IP_ADDRESSES             (8)

/* The max number of DNS SRV answer records 
 */ 
#define MAX_DNS_SRV_ANS                  (3)

/* The max number of DNS NAPTR answer records
 */
#define MAX_DNS_NAPTR_ANS                  (3)

/* The max length in bytes of DNS payloads  
 */ 
#define MAX_DNS_BUFFER_SIZE_BYTES        (512)

/* The SIP Stack needs an area to contruct (encode) payloads in SIP 
 * messages (i.e. SDP payloads, etc.).  This definition defines 
 * the max size of this construction area.
 */
#define PAYLOAD_SCRATCH_PAD_SIZE         (4096)

/* MAX number of digits a base 10 value could be DO NOT CHANGE 
 * this value without asking technical support first. 
 */
#define SIP_MAX_BASETEN_NUM_STRING       (16)

/* The MAX queue depth for internally used inter-task OSAL queues
 */
#define SIP_MSGQ_LEN       (8)

/**************************************************************
 *************** SYSTEM BEHAVIOURAL DEFINITIONS****************
 * The following definitions define certain behaviors 
 * of the SIP stack
 *************************************************************/


/* Define this if you would like to allow
 * "blind" NOTIFY's for MWI (voicemail box notification).
 * "blind" NOTIFY's are actually illegal but some vendors
 * (like vonage) use them.  If so, then uncomment the line below
 */
#define SIP_UAS_ALLOW_BLIND_NOTIFY

/* If this is defined then the stack will use the default behavior 
 * to autheticate users when the UA is unaware of the authentication 
 * realm.  In other words, when you define this then the the stack 
 * will use username: "Anonymous" & password: "" if the UA doesn't 
 * know about the realm. Otherwise, if the realm is unknown it will use 
 * it's own username and password (sip_auth.c)
 */
/* #define USE_RFC3261_USERNAME_DEFAULT */

/* Enable this if you want stricter searching for UA's when requests are
 * received. In other words, commenting this out will force the stack to 
 * find UA's in the UA database based on the entire Request-URI rather than 
 * just the username part of the URI
 */
/* #define UA_FIND_UA_VIA_USERNAME_ONLY */

/* Comment this out if you find that tags, branches,
 * uri names and user names excide the max length limit.
 * This will increase the MAX size of these
 * values. In other words, if your tight on heap memory 
 * availability you can try enabling this to get some 
 * savings.  Note, if you enable this be on the lookout 
 * for interoperability issues
 *
 */
/* #define SIP_HF_USE_SHORT_HF */

/* UA have a choice of de-registering with state or without.
 * De-registering without state means that a REGISTER request
 * is sent regardless of the current state of the registration 
 * process so the REGISTER will be sent no matter what and if the
 * REGISTER is challenged to authenticate it will simply ignore it.
 * De-registering with state means that if the REGISTER request sent 
 * to De-register a UA is challenged, then it will attempt to authenticate.
 * If you want the De-registration to happen statefully, then enable 
 * the definition below.
 */
#define SIP_UN_REG_WITH_AUTH

/* This definition represents the minimum length a SIP packet must be 
 * before we consider it to be "dummy packet".  If packets received
 * on the SIP interface are smaller then the value below, then they will
 * be quietly discarded.  "Dummy Packet" are sent by some remote SIP 
 * devices to a UA to keep firewalls alive.
 */
#define SIP_DUMMY_PACKET_MAX (10)

/* This definition represents the dummy packet sent to keep firewalls and NAT 
 * mappings refreshed
 */
#define SIP_DUMMY_PACKET     ("\r\n\r\n")

/* This value defines whether to use a custom D2 proprietary capabilities exchange
 * technique.  If this is defined then instead of using SIP OPTIONS requests
 * to perform a capability exchange it will use custom SIP MESSAGE's.
 * Additionally the definition defines what domain is using this
 * custom capability exchange mechanism and the identifying string
 * used to indicate that the SIP MESSAGE is specifically for a capability exchange.
 * To disable this D2 customization just comment both the values below out.
 */
#define SIP_CUSTOM_CAPABILITY_EXCHANGE ("sip2sip.info")

/**************************************************************
 **************** UA CONFIGURATION DEFINITIONS*****************
 * The following are definitions used for UA configurations 
 *************************************************************/

/* The max number of 'Address of Records' a UA can be configured with */
#define SIP_MAX_NUM_AOR                  (3)

/* The max number of Authentication Credential sets 
 * (a set consisting of username, password, realm) 
 * that a UA can be configured with 
 */
#define SIP_MAX_NUM_AUTH_CRED            (3)

/*
 * The Maximum number of dialogs for a UA.  Calculated as:
 * 2 for calls (voice and video)
 * 1 for possible conference (a.k.a merging)
 * 1 for subscription to conference event package
 * 1 for subscription to registration event package
 * 1 for subscription to dialog event package
 */
#define SIP_DIALOGS_PER_UA_MAX       (6)

/* This is the max size of the string used to specify 
 * coder types.  When UA_Create() is called developers can 
 * specify all the coders via a space delimited string ( i.e. "0 2 3 19").
 * This definition defines the max size of that string 
 */
#define SIP_MAX_CODER_STR_SIZE           (32)
/* This defines the max number of coders a UA could be configured with */
#define SYSDB_MAX_NUM_CODERS             (8)

/* The MAX number of header fields that can be reported 
 * to the application via the call back routine defined when 
 * UA_Create() is called
 */
#define SIP_MAX_HEADER_FIELDS            (16)

/* The MAX size of the "Event" header field value */
#define SIP_EVT_STR_SIZE_BYTES (384)

/* The MAX size of the "k" (Key) value used for mutual authentication for
 * AKA Version 1 & 2.
 */
#define SIP_AUTH_KEY_SIZE (16)

/* The MAX size of the optional "op" value used for mutual authentication for
 * AKA Version 1 & 2.
 */
#define SIP_AUTH_OP_SIZE (16)

/* The MAX size of the optional "amf" value used for mutual authentication for
 * AKA Version 1 & 2.
 */
#define SIP_AUTH_AMF_SIZE (2)

/**************************************************************
 ******************** DEBUGGING DEFINITIONS********************
 *************************************************************/

/* The default debug level to use when SIP debug logging 
 * is enabled.  All possible values are as follows:
 * 1 = log only errors
 * 2 = log errors and warnings
 * 3 = log everything including general logging 
 */
#define DEFAULT_MSG_LEVEL                (1)


/* The debug module needs a buffer to construct debug log messages.
 * This defintion defines the max size of this construction area.  
 * Note, the debug module will not overflow buffers. If this value 
 * is not big enough to accomidate the message that the debug module
 * wants to construct, then the message will be truncated.  Note, that
 * this buffer should be pretty big if you want to print entire SIP messages
 */
#define SIP_DEBUG_MAX_STRING_SIZE       (4096)

/**************************************************************
 **************** TRANSACTION MODULE CONFIGURATION ************
 * The following are definitions used exclusively in the 
 * tranaction module 
*************************************************************/


/* Define this is you want firewall 'pinhole' refreshing as 
 * defined RFC3581. In other words, if this is defined then INVITES will 
 * continue to be sent regardless of whether or not there was a 
 * provision response received. 
 */
//#define TRANSACTION_WITH_FIREWALL
/* If the above is defined then this defines the interval at which 
 * INVITE Reqeusts will continue to be sent to refresh firewall
 * pinholes
 */
#define TRANS_FIREWALL_REFRESH (20000) /* 20 seconds */

/* All transaction timer definitions as defined in section 7.7 of TS 24.229.
 * All times are in Milliseconds
 */
#define TRANS_TIMER_T1         (2000)
#define TRANS_TIMER_T2         (16000)
#define TRANS_TIMER_T4         (17000)

/* Default define */
#define TRANS_TIMER_A          (TRANS_TIMER_T1)
#define TRANS_TIMER_B          (TRANS_TIMER_T1 * 64) /* 128 seconds */

#define TRANS_TIMER_C          (3 * 60000) /* > 3min */
#define TRANS_TIMER_D          (128000) /* > 128 seconds */
#define TRANS_TIMER_E          (TRANS_TIMER_T1)
#define TRANS_TIMER_F          (TRANS_TIMER_T1 * 64) /* 128 seconds */
#define TRANS_TIMER_G          (TRANS_TIMER_T1)
#define TRANS_TIMER_H          (TRANS_TIMER_T1 * 64)
#define TRANS_TIMER_I          (TRANS_TIMER_T4)
#define TRANS_TIMER_J          (TRANS_TIMER_T1 * 64) /* 128 seconds */
#define TRANS_TIMER_K          (TRANS_TIMER_T4)


#if defined(PROVIDER_CMCC)
/* Define CMCC special timer */
#define TRANS_TIMER_TCALL        (10000) /* 10 seconds */
#endif

 /*
  * A timer used to catch duplicate INVITE's in the INVITE Server Transaction.
  * This is a bug in RFC3261.  This timer is used to fix the RFC bug.
  */
#define TRANS_TIMER_INVITE_CATCH_BUG (32000)


/**************************************************************
 ************ DIALOG/SESSION MODULE CONFIGURATION *************
 *************************************************************/

/* The max number of 'media' entries in a session description */
#define MAX_SESSION_MEDIA_STREAMS    (2)

/* The max number of 'alt' entries in a session description */
#define MAX_SESSION_SIT_CANDIDATES   (2)

/* The max size of SRTP key parameters in a session description */
#define MAX_SESSION_SRTP_PARAMS      (40)

/*
 * The max size of fields used in media & session description for encoder names
 * descriptions, and other general string use, etc.
 */
#define MAX_SESSION_MEDIA_STR       (32)
#define MAX_SESSION_MEDIA_LARGE_STR (192)
#define MAX_SESSION_PAYLOAD_SIZE    (2048)

/* maximum precondition status size. There might be 3 status type for each status, so maximum size should be 9 */
#define SIP_PREC_STATUS_SIZE_MAX    (9)

#endif

