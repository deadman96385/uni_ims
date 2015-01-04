
#include <rpmb_api.h>
/////test case
void test_rpmb_read_keyinfo(char *keyname)
{

	key_info keyinf;
	struct tm *t;

	int ret = rpmb_read_keyinfo(keyname, &keyinf);

	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_read_keyinfo() %s failed!\n", keyname);
	} else {
		ALOGE("rpmb_read_keyinfo() %s succeed!\n", keyname);
		ALOGE("timestored=\n");
		t = &(keyinf.timestored);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		      t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		      t->tm_min, t->tm_sec);

		ALOGE("timeaccessed=\n");
		t = &(keyinf.timeaccessed);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		      t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		      t->tm_min, t->tm_sec);

		ALOGE("timefailed=\n");
		t = &(keyinf.timefailed);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		      t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		      t->tm_min, t->tm_sec);

		ALOGE("timesfailed=%d,flags=0x%x\n", keyinf.timesfailed,
		      keyinf.flags);
		ALOGE("\n");
	}
}

void test_rpmb_read_opt(char *keyname)
{

	opt option;
	struct tm *t;

	int ret = rpmb_read_opt(keyname, &option);

	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_read_opt() %s failed!\n", keyname);
	} else {
		ALOGE("rpmb_read_opt() %s succeed!\n", keyname);

		ALOGE("flags=0x%x,locklimit=%d\n", option.flags,
		      option.locklimit);
		ALOGE("timelimit=\n");
		t = &(option.timelimit);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		      t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		      t->tm_min, t->tm_sec);

		ALOGE("\n");
	}
}

void test_rpmb_read_key(char *keyname)
{
	tks_key *testkey = (tks_key *) malloc(sizeof(tks_key));
	int i = 0;

	int ret = rpmb_read_key(keyname, testkey);

	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_read_key() %s failed!\n", keyname);
	} else {
		ALOGE("rpmb_read_key() %s succeed!\n", keyname);
		ALOGE("testkey->type=%d,keylen=%d\n", testkey->key_type,
		      testkey->keylen);
		ALOGE("testkey->keydata=0x");
		for (i = 0; i < testkey->keylen; i++)
			ALOGE("%x", testkey->keydata[i]);
		ALOGE("\n");
	}
	test_rpmb_read_keyinfo(keyname);
	test_rpmb_read_opt(keyname);
}

void testaes_rpmb_write_key(char *keyname)
{
	rpmb_keycontainer key;
	struct tm *t;
	time_t tt;
	int ret;

//prepare rpmb_keycontainer for rpmb_write_key() test
	key.option.flags = 0x1111;
	key.option.locklimit = 22222222;
	time(&tt);
	t = localtime(&tt);
	memcpy(&(key.option.timelimit), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timestored), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timefailed), t, sizeof(struct tm));
	key.keyinfo.timesfailed = 1;
	key.keyinfo.flags = 0xAA55AA55;
	key.key.key_type = KEY_AES_256;
	key.key.keylen = 32;
	memset(key.key.keydata, 0x11, key.key.keylen);
	ret = rpmb_write_key(keyname, key);
	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_write_key() failed!\n");
	} else {

		ALOGE("rpmb_write_key() succeed!\n");
	}
}

void testrsa_rpmb_write_key(char *keyname) 
{
	rpmb_keycontainer key;
	struct tm *t;
	time_t tt;
	int ret;

//prepare rpmb_keycontainer for rpmb_write_key() test
	key.option.flags = 0x1111;
	key.option.locklimit = 22222222;
	time(&tt);
	t = localtime(&tt);
	memcpy(&(key.option.timelimit), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timestored), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timefailed), t, sizeof(struct tm));
	key.keyinfo.timesfailed = 1;
	key.keyinfo.flags = 0xAA55AA55;
	key.key.key_type = KEY_RSA_2048_PRV;
	key.key.keylen = 1156;
	memset(key.key.keydata, 0xAA, key.key.keylen);
	ret = rpmb_write_key(keyname, key);
	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_write_key() failed!\n");
	} else {

		ALOGE("rpmb_write_key() succeed!\n");
	}
}

