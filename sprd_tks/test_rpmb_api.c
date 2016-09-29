
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
	
	t = localtime(&tt);
	memcpy(&(key.option.timelimit), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timestored), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timefailed), t, sizeof(struct tm));
	key.keyinfo.timesfailed = 1;
	key.keyinfo.flags = 0xAA55AA55;
	key.key.key_type = KEY_AES_256;
	
	memset(key.key.keydata, 0x11, key.key.keylen);
	
	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_write_key() failed!\n");
	} else {

		ALOGE("rpmb_write_key() succeed!\n");
	}
}


{
	rpmb_keycontainer key;
	struct tm *t;
	time_t tt;
	int ret;

//prepare rpmb_keycontainer for rpmb_write_key() test
	key.option.flags = 0x1111;
	key.option.locklimit = 22222222;
	
	t = localtime(&tt);
	memcpy(&(key.option.timelimit), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timestored), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timeaccessed), t, sizeof(struct tm));
	memcpy(&(key.keyinfo.timefailed), t, sizeof(struct tm));
	key.keyinfo.timesfailed = 1;
	key.keyinfo.flags = 0xAA55AA55;
	key.key.key_type = KEY_RSA_2048_PRV;
	
	
	
	if (ret == ERROR_RPMB_FAILED) {
		ALOGE("rpmb_write_key() failed!\n");
	} else {

		ALOGE("rpmb_write_key() succeed!\n");
	}
}


{
	
	
	

	testaes_rpmb_write_key(keyname1);

	test_rpmb_read_key(keyname1);

	test_rpmb_read_key(keyname2);

	testaes_rpmb_write_key(keyname2);

	test_rpmb_read_key(keyname2);
	



{
	
	
	

	testrsa_rpmb_write_key(keyname1);
	

	test_rpmb_read_key(keyname2);

	testrsa_rpmb_write_key(keyname2);
	
	



{
	
	
	int num;
	
	
	
		
		
		
			
		
		}
	



{
	
	
	
	
	
	
	
	
	
	
	
	
	
	



{
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
		sizeof(sysinfo.devcert.pubkey.mod));
	
		sizeof(sysinfo.devcert.devid));
	
		sizeof(sysinfo.devcert.signature));
	
	
	
		
		
		
		
		
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timeaccessed=\n");
		t = &(sysinfo.timeaccessed);
		
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timefailed\n");
		
		
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
		       t->tm_min, t->tm_sec);

		ALOGE("timesfailed=%d,codefailed=0x%x,keyname=%s,\n",
		      sysinfo_out.timesfailed, sysinfo.codefailed,
		      sysinfo.keyname);
		
		
		
			
		
		
			
		
		
			
		
		}
	



{
	
	
	
	
	
	
	
	
	
	
	
	
		
	



{
	
	
	
	
	
	
	
	
	
	
	
	
		
	



{

	int ret;
	
	
	

#ifndef TEST_FILE
clearRPMB( );//clear the real rpmb if not used file for test
#endif

	
	
		ALOGE("test end due to rpmb_initialize() failed!\n");
		goto end;
	}
	
	
	
	
	
	
	
	
	
	
	
///aes   write key info
	    ALOGE("----start test aes write/read keyinfo!----\n");
	
	
	
	
	
	
//rsa write key info
	    test_rpmb_read_keyinfo(keyname2);
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
      
	rpmb_deinitialize();

	return 0;
}

