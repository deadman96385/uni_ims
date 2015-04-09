/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#include <jni.h>
#include <stdlib.h>
#include <osal.h>
#include <vci.h>
#include <vier.h>


#define LOG_TAG "VIDEO_JNI"

#ifdef VIDEO_DEBUG_LOG
# define DBG(fmt, args...) \
        OSAL_logMsg("%s:%d (%s) " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#define ERR(fmt, args...) \
        OSAL_logMsg("%s:%d: (%s) ERROR" fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#else
# define DBG(fmt, args...)
# define ERR(fmt, args...)
#endif

extern char OSAL_msgQPathName[256];

/*
 * The JNI's access the Video Controller interface.
 * This mutex synchronizes the Video Controller interface accesses.
 */
static OSAL_SemId mVideoMutex;

/*
 * ======== init() ========
 * Starts the video task(s).
 * This function is called from Java.
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: init
 * Signature: (Ljava/lang/String;)I
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_init(
        JNIEnv *env_ptr,
        jclass cls,
        jstring str0)
{
    DBG("JNI - init()\n");
    
    /* Set up the path to use for OSAL Queues etc. */
    char *utf8_Osal_ptr = 0;
    utf8_Osal_ptr = (char *)(*env_ptr)->GetStringUTFChars(env_ptr, str0, 0);

    if (0 != utf8_Osal_ptr[0]) {
        DBG("using OSAL PATH:%s", utf8_Osal_ptr);
        strncpy(OSAL_msgQPathName, utf8_Osal_ptr, 256);
    }
    else {
        DBG("OSAL PATH NOT SET");
    }

    /* Available to use. */
    mVideoMutex = OSAL_semCountCreate(1);
    /*
     * Initialize VIER.
     */
    if (OSAL_FAIL == VIER_init()) {
        OSAL_logMsg("%s:%d Init vier failed.\n", __FUNCTION__, __LINE__);
        return (0);
    }
    (*env_ptr)->ReleaseStringUTFChars(env_ptr, str0, utf8_Osal_ptr);
    DBG("Video Controller starting\n");
    VCI_init();
    DBG("Video Controller started\n");
    
    return(0);
}

/*
 * ======== shutdown() ========
 * Stops the video task(s).
 * This function is called from Java.
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: shutdown
 * Signature: ()I
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_shutdown(
        JNIEnv *env_ptr,
        jclass cls)
{
    DBG("Video Controller stopping\n");
    VCI_shutdown();
    DBG("Video Controller stopped\n");
    /* Shutdown VIER */
    VIER_shutdown();
    DBG("Video task stopped\n");

    return(0);
}

/*
 * ======== sendFIR() ========
 *   Commands the video controller to send an RTCP FIR, also called an
 *   "instantaneous decoder refresh request". This requests the remote
 *   end to send a key-frame. This should be called when starting playback
 *   for the first time, as well as any time playback is restarted. See
 *   RFC 5104 section 3.5.1 for a description of the network details.
 * This function is called from Java.
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: sendFIR
 * Signature: ()I
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_sendFIR(
        JNIEnv *env_ptr,
        jclass cls)
{
    DBG("Requesting video engine to send RTCP\n");
    VCI_sendFIR();

    return(0);
}

/*
 * ======== getEvent() ========
 * Get Event from Video Controller.
 * This function is called from Java.
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: getEvent
 * Signature: ()I
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_getEvent(
        JNIEnv *env_ptr,
        jclass cls,
        jobject eventPtr,
        jobject eventDescPtr,
        jint timeout)
{
    vint     ret;
    VC_Event event;
    char     eventDesc[VCI_EVENT_DESC_STRING_SZ];
    jstring    str;
    jclass     clz;
    jmethodID  mid;
    vint     codecType;
    DBG("JNI - getEvent Enter \n");
    /* Currently, we only support H264 in Android.
     * "codecType" is not used.
     */
    ret = VCI_getEvent(&event, eventDesc, &codecType, timeout);
    DBG("VC App Event: %d", event);

    clz = (*env_ptr)->GetObjectClass(env_ptr, eventPtr);
    mid = (*env_ptr)->GetMethodID(env_ptr, clz, "set", "(I)V");
    (*env_ptr)->CallVoidMethod(env_ptr, eventPtr, mid, event);

    clz = (*env_ptr)->GetObjectClass(env_ptr, eventDescPtr);
    mid = (*env_ptr)->GetMethodID(env_ptr, clz, "set",
            "(Ljava/lang/String;)V");
    str = (*env_ptr)->NewStringUTF(env_ptr, eventDesc);
    (*env_ptr)->CallVoidMethod(env_ptr, eventDescPtr, mid, str);
    (*env_ptr)->DeleteLocalRef(env_ptr, str);

    DBG("JNI - getEvent Exit \n");

    return (ret);
}

