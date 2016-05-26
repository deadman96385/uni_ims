#include <gui/BufferQueue.h>
#include <gui/GLConsumer.h>
#include <binder/IMemory.h>
#include <utils/Log.h>
#include <semaphore.h>
#include <android/native_window_jni.h>
#include <android_runtime/android_graphics_SurfaceTexture.h>
#include <camera/CameraBase.h>
#include <camera/ICameraService.h>
#include <binder/IServiceManager.h>
#include <camera/CameraParameters.h>
#include <string.h>
#include <gui/Surface.h>

#include "VideoCallEngineCamera.h"

using namespace android;

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "VCECamera"
#endif

#define DEBUG_ENABLE 0
#if DEBUG_ENABLE
#define DLOG ALOGI
#else
#define DLOG(...)
#endif

sp<ANativeWindow> mPreviewNativeWindow;
sp<IGraphicBufferProducer> gproducer;

static const String16 *packageName = NULL;
static ImsCameraClient *cameraClient = NULL;

int cameraId = 1;
int usingFaceCamera = 1;
int sensorRotate = 0;
int numberOfCameras = 0;

camera_module_t *hw;
int previewTransform = 0;
int previewRotationAngle = 0;
int previewRotationOffset = 0;

int previewSurfaceReady = 0;
int startPreviewCalled = 0;
int startRecordingCalled = 0;

// default camera parmas
int fps = 30;
int height = 640;
int width = 480;

pthread_t cameraLoopThread;

int camera_cmd = 0;
void *cam_arg1;
void *cam_arg2;
void *cam_arg3;
void *camera_ret;

int processingFrameCount = 0;
int stoppingRecord = 0;

sp<ICameraService> cameraService;
sp<ICameraClient> client;

void *executeCommand(int command, void *arg1, void *arg2, void *arg3) {
  lockExecuteCommand();

  camera_cmd = command;
  cam_arg1 = arg1;
  cam_arg2 = arg2;
  cam_arg3 = arg3;

  signalNewCmdReady();

  int wait_cmd_ret = waitCmdExecuted();
  if (wait_cmd_ret != 0){
    camera_ret = (void *)wait_cmd_ret;
    ALOGE("executeCommand %d timeout, ret = %d", camera_cmd, camera_ret);
  }

  unlockExecuteCommand();
  return camera_ret;
}

void setCameraFPS(CameraParameters *ptr)
{
  char * fpsRanges = (char *)ptr->get("preview-fps-range-values");
  Vector<FPSRanges> fpsRangeList;
  ALOGI("setCameraFPS, supported = %s", fpsRanges);

  if(fpsRanges == NULL){
    ALOGE("setCameraFPS, no supported fps");
    return;
  }

  // got fps range list
  int i = 0;
  while(fpsRanges[i]!='\0')
  {
    if(fpsRanges[i] == '(')
    {
      i++;
    }
    char* start = &fpsRanges[i];
    while(fpsRanges[i] != ',')
    {
      i++;
    }
    char* end = &fpsRanges[i-1];
    int minFps = (int)strtol(start, &end, 10);
    if(fpsRanges[i] == ',')
    {
      i++;
    }
    start = &fpsRanges[i];
    while(fpsRanges[i] != ')')
    {
      i++;
    }
    end = &fpsRanges[i-1];
    int maxFps = (int)strtol(start, &end, 10);
    fpsRangeList.push(FPSRanges(minFps,maxFps));
    if(fpsRanges[i] == ')')
    {
      i++;
    }
    if(fpsRanges[i] == ',')
    {
      i++;
    }
  }

  // get fps range for the target fps
  for(i = fpsRangeList.size()-1;i >= 0;i--)
  {
    ALOGI("setCameraFPS, fps range %d -> %d",fpsRangeList[i].lo, fpsRangeList[i].hi);
    if(fpsRangeList[i].hi <= fps*1000)
    {
      char str[20];
      snprintf(str, sizeof(str), "%d,%d",fpsRangeList[i].lo,fpsRangeList[i].hi);
      ptr->set("preview-fps-range", str);
      break;
    }
  }
}

void setPreviewOrientationInternal(int rotation) {
  ALOGI("setPreviewOrientationInternal, rotation = %d", rotation);
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  ptr->iCamera->sendCommand(CAMERA_CMD_SET_DISPLAY_ORIENTATION, rotation, 0);
  camera_ret = (void *)0;
}

