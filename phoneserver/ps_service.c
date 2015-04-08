/*
 *
 * cmux.c: channel mux implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include "os_api.h"
#include "ps_service.h"
#include "pty.h"
#include "at_tok.h"
#include <cutils/properties.h>

#undef  PHS_LOGD
#define PHS_LOGD(x...)  ALOGD( x )

#define ETH_TD  "ro.modem.t.eth"
#define ETH_W  "ro.modem.w.eth"
#define ETH_L  "ro.modem.l.eth"
#define ETH_TL  "ro.modem.tl.eth"
#define ETH_LF  "ro.modem.lf.eth"

#define SYS_IFCONFIG_UP "sys.ifconfig.up"
#define SYS_IP_SET "sys.data.setip"
#define SYS_IP_CLEAR "sys.data.clearip"
#define SYS_MTU_SET "sys.data.setmtu"
#define SYS_IFCONFIG_DOWN "sys.ifconfig.down"
#define SYS_NO_ARP "sys.data.noarp"
#define SYS_IPV6_DISABLE "sys.data.IPV6.disable"
#define SYS_NET_ADDR "sys.data.net.addr"
#define RETRY_MAX_COUNT 1000
#define DEFAULT_PUBLIC_DNS2 "204.117.214.10"
//Due to real network limited, 2409:8084:8000:0010:2002:4860:4860:8888 maybe not correct
#define DEFAULT_PUBLIC_DNS2_IPV6 "2409:8084:8000:0010:2002:4860:4860:8888"
#define SSDA_MODE         "persist.radio.ssda.mode"
#define SSDA_TESTMODE "persist.radio.ssda.testmode"

#define   SYS_GSPS_ETH_UP_PROP        "ril.gsps.eth.up"
#define   SYS_GSPS_ETH_DOWN_PROP      "ril.gsps.eth.down"
#define   SYS_GSPS_ETH_IFNAME_PROP    "sys.gsps.eth.ifname"
#define   SYS_GSPS_ETH_LOCALIP_PROP   "sys.gsps.eth.localip"
#define   SYS_GSPS_ETH_PEERIP_PROP    "sys.gsps.eth.peerip"
#define   DHCP_DNSMASQ_LEASES_FILE    "/data/misc/dhcp/dnsmasq.leases"

struct ppp_info_struct ppp_info[MAX_PPP_NUM];
static char sSavedDns[IP_ADD_SIZE] = {0};
static char sSavedDns_IPV6[IP_ADD_SIZE*4] ={0};

mutex ps_service_mutex;
extern const char *modem;

extern int findInBuf(char *buf, int len, char *needle);

static int parse_peer_ip(char* ipaddr){

    FILE* fp;
    char line[1024];
    char* ptr, *p;

    if ((fp = fopen(DHCP_DNSMASQ_LEASES_FILE, "r")) == NULL) {
        PHS_LOGE("Fail to open %s, error : %s.", DHCP_DNSMASQ_LEASES_FILE, strerror(errno));
        return -1;
    }

    while(fgets(line, sizeof(line), fp) != NULL) {
        if ((ptr = strstr(line, "192.168.42.")) != NULL) {
            p = strchr(ptr, ' ');
            if (p == NULL) 
                return -1;
            *p = '\0';
            strcpy(ipaddr, ptr);
            PHS_LOGD("IP address : %s", ipaddr);
            return 0;
        }
    }
    return -1;
}

void ps_service_init(void)
{
    int i,maxPDPNum;

    maxPDPNum = getMaxPDPNum();
    memset(ppp_info,0x0,sizeof(ppp_info));
    for(i=0;i<maxPDPNum;i++) {
        ppp_info[i].state = PPP_STATE_IDLE;
        cond_init(&ppp_info[i].cond_timeout, NULL);
        mutex_init(&ppp_info[i].mutex_timeout,NULL);
    }

    mutex_init(&ps_service_mutex,NULL);
}

int get_ipv6addr(const char *prop, int cid)
{
    char iface[128] = {0};
    char linker[128] = {0};
    snprintf(iface, sizeof(iface), "%s%d", prop, cid-1);
    PHS_LOGD("query interface %s",iface);

    int max_retry = 120;//wait 12s
    int retry = 0;
    int setup_success = 0;
    while(!setup_success)
    {
        char rawaddrstr[INET6_ADDRSTRLEN], addrstr[INET6_ADDRSTRLEN];
        unsigned int prefixlen;
        int lasterror = 0, i, j, ret;
        char ifname[64];  // Currently, IFNAMSIZ = 16.
        FILE *f = fopen("/proc/net/if_inet6", "r");
        if (!f) {
            return -errno;
        }

        // Format:
        // 20010db8000a0001fc446aa4b5b347ed 03 40 00 01    wlan0
        while (fscanf(f, "%32s %*02x %02x %*02x %*02x %63s\n",
              rawaddrstr, &prefixlen, ifname) == 3) {
            // Is this the interface we're looking for?
            if (strcmp(iface, ifname)) {
                continue;
            }

            // Put the colons back into the address.
            for (i = 0, j = 0; i < 32; i++, j++) {
                addrstr[j] = rawaddrstr[i];
                if (i % 4 == 3) {
                    addrstr[++j] = ':';
                }
            }
            addrstr[j - 1] = '\0';

            PHS_LOGD("getipv6addr found ip %s",addrstr);
            // Don't add the link-local address
            if (strncmp(addrstr, "fe80:", 5) == 0) {
                PHS_LOGD("getipv6addr found fe80");
                continue;
            }
            char cmd[MAX_CMD * 2];
            sprintf(cmd, "setprop net.%s%d.ipv6_ip %s", prop, cid-1,addrstr);
            system(cmd);
            PHS_LOGD("getipv6addr propset %s ",cmd);
            setup_success = 1;
            break;

        }

        fclose(f);
        if(!setup_success)
        {
            usleep(100*1000);
            retry++;
        }
        if(retry == max_retry)
            break;
    }
    return setup_success;
}

int cvt_cgdata_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    char *at_in_str, *out;
    char buffer[50], error_str[30];
    char at_cmd_str[MAX_AT_CMD_LEN];
    int cid, ppp_index;
    int err, ret;

    PHS_LOGD("enter cvt_cgdata_set_req  ");
    if (req == NULL) {
        PHS_LOGD("leave cvt_cgdata_set_req AT_RESULT_NG");
        return AT_RESULT_NG;
    }
    PHS_LOGD("enter cvt_cgdata_set_req cmd:%s cmdlen:%d  ",req->cmd_str, req->len);
    memset(buffer,0,sizeof(buffer));
    memset(at_cmd_str,0,MAX_AT_CMD_LEN);

    at_in_str = buffer;
    strncpy(at_in_str, req->cmd_str, req->len);

    err = at_tok_start(&at_in_str, '=');
    if (err < 0) {
        PHS_LOGD("parse cmd error");
        return AT_RESULT_NG;
    }

    /*get L2P */
    err = at_tok_nextstr(&at_in_str, &out);
    if (err < 0) {
        PHS_LOGD("parse cmd error");
        return AT_RESULT_NG;
    }

    /*Get cid */
    err = at_tok_nextint(&at_in_str, &cid);
    if (err < 0) {
        PHS_LOGD("parse cmd error");
        return AT_RESULT_NG;
    }
    ppp_index = cid - 1;

    mutex_lock(&ps_service_mutex);
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    ppp_info[ppp_index].state = PPP_STATE_ACTING;
    PHS_LOGD("PPP_STATE_ACTING");
    ppp_info[ppp_index].pty = req->recv_pty;
    ppp_info[ppp_index].cmux = mux;
    ppp_info[ppp_index].cid = cid;
    ppp_info[ppp_index].error_num = -1;
    adapter_cmux_register_callback(mux,
            (void *)cvt_cgdata_set_rsp,
            ppp_index);
    ret = adapter_cmux_write_for_ps(mux, req->cmd_str, req->len, req->timeout);

    PHS_LOGD("PDP activate result:ret = %d,state=%d",ret,ppp_info[ppp_index].state);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("PDP activate timeout ");
        ppp_info[ppp_index].state = PPP_STATE_IDLE;
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }

    if(ppp_info[ppp_index].state != PPP_STATE_CONNECT)
    {
        PHS_LOGD("PDP activate error :%d",ppp_info[ppp_index].state);
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        if(ppp_info[ppp_index].error_num >= 0) {
            sprintf(error_str, "+CME ERROR: %d\r", ppp_info[ppp_index].error_num);
            adapter_pty_write(req->recv_pty, error_str, strlen(error_str));
        } else
            adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
        ppp_info[ppp_index].state = PPP_STATE_IDLE;
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }
    ppp_info[ppp_index].state = PPP_STATE_ESTING;
    ppp_info[ppp_index].manual_dns = 0;