/*
 * ======== sendH264EncodedFrame() ========
 * JNI call to pass EncodedFrame which should be sent as RTP.
 *
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: sendH264EncodedFrame
 * Signature: "(ILcom/d2tech/ica/ui/videocodecengine/util/EncodedFrame;)I";
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_sendH264EncodedFrame(
        JNIEnv *env_ptr,
        jclass cls,
        jobject encodedFrame)
{
    jclass     encodedFrameClass;
    jmethodID  midGetEncodedData;
    jmethodID  midGetTimeStamp;
    jmethodID  midGetRcsRtpExtnPayload;
    jbyteArray array;
    jlong tsMs;

    VC_EncodedFrame frame = {NULL, 0, 0, 0, 0};

    uint8 rcsRtpExtnPayload;
    vint length;

    DBG("JNI - send H264 Data - Enter\n");

    /* Wait till mutex is ready. */
    OSAL_semAcquire(mVideoMutex, OSAL_WAIT_FOREVER);

    /* get the EncodedFrame class. */
    encodedFrameClass = (*env_ptr)->GetObjectClass(env_ptr, encodedFrame);

    /* get the method ids. */
    midGetEncodedData = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "getEncodedData", "()[B");
    midGetTimeStamp = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "getTimestamp", "()J");
    midGetRcsRtpExtnPayload = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "getRcsRtpExtnPayload", "()B");
    
    /* get encoded data as jbyteArray from the encodedFrame. */
    array = (jbyteArray) (*env_ptr)->CallObjectMethod(env_ptr, encodedFrame, midGetEncodedData);

    /* Get the jbyteArray elements into jbyte ptr. */
    length = (*env_ptr)->GetArrayLength(env_ptr, array);
    jbyte* bufferPtr = (*env_ptr)->GetByteArrayElements(env_ptr, array, NULL);
    frame.data_ptr = OSAL_memAlloc(length * sizeof(uint8), 0);
    OSAL_memCpy(frame.data_ptr, bufferPtr, length);

    /* Get Timestamp from the encodedFrame. */
    tsMs = (*env_ptr)->CallLongMethod(env_ptr, encodedFrame, midGetTimeStamp);
    
    /* Get RTP Header Extension Payload from the encodedFrame. */
    rcsRtpExtnPayload = (*env_ptr)->CallByteMethod(env_ptr, encodedFrame, midGetRcsRtpExtnPayload);

    /* Release the jbyteArray. */
    (*env_ptr)->ReleaseByteArrayElements(env_ptr, array, bufferPtr, 0);

    frame.tsMs = tsMs;
    frame.length = length;
    frame.rcsRtpExtnPayload = rcsRtpExtnPayload;

    /* Send the data to the Video Controller. */
    VCI_sendEncodedFrame(&frame);

    /* Give the mutex. */
    OSAL_semGive(mVideoMutex);

    DBG("JNI - send H264 Data - Exit\n");

    return(0);
}

/*
 * ======== getH264EncodedFrame() ========
 * JNI call to get EncodedFrame which decoded in java.
 * Class: com_d2tech_ica_ui_videocodecengine_util_VideoJni
 * Method: getH264EncodedFrame
 * Signature: "(ILcom/d2tech/ica/ui/videocodecengine/util/EncodedFrame;)I";
 * Returns:
 *   0: Success.
 *  !0: Failed.
 */
