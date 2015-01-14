#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include "tks_lib.h"
#ifndef KERNEL_TEST
#include <cutils/properties.h>
#include <utils/Log.h>
#endif

#define ERROR_RPMB_SUCCESS 0
#define ERROR_RPMB_FAILED 1
#define KEYNAME_LEN 30		//key name space size (bytes num) including ending character '\0'
typedef char (*keyname_list_t)[KEYNAME_LEN];

int rpmb_initialize();		//
int rpmb_deinitialize();
int rpmb_read_userpass(char *keyname, tks_key * userpass);
int rpmb_read_key(char *keyname, tks_key * key);	//
int rpmb_read_keyinfo(char *keyname, key_info * keyinfo);	//
int rpmb_read_opt(char *keyname, opt * option);	//
int rpmb_read_sysinfo(sys_info * sysinfo);
int rpmb_write_userpass(char *keyname, tks_key userpass);
int rpmb_write_key(char *keyname, rpmb_keycontainer key);	//
int rpmb_write_keyinfo(char *keyname, key_info keyinfo);
int rpmb_write_sysinfo(sys_info sysinfo);
int rpmb_del_key(char *keyname);	//
int rpmb_getall_keyname(keyname_list_t * list, int *num);	//
