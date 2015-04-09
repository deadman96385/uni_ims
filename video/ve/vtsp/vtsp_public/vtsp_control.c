/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */
#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_toneLocal ========
 */
VTSP_Return VTSP_toneLocal(
    uvint           infc,
    uvint           templateTone,
    uvint           repeat,
    uint32          maxTime)
{
    VTSP_Return  e;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isToneTemplIdValid(infc, templateTone))) { 
        return (e);
    }

    /* Bounds check */
    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }
    if (maxTime > VTSP_TONE_TMAX) { 
        maxTime = VTSP_TONE_TMAX;
    }

    cmd.code = _VTSP_CMD_TONE_LOCAL;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = templateTone;
    cmd.msg.arg.arg1 = repeat;
    cmd.msg.arg.arg2 = maxTime;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_toneQuadLocal ========
 */
VTSP_Return VTSP_toneQuadLocal(
    uvint           infc,
    uvint           templateQuad,
    uvint           repeat,
    uint32          maxTime)
{
#ifdef VTSP_ENABLE_TONE_QUAD
    _VTSP_CmdMsg cmd;
    VTSP_Return  e;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isToneTemplIdValid(infc, templateQuad))) { 
        return (e);
    }

    /* Bounds check */
    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }
    if (maxTime > VTSP_TONE_TMAX) { 
        maxTime = VTSP_TONE_TMAX;
    }

    cmd.code = _VTSP_CMD_TONE_QUAD_LOCAL;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = templateQuad;
    cmd.msg.arg.arg1 = repeat;
    cmd.msg.arg.arg2 = maxTime;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
#else
    return (VTSP_E_CONFIG);
#endif
}

/*
 * ======== VTSP_toneLocalSequence ========
 */
VTSP_Return VTSP_toneLocalSequence(
    uvint   infc,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat)
{
    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;
    vint            index;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (NULL == toneId_ptr) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds < 1) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds > _VTSP_NUM_TONE_SEQUENCE_MAX) {
        return (VTSP_E_ARG);
    }
    for (index = (numToneIds - 1); index >= 0; index--) {
        if (VTSP_OK != 
                (e = _VTSP_isToneTemplIdValid(infc, toneId_ptr[index]))) { 
            return (e);
        }
    }

    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }

    cmd.code = _VTSP_CMD_TONE_LOCAL_SEQUENCE;
    cmd.infc = infc;
    cmd.msg.toneSequence.numToneIds = numToneIds;
    cmd.msg.toneSequence.control = control;
    cmd.msg.toneSequence.repeat = repeat;
    cmd.msg.toneSequence.maxTime = VTSP_TONE_TMAX;

    for (index = (numToneIds - 1); index >= 0; index--) {
        cmd.msg.toneSequence.toneIds[index] = 
            toneId_ptr[index];
    }

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_toneQuadLocalSequence ========
 */
VTSP_Return VTSP_toneQuadLocalSequence(
    uvint   infc,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat)
{
#ifdef VTSP_ENABLE_TONE_QUAD    
    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;
    vint            index;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (NULL == toneId_ptr) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds < 1) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds > _VTSP_NUM_TONE_SEQUENCE_MAX) {
        return (VTSP_E_ARG);
    }
    for (index = (numToneIds - 1); index >= 0; index--) {
        if (VTSP_OK != 
                (e = _VTSP_isToneTemplIdValid(infc, toneId_ptr[index]))) { 
            return (e);
        }
    }

    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }

    cmd.code = _VTSP_CMD_TONE_QUAD_LOCAL_SEQUENCE;
    cmd.infc = infc;
    cmd.msg.toneSequence.numToneIds = numToneIds;
    cmd.msg.toneSequence.control = control;
    cmd.msg.toneSequence.repeat = repeat;
    cmd.msg.toneSequence.maxTime = VTSP_TONE_TMAX;
    for (index = (numToneIds - 1); index >= 0; index--) {
        cmd.msg.toneSequence.toneIds[index] = 
            toneId_ptr[index];
    }

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
#else
    return (VTSP_E_CONFIG);
#endif
}
/*
 * ======== VTSP_toneLocalStop ========
 */
VTSP_Return VTSP_toneLocalStop(
    uvint             infc)
{
    return (VTSP_toneLocal(infc, 0, 0, 0));
}

/*
 * ======== VTSP_toneQuadLocalStop ========
 */
VTSP_Return VTSP_toneQuadLocalStop(
    uvint             infc)
{
    return (VTSP_toneQuadLocal(infc, 0, 0, 0));
}

/*
 * ======== VTSP_detect ========
 */
