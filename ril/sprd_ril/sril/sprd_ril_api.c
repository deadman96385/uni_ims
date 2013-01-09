#include "sprd_ril_api.h"
#include "sril_svcmode.h"
#include <utils/Log.h>

/* DATA SWITCH */
void sprd_DataSwitch(int request, void *data, size_t datalen, RIL_Token t)
{
	int err = RIL_E_SUCCESS;
	ATResponse  *p_response = NULL;
	char *cmd;
	int channelID;

	channelID = getChannel();

	err = at_send_command(ATch_type[channelID], "AT+SPSSSGFD", &p_response);
	if (err < 0 || p_response->success == 0) {
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		at_response_free(p_response);
	} else {
		at_response_free(p_response);
		p_response = NULL;
		asprintf(&cmd, "AT+CGATT=1");
		err = at_send_command(ATch_type[channelID], cmd, &p_response);
		if (err < 0 || p_response->success == 0) {
			RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		} else {
			RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
		}
		free(cmd);
		at_response_free(p_response);
	}

	putChannel(channelID);
}

/* OEM_IMEI_SIM_OUT */
void sprd_IMEISimOut(int request, void *data, size_t datalen, RIL_Token t)
{
	int err = RIL_E_SUCCESS;
	ATResponse  *p_response = NULL;
	char *cmd;
	int channelID;

	channelID = getChannel();

	asprintf(&cmd, "AT");  //FIX ME HERE, wait modem to finish the AT command
	err = at_send_command(ATch_type[channelID], cmd, &p_response);
	if (err < 0 || p_response->success == 0) {
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
	} else {
		RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	}
	free(cmd);
	at_response_free(p_response);

	putChannel(channelID);
}

/* sprd interface implementation */

int sprd_BandSelect(BAND_TYPE_T band_type)
{
	int err, ret;
	ATResponse  *p_response = NULL;
	char *cmd;
	int channelID;

	channelID = getChannel();
	asprintf(&cmd, "AT+SBAND=%d", band_type);
	err = at_send_command(ATch_type[channelID], cmd, &p_response);
	if (err < 0 || p_response->success == 0) {
		ret = -1;
	} else {
		ret = 0;
	}
	free(cmd);
	at_response_free(p_response);
	putChannel(channelID);

	return ret;

}

BAND_TYPE_T sprd_GetCurrentBand(void)
{
	int err;
	ATResponse  *p_response = NULL;
	char *line;
	int value;
	int channelID;
	BAND_TYPE_T band_type = BANDINVALID;

	channelID = getChannel();
	err = at_send_command_singleline(ATch_type[channelID], "AT+SBAND?", "+SBAND:", &p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	} else {
		line = p_response->p_intermediates->line;
		err = at_tok_start(&line);
		if (err < 0) goto error;

		err = at_tok_nextint(&line, &value);
		if (err < 0) goto error;

		band_type = value;
	}
	at_response_free(p_response);
	putChannel(channelID);
	return band_type;
error:
	at_response_free(p_response);
	putChannel(channelID);
	return band_type;
}