void startPreviewInternal(ImsCameraClient *ptr) {
  ALOGI("startPreviewInternal");
  if (ptr->iCamera.get() == NULL) {
    camera_ret = (void *)-1;
    return;
  }
  String8 params = ptr->iCamera->getParameters();
  CameraParameters *cParams = new CameraParameters();

  cParams->unflatten((const String8 &)params);
  if (ptr->iCamera.get() && gproducer.get()) {
    ALOGI("startPreviewInternal, setPreviewTarget : %d",
          ptr->iCamera->setPreviewTarget(gproducer));
  }

  //---using to callback camera info to vce
  CameraInfo info;
  cameraService->getCameraInfo(cameraId, &info);
  CustomCallbackParam callbackParam;

  callbackParam.type = CAMERA_ROTATION_CROP_PARAM;
  callbackParam.cropRotationParam.cameraFacing = info.facing;
  callbackParam.cropRotationParam.cameraMount = info.orientation;

  if (global_imscamera_cb.custom_cb != NULL) {
    global_imscamera_cb.custom_cb(&callbackParam);
  }

  cParams->set("sensor-rot", sensorRotate);
  cParams->set("sensor-orient", 1);
  cParams->set("ycbcr", 1);

  // set preview and record size;
  ALOGI("startPreviewInternal, preview/record size %d x %d", width, height);
  cParams->setPreviewSize(width, height);
  cParams->setVideoSize(width, height);

  cParams->set("recording-hint", "true");

  setCameraFPS(cParams);

  params = cParams->flatten();

  ptr->iCamera->setParameters(params);

  delete cParams;

  ALOGI(
      "startPreviewInternal, previewRotationOffset %d previewRotationAngle %d",
      previewRotationOffset, previewRotationAngle);
  setPreviewOrientationInternal((previewRotationOffset + previewRotationAngle) %
                                360);

  if (previewSurfaceReady == 1) {
    ALOGI("startPreviewInternal, startPreview %d",
          ptr->iCamera->startPreview());
  }
  startPreviewCalled = 1;
  camera_ret = (void *)0;
}

void startRecordingInternal(ImsCameraClient *ptr) {
  ALOGI("startRecordingInternal");
  if (ptr->iCamera.get() == NULL) {
    camera_ret = (void *)-1;
    return;
  }
  if (previewSurfaceReady == 1) {
    ptr->iCamera->sendCommand(CAMERA_CMD_ENABLE_SHUTTER_SOUND, 0, 0);
    ptr->iCamera->storeMetaDataInBuffers(1);
    ALOGI("startRecordingInternal, startRecording : %d",
          ptr->iCamera->startRecording());
    ALOGI("startRecordingInternal, recordingEnabled : %d",
          ptr->iCamera->recordingEnabled());
  }
  startRecordingCalled = 1;
  camera_ret = (void *)0;
}

void cleanEncoderNotification() {
  CustomCallbackParam callbackParam;

  callbackParam.type = CAMERA_STOP_RECORDING_PARAM;
  memset(&callbackParam.cropRotationParam, 0,
         sizeof(callbackParam.cropRotationParam));

  if (global_imscamera_cb.custom_cb != NULL) {
    global_imscamera_cb.custom_cb(&callbackParam);
  } else {
    ALOGI("cleanEncoderNotification, callback is null");
  }
}