/*    if (!isLte()) {
        snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+SIPCONFIG=%d\r",cid);
        adapter_cmux_register_callback(mux,
                (void *)cvt_sipconfig_rsp,
                ppp_index);
        ret = adapter_cmux_write_for_ps( mux, at_cmd_str, strlen(at_cmd_str), 10);
        if(ret == AT_RESULT_TIMEOUT)
        {
            PHS_LOGD("Get IP address timeout ");
            ppp_info[ppp_index].state = PPP_STATE_IDLE;
            adapter_pty_end_cmd(req->recv_pty );
            adapter_free_cmux_for_ps(mux);
            adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
            mutex_unlock(&ps_service_mutex);
            return AT_RESULT_OK;
        }
        if(ppp_info[ppp_index].state != PPP_STATE_ACTIVE)
        {
            PHS_LOGD("Getting IP addr and PDP activate error :%d",ppp_info[ppp_index].state);
            adapter_pty_end_cmd(req->recv_pty );
            adapter_free_cmux_for_ps(mux);
            adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
            ppp_info[ppp_index].state = PPP_STATE_IDLE;
            mutex_unlock(&ps_service_mutex);
            return AT_RESULT_OK;
        }
        if(ppp_info[ppp_index].state == PPP_STATE_ACTIVE)
        {
            PHS_LOGD("PS connected successful");
            adapter_pty_end_cmd(req->recv_pty );
            adapter_free_cmux_for_ps(mux);
            adapter_pty_write(req->recv_pty,"CONNECT\r",strlen("CONNECT\r"));
            mutex_unlock(&ps_service_mutex);
            return AT_RESULT_OK;
        }
    } else {*/
        int i, ip_state, count = 0;
        char prop[PROPERTY_VALUE_MAX];
        char linker[128] = {0};
        char ipv6_dhcpcd_cmd[128] = {0};
        if (!isLte()){
            snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+SIPCONFIG=%d\r",cid);
        }else{
            snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGCONTRDP=%d\r",cid);
        }
        adapter_cmux_register_callback(mux,
                (void *)cvt_cgcontrdp_rsp,
                ppp_index);
        ret = adapter_cmux_write_for_ps( mux, at_cmd_str, strlen(at_cmd_str), 10);
        if(ret == AT_RESULT_TIMEOUT) {
            PHS_LOGD("Get IP address timeout ");
            PHS_LOGD("PPP_STATE_DEACTING");
            ppp_info[ppp_index].state = PPP_STATE_DEACTING;
            snprintf(at_cmd_str,sizeof(at_cmd_str), "AT+CGACT=0,%d\r",cid);
            adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp1, (unsigned long)req->recv_pty);
            ret = adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                    req->timeout);
            if(ret == AT_RESULT_TIMEOUT) {
                PHS_LOGD("PDP deactivate timeout ");

                ppp_info[ppp_index].state = PPP_STATE_IDLE;
                adapter_pty_end_cmd(req->recv_pty );
                adapter_free_cmux_for_ps(mux);
                adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
                mutex_unlock(&ps_service_mutex);
                return AT_RESULT_OK;
            }
        }

        if(ppp_info[ppp_index].state == PPP_STATE_ACTIVE)
        {
            PHS_LOGD("PS connected successful");
            if(!strcmp(modem, "t")) {
                property_get(ETH_TD, prop, "veth");
            } else if(!strcmp(modem, "w")) {
                property_get(ETH_W, prop, "veth");
            } else if(!strcmp(modem, "l")) {
                property_get(ETH_L, prop, "veth");
            } else if(!strcmp(modem, "tl")) {
                property_get(ETH_TL, prop, "veth");
            } else if(!strcmp(modem, "lf")) {
                property_get(ETH_LF, prop, "veth");
            } else {
                PHS_LOGE("Unknown modem type, exit--4");
                exit(-1);
            }

            PHS_LOGD("PS ip_state = %d",ppp_info[cid-1].ip_state);

            if (ppp_info[cid-1].ip_state == IPV4V6) {
                snprintf(linker, sizeof(linker), "addr add %s/64 dev %s%d", ppp_info[cid-1].ipv6laddr, prop, cid-1);
                property_set(SYS_IP_SET, linker);
                PHS_LOGD("IPV6 setip linker = %s", linker);

                snprintf(linker, sizeof(linker), "link set %s%d mtu 1400", prop, cid-1);
                property_set(SYS_MTU_SET, linker);
                PHS_LOGD("IPV6 setmtu linker = %s", linker);

                //snprintf(linker, sizeof(linker), "link set %s%d arp off", prop, cid-1);
                //property_set(SYS_NO_ARP, linker);
                //PHS_LOGD("IPV6 arp linker = %s", linker);

                // up the net interface
                snprintf(linker, sizeof(linker), "link set %s%d up", prop, cid-1);
                property_set(SYS_IFCONFIG_UP, linker);
                PHS_LOGD("IPV6 setip linker = %s", linker);

                //able IPV6
                snprintf(linker, sizeof(linker), "0");
                property_set(SYS_IPV6_DISABLE, linker);
                PHS_LOGD("IPV6 able linker = %s", linker);
                //set net card addr
                snprintf(linker, sizeof(linker), "%s%d", prop, cid-1);
                property_set(SYS_NET_ADDR, linker);
                PHS_LOGD("Netcard addr linker = %s", linker);


                /* start data_on */
                property_set("ctl.start", "data_on");
                /* wait up to 10s for data_on execute complete */
                do {
                    property_get(SYS_IFCONFIG_UP, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_on exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);

                snprintf(ipv6_dhcpcd_cmd, sizeof(ipv6_dhcpcd_cmd), "dhcpcd_ipv6:%s%d", prop, cid-1);
                property_set("ctl.start", ipv6_dhcpcd_cmd);
                if(!get_ipv6addr(prop,cid))
                {
                    PHS_LOGD("getipv6addr state ipv4v6 get IPv6 address timeout ,just use ipv4");
                    // Just use IPV4
                    ppp_info[cid-1].ip_state =IPV4;
                    /*
                    ppp_info[ppp_index].state = PPP_STATE_IDLE;
                    adapter_pty_end_cmd(req->recv_pty );
                    adapter_free_cmux_for_ps(mux);
                    adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
                    mutex_unlock(&ps_service_mutex);
                    return AT_RESULT_OK;
                    */
                }else{
                    PHS_LOGD("IPV6 data_on execute done");

                    usleep(100*1000);
                }

                // set ip addr mtu to check
                snprintf(linker, sizeof(linker), "addr add %s dev %s%d", ppp_info[cid-1].ipladdr, prop, cid-1);
                property_set(SYS_IFCONFIG_UP, linker);
                PHS_LOGD("IPV4 setip linker = %s", linker);

                snprintf(linker, sizeof(linker), "link set %s%d mtu 1400", prop, cid-1);
                property_set(SYS_MTU_SET, linker);
                PHS_LOGD("IPV6 setmtu linker = %s", linker);

                // no arp
                snprintf(linker, sizeof(linker), "link set %s%d arp off", prop, cid-1);
                property_set(SYS_NO_ARP, linker);
                PHS_LOGD("IPV4 arp linker = %s", linker);

                /* start data_on */
                property_set("ctl.start", "data_on");
                /* wait up to 10s for data_on execute complete */
                do {
                    property_get(SYS_IFCONFIG_UP, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_on exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);
                PHS_LOGD("IPV4 data_on execute done");
            } else {
                if (ppp_info[cid-1].ip_state == IPV4) {
                    /* set property */
                    snprintf(linker, sizeof(linker), "addr add %s dev %s%d", ppp_info[cid-1].ipladdr, prop, cid-1);
                    property_set(SYS_IP_SET, linker);
                    PHS_LOGD("IPV4 setip linker = %s", linker);

                    snprintf(linker, sizeof(linker), "link set %s%d mtu 1400", prop, cid-1);
                    property_set(SYS_MTU_SET, linker);
                    PHS_LOGD("IPV6 setmtu linker = %s", linker);

                    // up the net interface
                    snprintf(linker, sizeof(linker), "link set %s%d up", prop, cid-1);
                    property_set(SYS_IFCONFIG_UP, linker);
                    PHS_LOGD("IPV4 ifconfig linker = %s", linker);

                    // no arp
                    snprintf(linker, sizeof(linker), "link set %s%d arp off", prop, cid-1);
                    property_set(SYS_NO_ARP, linker);
                    PHS_LOGD("IPV4 arp linker = %s", linker);

                    //disable IPV6
                    snprintf(linker, sizeof(linker), "1");
                    property_set(SYS_IPV6_DISABLE, linker);
                    PHS_LOGD("IPV6 disable linker = %s", linker);

                } else if (ppp_info[cid-1].ip_state == IPV6) {
                    snprintf(linker, sizeof(linker), "addr add %s/64 dev %s%d", ppp_info[cid-1].ipv6laddr, prop, cid-1);
                    property_set(SYS_IP_SET, linker);
                    PHS_LOGD("IPV6 setip linker = %s", linker);

                    snprintf(linker, sizeof(linker), "link set %s%d mtu 1400", prop, cid-1);
                    property_set(SYS_MTU_SET, linker);
                    PHS_LOGD("IPV6 setmtu linker = %s", linker);

                    //up the net interface
                    snprintf(linker, sizeof(linker), "link set %s%d up", prop, cid-1);
                    property_set(SYS_IFCONFIG_UP, linker);
                    PHS_LOGD("IPV6 ifconfig linker = %s", linker);

                    //no arp
                    snprintf(linker, sizeof(linker), "link set %s%d arp off", prop, cid-1);
                    property_set(SYS_NO_ARP, linker);
                    PHS_LOGD("IPV6 arp linker = %s", linker);

                    //able IPV6
                    snprintf(linker, sizeof(linker), "0");
                    property_set(SYS_IPV6_DISABLE, linker);
                    PHS_LOGD("IPV6 disable linker = %s", linker);
                }
                // set net card addr
                snprintf(linker, sizeof(linker), "%s%d", prop, cid-1);
                property_set(SYS_NET_ADDR, linker);
                PHS_LOGD("Netcard addr linker = %s", linker);


                char gspsprop[PROPERTY_VALUE_MAX];
                property_get(SYS_GSPS_ETH_UP_PROP, gspsprop, "0");
                if (atoi(gspsprop)) {
                    char peerip[64]="";
                    char devname[64]={0};
                    if (parse_peer_ip(peerip) == 0){
                        snprintf(devname, sizeof(devname), "%s%d",  prop, cid-1);
                        property_set(SYS_GSPS_ETH_IFNAME_PROP, devname);
                        property_set(SYS_GSPS_ETH_LOCALIP_PROP, ppp_info[cid-1].ipladdr);
                        property_set(SYS_GSPS_ETH_PEERIP_PROP,  peerip);
                    }
                }

                /* start data_on */
                property_set("ctl.start", "data_on");
                /* wait up to 10s for data_on execute complete */
                do {
                    property_get(SYS_IFCONFIG_UP, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_on exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);

                if (ppp_info[cid-1].ip_state == IPV6) {
                    snprintf(ipv6_dhcpcd_cmd, sizeof(ipv6_dhcpcd_cmd), "dhcpcd_ipv6:%s%d", prop, cid-1);
                    property_set("ctl.start", ipv6_dhcpcd_cmd);
                    if(!get_ipv6addr(prop,cid))
                    {
                        PHS_LOGD("getipv6addr state v6 get IPv6 address timeout ");
                        ppp_info[ppp_index].state = PPP_STATE_IDLE;
                        adapter_pty_end_cmd(req->recv_pty );
                        adapter_free_cmux_for_ps(mux);
                        adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
                        mutex_unlock(&ps_service_mutex);
                        return AT_RESULT_OK;
                    }
                }

                PHS_LOGD("data_on execute done");
            }

            adapter_pty_end_cmd(req->recv_pty );
            adapter_free_cmux_for_ps(mux);
            adapter_pty_write(req->recv_pty,"CONNECT\r",strlen("CONNECT\r"));
            mutex_unlock(&ps_service_mutex);
            return AT_RESULT_OK;
        }
//    }
    adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
    PHS_LOGD("Getting IP addr and PDP activate error :%d",ppp_info[ppp_index].state);
    ppp_info[ppp_index].state = PPP_STATE_IDLE;
    PHS_LOGD("PPP_STATE_IDLE");
    adapter_pty_end_cmd(req->recv_pty );
    adapter_free_cmux_for_ps(mux);
    mutex_unlock(&ps_service_mutex);
    return AT_RESULT_OK;
}

void ip_hex_to_str(unsigned int in, char *out, int out_size)
{
    int i;
    unsigned int mid;
    char str[10];

    for(i=3; i >= 0; i--) {
        mid = (in & (0xff << 8*i)) >> 8*i;
        sprintf(str,"%u", mid);
        if(i == 3){
            strncpy(out, str, out_size);
            out[out_size-1] = '\0';
        } else {
            strcat(out, str);
        }
        if(i != 0)
            strcat(out,".");
    }
}

void cvt_reset_dns2(char *out)
{
    if (strlen(sSavedDns) > 0) {
        PHS_LOGD("Use saved DNS2 instead.");
        memcpy(out, sSavedDns, sizeof(sSavedDns));
    } else {
        PHS_LOGD("Use default DNS2 instead.");
        sprintf(out,"%s", DEFAULT_PUBLIC_DNS2);
    }
}

int cvt_sipconfig_rsp(AT_CMD_RSP_T * rsp,
		unsigned long __attribute__((unused)) user_data)
{
    int ret;
    int err;
    char *input;
    int cid,nsapi;
    char *out;
    int ip_hex,dns1_hex, dns2_hex;
    char ip[IP_ADD_SIZE],dns1[IP_ADD_SIZE], dns2[IP_ADD_SIZE];
    char cmd[MAX_CMD * 2];
    char prop[PROPERTY_VALUE_MAX];
    char linker[128] = {0};
    int count = 0;

    PHS_LOGD("%s enter", __FUNCTION__);

    if (rsp == NULL) {
        PHS_LOGD("leave cvt_ipconf_rsp:AT_RESULT_NG");
        return AT_RESULT_NG;
    }

    memset(ip, 0, IP_ADD_SIZE);
    memset(dns1, 0, IP_ADD_SIZE);
    memset(dns2, 0, IP_ADD_SIZE);
    input = rsp->rsp_str;
    input[rsp->len-1] ='\0';
    if (findInBuf(input, rsp->len, "+SIPCONFIG")) {

        do {
            err = at_tok_start(&input, ':');
            if (err < 0) {
                break;
            }
            err = at_tok_nextint(&input, &cid);
            if (err < 0) {
                break;
            }
            err = at_tok_nextint(&input, &nsapi);
            if (err < 0) {
                break;
            }
            err = at_tok_nexthexint(&input, &ip_hex);	//ip
            if (err < 0) {
                break;
            }
            ip_hex_to_str(ip_hex,ip,sizeof(ip));
            err = at_tok_nexthexint(&input, &dns1_hex);	//dns1
            if (err < 0) {
                break;
            }
            else if(dns1_hex != 0x0) {
                ip_hex_to_str(dns1_hex,dns1,sizeof(dns1));
            }
            err = at_tok_nexthexint(&input, &dns2_hex);	//dns2
            if (err < 0) {
                break;
            } else if(dns2_hex != 0x0) {
                ip_hex_to_str(dns2_hex,dns2,sizeof(dns2));
            }
            if ((cid < getMaxPDPNum()) && (cid >=1)){
                /*Save ppp info */
                strlcpy(ppp_info[cid-1].ipladdr, ip,sizeof(ppp_info[cid-1].ipladdr));

                strlcpy(ppp_info[cid-1].dns1addr, dns1, sizeof(ppp_info[cid-1].dns1addr));

                strlcpy(ppp_info[cid-1].dns2addr, dns2, sizeof(ppp_info[cid-1].dns2addr));

                if (!strncasecmp(dns1, "0.0.0.0", sizeof("0.0.0.0"))) {	//no return dns
                    strlcpy(ppp_info[cid-1].dns1addr,ppp_info[cid-1].userdns1addr,sizeof(ppp_info[cid-1].dns1addr));
                }
                if (!strncasecmp(dns2, "0.0.0.0", sizeof("0.0.0.0"))) {	//no return dns
                    strlcpy(ppp_info[cid-1].dns2addr,ppp_info[cid-1].userdns2addr,sizeof(ppp_info[cid-1].dns2addr));
                }

                if(!strcmp(modem, "t")) {
                    property_get(ETH_TD, prop, "veth");
                } else if(!strcmp(modem, "w")) {
                    property_get(ETH_W, prop, "veth");
                } else if(!strcmp(modem, "l")) {
                    //to do
                } else if(!strcmp(modem, "tl")) {
                    property_get(ETH_TL, prop, "veth");
                } else if(!strcmp(modem, "lf")) {
                    property_get(ETH_LF, prop, "veth");
                } else {
                    PHS_LOGE("Unknown modem type, exit");
                    exit(-1);
                }
                /* set property */
                snprintf(linker, sizeof(linker), "addr add %s dev %s%d", ip, prop, cid-1);
                property_set(SYS_IP_SET, linker);
                PHS_LOGD("setip linker = %s", linker);

                snprintf(linker, sizeof(linker), "link set %s%d mtu 1400", prop, cid-1);
                property_set(SYS_MTU_SET, linker);
                PHS_LOGD("setmtu linker = %s", linker);

                snprintf(linker, sizeof(linker), "link set %s%d up", prop, cid-1);
                property_set(SYS_IFCONFIG_UP, linker);
                PHS_LOGD("setup linker = %s", linker);

                snprintf(linker, sizeof(linker), "link set %s%d arp off", prop, cid-1);
                property_set(SYS_NO_ARP, linker);
                PHS_LOGD("IPV4 arp linker = %s", linker);

                /* start data_on */
                property_set("ctl.start", "data_on");
                /* wait up to 10s for data_on execute complete */
                do {
                    property_get(SYS_IFCONFIG_UP, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_on exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);

                PHS_LOGD("data_on execute done");

                sprintf(cmd, "setprop net.%s%d.ip %s", prop, cid-1,ip);
                system(cmd);
                if(dns1_hex != 0x0)
                    sprintf(cmd, "setprop net.%s%d.dns1 %s", prop, cid-1, dns1);
                else
                    sprintf(cmd, "setprop net.%s%d.dns1 \"\"", prop, cid-1);
                system(cmd);
                if(dns2_hex != 0x0) {
                    if(!strcmp(dns1, dns2)) {
                        PHS_LOGD("Two DNS are the same,so need to reset dns2!!");
                        cvt_reset_dns2(dns2);
                    } else {
                        PHS_LOGD("Backup DNS2");
                        memset(sSavedDns, 0, sizeof(sSavedDns));
                        memcpy(sSavedDns, dns2, sizeof(dns2));
                    }
                    sprintf(cmd, "setprop net.%s%d.dns2 %s", prop, cid-1, dns2);
                } else {
                    PHS_LOGD("DNS2 is empty!!");
                    memset(dns2, 0, IP_ADD_SIZE);
                    cvt_reset_dns2(dns2);
                    sprintf(cmd, "setprop net.%s%d.dns2 %s", prop, cid-1, dns2);
                }
                system(cmd);

                ppp_info[cid-1].state = PPP_STATE_ACTIVE;
                PHS_LOGD("PPP_STATE_ACTIVE");
                PHS_LOGD("PS:cid=%d ip:%s,dns1:%s,dns2:%s",cid, ip, dns1, dns2);
            }else{
                ppp_info[cid-1].state = PPP_STATE_EST_UP_ERROR;
                PHS_LOGD("PPP_STATE_EST_UP_ERROR:cid of pdp is error!");
            }
        } while (0);
        return AT_RESULT_OK;
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }

    return AT_RESULT_NG;
}

int cvt_cgdata_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;
    int rsp_type;
    int ppp_index;
    char cmd[MAX_CMD * 2];
    char *input;
    int err, error_num;

    PHS_LOGD("enter cvt_cgdata_set_rsp");
    if (rsp == NULL) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG1");
        return AT_RESULT_NG;
    }
    rsp_type = adapter_get_rsp_type(rsp->rsp_str, rsp->len);
    PHS_LOGD("cvt_cgdata_set_rsp rsp_type=%d ", rsp_type);
    if (rsp_type == AT_RSP_TYPE_MID) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG2");
        return AT_RESULT_NG;
    }
    ppp_index = user_data;
    if (ppp_index < 0 || ppp_index >= getMaxPDPNum()) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG3");
        return AT_RESULT_NG;
    }

    if (rsp_type == AT_RSP_TYPE_CONNECT) {
        //	system("sh /etc/ppp/data-on 0 0  /dev/ts0710mux2 usepeerdns ");
        ppp_info[ppp_index].state = PPP_STATE_CONNECT;
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    }
    else  if(rsp_type == AT_RSP_TYPE_ERROR)
    {
        PHS_LOGD("PDP activate error\r");
        ppp_info[ppp_index].state = PPP_STATE_ACT_ERROR;
        input = rsp->rsp_str;
        if (strStartsWith(input, "+CME ERROR:")) {
            err = at_tok_start(&input, ':');
            if (err >= 0) {
                err = at_tok_nextint(&input, &error_num);
                if(err >= 0) {
                    if(error_num >= 0)
                        ppp_info[ppp_index].error_num = error_num;
                }
            }
        }
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
    }
    else
    {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG2");
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_cgact_deact_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int status, tmp_cid = -1;
    int err;
    char cmd[MAX_CMD], at_cmd_str[MAX_AT_CMD_LEN], 
            cgev_str[MAX_AT_CMD_LEN];
    char *at_in_str;
    char prop[PROPERTY_VALUE_MAX];
    char linker[128] = {0};
    int count = 0;
    int maxPDPNum = getMaxPDPNum();

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    memset(at_cmd_str, 0, MAX_AT_CMD_LEN);
    at_in_str = req->cmd_str;
    err = at_tok_start(&at_in_str, '=');
    if (err < 0) {
        return AT_RESULT_NG;
    }
    err = at_tok_nextint(&at_in_str, &status);
    if (err < 0 || status != 0) {
        return AT_RESULT_NG;
    }

    if (!isLte()) {
        if (at_tok_hasmore(&at_in_str)) {
            err = at_tok_nextint(&at_in_str, &tmp_cid);
            if (err < 0) {
                return AT_RESULT_NG;
            }

            mutex_lock(&ps_service_mutex);
            mux = adapter_get_cmux(req->cmd_type, TRUE);
            //if ((tmp_cid <= 3) && (ppp_info[tmp_cid - 1].state == PPP_STATE_ACTIVE)) { 	/*deactivate PDP connection */
            if (tmp_cid <= 6 && tmp_cid > 0) {	/*deactivate PDP connection */
                ppp_info[tmp_cid-1].state = PPP_STATE_DESTING;
                PHS_LOGD("PPP_STATE_DEACTING");
                ppp_info[tmp_cid-1].state = PPP_STATE_DEACTING;
                ppp_info[tmp_cid - 1].cmux = mux;
                snprintf(at_cmd_str,sizeof(at_cmd_str), "AT+CGACT=0,%d\r",tmp_cid);
                adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp2, (unsigned long)req->recv_pty);
                adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str), req->timeout);
                ppp_info[tmp_cid - 1].state = PPP_STATE_IDLE;

                usleep(200*1000);
                if(!strcmp(modem, "t")) {
                    property_get(ETH_TD, prop, "veth");
                } else if(!strcmp(modem, "w")) {
                    property_get(ETH_W, prop, "veth");
                } else if(!strcmp(modem, "l")) {
                    //to do
                } else if(!strcmp(modem, "tl")) {
                    property_get(ETH_TL, prop, "veth");
                } else if(!strcmp(modem, "lf")) {
                    property_get(ETH_LF, prop, "veth");
                } else {
                    PHS_LOGE("Unknown modem type, exit");
                    exit(-1);
                }
                /* clear IP addr */
                snprintf(linker, sizeof(linker), "addr flush dev %s%d", prop, tmp_cid-1);
                property_set(SYS_IP_CLEAR, linker);
                /* set property */
                snprintf(linker, sizeof(linker), "link set %s%d down", prop, tmp_cid-1);
                property_set(SYS_IFCONFIG_DOWN, linker);
                /* start data_off */
                property_set("ctl.start", "data_off");

                /* wait up to 10s for data_off execute complete */
                do {
                    property_get(SYS_IFCONFIG_DOWN, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_off exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);

                PHS_LOGD("data_off execute done");

                sprintf(cmd, "setprop net.%s%d.ip %s", prop, tmp_cid-1,"0.0.0.0");
                system(cmd);
                sprintf(cmd, "setprop net.%s%d.dns1 \"\"", prop, tmp_cid-1);
                system(cmd);
                sprintf(cmd, "setprop net.%s%d.dns2 \"\"", prop, tmp_cid-1);
                system(cmd);
            } else {
                snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGACT=0,%d\r", tmp_cid);
                adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp,
                    (unsigned long)req->recv_pty);
                adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                    req->timeout);
            }
            mutex_unlock(&ps_service_mutex);
        } else {
            int i;
            mutex_lock(&ps_service_mutex);
            mux = adapter_get_cmux(req->cmd_type, TRUE);
            snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGACT=0\r");
            adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp,
                    (unsigned long)req->recv_pty);
            adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                    req->timeout);

            for (i=0; i< maxPDPNum; i++) { 	/*deactivate PDP connection */
                PHS_LOGD("context id %d state : %d", i, ppp_info[i].state);
                if (ppp_info[i].state ==  PPP_STATE_ACTIVE) {	/*deactivate PDP connection */
                    ppp_info[i].cmux = mux;
                    ppp_info[i].state = PPP_STATE_IDLE;

                    if(!strcmp(modem, "t")) {
                        property_get(ETH_TD, prop, "veth");
                    } else if(!strcmp(modem, "w")) {
                        property_get(ETH_W, prop, "veth");
                    } else if(!strcmp(modem, "l")) {
                        //to do
                    } else if(!strcmp(modem, "tl")) {
                        property_get(ETH_TL, prop, "veth");
                    } else if(!strcmp(modem, "lf")) {
                        property_get(ETH_LF, prop, "veth");
                    } else {
                        PHS_LOGE("Unknown modem type, exit");
                        exit(-1);
                    }

                    /* clear IP addr */
                    snprintf(linker, sizeof(linker), "addr flush dev %s%d", prop, tmp_cid-1);
                    property_set(SYS_IP_CLEAR, linker);
                    /* set property */
                    snprintf(linker, sizeof(linker), "link set %s%d down", prop, i);
                    property_set(SYS_IFCONFIG_DOWN, linker);
                    /* start data_off */
                    property_set("ctl.start", "data_off");

                    /* wait up to 10s for data_off execute complete */
                    do {
                        property_get(SYS_IFCONFIG_DOWN, linker, "");
                        if(!strcmp(linker, "done"))
                            break;
                        count++;
                        PHS_LOGD("wait data_off exec %d times...", count);
                        usleep(10*1000);
                    }while(count < RETRY_MAX_COUNT);

                    PHS_LOGD("data_off execute done");

                    sprintf(cmd, "setprop net.%s%d.ip %s", prop, i,"0.0.0.0");
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.dns1 \"\"", prop, i);
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.dns2 \"\"", prop, i);
                    system(cmd);
                }
            }
            mutex_unlock(&ps_service_mutex);
        }
    } else {//LTE
        int tmp_cid2 = -1;
        char ipv6_dhcpcd_cmd[128] = {0};

        if (at_tok_hasmore(&at_in_str)) {
            err = at_tok_nextint(&at_in_str, &tmp_cid);
            if (err < 0) {
                return AT_RESULT_NG;
            }
            if (at_tok_hasmore(&at_in_str)) {
                PHS_LOGD("hasmore");
                err = at_tok_nextint(&at_in_str, &tmp_cid2);
                PHS_LOGD("hasmore  tmp_cid2 = %d", tmp_cid2);
                if (err < 0) {
                    return AT_RESULT_NG;
                }
            }

            mutex_lock(&ps_service_mutex);
            mux = adapter_get_cmux(req->cmd_type, TRUE);
            //if ((tmp_cid <= 3) && (ppp_info[tmp_cid - 1].state == PPP_STATE_ACTIVE)) { 	/*deactivate PDP connection */
            if (0 < tmp_cid && tmp_cid < 12) {	/*deactivate PDP connection */
                ppp_info[tmp_cid-1].state = PPP_STATE_DESTING;
                PHS_LOGD("PPP_STATE_DEACTING");
                ppp_info[tmp_cid-1].state = PPP_STATE_DEACTING;
                ppp_info[tmp_cid - 1].cmux = mux;
                if(tmp_cid2 != 0){
                   if (tmp_cid2 != -1) {
                        snprintf(at_cmd_str,sizeof(at_cmd_str), "AT+CGACT=0,%d,%d\r",tmp_cid,tmp_cid2);
                    } else {
                        snprintf(at_cmd_str,sizeof(at_cmd_str), "AT+CGACT=0,%d\r",tmp_cid);
                    }
                    PHS_LOGD("at_cmd_str= %s", at_cmd_str);
                    adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp2, (unsigned long)req->recv_pty);

                    adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                        req->timeout);
                }else{
                    adapter_pty_write(req->recv_pty,"OK\r",strlen("OK\r"));
                    adapter_pty_end_cmd(req->recv_pty);
                    adapter_free_cmux(mux);
                }
                ppp_info[tmp_cid - 1].state = PPP_STATE_IDLE;

                usleep(200*1000);
                if(!strcmp(modem, "t")) {
                    property_get(ETH_TD, prop, "veth");
                } else if(!strcmp(modem, "w")) {
                    property_get(ETH_W, prop, "veth");
                } else if(!strcmp(modem, "l")) {
                    property_get(ETH_L, prop, "veth");
                } else if(!strcmp(modem, "tl")) {
                    property_get(ETH_TL, prop, "veth");
                } else if(!strcmp(modem, "lf")) {
                    property_get(ETH_LF, prop, "veth");
                } else {
                    PHS_LOGE("Unknown modem type, exit--1");
                    exit(-1);
                }

                /* clear IP addr */
                snprintf(linker, sizeof(linker), "addr flush dev %s%d", prop, tmp_cid-1);
                property_set(SYS_IP_CLEAR, linker);
                /* set property */
                snprintf(linker, sizeof(linker), "link set %s%d down", prop, tmp_cid-1);
                property_set(SYS_IFCONFIG_DOWN, linker);
                /* start data_off */
                property_set("ctl.start", "data_off");

                /* wait up to 10s for data_off execute complete */
                do {
                    property_get(SYS_IFCONFIG_DOWN, linker, "");
                    if(!strcmp(linker, "done"))
                        break;
                    count++;
                    PHS_LOGD("wait data_off exec %d times...", count);
                    usleep(10*1000);
                }while(count < RETRY_MAX_COUNT);

                if (ppp_info[tmp_cid-1].ip_state == IPV6 ||
                    ppp_info[tmp_cid-1].ip_state == IPV4V6) {
                    snprintf(ipv6_dhcpcd_cmd, sizeof(ipv6_dhcpcd_cmd), "dhcpcd_ipv6:%s%d", prop, tmp_cid-1);
                    property_set("ctl.stop", ipv6_dhcpcd_cmd);
                }
                PHS_LOGD("data_off execute done");

                sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, tmp_cid-1,UNKNOWN);
                system(cmd);

                if ((tmp_cid2 != -1) && (tmp_cid2 != 0)) {
                    snprintf(linker, sizeof(linker), "link set %s%d down", prop, tmp_cid2-1);
                    property_set(SYS_IFCONFIG_DOWN, linker);
                    /* start data_off */
                    property_set("ctl.start", "data_off");

                    /* wait up to 10s for data_off execute complete */
                    do {
                        property_get(SYS_IFCONFIG_DOWN, linker, "");
                        if(!strcmp(linker, "done"))
                            break;
                        count++;
                        PHS_LOGD("wait data_off exec %d times...", count);
                        usleep(10*1000);
                    }while(count < RETRY_MAX_COUNT);

                    if (ppp_info[tmp_cid-1].ip_state == IPV6 ||
                        ppp_info[tmp_cid-1].ip_state == IPV4V6) {
                        snprintf(ipv6_dhcpcd_cmd, sizeof(ipv6_dhcpcd_cmd), "dhcpcd_ipv6:%s%d", prop, tmp_cid2-1);
                        property_set("ctl.stop", ipv6_dhcpcd_cmd);
                    }

                    PHS_LOGD("data_off execute done");

                    sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, tmp_cid2-1,UNKNOWN);
                    system(cmd);
                }
            } else {
                PHS_LOGD("Just send AT+CGACT ");
                snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGACT=0,%d\r", tmp_cid);
                adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp,
                        (unsigned long)req->recv_pty);
                adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                        req->timeout);
            }
            mutex_unlock(&ps_service_mutex);
        } else {
            int i;
            mutex_lock(&ps_service_mutex);
            mux = adapter_get_cmux(req->cmd_type, TRUE);
            snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGACT=0\r");
            adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp,
                    (unsigned long)req->recv_pty);
            adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                    req->timeout);

            for (i=0; i< maxPDPNum; i++) { 	/*deactivate PDP connection */
                PHS_LOGD("context id %d state : %d", i, ppp_info[i].state);
                if (ppp_info[i].state ==  PPP_STATE_ACTIVE) {	/*deactivate PDP connection */
                    ppp_info[i].cmux = mux;
                    ppp_info[i].state = PPP_STATE_IDLE;

                    usleep(200*1000);
                    if(!strcmp(modem, "t")) {
                        property_get(ETH_TD, prop, "veth");
                    } else if(!strcmp(modem, "w")) {
                        property_get(ETH_W, prop, "veth");
                    } else if(!strcmp(modem, "l")) {
                        property_get(ETH_L, prop, "veth");
                    } else if(!strcmp(modem, "tl")) {
                        property_get(ETH_TL, prop, "veth");
                    } else if(!strcmp(modem, "lf")) {
                        property_get(ETH_LF, prop, "veth");
                    } else {
                        PHS_LOGE("Unknown modem type, exit--2");
                        exit(-1);
                    }

                    snprintf(linker, sizeof(linker), "%s%d down", prop, i);
                    property_set(SYS_IFCONFIG_DOWN, linker);
                    /* start data_off */
                    property_set("ctl.start", "data_off");


                    /* wait up to 10s for data_off execute complete */
                    do {
                        property_get(SYS_IFCONFIG_DOWN, linker, "");
                        if(!strcmp(linker, "done"))
                            break;
                        count++;
                        PHS_LOGD("wait data_off exec %d times...", count);
                        usleep(10*1000);
                    }while(count < RETRY_MAX_COUNT);

                    if (ppp_info[i].ip_state == IPV6 ||
                        ppp_info[i].ip_state == IPV4V6) {
                        snprintf(ipv6_dhcpcd_cmd, sizeof(ipv6_dhcpcd_cmd), "dhcpcd_ipv6:%s%d", prop, i);
                        property_set("ctl.stop", ipv6_dhcpcd_cmd);
                    }
                    PHS_LOGD("data_off execute done");

                    sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, i,UNKNOWN);
                    system(cmd);
                }
            }
            mutex_unlock(&ps_service_mutex);
        }
    }
    return AT_RESULT_PROGRESS;
}

