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
#include "os_api.h"
#include "ps_service.h"
#include "pty.h"
#include "at_tok.h"
#include <cutils/properties.h>

#define SYS_IFCONFIG_UP "sys.ifconfig.up"
#define SYS_IFCONFIG_DOWN "sys.ifconfig.down"
#define SYS_NO_ARP "sys.data.noarp"

struct ppp_info_struct ppp_info[MAX_PPP_NUM];
mutex ps_service_mutex;

extern int findInBuf(char *buf, int len, char *needle);

void ps_service_init(void)
{
    int i;
    memset(ppp_info,0x0,sizeof(ppp_info));
    for(i=0;i<MAX_PPP_NUM;i++)
    {
        ppp_info[i].state = PPP_STATE_IDLE;
        cond_init(&ppp_info[i].cond_timeout, NULL);
        mutex_init(&ppp_info[i].mutex_timeout,NULL);
    }

    mutex_init(&ps_service_mutex,NULL);
}
int ps_service_wait_ppp_up(int index,int to)
{

    int seconds;
    int ret = AT_RESULT_OK;
    int err;
    struct timespec timeout;

    if(index >=MAX_PPP_NUM)
        return AT_RESULT_NG;

    mutex_lock(&ppp_info[index].mutex_timeout);
    timeout.tv_sec = time(NULL) + to;
    timeout.tv_nsec = 0;
    seconds = time((time_t *) NULL);
    PHS_LOGD("wait ppp uint[%d],up start[%d]\n",index,seconds);
    err = thread_cond_timedwait(&ppp_info[index].cond_timeout, &ppp_info[index].mutex_timeout,
            &timeout);

    if(err == ETIMEDOUT)
    {
        PHS_LOGD("wait ppp uint[%d],timeout\n",index);
        ret = AT_RESULT_TIMEOUT;
    }

    seconds = time((time_t *) NULL);
    PHS_LOGD("wait ppp uint[%d],up end[%d]\n",index,seconds);
    mutex_unlock(&ppp_info[index].mutex_timeout);
    return ret;
}

int ps_service_ppp_up_notify(int index)
{
    if(index >= MAX_PPP_NUM)
        return AT_RESULT_NG;
    int seconds = time((time_t *) NULL);
    PHS_LOGD("ppp up notify[%d],[%d]\n",index,seconds);
    mutex_lock(&ppp_info[index].mutex_timeout);
    PHS_LOGD("ppp up notify[%d],[%d]\n",index,seconds);
    thread_cond_signal(&ppp_info[index].cond_timeout);
    mutex_unlock(&ppp_info[index].mutex_timeout);
    return AT_RESULT_OK;
}