int sprd_DebugScreen(char **testbuffer)
{
	int err;
	ATResponse  *p_response = NULL;
	char *line;
	int channelID;
	int rat, grr_state;
	int mcc, mnc;
	int band, arfcn;
	int rx_pwr, rx_qual;
	int rxlev, txlev;
	int chanal_mode;
	int bsic, lac;
	int cs_rej, ps_rej;
	int rx_ch, rssi;
	int tx_ch,tx_pwr;
	int ecn0, rscp;
	int psc;
	char *mm_str, *gmm_str;

	channelID = getChannel();
	err = at_send_command_singleline(ATch_type[channelID], "AT+DBGSCRN?", "+DBGSCRN:", &p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	} else {
		line = p_response->p_intermediates->line;
		err = at_tok_start(&line);
		if (err < 0) goto error;

		err = at_tok_nextint(&line, &rat);
		if (err < 0) goto error;

		if(rat == 0) {   /* GSM */
			err = at_tok_nextint(&line, &grr_state);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &mcc);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &mnc);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &band);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &arfcn);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rx_pwr);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rx_qual);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rxlev);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &txlev);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &chanal_mode);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &bsic);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &lac);
			if (err < 0) goto error;

			err = at_tok_nextstr(&line, &mm_str);
			if (err < 0) goto error;

			err = at_tok_nextstr(&line, &gmm_str);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &cs_rej);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &ps_rej);
			if (err < 0) goto error;

			sprintf(testbuffer[0], "GSM GRR STATE: %d", grr_state);
			sprintf(testbuffer[1], "MCC: %x, MNC: %x", mcc, mnc);
			sprintf(testbuffer[2], "Band: %d, Arfcn: %d", band, arfcn);
			sprintf(testbuffer[3], "Rx Pwr: -%d, Rx Qual: %d", rx_pwr, rx_qual);
			sprintf(testbuffer[4], "Rx Lev: %d, Tx Lev: %d", rxlev, txlev);
			sprintf(testbuffer[5], "Chanal Mode: %d", chanal_mode);
			sprintf(testbuffer[6], "Bsic: %d, LAC: %d", bsic, lac);
			sprintf(testbuffer[7], "MM STATUS: %s", mm_str);
			sprintf(testbuffer[8], "GMM STATUS: %s", gmm_str);
			sprintf(testbuffer[9], "CS REJECT: %d", cs_rej);
			sprintf(testbuffer[10], "PS REJECT: %d", ps_rej);
		} else if(rat == 1) {  /* WCDMA */
			err = at_tok_nextint(&line, &mcc);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &mnc);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rx_ch);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rssi);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &tx_ch);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &tx_pwr);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &ecn0);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &rscp);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &psc);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &lac);
			if (err < 0) goto error;

			err = at_tok_nextstr(&line, &mm_str);
			if (err < 0) goto error;

			err = at_tok_nextstr(&line, &gmm_str);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &cs_rej);
			if (err < 0) goto error;

			err = at_tok_nextint(&line, &ps_rej);
			if (err < 0) goto error;

			sprintf(testbuffer[0], "MCC: %x, MNC: %x", mcc, mnc);
			sprintf(testbuffer[1], "Rx CH: %d, Rssi: %d", rx_ch, rssi);
			sprintf(testbuffer[2], "Tx CH: %d, TxPwr: %d", tx_ch, tx_pwr);
			sprintf(testbuffer[3], "Ecn0: %d, RSCP: %d", ecn0, rscp);
			sprintf(testbuffer[4], "Chanal Mode: %d", chanal_mode);
			sprintf(testbuffer[5], "PSC: %d, LAC: %d", psc, lac);
			sprintf(testbuffer[6], "MM STATUS: %s", mm_str);
			sprintf(testbuffer[7], "GMM STATUS: %s", gmm_str);
			sprintf(testbuffer[8], "CS REJECT: %d", cs_rej);
			sprintf(testbuffer[9], "PS REJECT: %d", ps_rej);
		}
	}
	at_response_free(p_response);
	putChannel(channelID);
	return -1;
error:
	at_response_free(p_response);
	putChannel(channelID);
	return 0;
}

/*BAND SELECT*/
/* ++ JSHAN added Band Selection functions ++ */

