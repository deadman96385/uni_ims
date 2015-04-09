/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30135 $ $Date: 2014-12-01 16:07:35 +0800 (Mon, 01 Dec 2014) $
 */
  
#ifndef _MC_TONE_H_
#define _MC_TONE_H_


/* 
 * These are template values used for tones that need to be configured.
 * These values are defined by reserved/available template values 
 * defined in VTSP.
 */
#define MC_TONE_TEMPLATE_CALL_WAITING   (6)
#define MC_TONE_TEMPLATE_RING           (12)
#define MC_TONE_TEMPLATE_DTMF           (13)

#define MC_TONE_DTMF_RELAY_VOLUME       (10)

vint MC_toneAsciiToDigit(
    char dtmfDigit);

#ifndef MC_NO_TONE
void MC_toneInit(void);

vint MC_toneControl(
    vint           infc,
    OSAL_Boolean   turnOn,
    ISI_AudioTone  tone,
    vint           duration);

vint MC_toneStreamControl(
    vint           infc,
    vint           streamId,
    OSAL_Boolean   turnOn,
    vint           digit,
    vint           duration);

vint MC_ringControl(
    vint  infc,
    vint  template);
#endif /* MC_NO_TONE. */
#endif
