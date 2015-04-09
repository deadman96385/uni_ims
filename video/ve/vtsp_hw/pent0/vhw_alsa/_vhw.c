/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10825 $ $Date: 2009-11-18 16:03:22 -0800 (Wed, 18 Nov 2009) $
 */

#include "vhw.h"
#include "_vhw.h"

extern int _VHW_sleep; 
extern int _VHW_sleepOld; 

//#define _VHW_DISABLE_PCM_OUT
//#define _VHW_DISABLE_PCM_IN
//#define _VHW_DEBUG
//#define _VHW_DEBUG_VERBOSE

struct timeval  timeLast;
snd_pcm_t      *playObj;
snd_pcm_t      *captureObj;

OSAL_SemId writeMutex;
OSAL_SemId readMutex;

/*
 * Voice hardware object
 */
static _VHW_Obj *_VHW_ptr = NULL;

void _VHW_checkOverflow(
    void)
{
#ifndef _VHW_DISABLE_PCM_OUT
    if(_VHW_ptr->hwWrIdx == _VHW_ptr->appWrIdx) {
        _VHW_ptr->appWrIdx += _VHW_WR_HEADSTART;
        if (_VHW_ptr->appWrIdx >= _VHW_WR_BUF_HW_NUM) {
            _VHW_ptr->appWrIdx -= _VHW_WR_BUF_HW_NUM;
        }
#ifdef _VHW_DEBUG        
        printf("WRITE OVERFLOW:\thwWr=%d, appWr=%d\n", _VHW_ptr->hwWrIdx, 
                _VHW_ptr->appWrIdx);
#endif
    }
#endif
}

void _VHW_checkUnderflow(
    void)
{
#ifndef _VHW_DISABLE_PCM_IN
    if(_VHW_ptr->hwRdIdx == _VHW_ptr->appRdIdx) {
        if (_VHW_ptr->appRdIdx - _VHW_RD_HEADSTART >= _VHW_RD_BUF_HW_NUM) {
            _VHW_ptr->appRdIdx += _VHW_RD_BUF_HW_NUM - _VHW_RD_HEADSTART;
        }
        else {
            _VHW_ptr->appRdIdx -= _VHW_RD_HEADSTART;
        }
#ifdef _VHW_DEBUG 
        printf("READ UNDERFLOW:\thwWr=%d, appWr=%d\n", _VHW_ptr->hwRdIdx, 
                _VHW_ptr->appRdIdx);
#endif
    }
#endif
}

