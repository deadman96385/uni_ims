// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#include <fcntl.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include "type.h"
#include "input.h"
#include "driver.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_input {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static const char INPUT_EVT_NAME[]  = "/dev/input/event";
//static const char DEV_NAME_KPD[]     = "sprd-keypad";
static const char DEV_NAME_KPD[]     = "sci-keypad";
static const char DEV_NAME_TP[]      = "pixcir_ts";

//------------------------------------------------------------------------------
static int             s_fdKPD   = -1;
static int             s_fdTP    = -1;
static volatile int    sRunning  = 0;
static pthread_mutex_t sMutxExit = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sCondExit = PTHREAD_COND_INITIALIZER;

static struct kpd_info_t sKeyInfo;

static int             openInputDev( const char * name );
static int             pollInputDev( int fd, int timeout );
static void          * inputThread( void *param );
//------------------------------------------------------------------------------

int inputOpen( void )
{   
    AT_ASSERT( -1 == s_fdKPD );
    AT_ASSERT( -1 == s_fdTP );
    
    sKeyInfo.key = -1;
    
	s_fdKPD = openInputDev(DEV_NAME_KPD);
    if( s_fdKPD < 0 ) {
        ERRMSG("open keypad '%s' fail!\n", DEV_NAME_KPD);
        return -1;
    }
/*  // it's ok, not used    
    s_fdTP = openInputDev(DEV_NAME_TP);
    if( s_fdTP < 0 ) {
        ERRMSG("open touchpanel '%s' fail!\n", DEV_NAME_TP);
        close(s_fdKPD);
        s_fdKPD = -1;
        return -2;
    }
*/    
	drvOpen();
	
    sRunning = 1;
    pthread_t      id;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&id, &attr, inputThread, NULL);
    
    return 0;
}

int inputKPDGetKeyInfo( struct kpd_info_t * info )
{
	AT_ASSERT( NULL != info );
	
    *info = sKeyInfo;
	return sKeyInfo.key;
}

int inputKPDWaitKeyPress( struct kpd_info_t * info, int timeout )
{
	AT_ASSERT( NULL != info );
	
	info->key = -1;
	info->row = 0xFFFF;
	info->col = 0xFFFF;
	info->gio = 0;
	
    if( s_fdKPD < 0 ) {
        ERRMSG("keypad '%s' unopened!\n", DEV_NAME_KPD);
        return -1;
    }
    
    if( pollInputDev(s_fdKPD, timeout) < 0 ) {
        return -2;
    } 
    
    struct input_event iev[64];
    int32_t readSize = read(s_fdKPD, iev, sizeof(iev));
    if (readSize < 0) {
        if (errno != EAGAIN && errno != EINTR) {
            WRNMSG("could not get event (errno=%d)", errno);
        }
        return -3;
    } else if ((readSize % sizeof(struct input_event)) != 0) {
        ERRMSG("could not get event (wrong size: %d)", readSize);
        return -4;
    }

    int num = readSize / sizeof(struct input_event);
    for( int i = 0; i < num; ++i ) {
        //DBGMSG("type = %d, code = %d, value = %d\n", iev[i].type, iev[i].code, iev[i].value);
        if( EV_KEY == iev[i].type ) {		
			info->key = iev[i].code;
			drvGetKpdInfo(iev[i].code, &(info->row), &(info->col), &(info->gio));

			DBGMSG("row = %d, col = %d, gio = %d\n", info->row, info->col, info->gio);
	    }
    }
    INFMSG("KPD: key = %d\n", info->key);
    
    return info->key;
}

int inputTPGetPoint( int *x, int *y, int timeout )
{
    if( s_fdTP < 0 ) {
        ERRMSG("touchpanel '%s' unopened!\n", DEV_NAME_TP);
        return -1;
    }
    
    if( pollInputDev(s_fdTP, timeout) < 0 ) {
        return -2;
    } 
    
    struct input_event iev[64];
    int32_t readSize = read(s_fdTP, iev, sizeof(iev));
    if (readSize < 0) {
        if (errno != EAGAIN && errno != EINTR) {
            WRNMSG("could not get event (errno=%d)", errno);
        }
        return -3;
    } else if ((readSize % sizeof(struct input_event)) != 0) {
        ERRMSG("could not get event (wrong size: %d)", readSize);
        return -4;
    }
    *x = *y = -1;
    int num = readSize / sizeof(struct input_event);
    for( int i = 0; i < num; ++i ) {
        if( EV_ABS == iev[i].type ) {
            //DBGMSG("type = %d, code = %d, value = %d\n", iev[i].type, iev[i].code, iev[i].value);
            if( 0x35 == iev[i].code || ABS_X == iev[i].code )
                *x = iev[i].value;
            else if ( 0x36 == iev[i].code || ABS_Y == iev[i].code )
                *y = iev[i].value;
	    }
    }
    int ret = ( *x >= 0 && *y >= 0 ) ? 0 : -1;
    
    INFMSG("TP: x = %d, y = %d\n", *x, *y);
    return ret;
}