void *cameraProcessThread(void *param __unused) {
  ImsCameraClient *ptr = ImsCameraClient::getInstance();

  ALOGI("cameraProcessThread, start");
  while (1) {
    waitingNewCmd();

    switch (camera_cmd) {
      case CAMERA_OPEN_CMD: {
        if (cameraService == NULL) {
          camera_ret = (void *)(-2);
          signalCmdExecuted();
          return NULL;
        }

        cameraId = (unsigned int)cam_arg1;

        numberOfCameras = cameraService->getNumberOfCameras();
        ALOGI("cameraProcessThread, numberOfCameras = %d, cameraId = %d",
              numberOfCameras, cameraId);
        CameraInfo info;
        status_t rc = cameraService->getCameraInfo(cameraId, &info);
        if (rc != 0) {
          ALOGE("cameraProcessThread, getCameraInfo failed %d", rc);
          camera_ret = (void *)(-2);
          signalCmdExecuted();
          return NULL;
        }
        usingFaceCamera = info.facing;
        sensorRotate = info.orientation;
        ALOGI("cameraProcessThread, usingFaceCamera = %d, sensorRotate = %d",
              usingFaceCamera, sensorRotate);

        ptr->state = INITIALIZING;

        client = new BufferClient();

        // get camera service with connectLegacy
        rc = cameraService->connectLegacy(client, cameraId,
                                          /*CAMERA_DEVICE_API_VERSION_3_2*/ -1,
                                          (const String16 &)*packageName,
                                          ICameraService::USE_CALLING_UID,
                                          ptr->iCamera);
        if (NO_ERROR != rc) {
          ALOGI("cameraProcessThread, connectLegacy failed, try connect");
          rc = cameraService->connect(client, cameraId,
                                      (const String16 &)*packageName, -1,
                                      ptr->iCamera);
          if (NO_ERROR != rc) {
            ALOGI("camera service connect failed, rc = %d", rc);
            signalCmdExecuted();
            return NULL;
          }
        }
        ALOGI("cameraProcessThread, camera service connect done");

        if (ptr->iCamera.get() == NULL) {
          ALOGI("cameraProcessThread, camera service connect failed %p",
                ptr->iCamera.get());
          camera_ret = (void *)-1;
          signalCmdExecuted();
          return NULL;
        }
        ptr->state = INITIALIZED;
        ALOGI("cameraProcessThread, camera service connect success");

        previewSurfaceReady = 0;
        startPreviewCalled = 0;
        startRecordingCalled = 0;
        previewTransform = 0;

        camera_ret = (void *)0;
      } break;

      case CAMERA_CLOSE_CMD: {
        if (ptr->iCamera.get() == NULL) {
          ALOGI("cameraProcessThread, CAMERA_CLOSE_CMD error");
          camera_ret = (void *)-1;
          signalCmdExecuted();
          return 0;
        }

        if (startRecordingCalled == 1) {
          if (ptr->iCamera.get()) {
            ptr->iCamera->stopRecording();
          }
          cleanEncoderNotification();
        }
        startRecordingCalled = 0;

        if (startPreviewCalled == 1) {
          if (ptr->iCamera.get()) {
            ptr->iCamera->stopPreview();
          }
        }
        startPreviewCalled = 0;

        ptr->state = UNINITIALIZED;
        ptr->iCamera->setPreviewCallbackFlag(CAMERA_FRAME_CALLBACK_FLAG_NOOP);
        // ptr->iCamera->setPreviewTarget(NULL);
        ptr->iCamera->disconnect();
        ptr->iCamera = 0;
        client = 0;
        previewRotationOffset = 0;
        previewRotationAngle = 0;
        signalCmdExecuted();
        return 0;
      }

      case SET_PREVIEW_SURFACE_CMD: {
        ALOGI("cameraProcessThread, mPreviewNativeWindow, %x",
              (int)(uintptr_t)mPreviewNativeWindow.get());
        if (ptr->iCamera.get()) {
          ALOGI("cameraProcessThread, setPreviewTarget ret = %d",
                ptr->iCamera->setPreviewTarget(gproducer));

          previewSurfaceReady = 1;

          if (startPreviewCalled) {
            startPreviewInternal(ptr);
          }

          if (startRecordingCalled) {
            startRecordingInternal(ptr);
          }
          camera_ret = (void *)0;
        } else {
          camera_ret = (void *)-1;
        }
      } break;

      case SET_PREVIEW_DISPLAY_ORIENTATION_CMD: {
        previewRotationAngle = (unsigned int)cam_arg1;
        ALOGI("cameraProcessThread, RotationOffset = %d, RotationAngle = %d",
              previewRotationOffset, previewRotationAngle);
        setPreviewOrientationInternal(
            (previewRotationOffset + previewRotationAngle) % 360);
      } break;

      case START_PREVIEW_CMD: {
        startPreviewInternal(ptr);
      } break;

      case STOP_PREVIEW_CMD: {
        if (startRecordingCalled == 1) {
          if (ptr->iCamera.get()) {
            ptr->iCamera->stopRecording();
          }
          cleanEncoderNotification();
        }
        startRecordingCalled = 0;

        if (startPreviewCalled == 1) {
          if (ptr->iCamera.get()) {
            ptr->iCamera->stopPreview();
          }
        }
        startPreviewCalled = 0;
        camera_ret = (void *)0;
      } break;

      case START_RECORDING_CMD: {
        startRecordingInternal(ptr);
      } break;

      case STOP_RECORDING_CMD: {
        int count = 0;
        if (startRecordingCalled == 1) {
          if (ptr->iCamera.get()) {
            ptr->iCamera->stopRecording();
          }
          cleanEncoderNotification();
        }
        ALOGI("cameraProcessThread, processingFrameCount = %d",
              processingFrameCount);
        startRecordingCalled = 0;
        camera_ret = (void *)0;
      } break;

      case SET_CAMERA_PARAM_CMD: {
        CameraParamContainer *setParams = (CameraParamContainer *)cam_arg1;
        switch (setParams->type) {
          case SET_FPS:
            fps = setParams->params.fps;
            ALOGI("cameraProcessThread, fps %d", fps);
            break;
          case SET_RESOLUTION:
            height = setParams->params.cameraResolution.height;
            width = setParams->params.cameraResolution.width;
            ALOGI("cameraProcessThread, width = %d, height = %d", width,
                  height);
            break;
          default:
            break;
        }
        camera_ret = (void *)0;
      } break;

      case GET_CAMERA_PARAM_CMD:
        break;

      case RELEASE_CAMERA_FRAME:
        const sp<IMemory> *dataPtr;
        dataPtr = (const sp<IMemory> *)cam_arg1;
        DLOG("cameraProcessThread, release recording frame %p", dataPtr->get());
        if (ptr->iCamera.get() != NULL) {
          ptr->iCamera->releaseRecordingFrame(*dataPtr);
        }
        break;
      default:
        ALOGI("cameraProcessThread, unknown cmd");
    }
    signalCmdExecuted();
  }
  ALOGI("cameraProcessThread, end");
}

