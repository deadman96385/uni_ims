/*
 *
 * debug.h: debug.h implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#ifndef __PHS_DEBUG_H
#define __PHS_DEBUG_H

#define LOG_TAG "phoneserver"
#include <utils/Log.h>

//#define PHS_DEBUG

#ifdef PHS_DEBUG
#define PHS_LOGD(x...) ALOGD( x )
#define PHS_LOGE(x...) ALOGE( x )
#else
#define PHS_LOGD(x...) do {} while(0)
#define PHS_LOGE(x...) do {} while(0)
#endif

#endif