vint _VHW_readPcm(
    vint   hwRdIdx,
    uvint  numBytes)
{
    uint8  *dst_ptr;
    uvint   readBytes;
#ifndef _VHW_DISABLE_PCM_IN
#ifdef _VHW_DEBUG
    struct  timeval tp1;
    struct  timeval tp2;

    if (0 != gettimeofday(&tp1, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return (-1);
    }
#endif
    readBytes = 0;

    /* Get indexed read buffer pointer */
    dst_ptr = (uint8*)(&_VHW_ptr->pcmIn[hwRdIdx * _VHW_RD_BUF_SZ_SAMP]);

    /* read from device */
    readBytes = snd_pcm_readi(captureObj, dst_ptr, _VHW_RD_BUF_SZ_SAMP);
    if (readBytes <= 0) {
        printf("ERROR Read\n");
    }
#ifdef _VHW_DEBUG
    if (0 != gettimeofday(&tp2, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return (-1);
    }
    vint sec = tp2.tv_sec-tp1.tv_sec;
    vint usec = tp2.tv_usec-tp1.tv_usec;
    if (0 != sec) {
        usec += 1000000;
    }
    if (usec > 10000) {
        printf("Read time:\tmsec=%d,\tread=%d\t\tappRd=%d, hwRd=%d\n\n",
            usec>>10, readBytes, _VHW_ptr->appRdIdx, hwRdIdx);
    }
#endif
#endif
    return (readBytes); 
}

vint _VHW_writePcm(
    uvint hwWrIdx,
    uvint numBytes)
{
    uint8 *src_ptr;
    uvint  writeBytes;
#ifndef _VHW_DISABLE_PCM_OUT
#ifdef _VHW_DEBUG
    struct timeval tp1;
    struct timeval tp2;

    if (0 != gettimeofday(&tp1, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return (-1);
    }
#endif
    writeBytes = 0;
    src_ptr = (uint8*)(&_VHW_ptr->pcmOut[hwWrIdx * _VHW_WR_BUF_SZ_SAMP]);    

    /* write to device */
    writeBytes = snd_pcm_writei(playObj, src_ptr, _VHW_WR_BUF_SZ_SAMP);
    
    if (writeBytes <= 0) {
        printf("ERROR Write\n");
    }
#ifdef _VHW_DEBUG
    if (0 != gettimeofday(&tp2, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return (-1);
    }
    vint sec = tp2.tv_sec-tp1.tv_sec;
    vint usec = tp2.tv_usec-tp1.tv_usec;
    if (0 != sec) {
        usec += 1000000;
    }
    if (usec > 10000) {
        printf("Write time:\tmsec=%d,\twrite=%d\t\tappWr=%d, hwWr=%d\n\n",
                usec>>10, writeBytes, _VHW_ptr->appWrIdx, hwWrIdx);
    }
#endif
#endif
     return(writeBytes);
}

void _VHW_pcmReadWriteUpdate(
    void)
{
    /* 
     * Handle PCM_IN 
     */
#ifndef _VHW_DISABLE_PCM_IN
    /*
     * Update App Read Index and handle wrap
     */
    _VHW_ptr->appRdIdx++;
    if (_VHW_RD_BUF_APP_NUM == _VHW_ptr->appRdIdx) {
        _VHW_ptr->appRdIdx = 0;
    }
    _VHW_checkUnderflow();

    /* Set app read pointer for next 10ms */
    _VHW_ptr->curIn_ptr = 
            &_VHW_ptr->pcmIn[_VHW_ptr->appRdIdx * _VHW_PCM_BUF_SZ];
#endif

    /* 
     * Handle PCM_OUT
     */
#ifndef _VHW_DISABLE_PCM_OUT
    /*
     * Update App Read Index and handle wrap
     */
    _VHW_ptr->appWrIdx++;
    if (_VHW_WR_BUF_APP_NUM == _VHW_ptr->appWrIdx) {
        _VHW_ptr->appWrIdx = 0;
    }
    _VHW_checkOverflow();

    /* Set app write pointer for next 10ms */
    _VHW_ptr->curOut_ptr = 
            &_VHW_ptr->pcmOut[_VHW_ptr->appWrIdx * _VHW_PCM_BUF_SZ];
#endif
}


OSAL_TaskReturn _VHW_readTask(
    OSAL_TaskArg taskArg)
{
    printf("_VHW_readTask - START\n");

    while (1) { 
        if (1 == _VHW_ptr->readFlag) {
            OSAL_semAcquire(readMutex, OSAL_WAIT_FOREVER);
        
            /* Read data */
            _VHW_readPcm(_VHW_ptr->hwRdIdx, _VHW_RD_BUF_SZ_SAMP);
            
            /*
             * Increment HW Read Index and handle wrap.
             */
            _VHW_ptr->hwRdIdx++;
            if (_VHW_RD_BUF_HW_NUM == _VHW_ptr->hwRdIdx) {
                _VHW_ptr->hwRdIdx = 0;
            }
            
            OSAL_semGive(readMutex);
            
            _VHW_checkUnderflow();

            usleep(_VHW_RD_SLEEP_TIME);
        }
        else {
            usleep(1000);
        }
    }
    return (0);
}


OSAL_TaskReturn _VHW_writeTask(
    OSAL_TaskArg taskArg)
{
    printf("_VHW_writeTask - START");
    

    while (1) {
        /* Write data and increment outBytes */
        if (1 == _VHW_ptr->writeFlag) {
            OSAL_semAcquire(writeMutex, OSAL_WAIT_FOREVER);
        
            /* Write data */
            _VHW_writePcm(_VHW_ptr->hwWrIdx, _VHW_WR_BUF_SZ_SAMP);

            /*
             * Increment HW Write Index and handle wrap.
             */           
            _VHW_ptr->hwWrIdx++;
            if (_VHW_WR_BUF_HW_NUM == _VHW_ptr->hwWrIdx) {
                _VHW_ptr->hwWrIdx = 0;
            }
            _VHW_checkOverflow();

            OSAL_semGive(writeMutex);

            usleep(_VHW_WR_SLEEP_TIME);
        }
        else {
            usleep(1000);
        }
    }

    return (0);
}

int _VHW_startTask()
{
    OSAL_TaskArg taskArg;

    taskArg = _VHW_ptr;
    
    printf("_VHW_startTask: _VHW_ptr=%x\n", (int)_VHW_ptr);

    if ((OSAL_taskCreate("_VHW_readTask",
             _VHW_TASK_PRIORITY, _VHW_TASK_STACKSIZE,
             (void *)_VHW_readTask, taskArg)) <= 0) {
        printf("%s:%d", __FILE__,__LINE__);
        return (-1);
    }

    if ((OSAL_taskCreate("_VHW_writeTask",
             _VHW_TASK_PRIORITY, _VHW_TASK_STACKSIZE,
             (void *)_VHW_writeTask, taskArg)) <= 0) {
        printf("%s:%d", __FILE__,__LINE__);
        return (-1);
    }
    return (0);
}

/*
 * ======== _VHW_sleepForPcmData() ========
 * Application calls this function to get data from the PCM highway.
 * Four pointers are returned.
 * 1. Data transmit pointer. This is the place where the application can put
 *    data to be transmitted in the next block.
 * 2. Data receive pointer. This is the pointer where the application can
 *    read just arrived block of data.
 *
 * If data is not immediately available, this function will sleep till 
 * the data is not avaialable.
 *
 * Return Values:
 * None.
 */
void _VHW_sleepForPcmData(
    int **tx_ptr,
    int **rx_ptr)
{
    vint sample;

    if (_VHW_sleep) {
        if (_VHW_sleepOld != _VHW_sleep) {
            _VHW_sleepOld = _VHW_sleep;
            _VHW_stopVoice();
        }
        usleep(10000);
    }
    else {
        if (_VHW_sleepOld != _VHW_sleep) {
            _VHW_sleepOld = _VHW_sleep;
            /* Start/Re-start voice */
            _VHW_startVoice();
        }
        /* Fill TX */
        for (sample = 0; sample < _VHW_PCM_BUF_SZ; sample++) {
            _VHW_ptr->curOut_ptr[sample] = 
                    (int16) (_VHW_ptr->pcmXmit_ary[sample]);
        }
        /* Fill RX */
        for (sample = 0; sample < _VHW_PCM_BUF_SZ; sample++) {
            _VHW_ptr->pcmRecv_ary[sample] = 
                    (vint)(_VHW_ptr->curIn_ptr[sample] >> 1);
        }
        /* Update Time to compute duration to block */
        _VHW_10msWait();

        /* Update Read / Write buffers */
        _VHW_pcmReadWriteUpdate();

        _VHW_ptr->count10ms++;
        if (100 == _VHW_ptr->count10ms) {
//            printf(". \n");
            _VHW_ptr->count10ms = 0;
        }
    }
    /*
     * Conversion buffers
     */
    *rx_ptr = _VHW_ptr->pcmRecv_ary;
    *tx_ptr = _VHW_ptr->pcmXmit_ary;
}

static void _VHW_closePcmIn(
    void)
{
    OSAL_semAcquire(readMutex, OSAL_WAIT_FOREVER);
    /*
     * Close capture device 
     */
    snd_pcm_close(captureObj);
    
    OSAL_semGive(readMutex);
}

static vint _VHW_openPcmIn(
    void) 
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t    bufSz;
    snd_pcm_uframes_t    periodSz;
    int                  err;

    periodSz = _VHW_RD_BUF_SZ_SAMP;
    bufSz    = 10 * periodSz;

    /*
     * Open PCM_IN Device FILE 
     */
    if ((err = snd_pcm_open (&captureObj, VHW_DEVNAME_DEF, 
            SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        printf("cannot open audio device %s (%s)\n", 
             VHW_DEVNAME_DEF,     snd_strerror(err));
    }
    printf("opened default capture device\n");
    
    /* 
     * Set PCM_IN configs 
     */
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        printf("cannot allocate hardware parameter structure (%s)\n",
             snd_strerror(err));
    }
             
    if ((err = snd_pcm_hw_params_any(captureObj, hw_params)) < 0) {
        printf("cannot initialize hardware parameter structure (%s)\n",
             snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_access(captureObj, hw_params, 
            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("cannot set access type (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_format(captureObj, hw_params, 
            SND_PCM_FORMAT_S16_LE)) < 0) {
        printf("cannot set sample format (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_rate(captureObj, hw_params, 
            _VHW_SAMPLE_RATE, 0)) < 0) {
        printf("%s:%d\n",  __FILE__, __LINE__);
        printf("cannot set sample rate (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_channels(captureObj, hw_params, 1)) < 0) {
        printf("cannot set channel count (%s)\n", snd_strerror(err));
    }
    
    if ((err = snd_pcm_hw_params_set_buffer_size_near(captureObj, hw_params,
            &bufSz)) < 0) {
        printf("%s:%d\n",  __FILE__, __LINE__);
        printf("cannot set buffer size (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_period_size_near(captureObj, hw_params,
            &periodSz, NULL)) < 0) {
        printf("%s:%d\n",  __FILE__, __LINE__);
        printf("cannot set period size (%s)\n", snd_strerror(err));
    }
   
    printf("DUMP params: CAPTURE\n");
    _VHW_dumpSndPcmParams(hw_params);

    /* Set HW params to ALSA device */
    if ((err = snd_pcm_hw_params(captureObj, hw_params)) < 0) {
        printf("cannot set parameters (%s)\n", snd_strerror(err));
    }
 
    snd_pcm_hw_params_free(hw_params);

    if ((err = snd_pcm_prepare(captureObj)) < 0) {
        printf("cannot prepare audio interface for use (%s)\n",
             snd_strerror(err));
    }
    return (0);
}

static void _VHW_closePcmOut(
    void)
{
    OSAL_semAcquire(writeMutex, OSAL_WAIT_FOREVER);
    
    /*
     * Close playback device
     */
    snd_pcm_close(playObj);
    
    OSAL_semGive(writeMutex);
}


static vint _VHW_openPcmOut(
    void)
{
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t    bufSz;
    snd_pcm_uframes_t    periodSz;
    int                  err;

    /* 
     * Open PCM_OUT Device FILE
     */
    periodSz = _VHW_WR_BUF_SZ_SAMP;
    bufSz    = 10 * periodSz;

    if ((err = snd_pcm_open(&playObj, VHW_DEVNAME_DEF,
            SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("cannot open audio device %s (%s)\n", 
                 VHW_DEVNAME_DEF, snd_strerror(err));
        return (-1);
    }
    printf("open default playback device\n");
    
    /* 
     * Set PCM_OUT configs
    */
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        printf("cannot allocate hardware parameter structure (%s)\n",
             snd_strerror(err));
    }
             
    if ((err = snd_pcm_hw_params_any(playObj, hw_params)) < 0) {
        printf("cannot initialize hardware parameter structure (%s)\n",
             snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_access(playObj, hw_params,
            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("cannot set access type (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_format(playObj, hw_params,
            SND_PCM_FORMAT_S16_LE)) < 0) {
        printf("cannot set sample format (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_rate(playObj, hw_params,
             _VHW_SAMPLE_RATE, 0)) < 0) {
        printf("cannot set sample rate (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_channels(playObj, hw_params, 1)) < 0) {
        printf("cannot set channel count (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_buffer_size_near(playObj, hw_params,
            &bufSz)) < 0) {
        printf("%s:%d\n",  __FILE__, __LINE__);
        printf("cannot set buffer size (%s)\n", snd_strerror(err));
    }

    if ((err = snd_pcm_hw_params_set_period_size_near(playObj, hw_params,
            &periodSz, NULL)) < 0) {
        printf("%s:%d\n",  __FILE__, __LINE__);
        printf("cannot set period size (%s)\n", snd_strerror(err));
    }
    
    printf("DUMP params: PLAYBACK after \n");
    _VHW_dumpSndPcmParams(hw_params);

    if ((err = snd_pcm_hw_params(playObj, hw_params)) < 0) {
        printf("cannot set parameters (%s)\n", snd_strerror(err));
    }

    snd_pcm_hw_params_free(hw_params);

    if ((err = snd_pcm_prepare(playObj)) < 0) {
        printf("cannot prepare audio interface for use (%s)\n",
             snd_strerror(err));
    }
    return (0);
}

/*
 * ======== _VHW_allocHw() ========
 * This function inits the interface to the low level hardware of 
 * the host processor.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes).
 */
int _VHW_allocHw(
    void)
{
     /* Allocate _VHW_OBj */
    _VHW_ptr = (_VHW_Obj *) OSAL_memCalloc(1, sizeof(_VHW_Obj), 0);

    writeMutex = OSAL_semMutexCreate();
    readMutex = OSAL_semMutexCreate();
    
    /* Init blocking semapohre */
    if (-1 == sem_init(&_VHW_ptr->sema, 0, 0)) {
        printf("%s:%d FAIL sem_init()", __FILE__, __LINE__);
        return (-1);
    }

    /* 
     * Zero TX/RX buffers 
     */
    memset(_VHW_ptr->pcmOut, 0, sizeof(_VHW_ptr->pcmOut));
    memset(_VHW_ptr->pcmIn, 0, sizeof(_VHW_ptr->pcmIn));
    memset(_VHW_ptr->pcmRecv_ary, 0, sizeof(_VHW_ptr->pcmRecv_ary));
    memset(_VHW_ptr->pcmXmit_ary, 0, sizeof(_VHW_ptr->pcmXmit_ary));

    /*
     * Start in sleeping state
     */
    _VHW_sleep = 1;
    _VHW_sleepOld = 1;
    
    return (0);
}

void _VHW_syncTime(
    void)
{
    struct timeval tp;

    if (0 != gettimeofday(&tp, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return;
    }

    _VHW_ptr->abst.tv_sec  = tp.tv_sec;
    _VHW_ptr->abst.tv_nsec = tp.tv_usec * 1000;

#ifdef _VHW_DEBUG
    memcpy(&timeLast, &tp, sizeof(tp));
    printf("GOT TIME tp sec=%d tp usec=%d\n",
            (vint)tp.tv_sec, (vint)tp.tv_usec); 
    printf("GOT TIME ts sec=%d ts nsec=%d\n",
            (vint)_VHW_ptr->abst.tv_sec, (vint)_VHW_ptr->abst.tv_nsec);
#endif
}

void _VHW_stopVoice(
    void)
{
    printf("_VHW_stopVoice\n");
    /*
     * STOP read / write tasks
     */
    _VHW_ptr->writeFlag  = 0;
    _VHW_ptr->readFlag  = 0;

    /*
     * Zero hw and app bufs
     */
    memset(_VHW_ptr->pcmOut, 0, sizeof(_VHW_ptr->pcmOut));
    memset(_VHW_ptr->pcmIn, 0, sizeof(_VHW_ptr->pcmIn));
    memset(_VHW_ptr->pcmRecv_ary, 0, sizeof(_VHW_ptr->pcmRecv_ary));
    memset(_VHW_ptr->pcmXmit_ary, 0, sizeof(_VHW_ptr->pcmXmit_ary));

    /* 
     * close the playback and capture devices
     */
    _VHW_closePcmIn();
    _VHW_closePcmOut();
}


void _VHW_startVoice(
    void)
{
    printf("%s:%d - _VHW_startVoice()\n", __FILE__, __LINE__);

    _VHW_ptr->count10ms  = 0;
    _VHW_ptr->curIn_ptr  = _VHW_ptr->pcmIn;
    _VHW_ptr->curOut_ptr = _VHW_ptr->pcmOut;
    _VHW_ptr->readFlag   = 0;
    _VHW_ptr->writeFlag  = 0;

    printf("TX: curOut_ptr=%x\n", (vint)_VHW_ptr->curOut_ptr);
    printf("RX: curIn_ptr=%x\n", (vint)_VHW_ptr->curIn_ptr);
    
    /* We open and close PCM IN dynamically in voice start and stop */
    if (0 != _VHW_openPcmIn()) {
        printf("FAILED to open and config PCM IN\n");
        return;
    }
    if (0 != _VHW_openPcmOut()) {
        printf("FAILED to open and config PCM OUT\n");
        return;
    }

    /* Set write HW / APP, app ahead of HW by X */
    _VHW_ptr->hwWrIdx = 0;
    _VHW_ptr->appWrIdx = _VHW_WR_HEADSTART;


    /* Set read HW / APP, HW ahead of HW by X */
    _VHW_ptr->hwRdIdx = _VHW_RD_HEADSTART;
    _VHW_ptr->appRdIdx = 0;

   /* 
    * Start tasks 
    */
#ifndef _VHW_DISABLE_PCM_IN
    _VHW_ptr->readFlag  = 1;
#endif
#ifndef _VHW_DISABLE_PCM_OUT
    _VHW_ptr->writeFlag = 1;
#endif

    /* Init time after buffers primed */
    _VHW_syncTime();

    return;
}

/*
 * ======== VHW_start() ========
 * Start the voice read and write tasks
 *
 * Return Values:
 * None.
 */
int _VHW_start(
    void)
{
    /* Start READ/WRITE tasks */
    _VHW_startTask();

    return (0);
}

/*
 * ======== VHW_shutdown() ========
 * This function closes the interface to the low level hardware of 
 * the host processor.
 *
 * Return Values:
 * None.
 */
void _VHW_shutdown(
    void)
{
    _VHW_closePcmIn();
    _VHW_closePcmOut();
    
    sem_destroy(&_VHW_ptr->sema);

    OSAL_semDelete(writeMutex);
    OSAL_semDelete(readMutex);
    
    OSAL_memFree(_VHW_ptr, 0);
}

/*
 * ======== _VHW_detach() ========
 * Puts VHW in low power mode by detaching audio.
 *
 * Return Values:
 */
void _VHW_detach(
    void)
{
    /*
     * This will cause the audio driver and vTSP to sleep.
     */
    _VHW_sleep = 1; 
}

/*
 * ======== _VHW_attach() ========
 * Puts VHW in full power mode by attaching audio.
 *
 * Return Values:
 */
void _VHW_attach(
    void)
{
    /*
     * This will cause the audio driver and vTSP to come out of sleep.
     */
    _VHW_sleep = 0;  
}


void _VHW_10msWait(
    void)
{
    struct timespec ts;
#ifdef _VHW_DEBUG
    struct timeval  timeNow;
    struct timeval  time10ms;
#endif
    
    _VHW_ptr->abst.tv_nsec  += 10 * 1000000;
       
    if (_VHW_ptr->abst.tv_nsec > 1000000000) {
        _VHW_ptr->abst.tv_nsec -= 1000000000;
        _VHW_ptr->abst.tv_sec  += 1;
    }
    memcpy(&ts, &_VHW_ptr->abst, sizeof(ts));
    
    /* block for remainder of 10ms */
    sem_timedwait(&_VHW_ptr->sema, &ts);

#ifdef _VHW_DEBUG_VERBOSE
    if (0 != gettimeofday(&timeNow, NULL)) {
        printf("%s:%d FAIL gettimeofday\n", __FILE__, __LINE__);
        return;
    }
    time10ms.tv_sec = timeNow.tv_sec - timeLast.tv_sec;
    time10ms.tv_usec = timeNow.tv_usec - timeLast.tv_usec;

    vint sec = time10ms.tv_sec;
    vint usec = time10ms.tv_usec;
    if (0 != sec) {
        usec += 1000000;
    }

    memcpy(&timeLast, &timeNow, sizeof(timeLast));

    printf("time10ms\tmsec=%d\t", usec>>10);
    printf("appRd=%d, hwRd=%d\t", _VHW_ptr->appRdIdx, _VHW_ptr->hwRdIdx); 
    printf("appWr=%d, hwWr=%d\n", _VHW_ptr->appWrIdx, _VHW_ptr->hwWrIdx); 
#endif
}

void _VHW_setVolume(
    _VHW_Obj *_VHW_ptr,
    uvint     device,
    uvint     volume)
{
    /* 
     * Set device volume
     */
}

void _VHW_printPcmConfig(
    void *config)
{
    /*
     * Print PCM Config 
     */
}

void _VHW_dumpBuffer(
        int16 *buf,
        vint   bytesRead)
{
    vint i;
    for (i = 0; i < bytesRead >> 3; i+=8) {
        printf("%d, %d, %d, %d, %d, %d, %d, %d\n", buf[i], buf[i+1], buf[i+2],
                buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
    }
}

void _VHW_dumpSndPcmParams(
    snd_pcm_hw_params_t *params)
{
    unsigned int val;
    snd_pcm_uframes_t frames;


    printf("_VHW_dumpSndPcmParams\n");

    snd_pcm_hw_params_get_access(params, &val);
    printf("hw->access=%d\n", val);
    snd_pcm_hw_params_get_format(params, (snd_pcm_format_t *)&val);
    printf("hw->format=%d\n", val);
    snd_pcm_hw_params_get_channels(params, &val);
    printf("hw->channels=%d\n", val);
    snd_pcm_hw_params_get_rate(params, &val, NULL);
    printf("hw->rate=%d\n", val);
    snd_pcm_hw_params_get_subformat(params, &val);
    printf("hw->subformat=%d\n", val);
    snd_pcm_hw_params_get_period_size(params, &frames, NULL);
    printf("hw->period_size=%lu\n", frames);
    snd_pcm_hw_params_get_period_time(params, &val, NULL);
    printf("hw->period_time=%d\n", val);
    snd_pcm_hw_params_get_buffer_size(params, &frames);
    printf("hw->buffer_size=%lu\n", frames);
}