JNIEXPORT jint JNICALL Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_getH264EncodedFrame(
        JNIEnv *env_ptr,
        jclass cls,
        jobject encodedFrame)
{
    jclass     encodedFrameClass;
    jmethodID  midSetEncodedData;
    jmethodID  midSetFlags;
    jmethodID  midSetRcsRtpExtnPayload;

    jbyteArray data;

    vint ret = -1;
    
    VC_EncodedFrame frame = {NULL, 0, 0, 0, 0};

    DBG("JNI - get H264 Data - Enter\n");

    /* Wait till mutex is ready. */
    OSAL_semAcquire(mVideoMutex, OSAL_WAIT_FOREVER);

    ret = VCI_getEncodedFrame(&frame);

    if (frame.length > 0) {
        DBG("get H264 Data - successful -  len: %d \n", frame.length);
        data = (*env_ptr)->NewByteArray(env_ptr, frame.length);
        (*env_ptr)->SetByteArrayRegion(env_ptr, data, 0, frame.length, (jbyte const *) frame.data_ptr);

        /* get the EncodedFrame class. */
        encodedFrameClass = (*env_ptr)->GetObjectClass(env_ptr, encodedFrame);
        /* get the method ids. */
        midSetEncodedData = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "setEncodedData", "([B)V");
        midSetFlags = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "setFlags", "(I)V");
        midSetRcsRtpExtnPayload = (*env_ptr)->GetMethodID(env_ptr, encodedFrameClass, "setRcsRtpExtnPayload", "(B)V");

        (*env_ptr)->CallVoidMethod(env_ptr, encodedFrame, midSetEncodedData, data);
        (*env_ptr)->CallVoidMethod(env_ptr, encodedFrame, midSetFlags, frame.flags);
        (*env_ptr)->CallVoidMethod(env_ptr, encodedFrame, midSetRcsRtpExtnPayload, frame.rcsRtpExtnPayload);
    }

    /* Give the mutex. */
    OSAL_semGive(mVideoMutex);

    DBG("JNI - get H264 Data - Exit\n");

    return ret;
}

static JNINativeMethod _JNIVE_methodTable[] = {
    { "init",                 "(Ljava/lang/String;)I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_init},
    { "shutdown",             "()I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_shutdown},
    { "sendFIR","()I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_sendFIR},
    { "getEvent",             "(Lcom/d2tech/isi/IntPointer;Lcom/d2tech/isi/StringPointer;I)I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_getEvent},
    { "sendH264EncodedFrame", "(Lcom/d2tech/ica/ui/videocodecengine/model/EncodedFrame;)I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_sendH264EncodedFrame},
    { "getH264EncodedFrame",  "(Lcom/d2tech/ica/ui/videocodecengine/model/EncodedFrame;)I",
        Java_com_d2tech_ica_ui_videocodecengine_util_VideoJni_getH264EncodedFrame},
};

/*
 * Store the JNI arguments for calls into the class.
 */
static jclass _JNIVE_clazz = NULL;
static JNIEnv *_JNIVE_env  = NULL;
/*
 * This function is called to register routines for the native C functions.
 */
static int _JNIVE_registerNativeMethods(
  JNIEnv                *env,
  const char            *className,
  const JNINativeMethod *gMethods,
  int                    numMethods)
{
  _JNIVE_env = env;
  _JNIVE_clazz = (*_JNIVE_env)->FindClass(env, className);
  if (NULL == _JNIVE_clazz) {
    return (-1);
  }
  if ((*env)->RegisterNatives(_JNIVE_env, _JNIVE_clazz, gMethods, numMethods) < 0) {
    return (-1);
  }
  return (0);
}

/*
 * This is the first routine that the JVM will call after loading the library.
 * The return value is the JNI version that is supported. If this function does
 * not exist, the version is assumed to be 1.1. In addition to this, this method
 * registers the native C routines for each exported routine. This is both a
 * memory saving and performance improving operation.
 */
jint JNI_OnLoad(
  JavaVM *vm, 
  void   *reserved)
{
  JNIEnv *env = NULL;
  if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_4) != JNI_OK) {
    return (-1);
  }
  
  if (_JNIVE_registerNativeMethods(env, "com/d2tech/ica/ui/videocodecengine/util/VideoJni",
      _JNIVE_methodTable, sizeof(_JNIVE_methodTable) / sizeof(JNINativeMethod)) != 0) {
    return (-1);
  }
  return (JNI_VERSION_1_4);
}
