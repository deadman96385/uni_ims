#ifndef VCECAMERA_VIDEOCALLENGINECAMERA_H_
#define VCECAMERA_VIDEOCALLENGINECAMERA_H_
#include "VideoCallEngineCameraInterface.h"
#include <ICamera.h>
#include <ICameraClient.h>
#include <ICameraServiceListener.h>

#undef LOG_TAG
#define LOG_TAG "VCECamera"

using namespace android;

// camera operation cmd
#define CAMERA_OPEN_CMD 1
#define CAMERA_CLOSE_CMD 2
#define SET_PREVIEW_SURFACE_CMD 3
#define SET_PREVIEW_DISPLAY_ORIENTATION_CMD 4
#define START_PREVIEW_CMD 5
#define STOP_PREVIEW_CMD 6
#define START_RECORDING_CMD 7
#define STOP_RECORDING_CMD 8
#define SET_CAMERA_PARAM_CMD 9
#define GET_CAMERA_PARAM_CMD 10
#define RELEASE_CAMERA_FRAME 11

/* Global Context of maintaining all the camera callbacks */
struct global_imscamera {
  ims_notify_callback notify_cb;
  ims_data_callback data_cb;
  ims_data_callback_timestamp data_cb_timestamp;  // callback frames with ts
  ims_custom_params_callback custom_cb;
  void* user;
};

/* Assigning all the fields to null */
struct global_imscamera global_imscamera_cb = {NULL, NULL, NULL, NULL, NULL};

sem_t cmdExecutedSem;
sem_t lockExecuteCommandSem;
sem_t waitNewCmdSem;
sem_t frameReleaseSem;

static int sem_timedwait(sem_t* sem_var) {
  struct timespec ts;
  int result;

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2; //timeout is 2 seconds
  result = sem_timedwait(sem_var, &ts);
  if (result) {
    ALOGI("Failure in [%s:%d] result = %d", __func__, __LINE__, result);
    return -1;
  }
  return 0;
}

//-----used to lock executeCommand()
inline void lockExecuteCommand() { sem_wait(&lockExecuteCommandSem); }
inline void unlockExecuteCommand() { sem_post(&lockExecuteCommandSem); }

//-----used to control loop to wait/process new cmd
inline void signalNewCmdReady() { sem_post(&waitNewCmdSem); }
inline void waitingNewCmd() { sem_wait(&waitNewCmdSem); }

//-----used to signal/wait for the cmd being executed
inline int waitCmdExecuted() { return sem_timedwait(&cmdExecutedSem); }
inline void signalCmdExecuted() { sem_post(&cmdExecutedSem); }

//-----used for freame release
inline void waitFrameReleased() { sem_timedwait(&frameReleaseSem); }
inline void signalReleaseFrame() { sem_post(&frameReleaseSem); }
//--------------------------

void initSem() {
  sem_init(&cmdExecutedSem, 0, 0);
  sem_init(&lockExecuteCommandSem, 0, 0);
  sem_init(&waitNewCmdSem, 0, 0);
  sem_init(&frameReleaseSem, 0, 1);
}

void destroySem() {
  sem_destroy(&cmdExecutedSem);
  sem_destroy(&lockExecuteCommandSem);
  sem_destroy(&waitNewCmdSem);
  sem_destroy(&frameReleaseSem);
}

struct FPSRanges {
int lo;
int hi;
FPSRanges() {
      lo = 0;
      hi = 0;
   }
FPSRanges(int low, int high) {
      lo = low;
      hi = high;
   }
};

typedef enum {
  UNINITIALIZED = 0,
  INITIALIZING = 1,
  INITIALIZED = 2,
  DEINITIALIZING = 3
} eCameraClientState;

class BufferClient : public BnCameraClient {
 public:
  virtual ~BufferClient() { ALOGI("~BufferClient"); };

  void notifyCallback(int32_t msgType __unused, int32_t ext1 __unused,
                      int32_t ext2 __unused) {
    ALOGI("notifyCallback");
  };
  void dataCallback(int32_t msgType __unused, const sp<IMemory>& data __unused,
                    camera_frame_metadata_t* metadata __unused) {
    ALOGI("dataCallback");
  };
  void dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                             const sp<IMemory>& data);
};

class ImsCameraClient {
 public:
  static ImsCameraClient* getInstance();
  int init(void* params);
  static int release();
  virtual ~ImsCameraClient();
  eCameraClientState state;
  CameraParameters params;
  sp<ICamera> iCamera;

 private:
  ImsCameraClient();
};

#endif /* VCECAMERA_VIDEOCALLENGINECAMERA_H_ */
