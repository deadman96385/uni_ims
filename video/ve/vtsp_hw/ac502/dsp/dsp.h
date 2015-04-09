/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
 *
 */

#ifndef _DSP_H_
#define _DSP_H_

#include <osal_types.h>

/*
 * Constants
 */
#define DSP_ENCODE                  (0)
#define DSP_DECODE                  (1)

#define DSP_VAD_ENABLE              (0x01)
#define DSP_GAMRNB_BITRATE_475      (0x02)
#define DSP_GAMRNB_BITRATE_515      (0x04)
#define DSP_GAMRNB_BITRATE_590      (0x08)
#define DSP_GAMRNB_BITRATE_670      (0x10)
#define DSP_GAMRNB_BITRATE_740      (0x20)
#define DSP_GAMRNB_BITRATE_795      (0x40)
#define DSP_GAMRNB_BITRATE_1020     (0x80)
#define DSP_GAMRNB_BITRATE_1220     (0x100)
#define DSP_GAMRWB_BITRATE_660      (0x200)
#define DSP_GAMRWB_BITRATE_885      (0x400)
#define DSP_GAMRWB_BITRATE_1265     (0x800)
#define DSP_GAMRWB_BITRATE_1425     (0x1000)
#define DSP_GAMRWB_BITRATE_1585     (0x2000)
#define DSP_GAMRWB_BITRATE_1825     (0x4000)
#define DSP_GAMRWB_BITRATE_1985     (0x8000)
#define DSP_GAMRWB_BITRATE_2305     (0x10000)
#define DSP_GAMRWB_BITRATE_2385     (0x20000)
#define DSP_G726_BITRATE_16KBPS     (0x40000)
#define DSP_G726_BITRATE_24KBPS     (0x80000)
#define DSP_G726_BITRATE_32KBPS     (0x100000)
#define DSP_G726_BITRATE_40KBPS     (0x200000)

typedef void *DSP_Instance;

typedef enum {
    DSP_ERROR = -1,
    DSP_OK    = 0
} DSP_Return;

typedef enum {
    DSP_CODER_TYPE_UNKNOWN  = -1,
    DSP_CODER_TYPE_G711U    = 0,
    DSP_CODER_TYPE_AMRNB_OA,
    DSP_CODER_TYPE_AMRNB_BE,
    DSP_CODER_TYPE_AMRWB_OA,
    DSP_CODER_TYPE_AMRWB_BE,
} DSP_CoderType;

/*
 * Function Prototypes
 */
DSP_Return DSP_init(
    void);

void DSP_shutdown(
    void);

DSP_Instance DSP_getInstance(
    DSP_CoderType   coderType,     /* enumeration of coder type */
    vint            encDec);       /* DSP_ENCODE or DSP_DECODE */

DSP_Instance DSP_encodeInit(
    DSP_Instance    instance,       /* specifies instance */
    DSP_CoderType   coderType,      /* enumeration of coder type */
    uint32          dspInitParam);  /* coder specific init parameter */

DSP_Instance DSP_decodeInit(
    DSP_Instance    instance,       /* specifies instance */
    DSP_CoderType   coderType,      /* enumeration of coder type */
    uint32          dspInitParam);  /* coder specific init parameter */

DSP_Return DSP_codecStop(
    void);

vint DSP_encode(                    /* number of encoded bytes, or error */
    DSP_CoderType   coderType,      /* enumeration of coder type */
    uint8          *packet_ptr,     /* pointer to packet data */
    vint           *speech_ptr);    /* pointer to speech samples */

void DSP_decode(                    /* number of decoded bytes, or error */
    DSP_CoderType   coderType,      /* enumeration of coder type */
    vint           *speech_ptr,     /* pointer to speech samples */
    uint8          *packet_ptr,     /* pointer to packet data */
    uvint           pSize);         /* number of bytes in packet_ptr */

#endif /* _DSP_H_ */
