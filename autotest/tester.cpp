// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#include <stdlib.h>

#include "type.h"
#include "audio.h"
#include "battery.h"
#include "bt.h"
#include "camera.h"
#include "cmmb.h"
#include "diag.h"
#include "driver.h"
#include "fm.h"
#include "gps.h"
#include "input.h"
#include "lcd.h"
#include "light.h"
#include "sensor.h"
#include "sim.h"
#include "tcard.h"
#include "tester.h"
#include "util.h"
#include "ver.h"
#include "vibrator.h"
#include "wifi.h"

#include <signal.h>
#include <cutils/properties.h>
#include <hardware_legacy/power.h>

#include <dlfcn.h> 
#include<stdlib.h>
#include <signal.h>
#include <stdio.h>
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------

#include <cutils/properties.h>


/** Base path of the hal modules */
#define HAL_LIBRARY_PATH1 "/system/lib/hw"
#define HAL_LIBRARY_PATH2 "/vendor/lib/hw"

static const char *variant_keys[] = {
    "ro.hardware",  /* This goes first so that it can pick up a different file on the emulator. */
    "ro.product.board",
    "ro.board.platform",
    "ro.arch"
};

static const int HAL_VARIANT_KEYS_COUNT =
    (sizeof(variant_keys)/sizeof(variant_keys[0]));


//#include<../../device/sprd/common/libs/libcamera/sc8830/inc/SprdCameraHardware_autest_Interface.h> 

//for camera auto-test by wangtao 2014-02-14  start
#define MIPI_YUV_YUV 0x0A
#define MIPI_YUV_JPG 0x09
#define MIPI_RAW_RAW 0x0C
#define MIPI_RAW_JPG 0x0D

enum auto_test_calibration_cmd_id {
	AUTO_TEST_CALIBRATION_AWB= 0,
	AUTO_TEST_CALIBRATION_LSC,
	AUTO_TEST_CALIBRATION_FLASHLIGHT,
	AUTO_TEST_CALIBRATION_CAP_JPG,
	AUTO_TEST_CALIBRATION_CAP_YUV,
	AUTO_TEST_CALIBRATION_MAX
};

    #define	MAX_LEN 200

	char path[MAX_LEN]=" ";//"/system/lib/hw/camera.scx15.so";

	char func_set_testmode[MAX_LEN]="_Z21autotest_set_testmodeiiiiii";
	char func_cam_from_buf[MAX_LEN]="_Z21autotest_cam_from_bufPPviPi";
	char func_close_testmode[MAX_LEN]="_Z23autotest_close_testmodev";

	FILE *fp = NULL;
	int filesize ;

	
	
	char prefix[MAX_LEN]="/data/image_save.%s";
    char test_name[MAX_LEN]="/data/image_save.%s";

	
	int8_t choice;

	void *camera_handle;
 	int ret;

	int camera_interface=MIPI_YUV_JPG;
	int maincmd=0;
	int image_width=640 ;
	int image_height=480;
	
	typedef int32_t (*at_set_testmode)(int camerinterface,int maincmd ,int subcmd,int cameraid,int width,int height);
	typedef int (*at_cam_from_buf)(void**pp_image_addr,int size,int *out_size);
	typedef int (*at_close_testmode)(void);

	at_set_testmode atcamera_set_testmode=NULL;
	at_cam_from_buf atcamera_get_image_from_buf=NULL;
	at_close_testmode atcamera_close_testmode=NULL;
	


	int find_cam_lib_path(char*path,int len)
	{
		char prop[PATH_MAX];
		char name[PATH_MAX]="camera";
		int i = 0;

		if(path==NULL)
			return 1;

		/* Loop through the configuration variants looking for a module */
		for (i=0 ; i<HAL_VARIANT_KEYS_COUNT+1 ; i++) {
			if (i < HAL_VARIANT_KEYS_COUNT) {
				if (property_get(variant_keys[i], prop, NULL) == 0) {
					continue;
				}
				snprintf(path, len, "%s/%s.%s.so",
						 HAL_LIBRARY_PATH2, name, prop);
				if (access(path, R_OK) == 0) break;

				snprintf(path, len, "%s/%s.%s.so",
						 HAL_LIBRARY_PATH1, name, prop);
				if (access(path, R_OK) == 0) break;
			} else {
				snprintf(path, len, "%s/%s.default.so",
						 HAL_LIBRARY_PATH2, name);
				if (access(path, R_OK) == 0) break;

				snprintf(path, len, "%s/%s.default.so",
						 HAL_LIBRARY_PATH1, name);
				if (access(path, R_OK) == 0) break;
			}
		}

	return 0;

	}

    int loadlibrary_camera_so()
	{
	find_cam_lib_path(path,sizeof(path));
	DBGMSG("path =%s\n",path);

    camera_handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	if (!camera_handle){
	char const *err_str = dlerror();
	
		return -1;
	}
	
	// open auto test mode 
	atcamera_set_testmode = (at_set_testmode)dlsym(camera_handle, func_set_testmode);
	if (atcamera_set_testmode==NULL) {
	dlclose(camera_handle);
	return -1;
	}

	atcamera_get_image_from_buf = (at_cam_from_buf)dlsym(camera_handle, func_cam_from_buf);
	if(atcamera_get_image_from_buf==NULL)
	{
		return -1;
	}

	// close auto test mode 
		
	atcamera_close_testmode = (at_close_testmode)dlsym(camera_handle, func_close_testmode);
	if(atcamera_close_testmode==NULL)
	{
		return -1;
	}
	return 0;
    }