int ps_service_start_ppp(int ppp_index)
{
    char cmd[MAX_CMD * 2];
    int ret;

    if(ppp_index >=MAX_PPP_NUM){
        return AT_RESULT_NG;
    }

    PHS_LOGD("start ppp...\n");
    ppp_info[ppp_index].state = PPP_STATE_ESTING;
    ppp_info[ppp_index].manual_dns = 0;
    if (ppp_info[ppp_index].manual_dns == 1) {
        PHS_LOGD("data-on without userpeerdns\n");
        sprintf(cmd,
                "sh /etc/ppp/data-on %d %d  /dev/ts0710mux%d ",
                0, ppp_index, ppp_index + 2);
    } else {
        PHS_LOGD("data-on with userpeerdns\n");
        sprintf(cmd,
                "sh /etc/ppp/data-on %d %d  /dev/ts0710mux%d %s ",
                0, ppp_index, ppp_index + 2, "usepeerdns");
    }

    PHS_LOGD("before start ppp-on script \n");
    system(cmd);
    PHS_LOGD("after start ppp-on script \n");

    PHS_LOGD("waiting ppp up...\r");
    ret = ps_service_wait_ppp_up(ppp_index, PPP_UP_TIMEOUT);
    PHS_LOGD("ppp up:ret=%d,state=%d\n",ret,ppp_info[ppp_index].state);
    if(ret == AT_RESULT_TIMEOUT)
    {
        return AT_RESULT_TIMEOUT;
    }

    if(ppp_info[ppp_index].state != PPP_STATE_ACTIVE)
    {
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int ps_service_stop_ppp(int ppp_index)
{
    char cmd[MAX_CMD * 2];

    if(ppp_index >=MAX_PPP_NUM){
        return AT_RESULT_NG;
    }

    sprintf(cmd, "sh /etc/ppp/data-off ppp%d ", ppp_index);
    PHS_LOGD("PPP_STATE_DESTING\n");
    system(cmd);
    return AT_RESULT_OK;
}

int cvt_cgdata_set_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    char *at_in_str, *out;
    char buffer[50], error_str[30];
    char at_cmd_str[MAX_AT_CMD_LEN];
    int cid, ppp_index;
    int err, ret;

    PHS_LOGD("enter cvt_cgdata_set_req  \n");
    if (req == NULL) {
        PHS_LOGD("leave cvt_cgdata_set_req AT_RESULT_NG\n");
        return AT_RESULT_NG;
    }
    PHS_LOGD("enter cvt_cgdata_set_req cmd:%s cmdlen:%d  \n",req->cmd_str, req->len);
    memset(buffer,0,sizeof(buffer));
    memset(at_cmd_str,0,MAX_AT_CMD_LEN);

    at_in_str = buffer;
    strncpy(at_in_str, req->cmd_str, req->len);

    err = at_tok_start(&at_in_str, '=');
    if (err < 0) {
        PHS_LOGD("parse cmd error\n");
        return AT_RESULT_NG;
    }

    /*get L2P */
    err = at_tok_nextstr(&at_in_str, &out);
    if (err < 0) {
        PHS_LOGD("parse cmd error\n");
        return AT_RESULT_NG;
    }

    /*Get cid */
    err = at_tok_nextint(&at_in_str, &cid);
    if (err < 0) {
        PHS_LOGD("parse cmd error\n");
        return AT_RESULT_NG;
    }
    if (cid > 3) {
        PHS_LOGD("invalid cid\n");
        return AT_RESULT_NG;
    }
    ppp_index = cid - 1;

    mutex_lock(&ps_service_mutex);

#ifndef CONFIG_VETH
    if(ppp_info[ppp_index].state != PPP_STATE_IDLE)
    {
        PHS_LOGD("cgdata:state error \n");
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_NG;
    }
#endif

    mux = adapter_get_cmux(req->cmd_type, TRUE);
    ppp_info[ppp_index].state = PPP_STATE_ACTING;
    PHS_LOGD("PPP_STATE_ACTING\n");
    ppp_info[ppp_index].pty = req->recv_pty;
    ppp_info[ppp_index].cmux = mux;
    ppp_info[ppp_index].cid = cid;
    ppp_info[ppp_index].error_num = -1;
    adapter_cmux_register_callback(mux,
            (void *)cvt_cgdata_set_rsp,
            ppp_index);
    ret = adapter_cmux_write_for_ps(mux, req->cmd_str, req->len, req->timeout);

    PHS_LOGD("PDP activate result:ret = %d,state=%d\n",ret,ppp_info[ppp_index].state);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("PDP activate timeout \n");
        ppp_info[ppp_index].state = PPP_STATE_IDLE;
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }

    if(ppp_info[ppp_index].state != PPP_STATE_CONNECT)
    {
        PHS_LOGD("PDP activate error :%d\n",ppp_info[ppp_index].state);
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
#ifdef CONFIG_VETH
    ppp_info[ppp_index].state = PPP_STATE_ESTING;
    ppp_info[ppp_index].manual_dns = 0;
    snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+SIPCONFIG=%d\r",cid);
    adapter_cmux_register_callback(mux,
            (void *)cvt_sipconfig_rsp,
            ppp_index);
    ret = adapter_cmux_write_for_ps( mux, at_cmd_str, strlen(at_cmd_str), 10);
    if(ret == AT_RESULT_TIMEOUT)
    {
        PHS_LOGD("Get IP address timeout \n");
        ppp_info[ppp_index].state = PPP_STATE_IDLE;
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }
    if(ppp_info[ppp_index].state != PPP_STATE_ACTIVE)
    {
        PHS_LOGD("Getting IP addr and PDP activate error :%d\n",ppp_info[ppp_index].state);
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
        ppp_info[ppp_index].state = PPP_STATE_IDLE;
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }
    if(ppp_info[ppp_index].state == PPP_STATE_ACTIVE)
    {
        PHS_LOGD("PS connected successful\n");
        adapter_pty_end_cmd(req->recv_pty );
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"CONNECT\r",strlen("CONNECT\r"));
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }
#else
    ret = ps_service_start_ppp(ppp_index);

    if(ret == AT_RESULT_OK && ppp_info[ppp_index].state == PPP_STATE_ACTIVE)
    {
        PHS_LOGD("PS connected successful\n");
        adapter_free_cmux_for_ps(mux);
        adapter_pty_write(req->recv_pty,"CONNECT\r",strlen("CONNECT\r"));
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_OK;
    }
#endif
    adapter_pty_write(req->recv_pty,"ERROR\r",strlen("ERROR\r"));
#ifdef CONFIG_VETH
    PHS_LOGD("Getting IP addr and PDP activate error :%d\n",ppp_info[ppp_index].state);
#else
    ps_service_stop_ppp(ppp_index);
#endif

    ppp_info[ppp_index].state = PPP_STATE_IDLE;
    PHS_LOGD("PPP_STATE_IDLE\n");
    adapter_pty_end_cmd(req->recv_pty );
    adapter_free_cmux_for_ps(mux);
    mutex_unlock(&ps_service_mutex);
    return AT_RESULT_OK;
}