VTSP_Return VTSP_detect(
    uvint             infc,
    uint16            detectMask)
{

    VTSP_Return  e;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    cmd.code = _VTSP_CMD_DETECT;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = detectMask;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_ring ========
 */
VTSP_Return VTSP_ring(
    uvint           infc,
    uvint           templateRing,
    uvint           numRings,
    uvint           maxTime,
    VTSP_CIDData   *cid_ptr)
{
    VTSP_Return     e;
    VTSP_Return     e2;
    _VTSP_CmdMsg    cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    e = _VTSP_isInfcFxs(infc);
    
    e2 = _VTSP_isInfcAudio(infc);

    if ((VTSP_OK != e) && (VTSP_OK != e2)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isRingTemplIdValid(infc, templateRing))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    cmd.msg.arg.arg3 = 0;
#if defined(VTSP_ENABLE_CIDS) || defined(VTSP_ENABLE_HANDSET_CIDS)
    /*
     * This block works when VTSP_ENABLE_CIDS is set.
     * If it's not set, the function will return and the call
     * process would be fail since caller VAPP_vtspConrolRing()
     * does not check the return value.
     * So add the primitive for this block to prevent the issue.
     */
    if (NULL != cid_ptr) { 
        if (VTSP_OK != (e = _VTSP_cidFormat(cid_ptr))) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (e);
        }

        if (VTSP_OK != (e = _VTSP_putCidData(infc, cid_ptr))) { 
            _VTSP_TRACE(__FILE__, __LINE__);
            return (e);
        }
        /* CID sent to Q successfully - enable CID with ring */
        cmd.msg.arg.arg3 = VTSP_EVENT_ACTIVE;
    }
#endif
    /* Bounds check */
    if (numRings > VTSP_RING_NMAX) { 
        numRings = VTSP_RING_NMAX;
    }
    if (maxTime > VTSP_RING_TMAX) { 
        maxTime = VTSP_RING_TMAX;
    }

    cmd.code = _VTSP_CMD_RING;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = templateRing;
    cmd.msg.arg.arg1 = numRings;
    cmd.msg.arg.arg2 = maxTime;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_ringStop ========
 */
VTSP_Return VTSP_ringStop(
    uvint             infc)
{
    return (VTSP_ring(infc, 0, 0, 0, NULL));
}

/*
 * ======== VTSP_infcControlIO ========
 * The value word is hardware specific.
 */
VTSP_Return VTSP_infcControlIO(
    uvint               infc,
    VTSP_ControlInfcIO  control,
    int32               value)
{
    VTSP_Return  e;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }

    cmd.code = _VTSP_CMD_INFC_CONTROL_IO;
    cmd.infc = infc;
    cmd.msg.control.code = control;

    cmd.msg.control.arg  = (int32)value;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);

}

/*
 * ======== VTSP_infcControlGain ========
 */
VTSP_Return VTSP_infcControlGain(
    uvint               infc,
    vint                gainTx,
    vint                gainRx)
{

    VTSP_Return  e;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    cmd.code = _VTSP_CMD_INFC_CONTROL_GAIN;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = gainTx;
    cmd.msg.arg.arg1 = gainRx;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_infcControlLine ========
 */
VTSP_Return VTSP_infcControlLine(
    uvint                 infc,
    VTSP_ControlInfcLine  control)
{

    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }

    cmd.code = _VTSP_CMD_INFC_CONTROL_HOOK;
    cmd.infc = infc;
    cmd.msg.control.code = control;
    cmd.msg.control.arg  = 0;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_cidDataPackInit ========
 *
 */
VTSP_Return VTSP_cidDataPackInit(
    VTSP_CIDData     *cid_ptr)
{
    _VTSP_CIDData  *obj_ptr;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }

    obj_ptr = (_VTSP_CIDData *)cid_ptr;

    /* data always has first two bytes = { msg type, payload len } 
     */
    OSAL_memSet(obj_ptr, 0, sizeof(_VTSP_CIDData));
    switch (_VTSP_object_ptr->cidFormat) { 
        case VTSP_TEMPL_CID_FORMAT_UK_DTMF:
            /* DTMF protocol format */
            obj_ptr->data[0] = 0;
            obj_ptr->len = 0;
            break;
        default:
            /* FSK protocol format */
            obj_ptr->data[0] = 0x80;
            obj_ptr->len = 2;
            break;
    }

    return (VTSP_OK);
}

/*
 * ======== VTSP_cidDataPack ========
 */