int inputClose( void )
{
    FUN_ENTER;
    
    if( sRunning ) {
        sRunning = 0;
        
        pthread_mutex_lock(&sMutxExit);
        struct timespec to;
        to.tv_nsec = 0;
        to.tv_sec  = time(NULL) + 3;
            
        if( pthread_cond_timedwait(&sCondExit, &sMutxExit, &to) < 0 ) {
            WRNMSG("wait thread exit timeout!\n");
        }
        pthread_mutex_unlock(&sMutxExit);
    }
    
    if( s_fdKPD >= 0 ) {
        close(s_fdKPD);
        s_fdKPD = -1;
    }
    
    if( s_fdTP >= 0 ) {
        close(s_fdTP);
        s_fdTP = -1;
    }

	drvClose();

    FUN_EXIT;
    return 0;
}

//==============================================================================
//==============================================================================
int openInputDev( const char * name )
{
    int fd = -1;
    int inputEvtNameLen = strlen(INPUT_EVT_NAME);
    AT_ASSERT(inputEvtNameLen < 30);
    
    char inputEvtName[32];
    strcpy(inputEvtName, INPUT_EVT_NAME);
    inputEvtName[inputEvtNameLen + 1] = 0;
    
    for( int i = 0; i < 16; ++i ) {
        inputEvtName[inputEvtNameLen] = (char)('0' + i);
        DBGMSG("input evt name = %s\n", inputEvtName);
        
        struct stat stt;
        if( stat(inputEvtName, &stt) != 0 ) {
            ERRMSG("stat '%s' error!\n", inputEvtName);
            break;
        }
        
        fd = open(inputEvtName, O_RDONLY);
        if( fd < 0 ) {
            WRNMSG("Failed to open %s, %s\n", inputEvtName, strerror(errno));
            continue;
        }
        
        char devName[32] = { 0 };
        if( ioctl(fd, EVIOCGNAME(sizeof(devName)), devName) > 0 && 
            strncmp(devName, name, strlen(name)) == 0 ) {
            INFMSG("open '%s' OK\n", name);
            break;    
        }
        
        WRNMSG("input evt name = %s, dev name = %s\n", inputEvtName, devName);
        close(fd);
        fd = -1;
    }
    
    if( fd >= 0 ) {
        if( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 ) {
            ERRMSG("fcntl: set to nonblock error: %s!\n", strerror(errno));
        }
    }
    
    return fd;
}

int pollInputDev( int fd, int timeout )
{
    struct pollfd plfd;
    plfd.fd      = fd;
    plfd.events  = POLLIN;
    plfd.revents = 0;
    
    int ret = poll(&plfd, 1, timeout);
    if( ret <= 0 ) {
        if( 0 == ret ) {
            WRNMSG("poll timeout (%d ms)\n", timeout);
        } else {
            ERRMSG("poll error: %s\n", strerror(errno));
        }
        return -1;
    }
    
    if( plfd.revents & POLLIN ) {
        return 0;
    }
    
    return -2;
}

void * inputThread( void *param )
{
    FUN_ENTER;
    
	struct kpd_info_t kpd_info;
    while( sRunning ) {
        if( inputKPDWaitKeyPress(&kpd_info, 2000) < 0 ) {
            sKeyInfo.key = -1;
            continue;
        }
		
		memcpy(&sKeyInfo, &kpd_info, sizeof(struct kpd_info_t));
    }
    
    pthread_mutex_lock(&sMutxExit);
    pthread_cond_signal(&sCondExit);
    pthread_mutex_unlock(&sMutxExit);
    
    FUN_EXIT;
    return NULL;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