#ifdef CONFIG_VETH
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

int cvt_sipconfig_rsp(AT_CMD_RSP_T * rsp, int user_data)
{
    int ret;
    int err;
    char *input;
    int cid,nsapi;
    char *out;
    int ip_hex,dns1_hex, dns2_hex;
    char ip[IP_ADD_SIZE],dns1[IP_ADD_SIZE], dns2[IP_ADD_SIZE];
    char cmd[MAX_CMD * 2];
    char prop[10];
    char linker[128] = {0};

    if (rsp == NULL) {
        PHS_LOGD("leave cvt_ipconf_rsp:AT_RESULT_NG\n");
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
            if ((cid <= MAX_PPP_NUM) && (cid >=1)){
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

                /* set property */
                snprintf(linker, sizeof(linker), "veth%d %s mtu 1400 netmask 255.255.255.255 up", cid-1, ip);
                property_set(SYS_IFCONFIG_UP, linker);
                snprintf(linker, sizeof(linker), "link set veth%d arp off", cid-1);
                property_set(SYS_NO_ARP, linker);
                /* start data_on */
                property_set("ctl.start", "data_on");

                sprintf(cmd, "setprop net.veth%d.ip %s", cid-1,ip);
                system(cmd);
                if(dns1_hex != 0x0)
                    sprintf(cmd, "setprop net.veth%d.dns1 %s", cid-1, dns1);
                else
                    sprintf(cmd, "setprop net.veth%d.dns1 \"\"", cid-1);
                system(cmd);
                if(dns2_hex != 0x0)
                    sprintf(cmd, "setprop net.veth%d.dns2 %s", cid-1, dns2);
                else
                    sprintf(cmd, "setprop net.veth%d.dns2 \"\"", cid-1);
                system(cmd);

                ppp_info[cid-1].state = PPP_STATE_ACTIVE;
                PHS_LOGD("PPP_STATE_ACTIVE\n");
                PHS_LOGD("PS:cid=%d ip:%s,dns1:%s,dns2:%s\n",cid, ip, dns1, dns2);
            }else{
                ppp_info[cid-1].state = PPP_STATE_EST_UP_ERROR;
                PHS_LOGD("PPP_STATE_EST_UP_ERROR:cid of pdp is error!\n");
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
#endif

int cvt_cgdata_set_rsp(AT_CMD_RSP_T * rsp, int user_data)
{
    int ret;
    int rsp_type;
    int ppp_index;
    char cmd[MAX_CMD * 2];
    char *input;
    int err, error_num;

    PHS_LOGD("enter cvt_cgdata_set_rsp\n");
    if (rsp == NULL) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG1\n");
        return AT_RESULT_NG;
    }
    rsp_type = adapter_get_rsp_type(rsp->rsp_str, rsp->len);
    PHS_LOGD("cvt_cgdata_set_rsp rsp_type=%d \n", rsp_type);
    if (rsp_type == AT_RSP_TYPE_MID) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG2\n");
        return AT_RESULT_NG;
    }
    ppp_index = user_data;
    if (ppp_index < 0 || ppp_index >= MAX_PPP_NUM) {
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG3\n");
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
        PHS_LOGD("leave cvt_cgdata_set_rsp:AT_RESULT_NG2\n");
        return AT_RESULT_NG;
    }
    return AT_RESULT_OK;
}

int cvt_ppp_up_rsp(char *rsp_str, int len)
{
    int ppp_index;
    int err;
    char *input = rsp_str;
    char *out;
    char dns1[IP_ADD_SIZE], dns2[IP_ADD_SIZE], tmp_ip_local[IP_ADD_SIZE],
         tmp_ip_remote[IP_ADD_SIZE];
    int data_link;
    PHS_LOGD("cvt_ppp_up_rsp PPP_UP:%s", rsp_str);

    do {
        err = at_tok_start(&input, '=');
        if (err < 0) {
            return AT_RESULT_NG;
        }
        err = at_tok_nextint(&input, &ppp_index);
        if (err < 0) {
            return AT_RESULT_NG;
        }
        if (ppp_index >= MAX_PPP_NUM) {
            return AT_RESULT_NG;
        }
        err = at_tok_nextstr(&input, &out);
        if (err < 0) {
            break;
        }
        strcpy(tmp_ip_local, out);
        err = at_tok_nextstr(&input, &out);
        if (err < 0) {
            break;
        }
        strcpy(tmp_ip_remote, out);
        err = at_tok_nextstr(&input, &out);
        if (err < 0) {
            strcpy(dns1, "0.0.0.0");
        }
        else
        {
            strcpy(dns1, out);
        }
        err = at_tok_nextstr(&input, &out);
        if (err < 0) {
            strcpy(dns2, "0.0.0.0");
        }
        else {
            strcpy(dns2, out);
        }

        /*Save ppp info */
        strlcpy(ppp_info[ppp_index].ipladdr, tmp_ip_local,
                sizeof(ppp_info[ppp_index].ipladdr));
#ifndef CONFIG_VETH
        strlcpy(ppp_info[ppp_index].ipraddr, tmp_ip_remote,
                sizeof(ppp_info[ppp_index].ipraddr));
#endif
        strlcpy(ppp_info[ppp_index].dns1addr, dns1, sizeof(ppp_info[ppp_index].dns1addr));

        strlcpy(ppp_info[ppp_index].dns2addr, dns2, sizeof(ppp_info[ppp_index].dns2addr));


        if (!strncasecmp(dns1, "0.0.0.0", sizeof("0.0.0.0"))) {	//no return dns
            PHS_LOGD
                ("cvt_ppp_up_rsp PPP_UP ppp_index=%d return dns1:%s",
                 ppp_index, dns1);
            strlcpy(ppp_info[ppp_index].dns1addr,
                    ppp_info[ppp_index].userdns1addr,
                    sizeof(ppp_info[ppp_index].dns1addr));
        }
        if (!strncasecmp(dns2, "0.0.0.0", sizeof("0.0.0.0"))) {	//no return dns
            PHS_LOGD
                ("cvt_ppp_up_rsp PPP_UP ppp_index=%d   return dns2:%s",
                 ppp_index, dns2);
            strlcpy(ppp_info[ppp_index].dns2addr,
                    ppp_info[ppp_index].userdns2addr,
                    sizeof(ppp_info[ppp_index].dns2addr));
        }
        ppp_info[ppp_index].state = PPP_STATE_ACTIVE;
        ps_service_ppp_up_notify(ppp_index);
        PHS_LOGD
            ("PS: PPP_STATE_ACTIVE index=%d iplocal:%s,ipremote:%s,dns1:%s,dns2:%s\n",
             ppp_index, tmp_ip_local, tmp_ip_remote, dns1, dns2);
        return AT_RESULT_OK;
    } while (0);
    ppp_info[ppp_index].state = PPP_STATE_EST_UP_ERROR;
    ps_service_ppp_up_notify(ppp_index);
    return AT_RESULT_OK;
}

int cvt_ppp_down_rsp(char *rsp_str, int len)
{
    int ppp_index;
    int err;
    char *input = rsp_str;
    char *out;
    cmux_t *mux;
    pty_t *ind_pty=NULL;
    char at_cmd_str[MAX_AT_CMD_LEN];
    char cgev_str[MAX_AT_CMD_LEN];

    PHS_LOGD("PPP_DOWN:%s", rsp_str);
    err = at_tok_start(&input, '=');
    if (err < 0) {
        return AT_RESULT_NG;
    }
    err = at_tok_nextint(&input, &ppp_index);
    if (err < 0) {
        return AT_RESULT_NG;
    }
    if (ppp_index >= MAX_PPP_NUM) {
        return AT_RESULT_NG;
    }
    switch (ppp_info[ppp_index].state) {
        case PPP_STATE_ESTING:
            PHS_LOGD("start ppp failure\n");
            ppp_info[ppp_index].state = PPP_STATE_EST_ERROR;
            ps_service_ppp_up_notify(ppp_index);
            PHS_LOGD("PPP_STATE_IDLE\n");

            /*here maybe do more to deactive pdp */
            break;
        case PPP_STATE_ACTIVE:
            PHS_LOGD("ppp connection terminate \n");
            mutex_lock(&ps_service_mutex);
            ppp_info[ppp_index].state = PPP_STATE_DEACTING;
#if defined CONFIG_SINGLE_SIM
            ind_pty = adapter_get_default_ind_pty();
            if(ind_pty != NULL)
            {
                sprintf(cgev_str,"\r\n+CGEV:NW DEACT IP,%s,%d\r",ppp_info[ppp_index].ipladdr,ppp_info[ppp_index].cid);
                adapter_pty_write(ind_pty, cgev_str, strlen(cgev_str));
            }
            PHS_LOGD("phoneserver send CGDEACT to make sure modem disconnect!\n");
            mux = adapter_get_cmux(AT_CMD_TYPE_PS, TRUE);
            sprintf(at_cmd_str, "AT+CGACT=0,%d\r", ppp_index+1);
            adapter_cmux_register_callback(mux,
                    (void *)cvt_cgact_deact_rsp1,
                    ppp_index);
            adapter_cmux_write_for_ps( mux, at_cmd_str, strlen(at_cmd_str), 150);
            ppp_info[ppp_index].state = PPP_STATE_IDLE;
            adapter_free_cmux_for_ps(mux);
            mutex_unlock(&ps_service_mutex);
            PHS_LOGD("PPP_STATE_IDLE\n");
#endif
            break;
            /*here maybe do more to deactive pdp and notify rild */
        case PPP_STATE_DESTING:
            ps_service_ppp_up_notify(ppp_index);
            PHS_LOGD("PPP_STATE_IDLE\n");
        default:
            break;
    }
    return AT_RESULT_OK;
}

int cvt_cgact_deact_req(AT_CMD_REQ_T * req)
{
    cmux_t *mux;
    int status, tmp_cid;		/*first parameter */
    int err;
    char cmd[MAX_CMD], at_cmd_str[MAX_AT_CMD_LEN], 
            cgev_str[MAX_AT_CMD_LEN];
    char *at_in_str;
    char prop[10];
    char linker[64] = {0};

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
    err = at_tok_nextint(&at_in_str, &tmp_cid);
    if (err < 0) {
        return AT_RESULT_NG;
    }

    mutex_lock(&ps_service_mutex);
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    //if ((tmp_cid <= 3) && (ppp_info[tmp_cid - 1].state == PPP_STATE_ACTIVE)) { 	/*deactivate PDP connection */
    if (tmp_cid <= 3) {	/*deactivate PDP connection */
        ppp_info[tmp_cid-1].state = PPP_STATE_DESTING;
        PHS_LOGD("PPP_STATE_DEACTING\n");
        ppp_info[tmp_cid-1].state = PPP_STATE_DEACTING;
        ppp_info[tmp_cid - 1].cmux = mux;
        snprintf(at_cmd_str,sizeof(at_cmd_str), "AT+CGACT=0,%d\r",tmp_cid);
        adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp2, (int)req->recv_pty);
        adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                req->timeout);
        ppp_info[tmp_cid - 1].state = PPP_STATE_IDLE;

#ifdef CONFIG_VETH
        usleep(200*1000);
        /* set property */
        snprintf(linker, sizeof(linker), "veth%d %s down", tmp_cid-1, "0.0.0.0");
        property_set(SYS_IFCONFIG_DOWN, linker);
        /* start data_off */
        property_set("ctl.start", "data_off");

        sprintf(cmd, "setprop net.veth%d.ip %s", tmp_cid-1,"0.0.0.0");
        system(cmd);
        sprintf(cmd, "setprop net.veth%d.dns1 \"\"", tmp_cid-1);
        system(cmd);
        sprintf(cmd, "setprop net.veth%d.dns2 \"\"", tmp_cid-1);
        system(cmd);
#endif
    } else {
        snprintf(at_cmd_str, sizeof(at_cmd_str), "AT+CGACT=0,%d\r", tmp_cid);
        adapter_cmux_register_callback(mux, cvt_cgact_deact_rsp,
                (int)req->recv_pty);
        adapter_cmux_write(mux, at_cmd_str, strlen(at_cmd_str),
                req->timeout);
    }
    mutex_unlock(&ps_service_mutex);
    return AT_RESULT_PROGRESS;
}

