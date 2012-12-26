#ifndef _SRIL_GPRS_H
#define _SRIL_GPRS_H

void sril_GprsHander(int request, void *data, size_t datalen, RIL_Token t);
void sril_OemSetDormancy(int request, void *data, size_t datalen, RIL_Token t);

#endif //_SRIL_GPRS_H
