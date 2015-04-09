/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14823 $ $Date: 2011-05-27 06:19:58 +0800 (Fri, 27 May 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_net.h>
#include <osal_msg.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"

static void _MC_switchCodec(
    MC_VtspCodecControl *codec_ptr,
    OSAL_Boolean         processor)
{
    if (OSAL_TRUE == processor) {
        //PHC_switchCodecToProcessor();
        MC_dbgPrintf("%s: Codec to processor\n", __FUNCTION__);
    }
    else {
        //PHC_switchCodecToModem();
        MC_dbgPrintf("%s: Codec to modem\n", __FUNCTION__);
    }
    
    return;
}

static void _MC_switchVoiceEngine(
    MC_VtspCodecControl *codec_ptr,
    OSAL_Boolean         on)
{
    if (OSAL_TRUE == on) {
        MC_dbgPrintf("%s: Switching Voice Engine to attach\n", __FUNCTION__);
        
        /* Attach the audio driver, using interface 0 */
        if (VTSP_OK != VTSP_infcControlIO(0, VTSP_CONTROL_INFC_IO_AUDIO_ATTACH,
                VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_ENABLE)) {
            MC_dbgPrintf("%s: FAILED to 'attach' the vtsp\n", __FUNCTION__);
        }
    }
    else {
        MC_dbgPrintf("%s: Switching Voice Engine to detach\n", __FUNCTION__);
        
        /* Detach or disable the vtsp */
        if (VTSP_OK != VTSP_infcControlIO(0, VTSP_CONTROL_INFC_IO_AUDIO_ATTACH, 
                VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_DISABLE)) {
            MC_dbgPrintf("%s: FAILED to 'detach' the vtsp\n", __FUNCTION__);
        }
    }
    return;
}

static void _MC_switchWiFi(
    MC_VtspCodecControl *codec_ptr,
    OSAL_Boolean         highPowerOn)
{
    /* Set up the right command */
    if (OSAL_TRUE == highPowerOn) {
//        PHC_setWifiHighPowerMode();
        MC_dbgPrintf("%s: Switching wifi power to hi\n", __FUNCTION__);
    }
    else {
//        PHC_setWifiLowPowerMode();
        MC_dbgPrintf("%s: Switching wifi power to low\n", __FUNCTION__);
    }
}
 
/* 
 * ======== _MC_vtspHwControl() ========
 * This function is called to control the audio codec, the voice engine and the 
 * mode of the wifi driver. 
 * 
 * This function works like a binary semaphore.  
 * Anytime a VTSP channel, which is used for IP (WiFi), is placed into a state
 * other than inactive then it takes a semaphore against.  
 * When a VTSP channel is placed into "inactive" it gives the semaphore back.
 * When the counting semaphore is '0' (zero), then the codec goes to the modem
 * the VTSP get's turned off and the mode of the wifi driver goes to "low power"
 * mode.
 *
 * Return Values:
 *  Nothing.
 */
void MC_vtspHwControl(
    MC_VtspCodecControl *codec_ptr,
    OSAL_Boolean         take)
{
    if (OSAL_TRUE == take) {
        codec_ptr->semaphore++;
        if (1 == codec_ptr->semaphore) {
            /* Then set the audio codec to the processor */
            _MC_switchCodec(codec_ptr, OSAL_TRUE);
            
            /* Turn the voice engine to full on */
            _MC_switchVoiceEngine(codec_ptr, OSAL_TRUE);
            
            /* Set the wifi driver to 'high power' */
            _MC_switchWiFi(codec_ptr, OSAL_TRUE);
        }
    }
    else {
        codec_ptr->semaphore--;
        if (0 >= codec_ptr->semaphore) {
            /* Then set the audio codec to the modem */
            _MC_switchCodec(codec_ptr, OSAL_FALSE);
            
            /* Turn the voice engine off */
            _MC_switchVoiceEngine(codec_ptr, OSAL_FALSE);
            
            /* Set the wifi driver to 'low power' */
            _MC_switchWiFi(codec_ptr, OSAL_FALSE);
            
            /* Reset just to make sure */
            codec_ptr->semaphore = 0;
        }
    }
    return;
}