int test_aeskey() 
{
	char *keyname1 = "aeskey001";
	char *keyname2 = "aeskey002";
	test_rpmb_read_key(keyname1);

	testaes_rpmb_write_key(keyname1);

	test_rpmb_read_key(keyname1);

	test_rpmb_read_key(keyname2);

	testaes_rpmb_write_key(keyname2);

	test_rpmb_read_key(keyname2);
	return 0;
}

int test_rsakey() 
{
	char *keyname1 = "rsakey001";
	char *keyname2 = "rsakey002";
	test_rpmb_read_key(keyname1);

	testrsa_rpmb_write_key(keyname1);
	test_rpmb_read_key(keyname1);

	test_rpmb_read_key(keyname2);

	testrsa_rpmb_write_key(keyname2);
	test_rpmb_read_key(keyname2);
	return 0;
}

 int test_rpmb_getall_keyname() 
{
	long ptr;
	keyname_list_t keynamelist;	//char (*keynamelist)[RPMB_KEYNAME_LEN-2];
	int num;
	int index = 0;
	 rpmb_getall_keyname(&keynamelist, &num);
	 if (num > 0)
		 {
		ALOGE("%d Key in rpmb, name list:\n", num);
		while (index < num)
			ALOGE("%s\n", keynamelist[index++]);
		free(keynamelist);	//NOTICE: must free the space mannually, which is allocated in rpmb_getall_keyname();
		}
	 return 0;
}