VTSP_Return VTSP_cidDataPack(
    VTSP_CIDDataFields   field,
    char                *string_ptr,
    VTSP_CIDData        *cid_ptr)
{
    _VTSP_CIDData   *obj_ptr;
    uvint            len;
    char            *dst_ptr;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (NULL == string_ptr) { 
        return (VTSP_E_ARG);
    }
    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }

    obj_ptr = (_VTSP_CIDData *)cid_ptr;
    len = 0;
    dst_ptr = (char *)&obj_ptr->data[obj_ptr->len];

    switch (_VTSP_object_ptr->cidFormat) { 
        case VTSP_TEMPL_CID_FORMAT_UK_DTMF:
            switch (field) { 
                case VTSP_CIDDATA_FIELD_NUMBER:
                    len = OSAL_strlen(string_ptr);
                    if (len + obj_ptr->len > 255 - 2) { 
                        /* String exceeds max CID length */
                        return (VTSP_E_RESOURCE);
                    }
                    *dst_ptr++ = 'A';       /* Start field for calling # */
                    obj_ptr->len += 1;
                    OSAL_strcpy(dst_ptr, string_ptr);
                    break;
                case VTSP_CIDDATA_FIELD_SPECIAL:
                    /* Special event notifications */
                    len = OSAL_strlen(string_ptr);
                    if (len + obj_ptr->len > 255 - 2) { 
                        /* String exceeds max CID length */
                        return (VTSP_E_RESOURCE);
                    }
                    *dst_ptr++ = 'B';       /* Start field for special info */
                    obj_ptr->len += 1;
                    OSAL_strcpy(dst_ptr, string_ptr);
                    break;

                default:
                    /* Discard other information, unhandled for CID-DTMF */
                    return (VTSP_E_RESOURCE);
            }
            obj_ptr->len += len;
            break;
        default:
            /* Other regions, FSK */
            switch (field) { 
                case VTSP_CIDDATA_FIELD_NAME:
                case VTSP_CIDDATA_FIELD_NUMBER:
                case VTSP_CIDDATA_FIELD_DATE_TIME:
                case VTSP_CIDDATA_FIELD_BLOCK:
                case VTSP_MODEMDID_FIELD_NUMBER:
                    len = OSAL_strlen(string_ptr);
                    if (len + obj_ptr->len > 255 - 2) { 
                        /* String exceeds max CID length */
                        return (VTSP_E_RESOURCE);
                    }
                    *dst_ptr++ = field;
                    *dst_ptr++ = len;
                    obj_ptr->len += 2;
                    OSAL_strcpy(dst_ptr, string_ptr);
                    break;

                case VTSP_CIDDATA_FIELD_VI:
                    if (len + obj_ptr->len > 255 - 2) { 
                        /* String exceeds max CID length */
                        return (VTSP_E_RESOURCE);
                    }
                    /*
                     * Modify the message type word for Message Waiting 
                     * Notification.
                     */
                    obj_ptr->data[0] = 0x82;
                    *dst_ptr++ = field;
                    /*
                     * Set parameter length.
                     */
                    *dst_ptr++ = string_ptr[0];
                    /*
                     * Set parameter Word (0xFF or 0x00).
                     */
                    *dst_ptr++ = string_ptr[1];
                    obj_ptr->len += 3;
                    break;
                    

                case VTSP_CIDDATA_FIELD_RAW:
                    len = OSAL_strlen(string_ptr);
                    /* Copy raw string data to cid data */
                    if (len + obj_ptr->len > 255) { 
                        /* String exceeds max CID length */
                        return (VTSP_E_RESOURCE);
                    }
                    OSAL_strcpy(dst_ptr, string_ptr);
                    break;
                default:
                    /* No such field is handled */
                    return (VTSP_E_RESOURCE);
            }
            obj_ptr->len += len;
            obj_ptr->data[obj_ptr->len] = 0;
            obj_ptr->data[1] = obj_ptr->len - 2;
            break;
    }

    return (VTSP_OK);
}

/*
 * ======== VTSP_cidDataUnpack ========
 */
