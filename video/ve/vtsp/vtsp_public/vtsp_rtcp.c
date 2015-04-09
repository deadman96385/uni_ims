/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
 *
 */
#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_rtcpCname() ========
 */
VTSP_Return VTSP_rtcpCname(
    uvint       infc,
    const char *name_ptr)
{
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * Check that interface is valid.
     */
    if (VTSP_OK != _VTSP_isInfcValid(infc)) {
        return (VTSP_E_INFC);
    }

    /*
     * Send the CNAME to VTSPR.
     */
    cmd.code = _VTSP_CMD_RTCP_CNAME;
    cmd.infc = infc;
    OSAL_strncpy((char *)(cmd.msg.cname.cname), name_ptr, _VTSP_RTCP_CNAME_CHARS);

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}