//for camera auto-test by wangtao 2014-02-14 end

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_tester {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF

//------------------------------------------------------------------------------

#define AT_WAKE_LOCK_NAME  "wl_autotest"

//------------------------------------------------------------------------------
struct diag_header_t {
	uint   sn;
	ushort len;
	uchar  cmd;
	uchar  sub_cmd;
};

#define DIAG_MAX_DATA_SIZE  400 // bytes

//------------------------------------------------------------------------------
/*
using namespace sci_aud;
using namespace sci_bat;
using namespace sci_bt;
using namespace sci_cam;
using namespace sci_diag;
using namespace sci_drv;
using namespace sci_fm;
using namespace sci_input;
using namespace sci_lcd;
using namespace sci_light;
using namespace sci_sim;
using namespace sci_tcard;
using namespace sci_vib;
using namespace sci_wifi;
*/
//------------------------------------------------------------------------------
typedef int  (*PFUN_TEST)(const uchar * data, int data_len, uchar *rsp, int rsp_size);

static int    testRegisterFun( uchar cmd, PFUN_TEST fun );

static int    testReserved(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testKPD(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testLCD(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testCamParal(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testGpio(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testTCard(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testSIM(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testAudioIN(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testAudioOUT(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testLKBV(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testFM(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testBT(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testWIFI(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testIIC(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testCharger(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testSensor(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testGps(const uchar * data, int data_len, uchar *rsp, int rsp_size);
static int    testmipiCamParal(const uchar * data, int data_len, uchar * rsp, int rsp_size);

//------------------------------------------------------------------------------
static PFUN_TEST       sTestFuns[DIAG_CMD_MAX];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void signalProcess( int signo )
{
	WRNMSG("On signal: %d\n", signo);

	test_Deinit();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int test_Init( void )
{
	acquire_wake_lock(PARTIAL_WAKE_LOCK, AT_WAKE_LOCK_NAME);

    FUN_ENTER;

	char ver[256];
	snprintf(ver, 256, "Autotest Version: %s %s", verGetDescripter(), verGetVer());
	DBGMSG("%s\n", ver);

	// disable java framework
	utilEnableService("media");
	utilDisableService("bootanim");
	utilDisableService("netd");
	utilDisableService("surfaceflinger");
	usleep(100 * 1000);
	if( utilDisableService("zygote") >= 0 ) {
		int sys_svc_pid = utilGetPidByName("system_server");
		DBGMSG("system_service pid = %d\n", sys_svc_pid);
		if( sys_svc_pid > 0 ) {
			kill(sys_svc_pid, SIGTERM);
		}
	}
	usleep(200 * 1000);
	utilDisableService("netd");
	utilDisableService("surfaceflinger");
	utilDisableService("zygote");
	usleep(200 * 1000);

	// for debug
	//usleep(1000 * 1000);
	//system("ps > /data/atps.log");
	//--------------------------------------------------------------------------
	//
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_RESETHAND | SA_NODEFER;
	sa.sa_handler = signalProcess;
	sigaction(SIGHUP,  &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);

	//--------------------------------------------------------------------------
	//
    memset(sTestFuns, 0, sizeof(sTestFuns));

	testRegisterFun(DIAG_CMD_RESERVED,  testReserved);
    testRegisterFun(DIAG_CMD_KPD,       testKPD);
    testRegisterFun(DIAG_CMD_LCD,       testLCD);
    testRegisterFun(DIAG_CMD_CAM_PARAL, testmipiCamParal);//testCamParal
    testRegisterFun(DIAG_CMD_CAM_MIPI, testmipiCamParal);
    testRegisterFun(DIAG_CMD_GPIO,      testGpio);
    testRegisterFun(DIAG_CMD_TCARD,     testTCard);
    testRegisterFun(DIAG_CMD_SIM,       testSIM);
    testRegisterFun(DIAG_CMD_AUDIO_IN,  testAudioIN);
    testRegisterFun(DIAG_CMD_AUDIO_OUT, testAudioOUT);
    testRegisterFun(DIAG_CMD_LKBV,      testLKBV);
    testRegisterFun(DIAG_CMD_FM,        testFM);
    testRegisterFun(DIAG_CMD_BT,        testBT);
    testRegisterFun(DIAG_CMD_WIFI,      testWIFI);
    testRegisterFun(DIAG_CMD_IIC,       testIIC);
    testRegisterFun(DIAG_CMD_CHARGE,    testCharger);
	testRegisterFun(DIAG_CMD_SENSOR,    testSensor);
	testRegisterFun(DIAG_CMD_GPS,       testGps);

	drvOpen();

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------

int test_DoTest( const uchar * req, int req_len, uchar * rsp, int rsp_size )
{
    FUN_ENTER;
    struct diag_header_t * dh = (struct diag_header_t *)req;
	DBGMSG("test_DoTest dh->len=%d,req_len=%d\n",dh->len,req_len);
	AT_ASSERT( dh->len <= req_len );
	AT_ASSERT( 0x38 == dh->cmd );

	if( dh->sub_cmd >= DIAG_CMD_MAX ) {
		ERRMSG("invalid sub cmd: 0x%x\n", dh->sub_cmd);
		return -1;
	}

	int ret = -1;
	if( sTestFuns[dh->sub_cmd] != NULL ) {
		int dpos = sizeof(struct diag_header_t);
		const uchar * data = (const uchar *)(req + dpos);
		ret = sTestFuns[dh->sub_cmd](data, req_len - dpos, rsp, rsp_size);
	}

    FUN_EXIT;
    return ret;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int testRegisterFun( uchar cmd, PFUN_TEST fun )
{
    AT_ASSERT( cmd < DIAG_CMD_MAX );
    
    if( cmd >= DIAG_CMD_MAX ) {
        ERRMSG("Invalid: cmd = %d, max = %d\n", cmd, DIAG_CMD_MAX);
        return -1;
    }
    
    if( sTestFuns[cmd] != NULL ) {
        WRNMSG("cmd(%d) already register!\n", cmd);
    }
    
    sTestFuns[cmd] = fun;
    
    return 0;
}

int testReserved(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
	FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

	int ret = 0;
    switch( *data ) {
    case 0: // version
        snprintf((char *)rsp, rsp_size, "Autotest Version: %s %s", verGetDescripter(), verGetVer());
		ret = strlen((char *)rsp);
        break;
    case 1: //
		dbgMsg2FileEnable(1);
        break;
    case 2: //
		dbgMsg2FileEnable(0);
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testKPD(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

	int ret = 0;

    switch( *data ) {
    case 1: // init
        if( inputOpen() < 0 ) {
            ret  = -1;
        }
        break;
    case 2: // get pressed key
    {
		struct kpd_info_t kpdinfo;

		ret = 6;
        if( inputKPDGetKeyInfo(&kpdinfo) < 0 ) {
            rsp[0]  = 0xFF; // col
			rsp[1]  = 0xFF; // row
			rsp[2]  = 0x00; // key high byte
			rsp[3]  = 0x00; // key low byte
			rsp[4]  = 0x00; // gpio high byte
			rsp[5]  = 0x00; // gpio low byte
        } else {
            rsp[0] = ((uchar)(kpdinfo.col)) & 0xFF;
			rsp[1] = ((uchar)(kpdinfo.row)) & 0xFF;
			rsp[2] = ((uchar)(kpdinfo.key >> 8));   // key high byte
			rsp[3] = ((uchar)(kpdinfo.key & 0xFF)); // key low byte
			rsp[4] = ((uchar)(kpdinfo.gio >> 8));   // gpio high byte
			rsp[5] = ((uchar)(kpdinfo.gio & 0xFF)); // gpio low byte
        }
    }
        break;
    case 3: // end test
        inputClose();
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testLCD(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

	int ret = 0;

    if( 0x01 == *data ) {

        uint value = ( (data[1] << 16) | (data[2] << 8) | data[3] );

        if( drvOpen() < 0 || drvLcdSendData(value) < 0 ) {
            ret = -1;
        }
		drvClose();
    } else if( 0x02 == *data ) {
        // close
    } else if( 0x11 == *data ) {
        if( drvOpen() < 0 || drvLcd_mipi_on() < 0 ) {
            ret = -1;
        }
    } else if( 0x12 == *data ) {
        // close
        if ( drvLcd_mipi_off() < 0 )
		ret = -1;
        drvClose();
    } else {
        ret = -1;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testCamParal(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;

    #define CAM_IDX       1
    #define CAM_W         240 //(320 * 2)
    #define CAM_H         320 //(240 * 2)
    #define CAM_PIX_SIZE  2
    #define CAM_DAT_SIZE  (240 * CAM_PIX_SIZE)

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    switch( data[0] ) {
    case 1:
        if( camOpen(CAM_IDX, CAM_W, CAM_H) < 0 ) {
            ret = -1;
        }
        break;
    case 2:
        ret = CAM_DAT_SIZE;
        if( camStart() < 0 || camGetData((uchar *)rsp, ret) < 0 ) {
            ret = -1;
        }
        break;
    case 3:
        camStop();
		camClose();
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

int testmipiCamParal(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
   int ret = 0;
    int rec_image_size=0;
    #define CAM_IDX       1
    #define CAM_W_VGA         640 
    #define CAM_H_VGA          480 
    #define CAM_PIX_SIZE  2
    #define CAM_DAT_SIZE_VGA  (CAM_W_VGA * CAM_H_VGA * CAM_PIX_SIZE)
    #define SBUFFER_SIZE  (600*1024)
    uchar * sBuffer = NULL;
    sBuffer = new uchar[600*1024];
    static int sensor_id = 0;//0:back  ,1:front

    FUN_ENTER;
    INFMSG("testmipiCamParal  data[0] = %d, data[1] = %d,  rsp_size =%d, sensor_id=%d \n", *data, data[1], rsp_size,sensor_id);

    switch( data[0] ) {
    case 1:
	  loadlibrary_camera_so();
        if( atcamera_set_testmode(MIPI_RAW_RAW,AUTO_TEST_CALIBRATION_CAP_YUV,0,sensor_id,CAM_W_VGA,CAM_H_VGA) < 0 ) {
            ret = -1;
        }
        break;
    case 2:
        if(atcamera_get_image_from_buf((void**)&sBuffer, SBUFFER_SIZE, &rec_image_size)< 0 ) {
            ret = -1;
        }else{
	     if(rec_image_size > rsp_size -1){
		 	memcpy(rsp, sBuffer, 768/*(rsp_size - 1)*/);
		       ret = 768;//rsp_size-1;
	     	}else{
		 	memcpy(rsp, sBuffer, 768/*rec_image_size*/);
		 	ret =  768;//rec_image_size;
	     	}
	 }
	  INFMSG("testmipiCamParal  rec_image_size = %d\n", rec_image_size);
        break;
    case 3:
   	 if(atcamera_close_testmode() < 0 ) {
            ret = -1;
        }
	    sensor_id = 0;
        break;
    case 4:
        sensor_id = data[1];
        break;
    default:
        break;
    }
	delete [] sBuffer;

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testGpio(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    if( 0 == data[0] ) { // input
        uchar val;
        if( drvOpen() >= 0 && drvGIODir(data[1], GPIO_DIR_IN) >= 0 &&
            drvGIOGet(data[1], &val) >= 0 ) {
            ret = 1;
            rsp[0] = val;
        } else {
            ret = -1; // fail
        }
		drvClose();
    } else if( 1 == data[0] ) { // output
        if( drvOpen() >= 0 && drvGIODir(data[1], GPIO_DIR_OUT) >= 0 &&
            drvGIOSet(data[1], data[2]) >= 0 ) {
            ret = 0;
        } else {
            ret = -1; // fail
        }
		drvClose();
    } else if( 2 == data[0] ) { // close
		// nothing
	} else {
		ret = -1;
		ERRMSG("invalid gpio data[0]\n");
	}

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testTCard(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = -1;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    if( tcardOpen() >= 0 && tcardIsPresent() >= 0 ) {
        ret = 0;
    }
	tcardClose();

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testSIM(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = -1;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    if( simOpen() >= 0 && simCheck(*data) >= 0 ) {
        ret = 0;
    }
	simClose();

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testAudioIN(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
	static int open = -1;

    int   ret = 0;
    uchar mic = data[0];
    uchar act = data[1];
    FUN_ENTER;
    ALOGE("wangzuo:data[0] = %d\n", *data);

    #define RECORD_SAMPLE_RATE  16000
    #define RECORD_MAX_SIZE     (1600)
    static uchar rcrdBuf[RECORD_MAX_SIZE];

    switch( act ) {
    case 0x01:
        audRcrderClose();
        open = audRcrderOpen(mic, RECORD_SAMPLE_RATE);
        break;
    case 0x02:
        if( open < 0 ) {
            ret = -1;
        }
        break;
    case 0x03:
    { //
		uchar idx = data[2];
		INFMSG("index = %d\n", idx);
		if( open < 0 ) {
			ret = -1;
			break;
		}

		if( 0 == idx ) {
			if( audRcrderRecord((uchar *)rcrdBuf, RECORD_MAX_SIZE) < 0 ) {
				ret = -1;
				break;
			}
		}
        ret = RECORD_MAX_SIZE - (idx * DIAG_MAX_DATA_SIZE);
		INFMSG("ret = %d\n", ret);

		if( ret > DIAG_MAX_DATA_SIZE ) {
			ret = DIAG_MAX_DATA_SIZE;
		} else if( ret <= 0 ) {
			ret = 0;
			break;
		}
        memcpy(rsp, rcrdBuf + idx * DIAG_MAX_DATA_SIZE, ret);
    }
        break;
    case 0x04:
        audRcrderClose();
        open = -1;
        break;
    case 0x05:
    // for test
/*    {
            #define RCRD_SIZE (160 * 1024)
            FILE  * fp;
            uchar * pcm = (uchar *)malloc(RCRD_SIZE);
            int size = RCRD_SIZE;

            if( audRcrderRecord(pcm, size) < 0 ) {
                ret = -1;
            }

            fp = fopen("/mnt/sdcard/rcrd.pcm", "w");
            if( fp ) {
                size = fwrite(pcm, 1, size, fp);
                fclose(fp);
            }
            free(pcm);
    }
*/
    // no break
    default:
        ret = -1;
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static const uchar s_pcmdata_mono[] = {
    0x92,0x02,0xcb,0x0b,0xd0,0x14,0x1d,0x1d,0xfc,0x24,0x17,0x2c,0x4a,0x32,0x69,0x37,
    0x92,0x3b,0x4e,0x3e,0x22,0x40,0x56,0x40,0x92,0x3f,0x12,0x3d,0x88,0x39,0x10,0x35,
    0xf0,0x2e,0x51,0x28,0xce,0x20,0x7f,0x18,0xd5,0x0f,0xda,0x06,0xdf,0xfd,0xa4,0xf4,
    0xa2,0xeb,0x39,0xe3,0x57,0xdb,0x3d,0xd4,0x1f,0xce,0xe2,0xc8,0xb1,0xc4,0xc0,0xc1,
    0xec,0xbf,0xc1,0xbf,0xa4,0xc0,0xf2,0xc2,0x18,0xc6,0xc2,0xca,0xc8,0xd0,0x36,0xd7,
    0xbb,0xde,0xe6,0xe6,0xa5,0xef,0xa6,0xf8,
};

static const uchar s_pcmdata_left[] = {
	0x92,0x02,0x00,0x00,0xCB,0x0B,0x00,0x00,0xD0,0x14,0x00,0x00,0x1D,0x1D,0x00,0x00,
	0xFC,0x24,0x00,0x00,0x17,0x2C,0x00,0x00,0x4A,0x32,0x00,0x00,0x69,0x37,0x00,0x00,
	0x92,0x3B,0x00,0x00,0x4E,0x3E,0x00,0x00,0x22,0x40,0x00,0x00,0x56,0x40,0x00,0x00,
	0x92,0x3F,0x00,0x00,0x12,0x3D,0x00,0x00,0x88,0x39,0x00,0x00,0x10,0x35,0x00,0x00,
	0xF0,0x2E,0x00,0x00,0x51,0x28,0x00,0x00,0xCE,0x20,0x00,0x00,0x7F,0x18,0x00,0x00,
	0xD5,0x0F,0x00,0x00,0xDA,0x06,0x00,0x00,0xDF,0xFD,0x00,0x00,0xA4,0xF4,0x00,0x00,
	0xA2,0xEB,0x00,0x00,0x39,0xE3,0x00,0x00,0x57,0xDB,0x00,0x00,0x3D,0xD4,0x00,0x00,
	0x1F,0xCE,0x00,0x00,0xE2,0xC8,0x00,0x00,0xB1,0xC4,0x00,0x00,0xC0,0xC1,0x00,0x00,
	0xEC,0xBF,0x00,0x00,0xC1,0xBF,0x00,0x00,0xA4,0xC0,0x00,0x00,0xF2,0xC2,0x00,0x00,
	0x18,0xC6,0x00,0x00,0xC2,0xCA,0x00,0x00,0xC8,0xD0,0x00,0x00,0x36,0xD7,0x00,0x00,
	0xBB,0xDE,0x00,0x00,0xE6,0xE6,0x00,0x00,0xA5,0xEF,0x00,0x00,0xA6,0xF8,0x00,0x00,
};

static const uchar s_pcmdata_right[] = {
	0x00,0x00,0x92,0x02,0x00,0x00,0xCB,0x0B,0x00,0x00,0xD0,0x14,0x00,0x00,0x1D,0x1D,
	0x00,0x00,0xFC,0x24,0x00,0x00,0x17,0x2C,0x00,0x00,0x4A,0x32,0x00,0x00,0x69,0x37,
	0x00,0x00,0x92,0x3B,0x00,0x00,0x4E,0x3E,0x00,0x00,0x22,0x40,0x00,0x00,0x56,0x40,
	0x00,0x00,0x92,0x3F,0x00,0x00,0x12,0x3D,0x00,0x00,0x88,0x39,0x00,0x00,0x10,0x35,
	0x00,0x00,0xF0,0x2E,0x00,0x00,0x51,0x28,0x00,0x00,0xCE,0x20,0x00,0x00,0x7F,0x18,
	0x00,0x00,0xD5,0x0F,0x00,0x00,0xDA,0x06,0x00,0x00,0xDF,0xFD,0x00,0x00,0xA4,0xF4,
	0x00,0x00,0xA2,0xEB,0x00,0x00,0x39,0xE3,0x00,0x00,0x57,0xDB,0x00,0x00,0x3D,0xD4,
	0x00,0x00,0x1F,0xCE,0x00,0x00,0xE2,0xC8,0x00,0x00,0xB1,0xC4,0x00,0x00,0xC0,0xC1,
	0x00,0x00,0xEC,0xBF,0x00,0x00,0xC1,0xBF,0x00,0x00,0xA4,0xC0,0x00,0x00,0xF2,0xC2,
	0x00,0x00,0x18,0xC6,0x00,0x00,0xC2,0xCA,0x00,0x00,0xC8,0xD0,0x00,0x00,0x36,0xD7,
	0x00,0x00,0xBB,0xDE,0x00,0x00,0xE6,0xE6,0x00,0x00,0xA5,0xEF,0x00,0x00,0xA6,0xF8,
};

int test_GetMonoPcm( const uchar ** pcm_data, int * pcm_bytes )
{
	*pcm_data  = s_pcmdata_mono;
	*pcm_bytes = sizeof s_pcmdata_mono;
	return 0;
}

int test_GetLeftPcm( const uchar ** pcm_data, int * pcm_bytes )
{
	*pcm_data  = s_pcmdata_left;
	*pcm_bytes = sizeof s_pcmdata_left;
	return 0;
}

int test_GetRightPcm( const uchar ** pcm_data, int * pcm_bytes )
{
	*pcm_data  = s_pcmdata_right;
	*pcm_bytes = sizeof s_pcmdata_right;
	return 0;
}

int testAudioOUT(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int   ret = 0;
    uchar out = data[0];
    uchar act = data[1];
    uchar chl = data[2];

    FUN_ENTER;
    INFMSG("out = %d, act = %d, chl = %d\n", out, act, chl);

	#define PLAY_SAMPLE_RATE  44100

    switch( act ) {
    case 0x01: {
        ALOGE("wangzuo:testAudioOUT 0x01");
		const uchar * pcm;
		int     pcm_len;
		if( 0 == chl ) {
			pcm     = s_pcmdata_mono;
			pcm_len = sizeof s_pcmdata_mono;
		} else if( 1 == chl ) {
			pcm     = s_pcmdata_left;
			pcm_len = sizeof s_pcmdata_left;
		} else {
			pcm     = s_pcmdata_right;
			pcm_len = sizeof s_pcmdata_right;
		}
        audPlayerClose();
        if( audPlayerOpen(out, PLAY_SAMPLE_RATE, chl, 0) < 0 ||
            audPlayerPlay(pcm, pcm_len) < 0 ) {
            ret = -1;
        }
	}
        break;
    case 0x02: {
        ALOGE("wangzuo:testAudioOUT 0x02 data + 3:%c,data_len-3:%d",data + 3,data_len - 3);
        audPlayerClose();
        if(audPlayerOpen(out, PLAY_SAMPLE_RATE, chl, 0) < 0 || audPlayerPlay(data + 3, data_len - 3) < 0){
            ret = -1;
        }
    }
        break;
    case 0x03:
        audPlayerClose();
        break;
    default:
        // for test
/*
        if( audPlayerOpen(out, 8000, 0, 0) >= 0 ) {
            #define M_SIZE (32 * 1024)
            uchar * pcm = (uchar *)malloc(M_SIZE);
            int size = 0;
            FILE * fp = fopen("/mnt/sdcard/hello.pcm", "r");
            if( fp != NULL ) {
                size = fread(pcm, 1, M_SIZE, fp);
                fclose(fp);
            } else {
                memset(pcm, 0 , M_SIZE);
            }

            if( audPlayerPlay(pcm, size) < 0 ) {
                ret = -1;
            }
            free(pcm);
        }
*/
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// lcd backlight, keypad backlight, vibrator, flashlight
int testLKBV(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;

    FUN_ENTER;
    INFMSG("data[0] = %d,data[1] = %d\n", *data,data[1]);

    switch( *data ) {
    case 0x00: //LCDBACKLIGHT
    {
        int value = data[1];
		lightOpen();
        if( value > 0 ) {
            lightSetLCD(value);
        } else {
            lightSetLCD(0);
            lightClose();
        }
    }
        break;
    case 0x02: // KPDBACKLIGHT
	{
        int value = data[1];
		lightOpen();
        if( value > 0 ) {
			lightSetKeypad(value);
        } else {
			lightSetKeypad(0);
            lightClose();
        }
    }
        break;
    case 0x03: // vibrator
    {
        int timeout = data[1];
        if( timeout > 0 ) {
			vibOpen();
            vibTurnOn(timeout);
        } else {
            vibTurnOff();
			vibClose();
        }
    }
        break;
    case 0x04: // flash light
    {
        int value = data[1];//0x11:high  ,0x01:low , 0x00:off
		if( flashlightSetValue(value) < 0 ) {
			ret = -1;
		}
    }
        break;
	case 0x06: // mic
	{
		uchar state = headsetPlugState();
		if (state >= 0){
			rsp[0] = state;
			ret = 1;
		}
	}
		break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testFM(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    switch( *data ) {
    case 1:
    {
        uint freq = ((data[1] << 16) | (data[2] << 8) | (data[3] << 0));

        if( fmOpen() < 0 || fmPlay(freq) < 0 ) {
            ret = -1;
        }
    }
        break;
    case 2:
        fmStop();
		fmClose();
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testBT(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;
    int num = 0;
    struct bdremote_t bdrmt[MAX_SUPPORT_RMTDEV_NUM];

    FUN_ENTER;
    INFMSG("BT Test CMD:%d\n", *data);

    //#define MAX_SUPPORT_RMTDEV_NUM 1

    switch( *data ) {
    case 1: // open bt
        if( !btIsOpened() ) {
            if( btOpen() < 0 ) {
                ret = -1;
            }
        }
        break;
    case 2: // inquire
	num = btGetInquireResult(bdrmt, MAX_SUPPORT_RMTDEV_NUM);
         	ret = -1;
        	if(BT_STATUS_INQUIRE_UNK== btGetInquireStatus()){
		DBGMSG("BT IRQ:Begin to Search!");
		btAsyncInquire();
        	}else if(BT_STATUS_INQUIRE_END== btGetInquireStatus() || BT_STATUS_INQUIRING== btGetInquireStatus()){
		if(btGetInquireResult(bdrmt, MAX_SUPPORT_RMTDEV_NUM) > 0){
			ret = 0;
		}
        }
        break;
    case 3: // get inquire
    {
        num = btGetInquireResult(bdrmt, 1);
        if( num > 0 ) {
            int i;
            uchar * pb = (uchar *)rsp;
            ret = num * 6;

            DBGMSG("BT get inquire:num = %d, pb = %p\n", num, pb);
            for( i = 0; i < num; ++i ) {
                //memcpy(pb, bdrmt[i].baddr, 6);
                pb[0] = bdrmt[i].addr_u8[5];  pb[1] = bdrmt[i].addr_u8[4];
                pb[2] = bdrmt[i].addr_u8[3];  pb[3] = bdrmt[i].addr_u8[2];
                pb[4] = bdrmt[i].addr_u8[1];  pb[5] = bdrmt[i].addr_u8[0];
                pb += 6;
            }
        } else {
            ret = -1;
        }
    }
        break;
    case 4:
        btClose();
        break;
    default:
        break;
    }
    DBGMSG("Return Value:%d",ret);
    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testWIFI(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;
    int wifiStatus = wifiGetStatus();

    #define MAX_SUPPORT_AP_NUM 10

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    switch( *data ) {
    case 1: // open wifi
        if( !(WIFI_STATUS_OPENED & wifiStatus) ) {
            if( wifiOpen() < 0 ) {
                ret = -1;
            }
        }
        break;
    case 2: // inquire
        if( !(WIFI_STATUS_SCAN_END & wifiStatus) ) {
            if( !(WIFI_STATUS_SCANNING & wifiStatus) ) {
                ret = wifiAsyncScanAP();
            }
        }
		
	/*if wifi don't successfully get the AP , it should be return fail*/
	if(  !(WIFI_STATUS_SCAN_END & wifiStatus) )
		ret = -1;
        break;
    case 3: // get inquired
    {
        struct wifi_ap_t aps[MAX_SUPPORT_AP_NUM];
        int num = wifiGetAPs(aps, MAX_SUPPORT_AP_NUM);
        if( num > 0 ) {
            int i;
            uchar * pb = (uchar *)rsp;
            ret = num * 6;

            //DBGMSG("num = %d, pb = %p\n", num, pb);
            for( i = 0; i < num; ++i ) {
                //memcpy(pb, aps[i].bmac, 6);
                pb[0] = aps[i].bmac[5];  pb[1] = aps[i].bmac[4];
                pb[2] = aps[i].bmac[3];  pb[3] = aps[i].bmac[2];
                pb[4] = aps[i].bmac[1];  pb[5] = aps[i].bmac[0];
                pb += 6;
            }
        } else {
            ret = -1;
        }
    }
        break;
    case 4:
        wifiClose();
        break;
    case 5: // get inquired ex
    {
        struct wifi_ap_t aps[MAX_SUPPORT_AP_NUM];
        int num = wifiGetAPs(aps, MAX_SUPPORT_AP_NUM);
        if( num > 0 ) {
            int i;
            uchar * pb = (uchar *)rsp;
            ret = num * (6 + 4);

            //DBGMSG("num = %d, pb = %p\n", num, pb);
            for( i = 0; i < num; ++i ) {
                //memcpy(pb, aps[i].bmac, 6);
                pb[0] = aps[i].bmac[5];  pb[1] = aps[i].bmac[4];
                pb[2] = aps[i].bmac[3];  pb[3] = aps[i].bmac[2];
                pb[4] = aps[i].bmac[1];  pb[5] = aps[i].bmac[0];
                pb += 6;
                //memcpy(pb, &(aps[i].signal), 4);
                int sig = aps[i].sig_level;
                pb[0] = (sig >> 0)  & 0xFF;
                pb[1] = (sig >> 8)  & 0xFF;
                pb[2] = (sig >> 16) & 0xFF;
                pb[3] = (sig >> 24) & 0xFF;
                pb += 4;
            }
        } else {
            ret = -1;
        }
    }
        break;

    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testIIC(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;
    uchar val;

    FUN_ENTER;
    INFMSG("addr = %02X, reg = %02X, bus = %02X, ack = %d\n", data[0], data[1], data[2], data[3]);

    if( drvOpen() >= 0 && drvI2CRead(data[2], data[0], data[1], &val) >= 0 ) {
        ret = 2;
        rsp[0] = 0x00;
        rsp[1] = val;
    } else {
        ret = -1;
    }

	drvClose();

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testCharger(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

    switch(*data)
    {
    case 1:
        //batOpen();
        if( batEnableCharger(1) < 0 ) {
            ret = -1;
        }
        break;
    case 2:
        if( batStatus() < 0 ) {
            ret = -1;
        }
        break;
    case 3:
        if( batEnableCharger(0) < 0 ) {
            ret = -1;
        }
        //batClose();
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testSensor(const uchar * data, int data_len, uchar *rsp, int rsp_size)
{
	int ret = 0;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

	ret = 1;
	if( sensorOpen() >= 0 && sensorActivate(data[0]) >= 0 ) {
		rsp[0] = 0x00;
	} else {
		rsp[0] = 0x01;
	}

	sensorClose();

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int testGps(const uchar * data, int data_len, uchar *rsp, int rsp_size)
{
	int ret = -1;

    FUN_ENTER;
    INFMSG("data[0] = %d\n", *data);

	switch(*data)
    {
	case 0: // init, not used
		break;
    case 1:
		if( gpsOpen() >= 0 ) {
			ret = 0;
		}
        break;
    case 2:
		if( gpsStart() >= 0 ) {
			ret = 0;
		}
        break;
    case 3:{
		int svn = gpsGetSVNum();
		rsp[0] = (uchar)svn;
		ret    = 1;
	}
        break;
	case 4:
        gpsStop();
		gpsClose();
		ret = 0;
        break;
    default:
        break;
    }

    FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int test_Deinit( void )
{
	FUN_ENTER;

	drvClose();
	release_wake_lock(AT_WAKE_LOCK_NAME);

	FUN_EXIT;
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