VTSP_Return VTSP_cidDataUnpack(
    VTSP_CIDDataFields   field,
    char                *string_ptr,
    VTSP_CIDData        *cid_ptr)
{
    _VTSP_CIDData   *obj_ptr;
    uvint            len;
    uvint            tlen;
    uint8           *src_ptr;
    uint8           *srcEnd_ptr;
    uvint            mtype;
    uvint            type;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (NULL == string_ptr) { 
        return (VTSP_E_ARG);
    }
    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }

    obj_ptr    = (_VTSP_CIDData *)cid_ptr;
    len        = 0;
    src_ptr    = (uint8 *)&obj_ptr->data[0];
    mtype      = *src_ptr++;
    tlen       = *src_ptr++;
    srcEnd_ptr = src_ptr + tlen;

    if (tlen < 3) {
        return (VTSP_E_NO_MSG);
    }

    switch (field) {
        case VTSP_CIDDATA_FIELD_NAME:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_NAME;
            break;
            
        case VTSP_CIDDATA_FIELD_NUMBER:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_NUMBER;
            break;
            
        case VTSP_CIDDATA_FIELD_DATE_TIME:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_DATE_TIME;
            break;

        case VTSP_CIDDATA_FIELD_VI:
            if (0x82 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_VI;
            break;
            
        case VTSP_CIDDATA_FIELD_RAW:
            OSAL_memCpy(string_ptr, (char *)src_ptr, tlen);
            string_ptr[tlen] = 0;
            return (VTSP_OK);

        default:
            /* No such field is handled */
            return (VTSP_E_RESOURCE);
    }

    type = *src_ptr++;
    len = *src_ptr++;
    while (field != (VTSP_CIDDataFields)type) {
        if (src_ptr >= (srcEnd_ptr - 2)) {
            return (VTSP_E_NO_MSG);
        }
        src_ptr += len;
        type = *src_ptr++;
        len = *src_ptr++;
    }
    OSAL_memCpy(string_ptr, (char *)src_ptr, len);
    string_ptr[len] = 0;
    
    return (VTSP_OK);
}

/*
 * ======== VTSP_cidGet ========
 */
VTSP_Return VTSP_cidGet(
    uvint             infc,
    VTSP_CIDData     *cid_ptr)
{
    _VTSP_CIDData  *obj_ptr;
    VTSP_Return     e;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }

    /* 
     * recv CID data from cidr q
     * VTSPR fills in the data only, not the size
     */
    obj_ptr    = (_VTSP_CIDData *)cid_ptr;
    if (VTSP_OK != (e = _VTSP_getCidData(infc, &obj_ptr->data[0]))) { 
        return (e);
    }

    /*
     * Set the total packet size, using the size in the message plus 2
     */
    obj_ptr->len = obj_ptr->data[1] + 2;        /* tlen + 2 */
    
    return (VTSP_OK);
}

/*
 * ======== VTSP_cidDataUnpackDtmf ========
 */
VTSP_Return VTSP_cidDataUnpackDtmf(
    VTSP_CIDDataFields   field,
    char                *string_ptr,
    VTSP_CIDData        *cid_ptr)
{
    _VTSP_CIDData   *obj_ptr;
    uvint            len;
    char            *src_ptr;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (NULL == string_ptr) { 
        return (VTSP_E_ARG);
    }
    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }

    obj_ptr = (_VTSP_CIDData *)cid_ptr;
    src_ptr = (char *)&obj_ptr->data[0];
    len = OSAL_strlen(src_ptr);
    string_ptr[0] = 0;
    if (len < 2) {
        return (VTSP_E_NO_MSG);
    }
    
    if (VTSP_CIDDATA_FIELD_NUMBER == field) {
        /*
         * A or D till next A, B, C, D
         */
        while (len-- > 0) {
            if (('A' == *src_ptr) || ('D' == *src_ptr)) {
                src_ptr++;
                break;
            }
            src_ptr++;
        }
       
        if (0 == len) {
            return (VTSP_E_NO_MSG);
        }
        
        while (len-- > 0) {
            if (('A' == *src_ptr) || ('B' == *src_ptr) || ('D' == *src_ptr)) {
                break;
            }
            *string_ptr++ = *src_ptr++;
        }
        *string_ptr = 0;
    }
    else if (VTSP_CIDDATA_FIELD_RAW == field) {
        OSAL_strcpy(string_ptr, src_ptr);
    }
    else {
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}
/*
 * ======== VTSP_cidOffhook ========
 */
VTSP_Return VTSP_cidOffhook(
    uvint             infc,
    VTSP_CIDData     *cid_ptr)
{

    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }

    /* Check for CID Offhook support in this region */
    switch (_VTSP_object_ptr->cidFormat) { 
        case VTSP_TEMPL_CID_FORMAT_UK_DTMF:
            return (VTSP_E_RESOURCE);
    }

    if (VTSP_OK != (e = _VTSP_cidFormat(cid_ptr))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* send CID data in cid q */
    if (VTSP_OK != (e = _VTSP_putCidData(infc, cid_ptr))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    } 
    
    cmd.code = _VTSP_CMD_CID_OFFHOOK;
    cmd.infc = infc;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_cidTx ========
 */
VTSP_Return VTSP_cidTx(
    uvint             infc)
{

    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isInfcFxs(infc))) { 
        return (e);
    }

    cmd.code = _VTSP_CMD_CID_TX;
    cmd.infc = infc;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}
