/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28791 $ $Date: 2014-09-11 15:28:46 +0800 (Thu, 11 Sep 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include "_iut_app.h"
#include "_iut_prt.h"
#include "_iut_cfg.h"
#include "_iut.h"
#include "_iut_api.h"
#include "_iut_menu.h"

//#include <uuid/uuid.h>

static const char _IUT_MENU_LargeMsg[] = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\
012345678901234567890123456789012345678900002048";

/*
 * ======== _IUT_menuD2DialPlan() ========
 *
 * This function identifies and properly constructs URI's based on
 * digits dialed by a user.  It is specifically used for internal D2 use.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuD2DialPlan(
    char *proxy_ptr,
    char *dial_ptr,
    char *digits_ptr,
    vint  sz)
{
    vint ip0;
    vint ip1;

    if (0 != proxy_ptr[0]) {
        /*
         * Add ua_ptr->proxy if available.
         */
        if (0 == OSAL_strncmp(proxy_ptr, "sip:", 4)) {
            OSAL_snprintf(dial_ptr, sz, "sip:%s@%s", digits_ptr, proxy_ptr + 4);
        }
        else if (0 == OSAL_strncmp(proxy_ptr, "sips:", 5)) {
            OSAL_snprintf(dial_ptr, sz, "sips:%s@%s", digits_ptr, proxy_ptr + 5);
        }
        else {
            OSAL_snprintf(dial_ptr, sz, "sip:%s@%s", digits_ptr, proxy_ptr);
        }
    }
    else {

        /*
         * No #, * in IP addr.
         */
        if ((digits_ptr[1] < '0') || (digits_ptr[1] > '9')) {
            digits_ptr[1] = '0';
        }
        if ((digits_ptr[2] < '0') || (digits_ptr[2] > '9')) {
            digits_ptr[2] = '0';
        }
        if ((digits_ptr[3] < '0') || (digits_ptr[3] > '9')) {
            digits_ptr[3] = '0';
        }
        if ((digits_ptr[4] < '0') || (digits_ptr[4] > '9')) {
            digits_ptr[4] = '0';
        }

        ip0 = ((digits_ptr[1] - '0') * 10) + (digits_ptr[2] - '0');
        ip1 = ((digits_ptr[3] - '0') * 10) + (digits_ptr[4] - '0');

        if ('2' == digits_ptr[0]) {
            OSAL_snprintf(dial_ptr, sz, "%s@172.16.%d.%d",
                    digits_ptr, ip0, ip1);
        }
        else if ('0' == digits_ptr[0]) {
            OSAL_snprintf(dial_ptr, sz, "%s@10.16.%d.%d",
                    digits_ptr, ip0, ip1);
        }
        else {
            if ( 0 == dial_ptr[0]) {
                dial_ptr[0] = 0;
            }
        }
    }
}

/*
 * ======== _IUT_menuScanc() ========
 *
 * Just like "getchar()".  this will retrieve character input from an
 * interface until a carriage return (a.k.a."\n") is received.
 *
 * Returns:
 *   Nothing.
 *
 */
static char _IUT_menuScanc(
    void)
{
    char ch;
    char ret = '\n';

    while (1) {
        ch = XXGETCH();
        if ('\n' == ch) {
            break;
        }
        else {
            ret = ch;
        }
    }

    return (ret);
}

/*
 * ======== _IUT_menuScans() ========
 *
 * Just like "scans()".  this will retrieve character input from an
 * interface until a carriage return (a.k.a."\n") is received.
 * When it returns, "str_ptr"  will be populated will a NULL terminated
 * string containing the users input.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuScans(
    char *str_ptr,
    int   n)
{
    char ch;
    if (0 == n) {
        return;
    }
    while (--n > 0) {
        ch = XXGETCH();
        if ('\n' == ch) {
            break;
        }
        else {
            *str_ptr++ = ch;
        }
    }
    *str_ptr = 0;
}

/*
 * ======== _IUT_menuScanl() ========
 *
 * Just like "scans()" but specially used for integers. This function will
 * retrieve character input from an interface until a carriage return
 * (a.k.a."\n") is received. It will then return the integer representation
 * of the input.
 *
 * Returns:
 *   The integer of the character input.
 *
 */
static vint _IUT_menuScanl(void)
{
    char buff[IUT_STR_SIZE + 1];

    _IUT_menuScans(buff, IUT_STR_SIZE);

    if (buff[0] == 0) {
        return (0);
    }
    return (OSAL_atoi(buff));
}