short imsCameraOpen(unsigned int cameraid) {
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  // Camera acquire part
  ALOGI("ImsCameraOpen");
  int err;
  int ret;

  initSem();

  processingFrameCount = 0;
  stoppingRecord = 0;

  packageName = new String16("media");

  // start camera cmd handle thread
  if ((err = pthread_create(&cameraLoopThread, NULL, cameraProcessThread,
                            (void *)NULL)) < 0) {
    ALOGI("ImsCameraOpen failed to create thread");
    return err;
  }

  unlockExecuteCommand();

  ret = (int)executeCommand(CAMERA_OPEN_CMD, (void *)cameraid, 0, 0);

  // Cleanup the resources we have allocated
  if (ret != 0) {
    ALOGI("ImsCameraOpen failed, ret = %d", ret);
    ptr->state = UNINITIALIZED;

    if (ret != -1){
        (void)pthread_join(cameraLoopThread, NULL);
    }

    destroySem();

    delete packageName;
    packageName = NULL;

    if (cameraClient) {
      delete cameraClient;
      cameraClient = NULL;
    }
  }
  ALOGI("ImsCameraOpen, ret = %d", ret);
  return ret;
}

short imsCameraRelease() {
  ALOGI("imsCameraRelease");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraRelease failed, uninitialized");
    return -1;
  }

  ptr->state = DEINITIALIZING;

  int ret = (int)executeCommand(CAMERA_CLOSE_CMD, 0, 0, 0);

  if (ret!= 0){
    ALOGE("imsCameraRelease, thread exit timeout");
  } else {
    (void)pthread_join(cameraLoopThread, NULL);
  }

  destroySem();

  delete packageName;
  packageName = NULL;

  if (cameraClient) {
    delete cameraClient;
    cameraClient = NULL;
  }
  ALOGI("imsCameraRelease, ret = %d", ret);
  return ret;
}

short imsCameraSetPreviewSurface(sp<IGraphicBufferProducer> *surface) {
  ALOGI("imsCameraSetPreviewSurface");
  int ret = 0;
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraSetPreviewSurface failed, uninitialized");
    return -1;
  }

  if (surface == NULL) {
    ALOGI("imsCameraSetPreviewSurface failed, surface is null");
    return 0;
  }

  sp<IGraphicBufferProducer> producer = *surface;
  gproducer = producer;
  mPreviewNativeWindow = new Surface(producer, true);

  if (mPreviewNativeWindow == NULL) {
    ALOGI("imsCameraSetPreviewSurface failed, mPreviewNativeWindow is null");
    return -1;
  }

  if (mPreviewNativeWindow.get() == NULL) {
    ALOGI(
        "imsCameraSetPreviewSurface failed, mPreviewNativeWindow.get() is "
        "null");
    return -1;
  }
  ret = (int)executeCommand(SET_PREVIEW_SURFACE_CMD, 0, 0, 0);
  ALOGI("imsCameraSetPreviewSurface, ret = %d", ret);
  return ret;
}

short imsCamerasetPreviewDisplayOrientation(unsigned int rotation) {
  ALOGI("imsCamerasetPreviewDisplayOrientation %d", rotation);
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCamerasetPreviewDisplayOrientation failed, uninitialized");
    return -1;
  }
  int ret = (int)executeCommand(SET_PREVIEW_DISPLAY_ORIENTATION_CMD,
                                (void *)rotation, 0, 0);
  ALOGI("imsCamerasetPreviewDisplayOrientation , ret = %d", ret);
  return ret;
}