int cvt_cgact_deact_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {

        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgact_deact_rsp1(AT_CMD_RSP_T * rsp,
		unsigned long __attribute__((unused)) user_data)
{
    int ret;
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {

        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgact_deact_rsp2(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {

        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data,rsp->rsp_str,rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgact_deact_rsp3(AT_CMD_RSP_T * rsp,
		unsigned long __attribute__((unused)) user_data)
{
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {

        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgact_act_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cgact_act_rsp,
            (unsigned long)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgact_act_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret;

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgdcont_read_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;

    if (req == NULL) {
        return AT_RESULT_NG;
    }

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    adapter_cmux_register_callback(mux, cvt_cgdcont_read_rsp,
            (unsigned long)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgdcont_read_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    int ret, err;
    int tmp_cid = 0;
    int in_len;
    char *input, *out;
    char at_cmd_str[MAX_AT_CMD_LEN], ip[128], net[128];

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    memset(at_cmd_str, 0, MAX_AT_CMD_LEN);
    memset(net, 0, 128);
    in_len = rsp->len;
    input = rsp->rsp_str;

    input[in_len - 1] = '\0';
    if (findInBuf(input, in_len, "+CGDCONT")) {

        do {
            err = at_tok_start(&input, ':');
            if (err < 0) {
                break;
            }
            err = at_tok_nextint(&input, &tmp_cid);
            if (err < 0) {
                break;
            }
            err = at_tok_nextstr(&input, &out);	//ip
            if (err < 0) {
                break;
            }
            strncpy(ip, out, sizeof(ip));
            ip[sizeof(ip)-1] = '\0';
            err = at_tok_nextstr(&input, &out);	//cmnet
            if (err < 0) {
                break;
            }
            strncpy(net, out, sizeof(net));
            net[sizeof(net)-1] = '\0';
            PHS_LOGD("  cvt_cgdcont_read_rsp cid =%d", tmp_cid);
            if ((tmp_cid <= getMaxPDPNum())
                    && (ppp_info[tmp_cid - 1].state ==
                        PPP_STATE_ACTIVE)) {
                if(ppp_info[tmp_cid-1].manual_dns == 1)
                {
                    snprintf(at_cmd_str, sizeof(at_cmd_str),
                            "+CGDCONT:%d,\"%s\",\"%s\",\"%s\",0,0,\"%s\",\"%s\"\r",
                            tmp_cid, ip, net,
                            ppp_info[tmp_cid - 1].ipladdr,
                            ppp_info[tmp_cid - 1].userdns1addr,
                            ppp_info[tmp_cid - 1].userdns2addr);
                }
                else
                {
                    snprintf(at_cmd_str,sizeof(at_cmd_str),
                            "+CGDCONT:%d,\"%s\",\"%s\",\"%s\",0,0,\"%s\",\"%s\"\r",
                            tmp_cid, ip, net,
                            ppp_info[tmp_cid - 1].ipladdr,
                            ppp_info[tmp_cid - 1].dns1addr,
                            ppp_info[tmp_cid - 1].dns2addr);
                }
            } else if ((tmp_cid <= getMaxPDPNum())) {
                snprintf(at_cmd_str, sizeof(at_cmd_str),
                        "+CGDCONT:%d,\"%s\",\"%s\",\"%s\",0,0,\"%s\",\"%s\"\r",
                        tmp_cid, ip, net, "0.0.0.0","0.0.0.0","0.0.0.0");
            } else {
                return AT_RESULT_OK;
            }
            at_cmd_str[MAX_AT_CMD_LEN -1] = '\0';
            PHS_LOGD
                ("  cvt_cgdcont_read_rsp pty_write cid =%d resp:%s",
                 tmp_cid, at_cmd_str);
            adapter_pty_write((pty_t *) user_data, at_cmd_str,
                    strlen(at_cmd_str));
        } while (0);
        return AT_RESULT_OK;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);

        //      adapter_pty_write(user_data, "OK\r",strlen("OK\r"));
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

int cvt_cgdcont_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int len, tmp_cid = 0;
    char *input;
    char at_str[200], ip[128], net[128], ipladdr[30], hcomp[30], dcomp[30];
    char *out;
    int err = 0, ret = 0;
    char at_cmd_str[MAX_AT_CMD_LEN];
    int maxPDPNum = getMaxPDPNum();

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    input = req->cmd_str;

    memset(at_str, 0, 200);
    memset(ip, 0, 128);
    memset(net, 0, 128);
    memset(ipladdr, 0, 30);
    memset(hcomp, 0, 30);
    memset(dcomp, 0, 30);

    err = at_tok_start(&input, '=');
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextint(&input, &tmp_cid);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    err = at_tok_nextstr(&input, &out);	//ip
    if (err < 0)
        goto end_req;
    strncpy(ip, out, sizeof(ip));
    ip[sizeof(ip)-1] = '\0';
    err = at_tok_nextstr(&input, &out);	//cmnet
    if (err < 0)
        goto end_req;
    strncpy(net, out, sizeof(net));
    net[sizeof(net)-1] = '\0';
    err = at_tok_nextstr(&input, &out);	//ipladdr
    if (err < 0)
        goto end_req;
    strncpy(ipladdr, out, sizeof(ipladdr));
    ipladdr[sizeof(ipladdr)-1] = '\0';
    err = at_tok_nextstr(&input, &out);	//dcomp
    if (err < 0)
        goto end_req;
    strncpy(dcomp, out, sizeof(dcomp));
    dcomp[sizeof(dcomp)-1] = '\0';
    err = at_tok_nextstr(&input, &out);	//hcomp
    if (err < 0)
        goto end_req;
    strncpy(hcomp, out, sizeof(hcomp));
    hcomp[sizeof(hcomp)-1] = '\0';
    //cp dns to ppp_info ?

    if(tmp_cid <= maxPDPNum)
    {
        strcpy(ppp_info[tmp_cid - 1].userdns1addr, "0.0.0.0");
        strcpy(ppp_info[tmp_cid - 1].userdns2addr, "0.0.0.0");
        ppp_info[tmp_cid-1].manual_dns = 0;
    }

    //dns1 , info used with cgdata
    err = at_tok_nextstr(&input, &out);	//dns1
    if (err < 0)
        goto end_req;
    if(tmp_cid <= maxPDPNum && *out!=0)
    {
        strncpy(ppp_info[tmp_cid - 1].userdns1addr, out, sizeof(ppp_info[tmp_cid - 1].userdns1addr));
        ppp_info[tmp_cid - 1].userdns1addr[sizeof(ppp_info[tmp_cid - 1].userdns1addr)-1] = '\0';
    }

    //dns2  , info used with cgdata
    err = at_tok_nextstr(&input, &out);	//dns2
    if (err < 0)
        goto end_req;

    if(tmp_cid <= maxPDPNum && *out!=0)
    {
        strncpy(ppp_info[tmp_cid - 1].userdns2addr, out, sizeof(ppp_info[tmp_cid - 1].userdns2addr));
        ppp_info[tmp_cid - 1].userdns2addr[sizeof(ppp_info[tmp_cid - 1].userdns2addr)-1] = '\0';
    }

    //cp dns to ppp_info ?
end_req:

    if(tmp_cid <= maxPDPNum)
    {
        if (strncasecmp(ppp_info[tmp_cid-1].userdns1addr, "0.0.0.0",
                    strlen("0.0.0.0"))) {
            ppp_info[tmp_cid-1].manual_dns = 1;
        }
    }

    //make sure ppp in idle
    mutex_lock(&ps_service_mutex);
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    mutex_unlock(&ps_service_mutex);

    snprintf(at_str, sizeof(at_str), "AT+CGDCONT=%d,\"%s\",\"%s\",\"%s\",%s,%s\r", tmp_cid, ip,
            net, ipladdr, dcomp, hcomp);
    len = strlen(at_str);
    PHS_LOGD("PS:%s", at_str);

    req->cmd_str = at_str;
    req->len = len;

    adapter_cmux_register_callback(mux, cvt_cgdcont_set_rsp,
            (unsigned long)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgdcont_set_rsp(AT_CMD_RSP_T * rsp, unsigned long user_data)
{
    if (rsp == NULL) {
        return AT_RESULT_NG;
    }
    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_pty_write((pty_t *) user_data, rsp->rsp_str, rsp->len);
        adapter_pty_end_cmd((pty_t *) user_data);
        adapter_free_cmux(rsp->recv_cmux);
        return AT_RESULT_OK;
    }
    return AT_RESULT_NG;
}

IP_TYPE read_ip_addr(char *raw, char *rsp)
{
    int comma_count = 0;
    int num = 0, comma4_num = 0, comma16_num = 0;
    int space_num = 0;
    char *buf = raw;
    int len = 0;
    int ip_type = UNKNOWN;

    if (raw != NULL) {
        len = strlen(raw);
        for (num=0 ; num<len; num++) {

            if (raw[num] == '.') {
                comma_count++;
            }

            if (raw[num] == ' ') {
                space_num = num;
                break;
            }

            if (comma_count == 4 && comma4_num == 0) {
                comma4_num = num;
            }

            if (comma_count > 7 && comma_count == 16) {
                comma16_num = num;
                break;
            }
        }

        if (space_num > 0) {
            buf[space_num] = '\0';
            ip_type = IPV6;
            memcpy(rsp, buf, strlen(buf)+1);
        } else if (comma_count >= 7) {
            if (comma_count == 7) { //ipv4
                buf[comma4_num] = '\0';
                ip_type = IPV4;
            } else { //ipv6
                buf[comma16_num] = '\0';
                ip_type = IPV6;
            }
            memcpy(rsp, buf, strlen(buf)+1);
        }
    }

    return ip_type;
}

int cvt_cgcontrdp_rsp(AT_CMD_RSP_T * rsp,
		unsigned long __attribute__((unused)) user_data)
{
    int ret;
    int err;
    char *input;
    int cid;
    char *local_addr_subnet_mask= NULL,*gw_addr= NULL, *dns_prim_addr= NULL, *dns_sec_addr= NULL;
    char ip[IP_ADD_SIZE*4],dns1[IP_ADD_SIZE*4], dns2[IP_ADD_SIZE*4];
    char cmd[MAX_CMD * 2];
    char prop[PROPERTY_VALUE_MAX];
    int count = 0;
    char *sskip;
    char *tmp;
    int skip;
    static int ip_type_num = 0;
    int ip_type;
    int maxPDPNum = getMaxPDPNum();

    if (rsp == NULL) {
        PHS_LOGD("leave cvt_cgcontrdp_rsp:AT_RESULT_NG");
        return AT_RESULT_NG;
    }

    memset(ip, 0, IP_ADD_SIZE*4);
    memset(dns1, 0, IP_ADD_SIZE*4);
    memset(dns2, 0, IP_ADD_SIZE*4);
    input = rsp->rsp_str;
    input[rsp->len-1] ='\0';


    if(!strcmp(modem, "t")) {
        property_get(ETH_TD, prop, "veth");
    } else if(!strcmp(modem, "w")) {
        property_get(ETH_W, prop, "veth");
    } else if(!strcmp(modem, "l")) {
        property_get(ETH_L, prop, "veth");
    } else if(!strcmp(modem, "tl")) {
        property_get(ETH_TL, prop, "veth");
    } else if(!strcmp(modem, "lf")) {
        property_get(ETH_LF, prop, "veth");
    } else {
        PHS_LOGE("Unknown modem type, exit--3");
        exit(-1);
    }

    PHS_LOGD("cvt_cgcontrdp_rsp: input = %s", input);
    if (findInBuf(input, rsp->len, "+CGCONTRDP") || findInBuf(input, rsp->len, "+SIPCONFIG")) {
        do {
            err = at_tok_start(&input, ':');
            if (err < 0) {
                goto error;
            }
            err = at_tok_nextint(&input, &cid);//cid
            if (err < 0) {
                goto error;
            }
            err = at_tok_nextint(&input, &skip);//bearer_id
            if (err < 0) {
                goto error;
            }
            err = at_tok_nextstr(&input, &sskip);//apn
            if (err < 0) {
                goto error;
            }
            if (at_tok_hasmore(&input)) {
                err = at_tok_nextstr(&input, &local_addr_subnet_mask);//local_addr_and_subnet_mask
                if (err < 0) {
                    goto error;
                }
                PHS_LOGD("cvt_cgcontrdp_rsp: after fetch local_addr_subnet_mask input = %s", input);
                if (at_tok_hasmore(&input)) {
                    err = at_tok_nextstr(&input, &sskip);//gw_addr
                    if (err < 0) {
                        goto error;
                    }
                    if (at_tok_hasmore(&input)) {
                        err = at_tok_nextstr(&input, &dns_prim_addr);//dns_prim_addr
                        if (err < 0) {
                            goto error;
                        }
                        strcpy(dns1, dns_prim_addr);
                        if (at_tok_hasmore(&input)) {
                            err = at_tok_nextstr(&input, &dns_sec_addr);//dns_sec_addr
                            if (err < 0) {
                                goto error;
                            }
                            strcpy(dns2, dns_sec_addr);
                        }
                    }
                }
            }

            if ((cid < maxPDPNum) && (cid >=1)) {
                ip_type = read_ip_addr(local_addr_subnet_mask, ip);
                PHS_LOGD("PS: cid = %d,  ip_type = %d, ip = %s, dns1 = %s, dns2 = %s", cid, ip_type, ip, dns1, dns2);

                if (ip_type == IPV6) { //ipv6
                    PHS_LOGD("cvt_cgcontrdp_rsp: IPV6");
                    if (!strncasecmp(ip, "0000:0000:0000:0000", strlen("0000:0000:0000:0000"))) { //incomplete address
                       tmp = strchr(ip,':');
                       if (tmp != NULL) {
                           sprintf(ip, "FE80%s",tmp);
                       }
                    }
                    memcpy(ppp_info[cid-1].ipv6laddr, ip, sizeof(ppp_info[cid-1].ipv6laddr));
                    memcpy(ppp_info[cid-1].ipv6dns1addr, dns1, sizeof(ppp_info[cid-1].ipv6dns1addr));
                    memcpy(ppp_info[cid-1].ipv6dns2addr, dns2, sizeof(ppp_info[cid-1].ipv6dns2addr));

                    sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, cid-1,IPV6);
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.ipv6_ip %s", prop, cid-1,ip);
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.ipv6_dns1 %s", prop, cid-1, dns1);
                    system(cmd);
                    if (strlen(dns2) != 0) {
                         if (!strcmp(dns1, dns2)) {
                            PHS_LOGD(
                                    "Two DNS are the same,so need to reset dns2!!");
                            if (strlen(sSavedDns_IPV6) > 0)
                            {
                                PHS_LOGD("Use saved DNS2 instead.");
                                memcpy(dns2, sSavedDns_IPV6, sizeof(sSavedDns_IPV6));
                            } else {
                                PHS_LOGD("Use default DNS2 instead.");
                                sprintf(dns2, "%s", DEFAULT_PUBLIC_DNS2_IPV6);
                            }
                            //cvt_reset_dns2(dns2);
                            } else {
                            PHS_LOGD("Backup DNS2");
                            memset(sSavedDns_IPV6, 0, sizeof(sSavedDns_IPV6));
                            memcpy(sSavedDns_IPV6, dns2, sizeof(dns2));
                        }
                    } else {
                        PHS_LOGD("DNS2 is empty!!");
                        memset(dns2, 0, IP_ADD_SIZE*4);
                        sprintf(dns2, "%s", DEFAULT_PUBLIC_DNS2_IPV6);
                    }
                    sprintf(cmd, "setprop net.%s%d.ipv6_dns2 %s", prop, cid-1, dns2);
                    system(cmd);

                    ppp_info[cid-1].ip_state = IPV6;
                    ip_type_num++;
                } else if (ip_type == IPV4) { //ipv4
                    PHS_LOGD("cvt_cgcontrdp_rsp: IPV4");
                    memcpy(ppp_info[cid-1].ipladdr, ip, sizeof(ppp_info[cid-1].ipladdr));
                    memcpy(ppp_info[cid-1].dns1addr, dns1, sizeof(ppp_info[cid-1].dns1addr));
                    memcpy(ppp_info[cid-1].dns2addr, dns2, sizeof(ppp_info[cid-1].dns2addr));

                    sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, cid-1,IPV4);
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.ip %s", prop, cid-1,ip);
                    system(cmd);
                    sprintf(cmd, "setprop net.%s%d.dns1 %s", prop, cid-1, dns1);
                    system(cmd);
                    if(strlen(dns2) != 0){
                        if(!strcmp(dns1, dns2)) {
                            PHS_LOGD("Two DNS are the same,so need to reset dns2!!");
                            cvt_reset_dns2(dns2);
                        } else {
                            PHS_LOGD("Backup DNS2");
                            memset(sSavedDns, 0, sizeof(sSavedDns));
                            memcpy(sSavedDns, dns2, IP_ADD_SIZE);
                        }
                    } else {
                        PHS_LOGD("DNS2 is empty!!");
                        memset(dns2, 0, IP_ADD_SIZE);
                        cvt_reset_dns2(dns2);
                    }
                    sprintf(cmd, "setprop net.%s%d.dns2 %s", prop, cid-1, dns2);
                    system(cmd);

                    ppp_info[cid-1].ip_state = IPV4;
                    ip_type_num++;
                } else {//unknown
                    ppp_info[cid-1].state = PPP_STATE_EST_UP_ERROR;
                    PHS_LOGD("PPP_STATE_EST_UP_ERROR: unknown ip type!");
                }

                if (ip_type_num > 1) {
                    PHS_LOGD("cvt_cgcontrdp_rsp: IPV4V6");
                    ppp_info[cid-1].ip_state = IPV4V6;
                    sprintf(cmd, "setprop net.%s%d.ip_type %d", prop, cid-1,IPV4V6);
                    system(cmd);
                }
                ppp_info[cid-1].state = PPP_STATE_ACTIVE;
                PHS_LOGD("PPP_STATE_ACTIVE");
            }
        } while (0);
        return AT_RESULT_OK;
    }

    if (adapter_cmd_is_end(rsp->rsp_str, rsp->len) == TRUE) {
        PHS_LOGD("cvt_cgcontrdp_rsp adapter cmd is end");
        adapter_cmux_deregister_callback(rsp->recv_cmux);
        adapter_wakeup_cmux(rsp->recv_cmux);
        ip_type_num = 0;
        return AT_RESULT_OK;
    }

    PHS_LOGD("cvt_cgcontrdp_rsp: AT_RESULT_OK");
    return AT_RESULT_OK;

error:
    return AT_RESULT_NG;
}

int getMaxPDPNum(void) {
    return isLte() ? MAX_PPP_NUM:MAX_PPP_NUM/2;
}

/* SPRD : for svlte & csfb @{ */
int getTestMode(void) {
    int testmode = 0;
    char prop[PROPERTY_VALUE_MAX]="";
    property_get(SSDA_TESTMODE, prop, "0");
    PHS_LOGD("ssda testmode: %s", prop);
    testmode = atoi(prop);
    if ((testmode == 13) || (testmode == 14)) {
        testmode = 255;
    }
    return testmode;
}

int isSvLte(void) {
    char prop[PROPERTY_VALUE_MAX]="";
    property_get(SSDA_MODE, prop, "0");
    PHS_LOGD("ssda mode: %s", prop);
    if (!strcmp(prop,"svlte") && (0 == getTestMode())) {
        PHS_LOGD("is svlte");
        return 1;
    }
    PHS_LOGD("isn't svlte");
    return 0;
}

int isLte(void) {
    char prop[PROPERTY_VALUE_MAX]="";
    property_get(SSDA_MODE, prop, "0");
    PHS_LOGD("ssda mode: %s", prop);
    if ((!strcmp(prop,"svlte")) || (!strcmp(prop,"tdd-csfb")) || (!strcmp(prop,"fdd-csfb"))
        || (!strcmp(prop,"csfb"))) {
        return 1;
    }
    return 0;
}