void sril_SetBandAuto(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandAllGSM(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "RAT: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	if(sprd_BandSelect(BANDALLGSM))
		sprintf(tmp, "BAND: FAIL");
	else
		sprintf(tmp, "BAND: BAND_AUTO");

	strncat(pTmpRsp->string, tmp, MAX_SVCSTR_PER_LINE - strlen(pTmpRsp->string) - 1);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandGSM850(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "RAT: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	if(sprd_BandSelect(BANDGSM850))
		sprintf(tmp, "BAND: FAIL");
	else
		sprintf(tmp, "BAND: BAND_GSM850_ONLY");

	strncat(pTmpRsp->string, tmp, MAX_SVCSTR_PER_LINE - strlen(pTmpRsp->string) - 1);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandGSM900(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "RAT: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	if(sprd_BandSelect(BANDGSM900))
		sprintf(tmp, "BAND: FAIL");
	else
		sprintf(tmp, "BAND: BAND_GSM900_ONLY");

	strncat(pTmpRsp->string, tmp, MAX_SVCSTR_PER_LINE - strlen(pTmpRsp->string) - 1);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandDCS1800(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "RAT: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	if(sprd_BandSelect(BANDDCS1800))
		sprintf(tmp, "BAND: FAIL");
	else
		sprintf(tmp, "BAND: BAND_DCS_1800");

	strncat(pTmpRsp->string, tmp, MAX_SVCSTR_PER_LINE - strlen(pTmpRsp->string) - 1);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandPCS1900(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[10] = {0};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "RAT: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	if(sprd_BandSelect(BANDPCS1900))
		sprintf(tmp, "BAND: FAIL");
	else
		sprintf(tmp, "BAND: BAND_PCS1900_ONLY");

	strncat(pTmpRsp->string, tmp, MAX_SVCSTR_PER_LINE - strlen(pTmpRsp->string) - 1);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandAllUMTS(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandUMTS_1(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandUMTS_2(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandUMTS_4(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandUMTS_5(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_SetBandUMTS_8(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
        pTmpRsp->reverse = 0;

        sprintf(pTmpRsp->string, "FUCTION NOT IMPLEMENTED !!!");

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_GetCurrentBand(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char tmp[MAX_SVCSTR_PER_LINE] = {0};
	BAND_TYPE_T band_type = BANDINVALID;
	char *band_str[] = {
		"GSM900_ONLY",
		"DCS1800_ONLY",
		"PCS1900_ONLY",
		"GSM850_ONLY",
		"GSM900\\DCS1800",
		"GSM850\\GSM900",
		"GSM850\\DCS1800",
		"GSM850\\PCS1900",
		"GSM900\\PCS1900",
		"GSM850\\GSM900\\DCS1800",
		"GSM850\\GSM900\\PCS1900",
		"DCS1800\\PCS1900",
		"GSM850\\DCS1800\\PCS1900",
		"GSM900\\DCS1800\\PCS1900",
		"AUTO"		
	};

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);		

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "Type: GSM_ONLY");

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	band_type = sprd_GetCurrentBand();
	if(band_type == BANDINVALID)
		sprintf(tmp, "Band: UNKNOWN");
	else
		sprintf(tmp, "Band: %s", band_str[band_type]);

	sprintf(pTmpRsp->string, "%s", tmp);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

/* -- JSHAN added Band Selection functions -- */

/* DEBUG SCREEN*/
void sril_DebugScreen(int request, void *data, size_t datalen, RIL_Token t)
{
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	char *testbuffer[MAX_SVCMENU_LINE];
	char *tmp;
	int i;

	ALOGD("%s\n", __FUNCTION__);

	tmp = (char *)malloc(MAX_SVCSTR_PER_LINE*MAX_SVCMENU_LINE);
	if (!tmp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(tmp, 0, MAX_SVCSTR_PER_LINE*MAX_SVCMENU_LINE);

	for(i = 0; i < MAX_SVCMENU_LINE; i++) {
		testbuffer[i] = tmp;
		tmp += MAX_SVCSTR_PER_LINE;
	}	

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
		return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);	

	if(!sprd_DebugScreen(testbuffer)) {
		pTmpRsp = pSvcRsp;	// Line[0]
		for(i = 0; i < MAX_SVCMENU_LINE; i++) {
			pTmpRsp->line = i;
			pTmpRsp->reverse = 0;
			sprintf(pTmpRsp->string, "%s", testbuffer[i]);
			pTmpRsp++;
		}
	}
	free(testbuffer[0]);
	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}