/*
 * ======== _IUT_menuParams() ========
 *
 * This function displays a menu and accepts input for settings related to
 * an ISI service configuration.
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuParams(
    IUT_HandSetObj *hs_ptr)
{
    char    ch;
    char    buff[IUT_STR_SIZE + 1];
    int     value, value2, value3, value4;

    while (1) {
        OSAL_logMsg( "<><><><> %s service settings (enter \\ to clear) <><><><>\n"
            "0=username (%s)\n"
            "1=password (%s)\n"
            "2=server (%s)\n"
            "3=outbound server (%s)\n"
            "4=stun server (%s)\n"
            "5=relay server (%s)\n"
            "6=xcap-root (%s)\n"
            "7=chat server (%s)\n"
            "8=realm (%s)\n"
            "9=uri (%s)\n"
            "a=privacy (%d)\n"
            "b=block user \n"
            "c=unblock user \n"
            "d=set fields\n"
            "e=set coders\n"
            "f=RTP security(%d)\n"
            "g=Service Name (%s)\n"
            "h=set conditional call forwarding\n"
            "i=clear conditional call forwarding\n"
            "j=set interface ip address(%s)\n"
            "k=set emergency(%d)\n"
            "l=set imei uri(%s)\n"
            "m=set port parameters\n"
            "n=set RCS provisionging data from xml file\n"
            "o=set ipsec port and spi\n"
            "q=quit\n",
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_NAME_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_USERNAME_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PASSWORD_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PROXY_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_OUTBOUND_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_STUN_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_RELAY_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_XCAP_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_CHAT_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_REALM_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_URI_F),
            IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CID_PRIVATE_F),
            IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_SECURITY_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_NAME_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_IF_ADDRESS_F),
            IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_EMERGENCY_F),
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_IMEI_F));
        ch = _IUT_menuScanc();

        switch(ch) {
            case '0':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_USERNAME_F, buff);
                break;
            case '1':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PASSWORD_F, buff);
                break;
            case '2':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PROXY_F, buff);
                break;
            case '3':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_OUTBOUND_F, buff);
                break;
            case '4':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_STUN_F, buff);
                break;
            case '5':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_RELAY_F, buff);
                break;
            case '6':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_XCAP_F, buff);
                break;
            case '7':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_CHAT_F, buff);
                break;
            case '8':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_REALM_F, buff);
                break;
            case '9':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_URI_F, buff);
                break;
            case 'a':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_CID_PRIVATE_F,
                        buff);
                break;
            case 'b':
                OSAL_logMsg("enter user:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appBlockUser(hs_ptr, buff, 1);
                break;
            case 'c':
                OSAL_logMsg("enter user:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appBlockUser(hs_ptr, buff, 0);
                break;
            case 'd':
                IUT_appSetFields(hs_ptr,
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PROXY_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_STUN_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_RELAY_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_XCAP_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_CHAT_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_OUTBOUND_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_USERNAME_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PASSWORD_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_REALM_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_URI_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_IMEI_F),
                    IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_EMERGENCY_F),
                    IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AUDIO_PORT_F),
                    IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_VIDEO_PORT_F),
                    IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_IF_ADDRESS_F),
                    IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CID_PRIVATE_F));
                OSAL_logMsg("fields set!\n");
                break;
            case 'e':
                OSAL_logMsg("change coder list ? <0/1>:\n");
                ch = _IUT_menuScanc();
                if ('1' == ch) {
                    OSAL_logMsg("pcmu:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PCMU_F, buff);

                    OSAL_logMsg("pcmu-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PPCMU_F, buff);

                    OSAL_logMsg("pcma:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PCMA_F, buff);

                    OSAL_logMsg("pcma-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PPCMA_F, buff);

                    OSAL_logMsg("g726-32:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_G726_F, buff);

                    OSAL_logMsg("g726-32-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PG726_F, buff);

                    OSAL_logMsg("g729:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_G729_F, buff);

                    OSAL_logMsg("g729-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PG729_F, buff);

                    OSAL_logMsg("iLBC:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_ILBC_F, buff);

                    OSAL_logMsg("ilbc-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PILBC_F, buff);

                    OSAL_logMsg("SILK:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_SILK_F, buff);

                    OSAL_logMsg("SILK-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PSILK_F, buff);

                    OSAL_logMsg("dtmfr:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_DTMFR_F, buff);

                    OSAL_logMsg("dtmfr-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PDTMFR_F, buff);

                    OSAL_logMsg("cn:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_CN_F, buff);

                    OSAL_logMsg("cn-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PCN_F, buff);

                    OSAL_logMsg("h264:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_H264_F, buff);

                    OSAL_logMsg("h264-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PH264_F, buff);

                    OSAL_logMsg("h263:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_H263_F, buff);

                    OSAL_logMsg("h263-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PH263_F, buff);

                    OSAL_logMsg("amrnb:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_AMRNB_F, buff);

                    OSAL_logMsg("amrnb-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PAMRNB_F, buff);

                    OSAL_logMsg("amrwb:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_AMRWB_F, buff);

                    OSAL_logMsg("amrwb-priority:\n");
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_PAMRWB_F, buff);
                }
                IUT_appSetCoders(hs_ptr, IUT_MAX_LINES,
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMU_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMU_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMA_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMA_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G726_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG726_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G729_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG729_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_ILBC_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PILBC_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_SILK_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PSILK_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G722_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG722_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_DTMFR_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PDTMFR_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CN_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCN_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H264_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH264_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H263_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH263_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRNB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRNB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRWB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRWB_F)
                        );
#if 0
                OSAL_logMsg("coders list is:\n"
                        "pcmu(%d) priority(%d)\n"
                        "pcma(%d) priority(%d)\n"
                        "g726-32(%d) priority(%d)\n"
                        "g729(%d) priority(%d)\n"
                        "ilbc(%d) priority(%d)\n"
                        "silk(%d) priority(%d)\n"
                        "dtmf(%d) priority(%d)\n"
                        "cn  (%d) priority(%d)\n"
                        "h264(%d) priority(%d)\n",
                        "h263(%d) priority(%d)\n",
                        "amrnb(%d) priority(%d)\n",
                        "amrwb(%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMU_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMU_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMA_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMA_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G726_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG726_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G729_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG729_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_ILBC_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PILBC_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_SILK_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PSILK_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G722_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG722_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_DTMFR_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PDTMFR_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CN_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCN_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H264_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH264_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H263_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH263_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRNB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRNB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRWB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRWB_F)
                        );
#endif
                OSAL_logMsg("coders list is:\n");
                OSAL_logMsg("pcmu   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMU_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMU_F));
                OSAL_logMsg("pcma   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCMA_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PPCMA_F));
                OSAL_logMsg("g726-32(%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G726_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG726_F));
                OSAL_logMsg("g729   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G729_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG729_F));
                OSAL_logMsg("ilbc   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_ILBC_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PILBC_F));
                OSAL_logMsg("silk   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_SILK_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PSILK_F));
                OSAL_logMsg("g722   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_G722_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PG722_F));
                OSAL_logMsg("dtmf   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_DTMFR_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PDTMFR_F));
                OSAL_logMsg("cn     (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CN_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PCN_F));
                OSAL_logMsg("h264   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H264_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH264_F));
                OSAL_logMsg("h263   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_H263_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PH263_F));
                OSAL_logMsg("amrnb   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRNB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRNB_F));
                OSAL_logMsg("amrwb   (%d) priority(%d)\n",
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_AMRWB_F),
                        IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_PAMRWB_F));
                break;
            case 'f':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_SECURITY_F, buff);
                break;
            case 'g':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_NAME_F, buff);
                break;
            case 'h':
                OSAL_logMsg("Enter condition number:\n");
                OSAL_logMsg("0 = unconditional\n");
                OSAL_logMsg("1 = busy\n");
                OSAL_logMsg("2 = no answer\n");
                OSAL_logMsg("3 = not reachable\n");
                value = _IUT_menuScanl();
                OSAL_logMsg("Enter forward target:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appForward(hs_ptr, buff, value, 1, 0);
                break;
            case 'i':
                OSAL_logMsg("Enter condition number:\n");
                OSAL_logMsg("0 = unconditional\n");
                OSAL_logMsg("1 = busy\n");
                OSAL_logMsg("2 = no answer\n");
                OSAL_logMsg("3 = not reachable\n");
                value = _IUT_menuScanl();
                IUT_appForward(hs_ptr, NULL, value, 0, 0);
                break;
            case 'j':
                OSAL_logMsg("Enter interface ip address:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_IF_ADDRESS_F, buff);
                OSAL_logMsg("Enter interface name:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSetInterface(hs_ptr, buff, hs_ptr->interfaceAddress);
                break;
            case 'k':
                OSAL_logMsg("Enter isEmergency call:\n");
                OSAL_logMsg("0 = emergency off\n");
                OSAL_logMsg("1 = emergency on\n");
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId,
                        IUT_CFG_EMERGENCY_F, buff);
                break;
            case 'l':
                OSAL_logMsg("Enter IMEI:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId,
                        IUT_CFG_IMEI_F, buff);
                break;
            case 'm':
                OSAL_logMsg("Enter Type:\n");
                OSAL_logMsg("%d = SIP\n", ISI_PORT_TYPE_SIP);
                OSAL_logMsg("%d = AUDIO\n", ISI_PORT_TYPE_AUDIO);
                OSAL_logMsg("%d = VIDEO\n", ISI_PORT_TYPE_VIDEO);
                value = _IUT_menuScanl();
                OSAL_logMsg("Enter Port:\n");
                value2 = _IUT_menuScanl();
                OSAL_logMsg("Enter Pool Size:\n");
                value3 = _IUT_menuScanl();
                IUT_appSetPort(hs_ptr, value2, value3, value);
                break;
            case 'n':
                OSAL_logMsg("Enter RCS provisioning data xml filename:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSetProvisioningData(hs_ptr, buff);
                break;
            case 'o':
                OSAL_logMsg("Ipsec start port:\n");
                value = _IUT_menuScanl();
                OSAL_logMsg("Ipsec port poolsize:\n");
                value2 = _IUT_menuScanl();
                OSAL_logMsg("Ipsec start spi:\n");
                value3 = _IUT_menuScanl();
                OSAL_logMsg("Ipsec spi poolsize:\n");
                value4 = _IUT_menuScanl();
                IUT_appSetIpsec(hs_ptr, value, value2, value3, value4);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuCall() ========
 *
 * This function displays a menu and accepts input for call events related to
 * an ISI call control.
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuCall(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    char            ch;
    char            buff[IUT_STR_SIZE + 1];
    vint            mask = 0;
    vint            otherPeer;
    vint            type = 0;
    vint            dir = 0;

    while (1) {
        OSAL_logMsg( "<><><><> init call <><><><>\n"
                "0=to uri (current=%s)\n"
                "1=forward uri (current=%s)\n"
                "2=xfer uri (current=%s)\n"
                "3=ack\n"
                "4=terminate\n"
                "5=reject\n"
                "6=accept\n"
                "7=hold\n"
                "8=resume\n"
                "9=forward\n"
                "a=init audio only\n"
                "b=modify\n"
                "c=blind xfer\n"
                "d=attended xfer\n"
                "e=handoff\n"
                "f=init audio+video\n"
                "i=init audio conf\n"
                "j=consultative xfer\n"
                "k=modify audio only\n"
                "l=modify audio+video\n"
                "m=accept modify\n"
                "n=reject modify(audio only)\n"
                "o=accept audio only\n"
                "p=init video only\n"
                "r=init emergency audio\n"
                "q=quit\n",
                IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_URI_F),
                IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_FORWARD_URI_F),
                IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_XFER_URI_F));
        ch = _IUT_menuScanc();

        switch(ch) {
            case '0':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_TO_URI_F, buff);
                break;
            case '1':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_FORWARD_URI_F,
                        buff);
                break;
            case '2':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_XFER_URI_F, buff);
                break;
            case '3':
                IUT_appCallAck(hs_ptr, peer);
                break;
            case '4':
                IUT_appCallTerminate(hs_ptr, peer);
                break;
            case '5':
                IUT_appCallReject(hs_ptr, peer);
                break;
            case '6':
                IUT_appCallAccept(hs_ptr, peer);
                break;
            case '7':
                IUT_appCallHold(hs_ptr, peer);
                break;
            case '8':
                IUT_appCallResume(hs_ptr, peer);
                break;
            case '9':
                IUT_appCallForward(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_FORWARD_URI_F));
                break;
            case 'a':
                mask = ISI_SESSION_TYPE_AUDIO;
                if (0 != IUT_cfgGetIntField(hs_ptr->serviceId,
                        IUT_CFG_SECURITY_F)) {
                    mask |= ISI_SESSION_TYPE_SECURITY_AUDIO;
                }
                
                IUT_appCall(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_URI_F), mask, ISI_SESSION_DIR_SEND_RECV, 
                        ISI_SESSION_DIR_NONE);
                break;
            case 'b':
                OSAL_logMsg("enter session type(audio:1, video:2):\n");
                type = (int)_IUT_menuScanl();
                OSAL_logMsg("enter direction:"
                        "(inactive:0 sendonly:1 recvonly:2 sendrecv:3)\n");
                dir = (int)_IUT_menuScanl();
                IUT_appCallModifyDir(hs_ptr, peer, type, dir);
                break;
            case 'c':
                IUT_appCallBlindTransfer(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_XFER_URI_F));
                break;
            case 'd':
                IUT_appCallAttendTransfer(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_XFER_URI_F));
                break;
            case 'e':
                if (IUT_appCallHandoff(hs_ptr, peer) != IUT_OK) {
                    OSAL_logMsg("Failed to handoff a call\n");
                    break;
                }
                OSAL_logMsg("Placing a new call for handoff. "
                        "You will be directed to the main menu\n");
                return;
            case 'f':
                IUT_appCall(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_URI_F), ISI_SESSION_TYPE_AUDIO |
                        ISI_SESSION_TYPE_VIDEO, ISI_SESSION_DIR_SEND_RECV,
                        ISI_SESSION_DIR_SEND_RECV);
                break;
            case 'i':
                mask = ISI_SESSION_TYPE_AUDIO | ISI_SESSION_TYPE_CONFERENCE;
                if (0 != IUT_cfgGetIntField(hs_ptr->serviceId,
                        IUT_CFG_SECURITY_F)) {
                    mask |= ISI_SESSION_TYPE_SECURITY_AUDIO;
                }
                IUT_appCall(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_URI_F), mask, ISI_SESSION_DIR_SEND_RECV,
                        ISI_SESSION_DIR_NONE);
                break;
            case 'j':
                OSAL_logMsg(IUT_MENU_ENTER_PEER);
                otherPeer = _IUT_menuScanl();
                IUT_appCallConsultativeTransfer(hs_ptr, peer, otherPeer);
                break;
            case 'k':
                IUT_appCallModifyAudio(hs_ptr, peer);
                break;
            case 'l':
                IUT_appCallModifyVideo(hs_ptr, peer);
                break;
            case 'm':
                IUT_appCallModifyAccept(hs_ptr, peer);
                break;
            case 'n':
                IUT_appCallModifyReject(hs_ptr, peer);
                break;
            case 'o':
                IUT_appCallAcceptAudioOnly(hs_ptr, peer);
                break;
            case 'p':
                mask = ISI_SESSION_TYPE_VIDEO;
                if (0 != IUT_cfgGetIntField(hs_ptr->serviceId,
                        IUT_CFG_SECURITY_F)) {
                    mask |= ISI_SESSION_TYPE_SECURITY_VIDEO;
                }
                
                IUT_appCall(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_URI_F), mask, ISI_SESSION_DIR_NONE, 
                        ISI_SESSION_DIR_SEND_RECV);
                break;
            case 'r':
                mask = ISI_SESSION_TYPE_AUDIO | ISI_SESSION_TYPE_EMERGENCY;
                if (0 != IUT_cfgGetIntField(hs_ptr->serviceId,
                        IUT_CFG_SECURITY_F)) {
                    mask |= ISI_SESSION_TYPE_SECURITY_AUDIO;
                }
                
                IUT_appCall(hs_ptr, peer,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_URI_F), mask, ISI_SESSION_DIR_SEND_RECV, 
                        ISI_SESSION_DIR_NONE);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuEvent() ========
 *
 * This function displays a menu and accepts input for event commands
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuEvent(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    int  value;

    while (1) {
        OSAL_logMsg( "<><><><> event <><><><>\n"
                "0=DTMF send inband (peer=%d)\n"
                "1=DTMF send oob (peer=%d)\n"
                "2=Local Tone Generate (peer=%d)\n"
                "3=Local Tone Stop (peer=%d)\n"
                "4=Set Tone Duration in ms (duration=%d)\n"
                "5=Send 'flash hook' (peer=%d)\n"
                "q=quit\n", peer, peer, peer, peer, hs_ptr->toneDuration, peer);

        value = (int)_IUT_menuScanc();

        switch(value) {
            case '0':
                OSAL_logMsg("enter digit:\n");
                value = (int)_IUT_menuScanc();
                IUT_appDigit(hs_ptr, peer, (char)value, 0,
                        hs_ptr->toneDuration);
                break;
            case '1':
                OSAL_logMsg("enter digit:\n");
                value = (int)_IUT_menuScanc();
                IUT_appDigit(hs_ptr, peer, (char)value, 1,
                        hs_ptr->toneDuration);
                break;
            case '2':
                OSAL_logMsg("enter tone number [0-%d]\n", (ISI_TONE_LAST - 1));
                value = _IUT_menuScanl();
                IUT_appTone(hs_ptr, peer, value, hs_ptr->toneDuration);
                break;
            case '3':
                IUT_appTone(hs_ptr, peer, ISI_TONE_LAST, 0);
                break;
            case '4':
                OSAL_logMsg("Enter New Duration:\n");
                hs_ptr->toneDuration = _IUT_menuScanl();
                break;
            case '5':
                IUT_appFlashhook(hs_ptr, peer);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}
/*
 * ======== IUT_menuContentShare() ========
 *
 * This function displays a menu and accepts input for sending
 * files/images messages
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuContentShare(
    IUT_HandSetObj *hs_ptr)
{
    char   ch;
    char   msg[IUT_MAX_IM_MSG_SIZE + 1];
    char   buff[IUT_STR_SIZE + 1];
    char   subject[IUT_STR_SIZE + 1] = "Here is your message!";
    vint   attribute = ISI_FILE_ATTR_RENDER;
    vint   report = ISI_FILE_SUCCESS_REPORT|ISI_FILE_FAILURE_REPORT;

    if ((hs_ptr == NULL) || (IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F) == NULL)) {
        OSAL_logMsg("Content Share not ready.  Did you initialize ISI UT and then allocate and activate a service?\n");
        return;
    }

    while (1) {
        OSAL_logMsg( "<><><><> File Transfer <><><><>\n"
                "0=destination (%s)\n"
                "1=subject (%s)\n"
                "2=send file\n"
                "3=acknowledge a file transfer\n"
                "4=accept a file transfer\n"
                "5=reject a file transfer\n"
                "6=cancel a file transfer\n"
                "7=render(1) or attachment(2)? %d\n"
                "q=quit\n",
                 IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F), subject, attribute);

        ch = _IUT_menuScanc();

        OSAL_memSet(buff, 0, IUT_STR_SIZE + 1);
        OSAL_memSet(msg, 0, IUT_MAX_IM_MSG_SIZE + 1);
        switch(ch) {
            case '0':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F, buff);
                break;
            case '1':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(subject, IUT_STR_SIZE);
                break;
            case '2':
                OSAL_logMsg("Enter absolute filepath:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                OSAL_logMsg( "Enter File Type:\n"
                    "0=none\n"
                    "1=text/plain\n"
                    "2=image/jpg\n"
                    "3=image/gif\n"
                    "4=image/bpm\n"
                    "5=image/png\n"
                    "6=video/3gp\n"
                    "7=video/mp4\n"
                    "8=video/wmv\n"
                    "9=audio/amr\n");
                ch = _IUT_menuScanc();
                OSAL_logMsg( "Enter 1=Success-Report or 2=Failure-Report or 3=Both:\n");
                report = _IUT_menuScanl();
                IUT_appContentShareSendFile(hs_ptr,
                        IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F),  /* dest */
                        subject,       /* subject */
                        buff,          /* filename */
                        (int)(ch - 48),/* file type */
                        attribute,     /* Render or Attachment? */
                        report         /* Success Failure report */);
                break;
            case '3':
                IUT_appAcknowledgeFileTransfer(hs_ptr, hs_ptr->fileId);
                break;
            case '4':
                IUT_appAcceptFileTransfer(hs_ptr, hs_ptr->fileId);
                break;
            case '5':
                OSAL_logMsg("Enter reason for rejecting:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appRejectFileTransfer(hs_ptr, hs_ptr->fileId, buff);
                break;
            case '6':
                OSAL_logMsg("Enter reason for cancelling:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appCancelFileTransfer(hs_ptr, hs_ptr->fileId, buff);
                break;
            case '7':
                OSAL_logMsg("Enter 1=render or 2=attachment: \n");
                attribute = _IUT_menuScanl();
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}


/*
 * ======== _IUT_menuIm() ========
 *
 * This function displays a menu and accepts input for sending
 * instant/text messages
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuIm(
    IUT_HandSetObj *hs_ptr)
{
    char   ch;
    char   msg[IUT_MAX_IM_MSG_SIZE + 1];
    char   buff[IUT_STR_SIZE + 1];
    vint   len;
    char  *msg_ptr;
    vint deliveryreport = 0;
    vint displayreport = 0;

    while (1) {
        OSAL_logMsg( "<><><><> message <><><><>\n"
                "0=destination (%s)\n"
                "1=send message\n"
                "2=send large message\n"
                "3=delivery report toggle (%d)\n"
                "4=display report toggle (%d)\n"
                "q=quit\n",
                 IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F), deliveryreport, displayreport);

        ch = _IUT_menuScanc();

        OSAL_memSet(buff, 0, IUT_STR_SIZE + 1);
        OSAL_memSet(msg, 0, IUT_MAX_IM_MSG_SIZE + 1);
        switch(ch) {
            case '0':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F, buff);
                break;
            case '1':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(msg, IUT_MAX_IM_MSG_SIZE);
                IUT_appMessageSend(hs_ptr,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_MSG_URI_F), msg, deliveryreport, displayreport);
                break;
            case '2':
                OSAL_logMsg("Enter Message Length\n");
                if (0 == (len = _IUT_menuScanl())) {
                    OSAL_logMsg("Error sending message\n");
                    break;
                }
                if (NULL == (msg_ptr = OSAL_memAlloc(len + 1, 0))) {
                    OSAL_logMsg("Error sending message\n");
                    break;
                }
                /* Fill the buffer with something */
                OSAL_memSet(msg_ptr, 'd', (len - 1));
                *(msg_ptr + (len - 1)) = 't';
                /* NUll terminate */
                *(msg_ptr + len) = 0;
                IUT_appMessageSend(hs_ptr,
                        IUT_cfgGetStrField(hs_ptr->serviceId,
                        IUT_CFG_TO_MSG_URI_F), msg_ptr, deliveryreport, displayreport);
                /* Free the memory */
                OSAL_memFree(msg_ptr, 0);
                break;
            case '3':
                deliveryreport = (!deliveryreport);
                break;
            case '4':
                displayreport = (!displayreport);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuCapabilitiesExchange() ========
 *
 * This function displays a menu and accepts input for sending
 * capabilities exchange messages
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuCapabilitiesExchange(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    char   ch;
    char   xml[IUT_MAX_IM_MSG_SIZE + 1];
    char   line[IUT_STR_SIZE + 1];
    char   buff[IUT_STR_SIZE + 1];
    vint   len = 0;
    uint8  ipVoiceCall = 0;
    uint8  ipVideoCall = 0;
    uint8  chat = 0;
    uint8  imageShare = 0;
    uint8  videoShare = 0;
    uint8  fileTransfer = 0;
    
    if ((hs_ptr == NULL) || (IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F) == NULL)) {
        OSAL_logMsg("service not ready\n");
        return;
    }

    while (1) {
        OSAL_logMsg( "<><><><> capabilities exchange <><><><>\n"
                "0=destination (%s)\n"
                "====toggle options===\n"
                "1 = IP Voice Call: %d \n"
                "2 = IP Video Call: %d \n"
                "3 = IM:            %d \n"
                "4 = Image Share:   %d \n"
                "5 = Video Share:   %d \n"
                "6 = File Transfer: %d \n"
                "======================\n"
                "a=set capabilities\n"
                "b=send capabilities\n"
                "q=quit\n",
                 IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F),
                 ipVoiceCall,
                 ipVideoCall,
                 chat,
                 imageShare,
                 videoShare,
                 fileTransfer);

        ch = _IUT_menuScanc();

        OSAL_memSet(buff, 0, IUT_STR_SIZE + 1);
        OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
        OSAL_memSet(xml, 0, IUT_MAX_IM_MSG_SIZE + 1);
        len = 0;

        OSAL_strncpy(line, "<capabilities>\r\n", IUT_STR_SIZE);
        OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
        len += OSAL_strlen(line);

        if (ipVoiceCall) {
            OSAL_strncpy(line, "    <feature name=\"ip voice call\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        if (ipVideoCall) {
            OSAL_strncpy(line, "    <feature name=\"ip video call\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        if (chat) {
            OSAL_strncpy(line, "    <feature name=\"chat\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        if (imageShare) {
            OSAL_strncpy(line, "    <feature name=\"image share\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        if (videoShare) {
            OSAL_strncpy(line, "    <feature name=\"video share\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        if (fileTransfer) {
            OSAL_strncpy(line, "    <feature name=\"file transfer\"/>\r\n", IUT_STR_SIZE);
            OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
            len += OSAL_strlen(line);
        }

        OSAL_strncpy(line, "</capabilities>\r\n", IUT_STR_SIZE);
        OSAL_strncpy(&xml[len], line, IUT_MAX_IM_MSG_SIZE);
        len += OSAL_strlen(line);

        switch(ch) {
            case '0':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F, buff);
                break;
            case '1':
                ipVoiceCall = !(ipVoiceCall);
                break;
            case '2':
                ipVideoCall = !(ipVideoCall);
                break;
            case '3':
                chat = !(chat);
                break;
            case '4':
                imageShare = !(imageShare);
                break;
            case '5':
                videoShare = !(videoShare);
                break;
            case '6':
                fileTransfer = !(fileTransfer);
                break;
            case 'a':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                IUT_appCapabilitiesSet(hs_ptr, xml);
                break;
            case 'b':
                OSAL_logMsg(IUT_MENU_ENTER_VALUE);
                IUT_appCapabilitiesSend(hs_ptr,
                        IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_TO_MSG_URI_F),
                        peer,
                        xml);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuService() ========
 *
 * This function displays a menu and accepts input for configuring ISI services
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuService(
    IUT_HandSetObj *hs_ptr)
{
    vint    serviceId;
    vint    protocol;
    char    ch;

    if (hs_ptr == NULL) {
        serviceId = 0;
    }
    else {
        serviceId = (vint)hs_ptr->serviceId;
    }

    while (1) {
        OSAL_logMsg( "<><><><> service <><><><>\n"
                "0=free\n"
                "1=allocate\n"
                "2=activate\n"
                "3=deactivate\n"
                "4=set params\n"
                "5=get next service(current=%d)\n"
                "q=quit\n",
                serviceId);

        ch = _IUT_menuScanc();

        switch(ch) {
            case '0':
                if (hs_ptr == NULL) {
                    OSAL_logMsg(IUT_MENU_SERVICE_ERROR_STR);
                    break;
                }
                IUT_cfgClearServiceId(hs_ptr->serviceId);
                IUT_appServiceFree(hs_ptr);
                break;
            case '1':
                OSAL_logMsg("SIP=%d/XMPP=%d/GSM_AT=%d/YAHOO=%d/SAMETIME=%d/SKYPE=%d\n: ",
                        ISI_PROTOCOL_SIP,
                        ISI_PROTOCOL_XMPP,
                        ISI_PROTOCOL_GSM,
                        ISI_PROTOCOL_YAHOO,
                        ISI_PROTOCOL_SAMETIME,
                        6);
                protocol = _IUT_menuScanl();

                /* Try to find an available resource */
                hs_ptr = IUT_appServiceFind((ISI_Id)0);
                if (hs_ptr == NULL) {
                    OSAL_logMsg("Failed! No more resources"
                            " to accomodate this service\n");
                    break;
                }
                IUT_appServiceAlloc(hs_ptr, protocol);
                /*
                 * Set the last allocated service as the current
                 * active service
                 */
                IUT_appServiceSet(hs_ptr->serviceId);
                if (IUT_cfgSetServiceId(hs_ptr->serviceId) != IUT_OK) {
                    /* No configuration object to match */
                    OSAL_logMsg("Failed! No more config resources"
                            " to accomodate this service\n");
                    IUT_appServiceFree(hs_ptr);
                }
                break;
            case '2':
                if (hs_ptr == NULL) {
                    OSAL_logMsg(IUT_MENU_SERVICE_ERROR_STR);
                    break;
                }
                IUT_appSetActivation(hs_ptr, 1);
                break;
            case '3':
                if (hs_ptr == NULL) {
                    OSAL_logMsg(IUT_MENU_SERVICE_ERROR_STR);
                    break;
                }
                IUT_appSetActivation(hs_ptr, 0);
                break;
            case '4':
                if (hs_ptr == NULL) {
                    OSAL_logMsg(IUT_MENU_SERVICE_ERROR_STR);
                    break;
                }
                _IUT_menuParams(hs_ptr);
                break;
            case '5':
                if (NULL != (hs_ptr = IUT_appGetNextService(hs_ptr))) {
                    serviceId = hs_ptr->serviceId;
                }
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuPres() ========
 *
 * This function displays a menu and accepts input for stimulating
 * commands related to "presence". i.e. adding/removing contacts in
 * a Roster list, allowing/denying other contacts access to your
 * presence state, etc...
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuPres(
    IUT_HandSetObj *hs_ptr)
{
    char   buff[IUT_STR_SIZE + 1];
    char   to[IUT_STR_SIZE + 1];
    char   ch;
    long   entry;

    while (1) {
        OSAL_logMsg( "<><><><> service <><><><>\n"
                "0=send presence available\n"
                "1=send presence unavailable\n"
                "2=send presence dnd\n"
                "3=Add contact to roster\n"
                "4=Remove contact from roster\n"
                "5=print roster\n"
                "6=subscribe\n"
                "7=unsubscribe\n"
                "8=subscription accept\n"
                "9=subscription deny\n"
                "a=Friend Menu\n"
                "b=set active contact\n"
                "q=quit\n");

        ch = _IUT_menuScanc();
        switch(ch) {
            case '0':
                IUT_appPresenceSend(hs_ptr, 1, NULL);
                break;
            case '1':
                IUT_appPresenceSend(hs_ptr, 0, NULL);
                break;
            case '2':
                OSAL_logMsg("enter a status string:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                if (buff[0] == 0) {
                    IUT_appPresenceSend(hs_ptr, 2, NULL);
                }
                else {
                    IUT_appPresenceSend(hs_ptr, 2, buff);
                }
                break;
            case '3':
                /* Enter in te JID */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(to, IUT_STR_SIZE);
                
                /* Enter in an option group value */
                OSAL_logMsg(IUT_MENU_ENTER_GRP);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                
                IUT_appRosterAdd(hs_ptr, to, buff);
                break;
            case '4':
                /* Enter in te JID */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(to, IUT_STR_SIZE);
                
                /* Enter in an option group value */
                OSAL_logMsg(IUT_MENU_ENTER_GRP);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                
                IUT_appRosterRemove(hs_ptr, to, buff);
                break;
            case '5':
                IUT_prtRosterList(hs_ptr->roster);
                break;
            case '6':
                /* Get the jid of the desired entity */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSubscribe(hs_ptr, buff);
                break;
            case '7':
                /* Get the jid of the desired entity */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appUnsubscribe(hs_ptr, buff);
                break;
            case '8':
                /* Get the jid of the desired entity */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSubscribeAllow(hs_ptr, buff, 1);
                break;
            case '9':
                /* Get the jid of the desired entity */
                OSAL_logMsg(IUT_MENU_ENTER_JID);
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSubscribeAllow(hs_ptr, buff, 0);
                break;
            case 'a':
                OSAL_logMsg("UNSUPPORTED FEATURE\n");
                break;
            case 'b':
                if (hs_ptr->proto == ISI_PROTOCOL_XMPP) {
                    OSAL_logMsg("roster entry # :\n");
                    entry = _IUT_menuScanl();
                    if ((entry > IUT_MAX_NUM_ROSTERS) || (entry < 0)) {
                        OSAL_logMsg("Invalid entry\n");
                        break;
                    }
                    IUT_cfgSetField(hs_ptr->serviceId,
                            IUT_CFG_TO_URI_F, hs_ptr->roster[entry].contact);

                    IUT_cfgSetField(hs_ptr->serviceId,
                            IUT_CFG_TO_MSG_URI_F,
                            hs_ptr->roster[entry].contact);

                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_FORWARD_URI_F,
                            hs_ptr->roster[entry].contact);

                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_XFER_URI_F,
                            hs_ptr->roster[entry].contact);
                }
                else if (hs_ptr->proto == ISI_PROTOCOL_SIP) {
                    OSAL_logMsg("contact # :\n");
                    _IUT_menuScans(buff, sizeof(buff));

                    /*
                     * Copy the current 'to' field, this is because we
                     * can rewrite it in the next call
                     */
                    OSAL_strncpy(to, IUT_cfgGetStrField(hs_ptr->serviceId,
                            IUT_CFG_TO_URI_F), IUT_STR_SIZE);

                    _IUT_menuD2DialPlan(
                            IUT_cfgGetStrField(hs_ptr->serviceId,
                            IUT_CFG_PROXY_F), to, buff, IUT_STR_SIZE);

                    /* Now reset the 'to' field because it may have changed */
                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_TO_URI_F, to);

                    IUT_cfgSetField(hs_ptr->serviceId,
                            IUT_CFG_TO_MSG_URI_F, to);

                    IUT_cfgSetField(hs_ptr->serviceId,
                            IUT_CFG_FORWARD_URI_F, to);

                    IUT_cfgSetField(hs_ptr->serviceId, IUT_CFG_XFER_URI_F, to);
                }
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuConf() ========
 *
 * This function displays a menu and accepts input for controlling
 * call conferences.
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuConf(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    char            ch;
    IUT_HandSetObj *confHs_ptr;
    vint            confServiceId;
    vint            confPeer;
    char            buff[IUT_STR_SIZE + 1];

    while (1) {
        OSAL_logMsg( "<><><><> conference <><><><>\n"
                "0=add (peer=%d)\n"
                "1=remove (peer=%d)\n"
                "q=quit\n",
                peer,
                peer);

        ch = _IUT_menuScanc();

        switch(ch) {
            case '1':
            case '0':
                OSAL_logMsg("enter 'service' to conf in:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                confServiceId = OSAL_atoi(buff);
                OSAL_logMsg("enter 'peer' of service to conf in:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                confPeer = OSAL_atoi(buff);
                confHs_ptr = IUT_appServiceFind(confServiceId);
                if (confServiceId == 0 || confHs_ptr == NULL) {
                    OSAL_logMsg("The service ID you want to conference"
                            " in, is invalid!\n");
                    break;
                }
                if (ch == '0') {
                    IUT_appConfAdd(hs_ptr, peer, confHs_ptr, confPeer);
                }
                else {
                    IUT_appConfRemove(hs_ptr, peer, confHs_ptr, confPeer);
                }
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== _IUT_menuChat() ========
 *
 * This function displays a menu and accepts input for controlling
 * chat sessions.
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_menuChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    char            ch;
    char            buff[IUT_STR_SIZE + 1];
    char            buff2[IUT_STR_SIZE + 1];
    //uuid_t          myuuid;
    uint8           deliveryReports = 1;
    uint8           displayReports = 0;
    vint            bytes;
    vint            pos;
    vint            bytesLeft;
    vint            count;
    while (1) {
        OSAL_logMsg( "<><><><> Group Chat Room <><><><>\n"
                "0=create group chat room\n"
                "1=create Adhoc group chat\n"
                "2=initiate 1To1 chat\n"
                "3=destroy group chat\n"
                "4=invite someone\n"
                "5=send message\n"
                "6=send file\n"
                "7=kick someone\n"
                "8=join a room\n"
                "9=acknowledge chat\n"
                "a=accept\n"
                "b=disconnect chat\n"
                "c=reject\n"
                "d=set group chat room presence to available\n"
                "e=set group chat room presence to busy\n"
                "f=send large message\n"
                "g=toggle message delivery reports:%d\n"
                "h=toggle message display reports:%d\n"
                "i=composing message\n"
                "q=quit\n",
                deliveryReports,
                displayReports);

        ch = _IUT_menuScanc();

        switch(ch) {
            case '0':
                OSAL_logMsg("enter 'room name' to create:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                //uuid_generate(myuuid);
                //uuid_unparse(myuuid, buff);
                //OSAL_snprintf(buff2, IUT_STR_SIZE, "%s%s%s", "private-chat-", buff, "@groupchat.google.com");
                OSAL_snprintf(buff2, IUT_STR_SIZE, "%s", buff);
                //OSAL_logMsg("conference:%s\n", buff2);
                IUT_appCreateGroupChat(hs_ptr, peer, buff2, "Discussing Dow Jones");
                break;
            case '1':
                OSAL_logMsg("enter up to 3 remote party's to invite...\n");
                buff2[0] = 0;
                bytesLeft = IUT_STR_SIZE;
                pos = 0;
                count = 0;
                while (count < 3) {
                    OSAL_logMsg("enter #%d 'remote party' to contact or please <enter> for none:\n", count + 1);
                    _IUT_menuScans(buff, IUT_STR_SIZE);
                    if (0 == buff[0]) {
                        break;
                    }
                    bytes = OSAL_snprintf(buff2 + pos, bytesLeft, "%s,", buff);
                    if (bytes > bytesLeft) {
                        /* We are done...no more room. */
                        break;
                    }
                    bytesLeft -= bytes;
                    pos += bytes;
                    count++;
                }
                IUT_appCreateAdhocGroupChat(hs_ptr, peer, buff2, NULL, "Discussing Adhoc chat interworkings");
                break;
            case '2':
                OSAL_logMsg("enter address:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                OSAL_logMsg("enter a message to include or <enter> to ignore:\n");
                _IUT_menuScans(buff2, IUT_STR_SIZE);
                IUT_appInitiateChat(hs_ptr, peer, buff, "", buff2, deliveryReports, displayReports);
                break;
            case '3':
                IUT_appDestroyGroupChat(hs_ptr, peer,
                        "I'm Stephen The Destroyer!");
                break;
            case '4':
                OSAL_logMsg("enter address to invite:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appInviteGroupChat(hs_ptr, peer, buff,
                        "Please join our discussion");
                break;
            case '5':
                OSAL_logMsg("enter message:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appSendMessageChat(hs_ptr, peer, buff, deliveryReports, displayReports);
                break;
            case '6':
                OSAL_logMsg("Enter absolute filepath:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                OSAL_logMsg( "Enter File Type:\n"
                    "0=none\n"
                    "1=text/plain\n"
                    "2=image/jpg\n"
                    "3=image/gif\n"
                    "4=image/bpm\n"
                    "5=image/png\n"
                    "6=video/3gp\n"
                    "7=video/mp4\n"
                    "8=video/wmv\n"
                    "9=audio/amr\n");
                ch = _IUT_menuScanc();
                IUT_appSendFileChat(hs_ptr, peer, buff, (int)(ch - 48));
                break;
            case '7':
                OSAL_logMsg("enter address to 'kick':\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                IUT_appKickGroupChat(hs_ptr, peer, buff, "You're being rude");
                break;
            case '8':
                OSAL_logMsg("enter room name to join:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                OSAL_logMsg("enter a password or hit return for none:\n");
                _IUT_menuScans(buff2, IUT_STR_SIZE);
                IUT_appJoinGroupChat(hs_ptr, peer, buff, buff2);
                break;
            case '9':
                IUT_appAcknowledgeChat(hs_ptr, peer);
                break;
            case 'a':
                IUT_appAcceptChat(hs_ptr, peer);
                break;
            case 'b':
                IUT_appDisconnectChat(hs_ptr, peer);
                break;
            case 'c':
                IUT_appRejectChat(hs_ptr, peer);
                break;
            case 'd':
                IUT_appSendPresenceGroupChat(hs_ptr, peer, 1, "I'm available");
                break;
            case 'e':
                OSAL_logMsg("enter a status string:\n");
                _IUT_menuScans(buff, IUT_STR_SIZE);
                if (buff[0] == 0) {
                    IUT_appSendPresenceGroupChat(hs_ptr, peer, 2, NULL);
                }
                else {
                    IUT_appSendPresenceGroupChat(hs_ptr, peer, 2, buff);
                }
                break;
            case 'f':
                OSAL_logMsg("Sending %d bytes of data\n", sizeof(_IUT_MENU_LargeMsg));
                IUT_appSendMessageChat(hs_ptr, peer, (char *)_IUT_MENU_LargeMsg, deliveryReports, displayReports);
                break;
            case 'g':
                deliveryReports = (!deliveryReports);
                break;
            case 'h':
                displayReports = (!displayReports);
                break;
            case 'i':
                IUT_appComposingMessageChat(hs_ptr, peer);
                break;
            case 'q':
                return;
            default:
                break;
        }
    }
}

/*
 * ======== IUT_menuMain() ========
 *
 * This function displays the menu and accepts input for selecting
 * other nested menus.
 *
 * The menu runs in a loop until the user selects 'q' to exit this menu.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_menuMain(void)
{
    char            ch;
    vint            serviceId;
    IUT_HandSetObj *hs_ptr;
    vint            peer;

    while (1) {

        hs_ptr  = IUT_appServiceGet();
        peer    = IUT_appPeerGet();

        if (hs_ptr == NULL) {
            serviceId = 0;
        }
        else {
            serviceId = (vint)hs_ptr->serviceId;
        }
        OSAL_logMsg( "<><><><> main <><><><>\n"
                "0=init\n"
                "1=shut\n"
                "2=service\n"
                "3=call\n"
                "4=presence\n"
                "5=event\n"
                "6=message\n"
                "7=conference\n"
                "8=Chat\n"
                "9=set peer (current=%d)\n"
                "a=set service (current=%d)\n"
                "b=print all available protocols and services\n"
                "c=save %s\n"
                "d=perform API test. This is for experts only\n"
                "e=file transfer (file/image share)\n"
                "f=capabilities exchange\n"
                "g=set feature\n"
                "q=quit\n",
                peer,
                serviceId,
                IUT_CONFIG_FILE_NAME);
        ch = _IUT_menuScanc();

        if (ch > '2' && ch < '8' && ch != 'c') {
            if (hs_ptr == NULL) {
                OSAL_logMsg(IUT_MENU_SERVICE_ERROR_STR);
                continue;
            }
        }

        switch(ch) {
            case '0':
                IUT_appInit();
                break;
            case '1':
                IUT_appShutdown();
                break;
            case '2':
                _IUT_menuService(hs_ptr);
                break;
            case '3':
                _IUT_menuCall(hs_ptr, peer);
                break;
            case '4':
                _IUT_menuPres(hs_ptr);
                break;
            case '5':
                _IUT_menuEvent(hs_ptr, peer);
                break;
            case '6':
                _IUT_menuIm(hs_ptr);
                break;
            case '7':
                _IUT_menuConf(hs_ptr, peer);
                break;
            case '8':
                _IUT_menuChat(hs_ptr, peer);
                break;
            case '9':
                OSAL_logMsg("enter peer:\n");
                ch = _IUT_menuScanc();
                IUT_appPeerSet(ch - '0');
                break;
            case 'a':
                OSAL_logMsg("enter service:\n");
                ch = _IUT_menuScanc();
                IUT_appServiceSet(ch - '0');
                break;
            case 'b':
                IUT_appServicePrint();
                break;
            case 'c':
                IUT_cfgWrite(NULL);
                OSAL_logMsg("Done saving state file\n");
                _IUT_menuScanc();
                break;
            case 'd':
                IUT_apiMainMenu(hs_ptr);
                break;
            case 'e':
                _IUT_menuContentShare(hs_ptr);
                break;
            case 'f':
                _IUT_menuCapabilitiesExchange(hs_ptr, peer);
                break;
            case 'g':
                OSAL_logMsg("enter feature bit mask(VoLTE call:1, VoLTE SMS:2, RCS:4)\n");
                ch = _IUT_menuScanc();
                IUT_appSetFeature(ch - '0');
                break;
            case 'q':
                OSAL_logMsg("ARE YOU SURE!!?? <0/1>:\n");
                ch = _IUT_menuScanc();
                if (ch == '1') {
                    return;
                }
                break;
            default:
                break;
        }
    }
}


