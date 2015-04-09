/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30135 $ $Date: 2014-12-01 16:07:35 +0800 (Mon, 01 Dec 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_net.h>
#include <osal_msg.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"
#ifndef MC_NO_TONE
#include "_mc_tone.h"
#endif


#ifndef MC_NO_TONE
/* Object used to lookup frequencies for DTMF digits */
static int _MC_toneDtmfFreq[16][2] = {
    {941, 1336}, {697, 1209}, {697, 1336},  /* 0 1 2 */
    {697, 1477}, {770, 1209}, {770, 1336},  /* 3 4 5 */
    {770, 1477}, {852, 1209}, {852, 1336},  /* 6 7 8 */
    {852, 1477}, {941, 1209}, {941, 1477},  /* 9 * # */
    {697, 1633}, {770, 1633}, {852, 1633},  /* A B C */
    {941, 1633}                             /* D     */
};

static vint _MC_toneConfigDtmf(
    vint          digit,
    vint          duration,
    vint          template)
{
    VTSP_ToneTemplate  toneTemplate;
    
    if (0 > digit || 15 < digit) {
        /* Digit is no good! Return an error */
        return (MC_ERR);
    }
    
    /* Configure the template */
    OSAL_memSet(&toneTemplate, 0, sizeof(VTSP_ToneTemplate));
    
    toneTemplate.freq1      = _MC_toneDtmfFreq[digit][0];
    toneTemplate.freq2      = _MC_toneDtmfFreq[digit][1];
    toneTemplate.power1     = -20;
    toneTemplate.power2     = -20;        
    toneTemplate.cadences   = 1;
    toneTemplate.make1      = duration;
    toneTemplate.repeat1    = 1;
    if (VTSP_OK != VTSP_configTone(template, &toneTemplate)) {
        MC_dbgPrintf("%s: Failed to init DTMF tone template\n", __FUNCTION__);
        return (MC_ERR);
    }
    return (MC_OK);
}
#endif

/* 
 * ======== MC_toneAsciiToDigit() ========
 * Converts a ascii char value representing the DTMF digit to an 
 * enumerated value (0 through 15) representing a DTMF digit.
 * 
 * Return Values:
 * 0 - 15 : The zero based DTMF digit.
 */
vint MC_toneAsciiToDigit(
    char dtmfDigit)
{
    vint digit;
    switch (dtmfDigit) {
        case '*':
            digit = 10;
            break;
        case '#':
            digit = 11;
            break;
        case 'A':
        case 'a':
            digit = 12;
            break;
        case 'B':
        case 'b':
            digit = 13;
            break;
        case 'C':
        case 'c':
            digit = 14;
            break;
        case 'D':
        case 'd':
            digit = 15;
            break;
        default:
            if (48 <= dtmfDigit) {
                digit = (dtmfDigit - 48);
            }
            else {
                digit = dtmfDigit;
            }
            break;
    }
    return (digit);
}

#ifndef MC_NO_TONE
/* 
 * ======== MC_toneInit() ========
 * Initializes tone templates.
 * 
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 * 
 */
void MC_toneInit(void)
{
    VTSP_ToneTemplate  toneTemplate;

    /* First init a call waiting tone. The template number is 6 */
    OSAL_memSet(&toneTemplate, 0, sizeof(VTSP_ToneTemplate));
    toneTemplate.freq1      = 400;
    toneTemplate.freq2      = 0;
    toneTemplate.power1     = -20;
    toneTemplate.power2     = 0;        
    toneTemplate.cadences   = 1;
    toneTemplate.make1      = 200;
    toneTemplate.break1     = 0;
    toneTemplate.repeat1    = 1;
    if (VTSP_OK != VTSP_configTone(MC_TONE_TEMPLATE_CALL_WAITING, 
            &toneTemplate)) {
        MC_dbgPrintf("%s: Failed to init call waiting tone template\n",
                __FUNCTION__);
    }
    
    /* Init the ring tone template */
    OSAL_memSet(&toneTemplate, 0, sizeof(VTSP_ToneTemplate));
    toneTemplate.freq1      = 1000;
    toneTemplate.freq2      = 2500;
    toneTemplate.power1     = 0;
    toneTemplate.power2     = 0;        
    
    toneTemplate.cadences   = 3;
    toneTemplate.make1      = 0;
    toneTemplate.break1     = 1000;
    toneTemplate.repeat1    = 1;
    
    toneTemplate.make2      = 50;
    toneTemplate.break2     = 50;
    toneTemplate.repeat2    = 20;        
    
    toneTemplate.make3      = 0;
    toneTemplate.break3     = 3000;
    toneTemplate.repeat3    = 1;        
    
    if (VTSP_OK != VTSP_configTone(MC_TONE_TEMPLATE_RING, &toneTemplate)) {
        MC_dbgPrintf("%s: Failed to init tone used to ring (notify)\n",
                __FUNCTION__);
    }
    return;
}

/* 
 * ======== MC_toneControl() ========
 * Starts or stops a tone on a local user's interface.
 * 
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 * 
 */
vint MC_toneControl(
    vint           infc,
    OSAL_Boolean   turnOn,
    ISI_AudioTone  tone,
    vint           duration)
{
    vint      temp;
    uvint     toneSeq[1];
    
    if (0 == duration) {
        duration = (vint)VTSP_TONE_TMAX;
    }
    
    /* Map tone to template. */
    switch (tone) {
        case ISI_TONE_RINGBACK:
#ifdef MC_DISABLE_RINGBACK
            return (MC_OK); // just ignore the command to start/stop tone.
#else
            temp = 5;
            break;
#endif
        case ISI_TONE_CALL_WAITING:    
            temp = 6;
            break;
        case ISI_TONE_BUSY:
            temp = 2;
            break;
        case ISI_TONE_ERROR:
            temp = 3;
            break;
        case ISI_TONE_LAST:
            /* a.k.a. silence */
            temp = 0;
            break;
        case ISI_TONE_DTMF_0:
        case ISI_TONE_DTMF_1:
        case ISI_TONE_DTMF_2:
        case ISI_TONE_DTMF_3:
        case ISI_TONE_DTMF_4:
        case ISI_TONE_DTMF_5:
        case ISI_TONE_DTMF_6:
        case ISI_TONE_DTMF_7:
        case ISI_TONE_DTMF_8:
        case ISI_TONE_DTMF_9:
        case ISI_TONE_DTMF_STAR:
        case ISI_TONE_DTMF_POUND:
        case ISI_TONE_DTMF_A:
        case ISI_TONE_DTMF_B:
        case ISI_TONE_DTMF_C:
        case ISI_TONE_DTMF_D:
            if (MC_OK != _MC_toneConfigDtmf(tone, duration,
                    MC_TONE_TEMPLATE_DTMF)) {
                return (MC_ERR);
            }
            temp = MC_TONE_TEMPLATE_DTMF;
            break;
        default:
            return (MC_ERR);
    }
    
    if (OSAL_FALSE == turnOn) {
        /* Then it's 'tone off!' */
        if (VTSP_OK != VTSP_toneLocalStop(infc)) {
            return (MC_ERR);
        }
        return (MC_OK);
    }
    
    if (ISI_TONE_CALL_WAITING == tone) {
        /* Set the template in the 'toneSeq' */
        toneSeq[0] = temp;
        /* Play the tone for 20 seconds max */
        if (VTSP_OK !=  VTSP_toneLocalSequence(infc, toneSeq, 1, 
                VTSP_TONE_BREAK_MIX, 1)) {
            return (MC_ERR);
        }
    }
    else {
        OSAL_logMsg("MC sending tone template=%d", temp);
        /* Start a new tone. */
        if (VTSP_OK != VTSP_toneLocal(infc, temp, VTSP_TONE_NMAX, duration)) {
            return (MC_ERR);
        }
    }
    return (MC_OK);
}

/* 
 * ======== MC_toneControlStream() ========
 * Starts or stops a tone to an outbound network audio stream.
 * 
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 * 
 */
vint MC_toneStreamControl(
    vint           infc,
    vint           streamId,
    OSAL_Boolean   turnOn,
    vint           tone,
    vint           duration)
{
     /* verify the tone value */
    if (OSAL_FALSE == turnOn) {
        if (VTSP_OK != VTSP_streamToneStop(infc, streamId)) {
            return (MC_ERR);
        }
        return (MC_OK);
    }
    
    /* Then config the tone template and generate */
    if (MC_OK != _MC_toneConfigDtmf(tone, duration, MC_TONE_TEMPLATE_DTMF)) {
        return (MC_ERR);
    }    
    
    /* The tone is now configured, so generate it */
    if (VTSP_OK != VTSP_streamTone(infc, streamId,  MC_TONE_TEMPLATE_DTMF, 1, 
            duration)) {
        return (MC_ERR);
    }
    return (MC_OK);
}


/* 
 * ======== MC_ringControl() ========
 * Starts or stops ring on an interface.
 * 
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 * 
 */
vint MC_ringControl(
    vint  infc,
    vint  template)
{
    if (template >= 0) {
        /*
         * Ring with template that was set at configuration time.
         */
        if (VTSP_OK != VTSP_toneLocal(infc, template,
                VTSP_TONE_NMAX, VTSP_TONE_TMAX)) {
            return (MC_ERR);
        }
#if 0 // ZK, this is not implemented yet
        if (VTSP_OK != VTSP_infcControlIO(infc, VTSP_MASK_HW_SPEAKER, 1)) {
            return (MC_ERR);
        }
#endif        
    }
    else {
        if (VTSP_OK != VTSP_toneLocalStop(infc)) {
            return (MC_ERR);
        }
#if 0 // ZK, this is not implemented yet
        if (VTSP_OK != VTSP_infcControlIO(infc, VTSP_MASK_HW_SPEAKER, 0)) {
            return (MC_ERR);
        }
#endif
    }
    return (MC_OK);
}
#endif /* MC_NO_TONE. */