int test_write_keyinfo(char *keyname) 
{
	key_info keyinfo;
	int ret;
	struct tm *t;
	time_t tt;
	time(&tt);
	t = localtime(&tt);
	t->tm_year = 80;
	memcpy(&(keyinfo.timestored), t, sizeof(struct tm));
	memcpy(&(keyinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(keyinfo.timefailed), t, sizeof(struct tm));
	keyinfo.timesfailed = 20;
	keyinfo.flags = 0x11223344;
	ret = rpmb_write_keyinfo(keyname, keyinfo);
	return 0;
}

 int test_rw_sysinfo() 
{
	int ret, i = 0;
	sys_info sysinfo, sysinfo_out;
	 struct tm *t;
	time_t tt;
	time(&tt);
	t = localtime(&tt);
	memcpy(&(sysinfo.timeinit), t, sizeof(struct tm));
	memcpy(&(sysinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(sysinfo.timefailed), t, sizeof(struct tm));
	sysinfo.timesfailed = 10;
	sysinfo.codefailed = 0x11111111;
	memset(sysinfo.keyname, '\0', sizeof(sysinfo.keyname));
	memcpy(sysinfo.keyname, "devicekeyabcd", strlen("devicekeyabcd"));
	sysinfo.devcert.pubkey.exp = 0x01010101;
	memset(sysinfo.devcert.pubkey.mod, 0xCC,
		sizeof(sysinfo.devcert.pubkey.mod));
	memcpy(sysinfo.devcert.devid, "123456789abcdef",
		sizeof(sysinfo.devcert.devid));
	memset(sysinfo.devcert.signature, 0xDD,
		sizeof(sysinfo.devcert.signature));
	 ret = rpmb_write_sysinfo(sysinfo);
	ret = rpmb_read_sysinfo(&sysinfo_out);
	if (ret == ERROR_RPMB_SUCCESS)
		 {
		ALOGE("test_rw_sysinfo() succeed!\n");
		ALOGE("timeinit=\n");
		t = &(sysinfo_out.timeinit);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timeaccessed=\n");
		t = &(sysinfo.timeaccessed);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timefailed\n");
		t = &(sysinfo.timefailed);
		ALOGE("%4dyear%02dmonth%02dday%02d:%02d:%02d\n",
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timesfailed=%d,codefailed=0x%x,keyname=%s,\n",
		      sysinfo_out.timesfailed, sysinfo.codefailed,
		      sysinfo.keyname);
		ALOGE("devcert.pubkey.exp=0x%x\n", sysinfo.devcert.pubkey.exp);
		ALOGE("devcert.pubkey.mod=");
		for (i = 0; i < 256; i++)
			ALOGE("%x\n", sysinfo.devcert.pubkey.mod[i]);
		ALOGE("devcert.devid=");
		for (i = 0; i < 15; i++)
			ALOGE("%c\n", sysinfo.devcert.devid[i]);
		ALOGE("devcert.signature=");
		for (i = 0; i < 256; i++)
			ALOGE("%x\n", sysinfo.devcert.signature[i]);
		ALOGE("\n");
		}
	return ERROR_RPMB_SUCCESS;
}

 int test_rw_aes_userpass(char *keyname) 
{
	tks_key userpass;
	tks_key userpass_out;
	int i = 0;
	 userpass.key_type = KEY_AES_256;
	userpass.keylen = 32;
	memset(userpass.keydata, 0x11, userpass.keylen);
	 rpmb_write_userpass(keyname, userpass);
	 rpmb_read_userpass(keyname, &userpass_out);
	 ALOGE("userpass_out.type=%d \n", userpass_out.key_type);
	ALOGE("userpass_out.keylen=%d \n", userpass_out.keylen);
	 ALOGE("userpass_out.keydata=0x\n");
	for (i = 0; i < userpass_out.keylen; i++)
		ALOGE("%x\n", userpass_out.keydata[i]);
	 return 0;
 }

 int test_rw_rsa_userpass(char *keyname) 
{
	tks_key userpass;
	tks_key userpass_out;
	int i = 0;
	 userpass.key_type = KEY_RSA_2048_PRV;
	userpass.keylen = 1156;
	memset(userpass.keydata, 0x22, userpass.keylen);
	rpmb_write_userpass(keyname, userpass);
	 rpmb_read_userpass(keyname, &userpass_out);
	 ALOGE("userpass_out.type=%d \n", userpass_out.key_type);
	ALOGE("userpass_out.keylen=%d \n", userpass_out.keylen);
	 ALOGE("userpass_out.keydata=0x\n");
	for (i = 0; i < userpass_out.keylen; i++)
		ALOGE("%x\n", userpass_out.keydata[i]);
	return 0;
 }

int main()
{

	int ret;
	char *keyname1 = "aeskey001";
	char *keyname2 = "rsakey001";
	int i = 0;

#ifndef TEST_FILE
clearRPMB( );//clear the real rpmb if not used file for test
#endif

	 ret = rpmb_initialize();
	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("test end due to rpmb_initialize() failed!\n");
		goto end;
	}
	  test_aeskey();
	test_rsakey();
	test_rpmb_getall_keyname();
	 rpmb_del_key(keyname1);
	test_rpmb_getall_keyname();
	 rpmb_del_key(keyname2);
	test_rpmb_getall_keyname();
	 test_aeskey();
	test_rsakey();
	test_rpmb_getall_keyname();
	 
///aes   write key info
	    ALOGE("----start test aes write/read keyinfo!----\n");
	test_rpmb_read_keyinfo(keyname1);
	test_write_keyinfo(keyname1);
	test_rpmb_read_keyinfo(keyname1);
	ALOGE("----end test aes write/read keyinfo!----\n");
	ALOGE("----start test rsa write/read keyinfo!----\n");
	
//rsa write key info
	    test_rpmb_read_keyinfo(keyname2);
	test_write_keyinfo(keyname2);
	test_rpmb_read_keyinfo(keyname2);
	ALOGE("----end test rsa write/read keyinfo!----\n");
	 ALOGE("----start test write/read sysinfo!----\n");
	test_rw_sysinfo();
	 ALOGE("----end test write/read sysinfo!----\n");
	 ALOGE("----start test aes write/read userpass!----\n");
	test_rw_aes_userpass(keyname1);
	ALOGE("----end test aes write/read userpass!----\n");
	 ALOGE("----start test rsa write/read userpass!----\n");
	test_rw_rsa_userpass(keyname2);
	ALOGE("----end test rsa write/read userpass!----\n");
	 test_rpmb_getall_keyname();
	rpmb_del_key(keyname1);
	test_rpmb_getall_keyname();
	 rpmb_del_key(keyname2);
	test_rpmb_getall_keyname();
         end:
	rpmb_deinitialize();

	return 0;
}