short imsCameraStartPreview() {
  ALOGI("imsCameraStartPreview");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraStartPreview failed, uninitialized");
    return -1;
  }
  int ret = (int)executeCommand(START_PREVIEW_CMD, 0, 0, 0);
  ALOGI("imsCameraStartPreview, ret = %d", ret);
  return 0;
}
short imsCameraStopPreview() {
  ALOGI("imsCameraStopPreview");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraStopPreview failed, uninitialized");
    return -1;
  }
  int ret = (int)executeCommand(STOP_PREVIEW_CMD, 0, 0, 0);
  ALOGI("imsCameraStopPreview, ret = %d", ret);
  return 0;
}
short imsCameraStartRecording() {
  ALOGI("imsCameraStartRecording");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraStartRecording failed, uninitialized");
    return -1;
  }
  stoppingRecord = 0;
  int ret = (int)executeCommand(START_RECORDING_CMD, 0, 0, 0);
  ALOGI("imsCameraStartRecording, ret = %d", ret);
  return 0;
}

short imsCameraStopRecording() {
  ALOGI("imsCameraStopRecording");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraStopRecording failed, uninitialized");
    return -1;
  }
  waitFrameReleased();
  stoppingRecord = 1;
  int ret = (int)executeCommand(STOP_RECORDING_CMD, 0, 0, 0);
  signalReleaseFrame();
  ALOGI("imsCameraStopRecording, ret = %d", ret);
  return 0;
}

short imsCameraSetParameter(CameraParamContainer setParams) {
  ALOGI("imsCameraSetParameter");
  ImsCameraClient *ptr = ImsCameraClient::getInstance();
  if (ptr->state != INITIALIZED) {
    ALOGI("imsCameraSetParameter failed, uninitialized");
    return -1;
  }
  int ret = (int)executeCommand(SET_CAMERA_PARAM_CMD, &setParams, 0, 0);
  ALOGI("imsCameraSetParameter, ret = %d", ret);

  return ret;
}
CameraParams getCameraParameter(eParamType paramKey __unused) {
  CameraParams p;
  return p;
}

/*
 * used to register the call back method to camera
 * data_cb_timestamp implements the real processing for camera frames
 */
short registerCallbacks(ims_notify_callback notify_cb,
                        ims_data_callback data_cb,
                        ims_data_callback_timestamp data_cb_timestamp,
                        ims_custom_params_callback custom_cb,
                        void *user __unused) {
  ALOGI("registerCallbacks ");
  /* Having the callbacks in global context */
  global_imscamera_cb.notify_cb = notify_cb;
  global_imscamera_cb.data_cb = data_cb;
  global_imscamera_cb.data_cb_timestamp = data_cb_timestamp;
  global_imscamera_cb.custom_cb = custom_cb;
  return 0;
}

void registerCameraCallbacks(ims_notify_callback notifyCb __unused) { return; }

ImsCameraClient::ImsCameraClient() {
  state = UNINITIALIZED;
  ALOGI("new ImsCameraClient %p", this);
  sp<IServiceManager> service = defaultServiceManager();
  sp<IBinder> service_binder = service->getService(String16("media.camera"));
  cameraService = interface_cast<ICameraService>(service_binder);
  iCamera = NULL;
}

ImsCameraClient::~ImsCameraClient() {}

ImsCameraClient *ImsCameraClient::getInstance() {
  DLOG("ImsCameraClient, getInstance");
  if (cameraClient == NULL) {
    ALOGI("ImsCameraClient, getInstance, new");
    cameraClient = new ImsCameraClient();
  }
  return cameraClient;
}
void BufferClient::dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                                         const sp<IMemory> &data) {
  DLOG("dataCallbackTimestamp %p", data.get());
  ImsCameraClient *ptr = ImsCameraClient::getInstance();

  if (global_imscamera_cb.data_cb_timestamp && (stoppingRecord != 1)) {
    waitFrameReleased();
    if (stoppingRecord != 1) {
      processingFrameCount++;
      global_imscamera_cb.data_cb_timestamp(timestamp, msgType, data, NULL);
      executeCommand(RELEASE_CAMERA_FRAME, (void *)(&data), 0, 0);
      DLOG("dataCallbackTimestamp releasing data frame %p", data.get());
      processingFrameCount--;
    }
    signalReleaseFrame();
  }
  DLOG("dataCallbackTimestamp end");
}

int ImsCameraClient::init(void *params __unused) { return 0; }

int ImsCameraClient::release() { return 0; }