int cvt_cgact_deact_rsp(AT_CMD_RSP_T * rsp, int user_data)
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

int cvt_cgact_deact_rsp1(AT_CMD_RSP_T * rsp, int user_data)
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

int cvt_cgact_deact_rsp2(AT_CMD_RSP_T * rsp, int user_data)
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

int cvt_cgact_deact_rsp3(AT_CMD_RSP_T * rsp, int user_data)
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
            (int)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgact_act_rsp(AT_CMD_RSP_T * rsp, int user_data)
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
            (int)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgdcont_read_rsp(AT_CMD_RSP_T * rsp, int user_data)
{
    int ret, err;
    int tmp_cid = 0;
    int in_len;
    char *input, *out;
    char at_cmd_str[MAX_AT_CMD_LEN], ip[30], net[30];

    if (rsp == NULL) {
        return AT_RESULT_NG;
    }

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
            PHS_LOGD("\n  cvt_cgdcont_read_rsp cid =%d\n", tmp_cid);
            if ((tmp_cid <= MAX_PPP_NUM)
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
            } else if ((tmp_cid <= MAX_PPP_NUM)) {
                snprintf(at_cmd_str, sizeof(at_cmd_str),
                        "+CGDCONT:%d,\"%s\",\"%s\",\"%s\",0,0,\"%s\",\"%s\"\r",
                        tmp_cid, ip, net, "0.0.0.0","0.0.0.0","0.0.0.0");
            } else {
                return AT_RESULT_OK;
            }
            at_cmd_str[MAX_AT_CMD_LEN -1] = '\0';
            PHS_LOGD
                ("\n  cvt_cgdcont_read_rsp pty_write cid =%d resp:%s\n",
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
    char at_str[200], ip[30], net[30], ipladdr[30], hcomp[30], dcomp[30];
    char *out;
    int err = 0, ret = 0;
    char at_cmd_str[MAX_AT_CMD_LEN];

    if (req == NULL) {
        return AT_RESULT_NG;
    }
    input = req->cmd_str;

    memset(at_str, 0, 200);
    memset(ip, 0, 30);
    memset(net, 0, 30);
    memset(ipladdr, 0, 30);
    memset(hcomp, 0, 30);
    memset(dcomp, 0, 30);
    at_tok_start(&input, '=');
    at_tok_nextint(&input, &tmp_cid);

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

    if(tmp_cid <= MAX_PPP_NUM)
    {
        strcpy(ppp_info[tmp_cid - 1].userdns1addr, "0.0.0.0");
        strcpy(ppp_info[tmp_cid - 1].userdns2addr, "0.0.0.0");
        ppp_info[tmp_cid-1].manual_dns = 0;
    }

    //dns1 , info used with cgdata
    err = at_tok_nextstr(&input, &out);	//dns1
    if (err < 0)
        goto end_req;
    if(tmp_cid <= MAX_PPP_NUM && *out!=0)
    {
        strncpy(ppp_info[tmp_cid - 1].userdns1addr, out, sizeof(ppp_info[tmp_cid - 1].userdns1addr));
        ppp_info[tmp_cid - 1].userdns1addr[sizeof(ppp_info[tmp_cid - 1].userdns1addr)-1] = '\0';
    }

    //dns2  , info used with cgdata
    err = at_tok_nextstr(&input, &out);	//dns2
    if (err < 0)
        goto end_req;

    if(tmp_cid <= MAX_PPP_NUM && *out!=0)
    {
        strncpy(ppp_info[tmp_cid - 1].userdns2addr, out, sizeof(ppp_info[tmp_cid - 1].userdns2addr));
        ppp_info[tmp_cid - 1].userdns2addr[sizeof(ppp_info[tmp_cid - 1].userdns2addr)-1] = '\0';
    }

    //cp dns to ppp_info ?
end_req:

    if(tmp_cid <=MAX_PPP_NUM)
    {
        if (strncasecmp(ppp_info[tmp_cid-1].userdns1addr, "0.0.0.0",
                    strlen("0.0.0.0"))) {
            ppp_info[tmp_cid-1].manual_dns = 1;
        }
    }

    //make sure ppp in idle
    mutex_lock(&ps_service_mutex);
#ifndef  CONFIG_VETH
    if(ppp_info[tmp_cid -1].state != PPP_STATE_IDLE)
    {
        PHS_LOGD("cgdata:state error \n");
        mutex_unlock(&ps_service_mutex);
        return AT_RESULT_NG;
    }
#endif
    mux = adapter_get_cmux(req->cmd_type, TRUE);
    mutex_unlock(&ps_service_mutex);

    snprintf(at_str, sizeof(at_str), "AT+CGDCONT=%d,\"%s\",\"%s\",\"%s\",%s,%s\r", tmp_cid, ip,
            net, ipladdr, dcomp, hcomp);
    len = strlen(at_str);
    PHS_LOGD("\nPS:%s\n", at_str);

    req->cmd_str = at_str;
    req->len = len;

    adapter_cmux_register_callback(mux, cvt_cgdcont_set_rsp,
            (int)req->recv_pty);
    adapter_cmux_write(mux, req->cmd_str, req->len, req->timeout);
    return AT_RESULT_PROGRESS;
}

int cvt_cgdcont_set_rsp(AT_CMD_RSP_T * rsp, int user_data)
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

