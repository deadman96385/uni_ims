// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <dirent.h>
#include <fcntl.h>

#include "type.h"
#include "tcard.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_tcard {
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

static const char TCARD_VOLUME_NAME[]  = "sdcard";
static char TCARD_DEV_NAME[]     = " ";//"../devices/platform/sprd-sdhci.0/mmc_host/mmc1/mmc1:1234/block/mmcblk0";//"/devices/platform/sprd-sdhci.0/mmc_host/mmc0";//"/devices/platform/mxsdhci.2/mmc_host/mmc2";
static const char SYS_BLOCK_PATH[]     = "/sys/block";
static const char TCARD_FILE_NAME[]    = "/mnt/sdcard/sciautotest";
static const char TCARD_TEST_CONTENT[] = "SCI TCard: 2012-11-12";
//------------------------------------------------------------------------------

#define TCARD_DEV_PATH_OPEN "/sys/devices/platform/sprd-sdhci.0/mmc_host/mmc1";
#define TCARD_DEV_PATH_1 "../devices/platform/sprd-sdhci.0/mmc_host/mmc1";
#define TCARD_DEV_PATH_2 "block/mmcblk0";
#define TCARD_DEV_PATH_3 "block/mmcblk1";

int tcard_get_dev_path(char*path)
{
	DIR *dir;
	struct dirent *de;
	int found = 0;
	char dirpath_open[] = TCARD_DEV_PATH_OPEN;
	char dirpath_1[] = TCARD_DEV_PATH_1;

	dir = opendir(dirpath_open);
	DBGMSG("dir = %d \n ",dir);
	if (dir == NULL)
		return -1;

	while((de = readdir(dir))) {
		if (strncmp(de->d_name, "mmc1:", strlen("mmc1:")) != 0) {
		    continue;
		}
		sprintf(path, "%s/%s", dirpath_1, de->d_name);
	    found = 1;
	}
	closedir(dir);

	if (found) {
		DBGMSG("the tcard dir is %s \n",path);
		return 0;
	}else {
		DBGMSG("the tcard dir is not found \n");
		return -1;
	}
}



//------------------------------------------------------------------------------
int tcardOpen( void )
{
	int ret = 0;
	ret = tcard_get_dev_path(TCARD_DEV_NAME);
	DBGMSG("ret %d \n",ret);
	return ret;
}

//------------------------------------------------------------------------------
int tcardIsPresent( void )
{
    DIR * dirBlck = opendir(SYS_BLOCK_PATH);
    if( NULL == dirBlck ) {
        ERRMSG("opendir '%s' fail: %s(%d)\n", SYS_BLOCK_PATH, strerror(errno), errno);
        return -1;
    }
    int present = -2;

    char dirpath_2[] = TCARD_DEV_PATH_2;
    char dirpath_3[] = TCARD_DEV_PATH_3;
    char tcard_dev_path2[128];
    char tcard_dev_path3[128];
    sprintf(tcard_dev_path2, "%s/%s", TCARD_DEV_NAME, dirpath_2);
    sprintf(tcard_dev_path3, "%s/%s", TCARD_DEV_NAME, dirpath_3);
    
    char realName[256];
    char linkName[128];
    strncpy(linkName, SYS_BLOCK_PATH, sizeof(linkName) - 1);
    char * pname = linkName + strlen(linkName);
    *pname++ = '/';
    
    struct dirent *de;
    while( (de = readdir(dirBlck)) ) {
        if (de->d_name[0] == '.' || DT_LNK != de->d_type )
            continue;

        strncpy(pname, de->d_name, 64);
    
        int len = readlink(linkName, realName, sizeof realName);
        if( len < 0 ) {
            ERRMSG("readlink error: %s(%d)\n", strerror(errno), errno);
            continue;
        } 
        realName[len] = 0;
        
        DBGMSG("link name = %s, real name = %s, TCARD_DEV_NAME=%s\n", linkName, realName,TCARD_DEV_NAME);
        if(( strstr(realName, tcard_dev_path2) != NULL ) || ( strstr(realName, tcard_dev_path3) != NULL )) {
            present = 1;
            DBGMSG("TCard is present.\n");
            break;
        }
    }
    
    closedir(dirBlck);
    return present;
}

int tcardIsMount( void )
{
    char device[256];
    char mount_path[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        ERRMSG("Error opening /proc/mounts (%s)", strerror(errno));
        return -1;
    }

    int mount = 0;
    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s\n", device, mount_path);
        DBGMSG("dev = %s, mount = %s\n", device, mount_path);
        if( NULL != strstr(mount_path, TCARD_VOLUME_NAME)) {
            mount = 1;
            INFMSG("TCard mount path: '%s'\n", mount_path);
            break;
        }
    }

    fclose(fp);
    return mount;
}

int tcardRWTest( void )
{
    FILE * fp = fopen(TCARD_FILE_NAME, "w");
    if( NULL == fp ) {
        ERRMSG("open '%s'(rw) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -1;
    }
    
    if( fwrite(TCARD_TEST_CONTENT, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        ERRMSG("write '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -2;
    }
    fclose(fp);
    
    fp = fopen(TCARD_FILE_NAME, "r");
    if( NULL == fp ) {
        ERRMSG("open '%s'(ronly) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -3;
    }
    
    AT_ASSERT( sizeof(TCARD_TEST_CONTENT) < 128 );
    
    char buf[128];
    
    if( fread(buf, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        ERRMSG("read '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -4;
    }
    fclose(fp);
    
    unlink(TCARD_FILE_NAME);
    
    if( strncmp(buf, TCARD_TEST_CONTENT, sizeof(TCARD_TEST_CONTENT) - 1) ) {
        ERRMSG("read = %s, dst = %s\n", buf, TCARD_TEST_CONTENT);
        return -5;
    }
    
    INFMSG("TFlash Card rw OK.\n");
    return 0;
}

//------------------------------------------------------------------------------
int tcardClose( void )
{
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
