// 
// Spreadtrum Auto Tester
//
// anli   2012-11-29
//
#include <dlfcn.h>
#include <fcntl.h>

#include <hardware/fm.h>
//#include <media/AudioSystem.h>

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "type.h"
#include "fm.h"
#include "perm.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_fm {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace android;
using namespace at_perm;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static hw_module_t * s_hwModule = NULL;
static fm_device_t * s_hwDev    = NULL;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int fmOpen( void )
{
	if( NULL != s_hwDev ) {
		WRNMSG("already opened.\n");
		return 0;
	}

    int err = hw_get_module(FM_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if( err || NULL == s_hwModule ) {
		ERRMSG("hw_get_module: err = %d\n", err);
		return ((err > 0) ? -err : err);
	}

	err = s_hwModule->methods->open(s_hwModule, FM_HARDWARE_MODULE_ID,
					(hw_device_t**)&s_hwDev);
    if( err || NULL == s_hwDev ) {
		ERRMSG("open err = %d!\n", err);
		return ((err > 0) ? -err : err);
	}

    permInstallService(NULL);

	AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_SPEAKER,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");
    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");
    
    return 0;
}

//
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)

//------------------------------------------------------------------------------
int fmPlay( uint freq )
{
    if( NULL == s_hwDev ) {
        ERRMSG("not opened!\n");
        return -1;
    }

	status_t   status;
#if 0
	AudioTrack atrk;

	atrk.set(AUDIO_STREAM_FM, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO);
	atrk.start();

	AudioTrack::Buffer buffer;
	buffer.frameCount = 64;
	status = atrk.obtainBuffer(&buffer, 1);
    if (status == NO_ERROR) {
		memset(buffer.i8, 0, buffer.size);
		atrk.releaseBuffer(&buffer);
	} else if (status != TIMED_OUT && status != WOULD_BLOCK) {
		ERRMSG("cannot write to AudioTrack: status = %d\n", status);
	}
	atrk.stop();
#endif
	int ret = s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 1);
	//DBGMSG("setControl:  TAVARUA = %d\n", ret);
	usleep(20 * 1000);

    uint volume = 12; // max  [0 - 15]
    ret = s_hwDev->setControl(s_hwDev, V4L2_CID_AUDIO_VOLUME, volume);
	//DBGMSG("setControl:  VOLUME = %d\n", ret);

	ret = s_hwDev->setFreq(s_hwDev, freq);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
        return ret;
    }
    //usleep(20 * 1000);

	status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");

	if( NO_ERROR != status ) {
        ERRMSG("out to fm headset error!\n");
        return -3;
    }

    return 0;
}

//------------------------------------------------------------------------------
int fmStop( void )
{
	if( NULL != s_hwDev ) {

		AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "");

		s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 0);
	}

    return 0;
}

int fmClose( void )
{
	if( NULL != s_hwDev && NULL != s_hwDev->common.close ) {
		s_hwDev->common.close( &(s_hwDev->common) );
	}
	s_hwDev = NULL;

    if( NULL != s_hwModule ) {
        dlclose(s_hwModule->dso);
		s_hwModule = NULL;
    }

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
