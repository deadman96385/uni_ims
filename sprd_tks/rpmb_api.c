#define LOG_TAG "tks_lib"

#include "rpmb_api.h"

////////////////rpmb_api.c start
//#define NULL 0

#ifdef KERNEL_TEST
#define ALOGD    printf
#define ALOGE    printf
#endif

#define KEYTYPE_AES 0		//key type in rpmb:AES
#define KEYTYPE_RSA 1		//key type in rpmb:RSA

#define KEYTYPE_USERPASS 0	//key type:userpass key
#define KEYTYPE_KEY 1		//key type: key

#define RPMB_SYSINFO_START 0
#define RPMB_SYSINFO_LEN 608

#define RPMB_AES_START 616	//(RPMB_SYSINFO_LEN+4+4)
#define RPMB_AES_LEN 144//128

#define RPMB_RSA_LEN 1264

#define RPMB_KEYNAME_LEN (KEYNAME_LEN+2)	//32
#define RPMB_OPT_LEN 14
#define RPMB_KEYINFO_LEN 32
#define RPMB_SYSINFO_LEN 608

//#define TEST_FILE
#ifdef TEST_FILE
#include <linux/stat.h>
#define SIZE_RPMB (2*1024*1024)
#define RPMB_DATA_SIZE 256
#else
#define rpmb_dev_path  "/dev/block/mmcblk0rpmb"
int sizeRPMB=0;//total bytes len of RPMB dev
int blksizeRPMB=256;//bytes len of one block in RPMB
#define SIZE_RPMB  sizeRPMB
#define RPMB_DATA_SIZE blksizeRPMB
#endif

#define SIZE_KEYAREA (SIZE_RPMB-RPMB_SYSINFO_LEN-8)

const byte rpmb_emptybyte = 0x00;

typedef struct keymap_st {
	int id;			//id=0,1,2,...id表示第几个aes/rsa块，可以不连续，密钥被删除后对应rpmb位置清空，//譬如原来aes_key[0]，1，2，3连续四个块都有密钥，1对应的密钥被删除，还需将aes_key[1]结构体的name清空；
	byte name[RPMB_KEYNAME_LEN];	//max 32B *.u/*.k
	int type;
	struct keymap_st *prev;
	struct keymap_st *next;
} keymap;

keymap *aes_key_list = NULL;
keymap *rsa_key_list = NULL;
int aes_key_list_len;		//numbers of keymap in aes_key_list
int rsa_key_list_len;		//numbers of keymap in rsa_key_list
static int fd_rpmbdev = -1;
static int is_initialized = 0;

#ifdef TEST_FILE
uint8_t  rpmb_blk_read(char *blk_data,uint16_t blk_index,uint8_t block_count)
{
	lseek(fd_rpmbdev, RPMB_DATA_SIZE*blk_index, SEEK_SET);
	read(fd_rpmbdev, blk_data, RPMB_DATA_SIZE*block_count);
	return 0;
}

uint8_t rpmb_blk_write(char *write_data,uint16_t blk_index)
{
    	lseek(fd_rpmbdev, RPMB_DATA_SIZE*blk_index, SEEK_SET);
	write(fd_rpmbdev, write_data, RPMB_DATA_SIZE);
	return 0;
}
#endif

static int read_rpmb_pos(int fd, int pos, int whence, byte * buf, int len)
{
#if 0//def TEST_FILE
	lseek(fd, pos, whence);
	read(fd, buf, len);
#else
	//byte *totalblkdata=malloc(len+2*RPMB_DATA_SIZE);
       int positivePos;
	uint16_t startblk_index=0;
	uint16_t offset_in_startblk=0;
	int restLen,restBlkCount,totalBlkCount;

//get the byte offset position in positive sequence
       if (whence==SEEK_SET)
	   	positivePos=pos;
	else
	{

	       positivePos=SIZE_RPMB-1+pos;
	}
//get the start block number and offset in this block
       startblk_index=positivePos/RPMB_DATA_SIZE;
	offset_in_startblk=positivePos-startblk_index*RPMB_DATA_SIZE;

//get the rest bytes len except the bytes in the first block
	restLen=len-(RPMB_DATA_SIZE-offset_in_startblk);

	if (restLen>0)
	{
	     restBlkCount=restLen/RPMB_DATA_SIZE;
	     if (restLen%RPMB_DATA_SIZE>0)
		 	restBlkCount+=1;

	}
	else
		restBlkCount=0;

       totalBlkCount=1+restBlkCount;// "1" means the startblk

	char *blkData=malloc(totalBlkCount*RPMB_DATA_SIZE);
#if 0////rpmb_blk_read several blocks one time
       if (0==rpmb_blk_read(blkData,startblk_index,totalBlkCount))
	{
          memcpy(buf,blkData+offset_in_startblk,len);
	   free(blkData);
	   blkData=NULL;
	   return ERROR_RPMB_SUCCESS;
       }
	 else
	 {
	   free(blkData);
	   blkData=NULL;
	   return ERROR_RPMB_FAILED;
	 }
#else//rpmb_blk_read 1 block one time
       int index=0;
struct timeval tvs,tve;
       while (index<totalBlkCount)
       {
               gettimeofday(&tvs,NULL);
           if (1==rpmb_blk_read(blkData+index*RPMB_DATA_SIZE,startblk_index+index,1))
	    {
	        free(blkData);
	        blkData=NULL;
	        return ERROR_RPMB_FAILED;
	    }
		      gettimeofday(&tve,NULL);
        double span = tve.tv_sec-tvs.tv_sec + (tve.tv_usec-tvs.tv_usec)/1000000.0;

ALOGD("read block%d,spend time=%.12f Seconds\n",startblk_index+index,span);
	    index+=1;
       }
       memcpy(buf,blkData+offset_in_startblk,len);
	free(blkData);
	blkData=NULL;
	return ERROR_RPMB_SUCCESS;
#endif
#endif

}

static int write_rpmb_pos(int fd, int pos, int whence, byte * buf, int len)
{
#if 0 //def TEST_FILE
	lseek(fd, pos, whence);
	write(fd, buf, len);
#else
	uint8_t ret=1;//0 means succeed;1 failed
	int positivePos;
       uint16_t startblk_index=0;
	uint16_t offset_in_startblk=0;
	char blkdata[257]={0};
	uint16_t offset_inbuf=0;
//ALOGE(" write_rpmb_pos() pos=%d,len=%d,whence=%d\n",pos,len,whence);
//get the byte offset position in positive sequence
       if (whence==SEEK_SET)
	   	positivePos=pos;
	else
	{
	       positivePos=SIZE_RPMB-1+pos;
	}
//get the start block number and offset in this block
       startblk_index=positivePos/RPMB_DATA_SIZE;
	offset_in_startblk=positivePos-startblk_index*RPMB_DATA_SIZE;

       //write the data of buffer from head if not a whole block
	if (offset_in_startblk!=0)
	{

	        ret=rpmb_blk_read(blkdata,startblk_index,1);
               if (len<=(RPMB_DATA_SIZE-offset_in_startblk))
		      offset_inbuf=len;
		  else
		  	offset_inbuf=RPMB_DATA_SIZE-offset_in_startblk;

		 memcpy(blkdata+offset_in_startblk,buf,offset_inbuf);
		 ret=rpmb_blk_write(blkdata,startblk_index);
		 startblk_index+=1;

	}
	//write next whole block
	while ((len-offset_inbuf)>=RPMB_DATA_SIZE)
	{
	        ret=rpmb_blk_write((char *)(buf+offset_inbuf),startblk_index);
		 offset_inbuf+=RPMB_DATA_SIZE;
		 startblk_index+=1;
	}
       //write the data of buffer at tail if not a whole block
       if (offset_inbuf<len)
       {
               ret=rpmb_blk_read(blkdata,startblk_index,1);
		 memcpy(blkdata,buf+offset_inbuf,len-offset_inbuf);
		 ret=rpmb_blk_write(blkdata,startblk_index);
       }
#endif
	return ERROR_RPMB_SUCCESS;
}

static keymap *search_keymap(byte * keyname_ex)
{
	keymap *keyptr = NULL;
	int listnum = 0;

	while (listnum < 2) {
		if (listnum == 0)
			keyptr = aes_key_list;
		else
			keyptr = rsa_key_list;

		while (keyptr != NULL) {
			if (keyptr->name[0] != rpmb_emptybyte) {
				if ((strlen((char *)(keyptr->name)) ==
				     strlen((char *)keyname_ex))
				    &&
				    (memcmp
				     (keyptr->name, keyname_ex,
				      strlen((char *)keyname_ex)) == 0))
					return keyptr;
			}
			keyptr = keyptr->next;
		}
		listnum += 1;
	}

	return NULL;
}

int rpmb_initialize(void)
{
	int aes_total_num = 0;
	int rsa_total_num = 0;
	int index = 0;
	byte keyname[RPMB_KEYNAME_LEN];
	keymap *temp_key = NULL;
	keymap *current_key = NULL;
	int i = 0;
	if (is_initialized) {
		return ERROR_RPMB_SUCCESS;
	}
#ifdef TEST_FILE
	fd_rpmbdev = open("/fake_rpmb.txt", O_RDWR);
	if (fd_rpmbdev < 0) {
		ALOGE
		    ("rpmb_initialize() fake_rpmb.txt not found ,will create new one!\n");
		byte fillbuf[128];
		memset(fillbuf, rpmb_emptybyte, 128);
#ifdef KERNEL_TEST
		fd_rpmbdev = open("/fake_rpmb.txt", O_CREAT | O_RDWR, 666);	// S_IRUSR | S_IWUSR);
#else
		fd_rpmbdev = open("/data/data/fake_rpmb.txt", O_CREAT | O_RDWR, 666);	// S_IRUSR | S_IWUSR);
#endif
		if (fd_rpmbdev < 0) {
			ALOGE("rpmb_initialize() create fake_rpmb.txt failed!\n");
			return ERROR_RPMB_FAILED;
		} else
			ALOGE
			    ("rpmb_initialize() create fake_rpmb.txt succeed!\n");

		lseek(fd_rpmbdev, SIZE_RPMB - 1, SEEK_SET);
		write(fd_rpmbdev, "1", 1);
//initialize the full file with rpmb_emptybyte
		lseek(fd_rpmbdev, 0, SEEK_SET);
		int j = SIZE_RPMB / 128;
		while (j > 0) {
			write(fd_rpmbdev, fillbuf, sizeof(fillbuf));
			j -= 1;
		}
	}
#else
       rpmb_get_size(&sizeRPMB,&blksizeRPMB);
	fd_rpmbdev = open(rpmb_dev_path, O_RDWR);
	ALOGE("rpmb_initialize() SIZE_RPMB=%d,RPMB_DATA_SIZE=%d!",SIZE_RPMB,RPMB_DATA_SIZE);

#endif
	if (fd_rpmbdev < 0)
		ALOGE("open rpmb device failed!!\n");

	read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
		      (byte *) & aes_total_num, 4);
	read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4, SEEK_SET,
		      (byte *) & rsa_total_num, 4);

	while (index < aes_total_num)	//initialize aes_key_list
	{
		temp_key = (keymap *) malloc(sizeof(keymap));
		read_rpmb_pos(fd_rpmbdev, RPMB_AES_START + RPMB_AES_LEN * i,
			      SEEK_SET, (byte *) keyname, RPMB_KEYNAME_LEN);
		if (keyname[0] != rpmb_emptybyte)	//is valid key
		{
			index += 1;
		}
		temp_key->id = i;
		memcpy(temp_key->name, keyname, RPMB_KEYNAME_LEN);
		temp_key->prev = NULL;
		temp_key->next = NULL;
		temp_key->type = KEYTYPE_AES;

		if (aes_key_list == NULL)
			aes_key_list = temp_key;
		else {
			current_key->next = temp_key;
			temp_key->prev = current_key;
		}
		current_key = temp_key;
		i += 1;
	}
	aes_key_list_len = i;
	index = 0;
	i = 0;
	while (index < rsa_total_num)	//initialize rsa_key_list
	{
		temp_key = (keymap *) malloc(sizeof(keymap));
		read_rpmb_pos(fd_rpmbdev, ~(RPMB_RSA_LEN * (i + 1)) + 1,
			      SEEK_END, (byte *) keyname, RPMB_KEYNAME_LEN);
		if (keyname[0] != rpmb_emptybyte)	//is valid key
		{
			index += 1;
		}
		temp_key->id = i;
		memcpy(temp_key->name, keyname, RPMB_KEYNAME_LEN);
		temp_key->prev = NULL;
		temp_key->next = NULL;
		temp_key->type = KEYTYPE_RSA;

		if (rsa_key_list == NULL)
			rsa_key_list = temp_key;
		else {
			current_key->next = temp_key;
			temp_key->prev = current_key;
		}
		current_key = temp_key;
		i += 1;
	}
	rsa_key_list_len = i;
	is_initialized = 1;
	return ERROR_RPMB_SUCCESS;

}

static byte *getkey_flat(keymap * kmap)
{
	byte *tkskey_flat = NULL;

	if (kmap->type == KEYTYPE_AES) {
		tkskey_flat = (byte *) malloc(RPMB_AES_LEN);
		if (ERROR_RPMB_FAILED==read_rpmb_pos(fd_rpmbdev,
			      RPMB_AES_START + RPMB_AES_LEN * (kmap->id),
			      SEEK_SET, (byte *) tkskey_flat, RPMB_AES_LEN))
	       {
                           free(tkskey_flat);
			       tkskey_flat=NULL;
				return NULL;
		}
	} else {
		tkskey_flat = (byte *) malloc(RPMB_RSA_LEN);
		if (ERROR_RPMB_FAILED==read_rpmb_pos(fd_rpmbdev, ~(RPMB_RSA_LEN * (kmap->id + 1)) + 1,
			      SEEK_END, (byte *) tkskey_flat, RPMB_RSA_LEN))
	       {
                           free(tkskey_flat);
			       tkskey_flat=NULL;
				return NULL;
		}
	}

	return tkskey_flat;
}

static int reconstruct_keyname(int keytypeuser, byte * keyname_ex,
			       char *keyname)
{
	if (strlen(keyname) > (RPMB_KEYNAME_LEN - 3)) {
		ALOGE("rpmb_del_key error: keyname length error\n");
		return ERROR_RPMB_FAILED;
	}
	memcpy(keyname_ex, keyname, strlen(keyname));
	if (keytypeuser == KEYTYPE_KEY) {
		keyname_ex[strlen(keyname)] = '.';
		keyname_ex[strlen(keyname) + 1] = 'k';
	} else if (keytypeuser == KEYTYPE_USERPASS) {
		keyname_ex[strlen(keyname)] = '.';
		keyname_ex[strlen(keyname) + 1] = 'u';
	} else {
		ALOGE("reconstruct_keyname() error: keytypeuser %d error\n",
		      keytypeuser);
		return ERROR_RPMB_FAILED;
	}
	keyname_ex[strlen(keyname) + 2] = '\0';
	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
读取某个密钥（名称为keyname的）对应的userpass
参数定义：
[in] char* keyname:密钥名称
[out] tks_key upserpass：用于返回keyname的userpass
返 回 值：
return:
0:succeed
1:failed
*/
int rpmb_read_userpass(char *keyname, tks_key * userpass)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_read_userpass()>>> rpmb_initialize() failed! \n");

//construct userpass name *.u

	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_USERPASS, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;

	keymap *key = search_keymap(keyname_ex);
	if (NULL == key) {
		ALOGE("rpmb_read_userpass() error: %s not found\n", keyname_ex);
		return ERROR_RPMB_FAILED;
	}

	byte *tkskey_flat = getkey_flat(key);
       if (NULL!=tkskey_flat)
       {
	userpass->key_type = *(int *)(tkskey_flat + RPMB_KEYNAME_LEN);

	userpass->keylen = *(int *)(tkskey_flat + RPMB_KEYNAME_LEN + 4);

	memcpy(userpass->keydata, tkskey_flat + RPMB_KEYNAME_LEN + 4 + 4,
	       userpass->keylen);
	free(tkskey_flat);
	tkskey_flat = NULL;

	return ERROR_RPMB_SUCCESS;
       }
	else
	return ERROR_RPMB_FAILED;
}

/*
功    能：
读取某个密钥
参数定义：
[in] char* keyname:密钥名称
[out] tks_key key:用于返回keyname的密钥内容
return:
0:succeed
1:failed
*/
int rpmb_read_key(char *keyname, tks_key * key)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_read_key() >>> rpmb_initialize() failed! \n");
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL == kmap) {
		ALOGE("rpmb_read_key() error: %s not found\n", keyname_ex);
		return ERROR_RPMB_FAILED;
	}

	byte *tkskey_flat = getkey_flat(kmap);

	int tks_key_offset = 0;
       if (NULL!=tkskey_flat)
       {
	tks_key_offset = RPMB_KEYNAME_LEN + RPMB_OPT_LEN + RPMB_KEYINFO_LEN;
	key->key_type = *(int *)(tkskey_flat + tks_key_offset);
	key->keylen = *(int *)(tkskey_flat + tks_key_offset + 4);
	memcpy(key->keydata, tkskey_flat + tks_key_offset + 4 + 4, key->keylen);

	free(tkskey_flat);
	tkskey_flat = NULL;

	return ERROR_RPMB_SUCCESS;
       }
	else
	    return ERROR_RPMB_FAILED;
}

/*
功    能：
读取某个密钥的信息
参数定义：
[in] char* keyname:要读取的密钥的名称
[out] key_info *keyinfo:用于返回keyname的密钥信息
return:
0:succeed
1:failed
*/
int rpmb_read_keyinfo(char *keyname, key_info * keyinfo)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_read_keyinfo() >>> rpmb_initialize() failed! \n");
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL == kmap) {
		ALOGE("rpmb_read_keyinfo() error: %s not found\n", keyname_ex);
		return ERROR_RPMB_FAILED;
	}

	byte *tkskey_flat = getkey_flat(kmap);
       if (NULL!=tkskey_flat)
       {
	int tks_key_offset = RPMB_KEYNAME_LEN + RPMB_OPT_LEN;

	long time_long = *(long *)(tkskey_flat + tks_key_offset);
	struct tm *temp_tm = localtime(&time_long);
	memcpy(&(keyinfo->timestored), temp_tm, sizeof(struct tm));

	time_long = *(long *)(tkskey_flat + tks_key_offset + 8);
	temp_tm = localtime(&time_long);
	memcpy(&(keyinfo->timeaccessed), temp_tm, sizeof(struct tm));

	time_long = *(long *)(tkskey_flat + tks_key_offset + 16);
	temp_tm = localtime(&time_long);
	memcpy(&(keyinfo->timefailed), temp_tm, sizeof(struct tm));

	keyinfo->timesfailed = *(int *)(tkskey_flat + tks_key_offset + 24);

	keyinfo->flags = *(int *)(tkskey_flat + tks_key_offset + 28);
       free(tkskey_flat);
	   tkskey_flat=NULL;

	return ERROR_RPMB_SUCCESS;
       }
	   else
	   	return ERROR_RPMB_FAILED;
}

/*
功    能:
读取某个key的控制选项参数
参数定义：
[in] char *keyname:要读取的key的名字
[out] opt *option;用于返回控制选项参数
return:
0:succeed
1:failed
*/
int rpmb_read_opt(char *keyname, opt * option)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_read_opt() >>> rpmb_initialize() failed! \n");
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL == kmap) {
		ALOGE("rpmb_read_opt() error: %s not found\n", keyname_ex);
		return ERROR_RPMB_FAILED;
	}
	byte *tkskey_flat = getkey_flat(kmap);

	int tks_key_offset = RPMB_KEYNAME_LEN;
       if (NULL!=tkskey_flat)
       {
	option->flags = *(uint16_t *) (tkskey_flat + tks_key_offset);

	option->locklimit = *(int *)(tkskey_flat + tks_key_offset + 2);

	long time_long = *(long *)(tkskey_flat + tks_key_offset + 2 + 4);
	struct tm *temp_tm = localtime(&time_long);
	memcpy(&(option->timelimit), temp_tm, sizeof(struct tm));
       free(tkskey_flat);
	   tkskey_flat=NULL;
	return ERROR_RPMB_SUCCESS;
       }
	   else
	   	return ERROR_RPMB_FAILED;
}

/*
功    能：
读取安全系统信息
参数定义：
[out] sys_info* sysinfo:用于返回系统信息
return:
0:succeed
1:failed
*/
int rpmb_read_sysinfo(sys_info * sysinfo)
{
	byte *sysinfo_flat = (byte *) malloc(RPMB_SYSINFO_LEN);

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_read_sysinfo() >>> rpmb_initialize() failed! \n");

	read_rpmb_pos(fd_rpmbdev, 0, SEEK_SET, (byte *) sysinfo_flat,
		      RPMB_SYSINFO_LEN);

	long time_long = *(long *)(sysinfo_flat);
	struct tm *temp_tm = localtime(&time_long);
	memcpy(&(sysinfo->timeinit), temp_tm, sizeof(struct tm));

	time_long = *(long *)(sysinfo_flat + 8);
	temp_tm = localtime(&time_long);
	memcpy(&(sysinfo->timeaccessed), temp_tm, sizeof(struct tm));

	time_long = *(long *)(sysinfo_flat + 16);
	temp_tm = localtime(&time_long);
	memcpy(&(sysinfo->timefailed), temp_tm, sizeof(struct tm));

	sysinfo->timesfailed = *(int *)(sysinfo_flat + 24);

	sysinfo->codefailed = *(int *)(sysinfo_flat + 28);

	if (!(*(byte *) (sysinfo_flat + 32) == rpmb_emptybyte))
		memcpy(sysinfo->keyname, sysinfo_flat + 32, 30);

	byte *devcert = sysinfo_flat + 32 + 30;

	sysinfo->devcert.pubkey.exp = *(int *)devcert;
	memcpy(sysinfo->devcert.pubkey.mod, devcert + 4, 256);

	memcpy(sysinfo->devcert.devid, devcert + 4 + 256, 15);

	memcpy(sysinfo->devcert.signature, devcert + 4 + 256 + 15, 256);

	return ERROR_RPMB_SUCCESS;
}

static int clearkey(keymap * kmap)
{
	byte *tkskey_flat = NULL;
	int total_num = 0;
	if (kmap->type == KEYTYPE_AES) {
		//clear the aes key area in rpmb
		tkskey_flat = (byte *) malloc(RPMB_AES_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_AES_LEN);
		write_rpmb_pos(fd_rpmbdev,
			       RPMB_AES_START + RPMB_AES_LEN * (kmap->id),
			       SEEK_SET, (byte *) tkskey_flat, RPMB_AES_LEN);
		//update the total num of aes key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
			      (byte *) & total_num, 4);
		total_num -= 1;
		write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
			       (byte *) & total_num, 4);

	} else {
		//clear the rsa key area in rpmb
		tkskey_flat = (byte *) malloc(RPMB_RSA_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_RSA_LEN);
		write_rpmb_pos(fd_rpmbdev, ~(RPMB_RSA_LEN * (kmap->id + 1)) + 1,
			       SEEK_END, (byte *) tkskey_flat, RPMB_RSA_LEN);
		//update the total num of rsa key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4, SEEK_SET,
			      (byte *) & total_num, 4);
		total_num -= 1;
		write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4, SEEK_SET,
			       (byte *) & total_num, 4);
	}
	free(tkskey_flat);
	tkskey_flat = NULL;

	if (kmap->next == NULL)	//the last one of list
	{

		if (kmap == aes_key_list)	//the only one key in aes_key_list
		{
			free(aes_key_list);
			aes_key_list = NULL;
			aes_key_list_len = 0;
		} else if (kmap == rsa_key_list)	//the only one key in rsa_key_list
		{
			free(rsa_key_list);
			rsa_key_list = NULL;
			rsa_key_list_len = 0;
		} else {
			keymap *currentkey;
///////
			if (kmap->type == KEYTYPE_AES) {
				while (1)	//free all the null key next to
				{
					currentkey = kmap;
					kmap = currentkey->prev;
					if (kmap != NULL)
						kmap->next = NULL;
					free(currentkey);
					currentkey = NULL;
					aes_key_list_len -= 1;
					if ((kmap == NULL)
					    || (kmap->name[0] !=
						rpmb_emptybyte))
						break;

				}
				if (aes_key_list_len == 0)
					aes_key_list = NULL;

			} else {
				while (1) {
					currentkey = kmap;
					kmap = currentkey->prev;
					if (kmap != NULL)
						kmap->next = NULL;
					free(currentkey);
					currentkey = NULL;
					rsa_key_list_len -= 1;
					if ((kmap == NULL)
					    || (kmap->name[0] !=
						rpmb_emptybyte))
						break;
				}
				if (rsa_key_list_len == 0)
					rsa_key_list = NULL;
			}
		}
	} else
		memset(kmap->name, rpmb_emptybyte, RPMB_KEYNAME_LEN);

	return ERROR_RPMB_SUCCESS;
}

static keymap *search_keymap_null(int keytype_user)
{
	keymap *currentkey = NULL;
	keymap *keylist = NULL;
	if (keytype_user == KEYTYPE_AES)
		keylist = aes_key_list;
	else
		keylist = rsa_key_list;

	currentkey = keylist;

	while (currentkey != NULL) {
		if (currentkey->name[0] == rpmb_emptybyte)
			break;
		else
			currentkey = currentkey->next;
	}

	if (currentkey != NULL) {
		return currentkey;
	} else {

		int aes_num, rsa_num;
		if (keytype_user == KEYTYPE_AES) {
			aes_num = aes_key_list_len + 1;
			rsa_num = rsa_key_list_len;
		} else {
			aes_num = aes_key_list_len;
			rsa_num = rsa_key_list_len + 1;
		}

		if ((aes_num * RPMB_AES_LEN + rsa_num * RPMB_RSA_LEN) <= SIZE_KEYAREA)	//RPMB still have empty space to store new key
		{
			currentkey = (keymap *) malloc(sizeof(keymap));
			currentkey->next = NULL;
			memset(currentkey->name, rpmb_emptybyte,
			       RPMB_KEYNAME_LEN);
			if (keytype_user == KEYTYPE_AES) {
				currentkey->id = aes_key_list_len;
				currentkey->type = KEYTYPE_AES;
//add new key map to list and modify the list len
				aes_key_list_len += 1;
				if (aes_key_list == NULL)
					aes_key_list = currentkey;
				else {
					keymap *tempkey = aes_key_list;
					while (tempkey->next != NULL)
						tempkey = tempkey->next;
					tempkey->next = currentkey;
					currentkey->prev = tempkey;
				}

			} else {
				currentkey->id = rsa_key_list_len;
				currentkey->type = KEYTYPE_RSA;
//add new key map to list and modify the list len
				rsa_key_list_len += 1;
				if (rsa_key_list == NULL)
					rsa_key_list = currentkey;
				else {
					keymap *tempkey = rsa_key_list;
					while (tempkey->next != NULL)
						tempkey = tempkey->next;
					tempkey->next = currentkey;
					currentkey->prev = tempkey;
				}
			}
			return currentkey;

		} else
			return NULL;

	}
	ALOGE("search_keymap_null() error: should never reach here!\n");
	return NULL;

}

static int write_userpass(keymap * kmap, byte * keyname_ex, tks_key userpass)
{
	byte *tkskey_flat = NULL;
	int total_num = 0;
	if (kmap->type == KEYTYPE_AES) {
		tkskey_flat = (byte *) malloc(RPMB_AES_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_AES_LEN);
	} else {
		tkskey_flat = (byte *) malloc(RPMB_RSA_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_RSA_LEN);
	}

	memcpy(tkskey_flat, keyname_ex, strlen((char *)keyname_ex) + 1);
	memcpy(tkskey_flat + RPMB_KEYNAME_LEN, (byte *) & (userpass.key_type),
	       4);
	memcpy(tkskey_flat + RPMB_KEYNAME_LEN + 4, (byte *) & (userpass.keylen),
	       4);
	memcpy(tkskey_flat + RPMB_KEYNAME_LEN + 4 + 4, userpass.keydata,
	       userpass.keylen);

	if (kmap->type == KEYTYPE_AES) {
		write_rpmb_pos(fd_rpmbdev,
			       RPMB_AES_START + RPMB_AES_LEN * (kmap->id),
			       SEEK_SET, (byte *) tkskey_flat, RPMB_AES_LEN);

		//update the total num of aes key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
			      (byte *) & total_num, 4);
		if (kmap->name[0] == rpmb_emptybyte)	//if (name_exist[0]==rpmb_emptybyte)//new key space need to update the number;otherwise is to overwrite a key space which is already have the named key,so no need to update the number
		{
			if ((*((byte *) & total_num) == rpmb_emptybyte) && (*(((byte *) & total_num) + 1) == rpmb_emptybyte) && (*(((byte *) & total_num) + 2) == rpmb_emptybyte) && (*(((byte *) & total_num) + 3) == rpmb_emptybyte))	//still not initialize
				total_num = 0;
			total_num += 1;
			write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
				       (byte *) & total_num, 4);
		}
	} else {
		write_rpmb_pos(fd_rpmbdev, ~(RPMB_RSA_LEN * (kmap->id + 1)) + 1,
			       SEEK_END, (byte *) tkskey_flat, RPMB_RSA_LEN);

		//update the total num of rsa key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4, SEEK_SET,
			      (byte *) & total_num, 4);
		if (kmap->name[0] == rpmb_emptybyte)	//if (name_exist[0]==rpmb_emptybyte)//new key space need to update the number;otherwise is to overwrite a key space which is already have the named key,so no need to update the number
		{
			if ((*((byte *) & total_num) == rpmb_emptybyte) && (*(((byte *) & total_num) + 1) == rpmb_emptybyte) && (*(((byte *) & total_num) + 2) == rpmb_emptybyte) && (*(((byte *) & total_num) + 3) == rpmb_emptybyte))	//still not initialize
				total_num = 0;
			total_num += 1;
			write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4,
				       SEEK_SET, (byte *) & total_num, 4);
		}
	}
	free(tkskey_flat);
	tkskey_flat = NULL;
	memcpy(kmap->name, keyname_ex, RPMB_KEYNAME_LEN);

	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
写某个密钥（名称为keyname）对应的userpass，如果已经存在则覆盖旧的
参数定义：
[in] char* keyname:密钥名称
[in] tks_key：要保存的userpass
return:
0:succeed
1:failed
*/
int rpmb_write_userpass(char *keyname, tks_key userpass)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_write_userpass() >>> rpmb_initialize() failed! \n");
//construct userpass name *.u
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_USERPASS, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL != kmap)	//the named userpass already exist
	{
		if ((kmap->type == KEYTYPE_AES) && ((userpass.key_type == KEY_AES_128) || (userpass.key_type == KEY_AES_256)))	//key type is the same with old:aes
		{

			write_userpass(kmap, keyname_ex, userpass);
			return ERROR_RPMB_SUCCESS;
		} else if ((kmap->type == KEYTYPE_RSA) && ((userpass.key_type == KEY_RSA_1024_PUB) || (userpass.key_type == KEY_RSA_1024_PRV) || (userpass.key_type == KEY_RSA_2048_PUB) || (userpass.key_type == KEY_RSA_2048_PRV)))	//key type is the same with old:rsa
		{
			write_userpass(kmap, keyname_ex, userpass);
			return ERROR_RPMB_SUCCESS;
		} else
			clearkey(kmap);
	}

	if ((userpass.key_type == KEY_AES_128)
	    || (userpass.key_type == KEY_AES_256))
		kmap = search_keymap_null(KEYTYPE_AES);
	else if ((userpass.key_type == KEY_RSA_1024_PUB)
		 || (userpass.key_type == KEY_RSA_1024_PRV)
		 || (userpass.key_type == KEY_RSA_2048_PUB)
		 || (userpass.key_type == KEY_RSA_2048_PRV))
		kmap = search_keymap_null(KEYTYPE_RSA);
	else {
		ALOGE("rpmb_write_userpass() error: userpass type %d error!\n",
		      userpass.key_type);
		return ERROR_RPMB_FAILED;
	}
	if (NULL == kmap) {
		ALOGE("rpmb_write_userpass() error: find NULL keymap error!\n");
		return ERROR_RPMB_FAILED;
	} else {
		write_userpass(kmap, keyname_ex, userpass);
	}
	return ERROR_RPMB_SUCCESS;
}

static int write_key(keymap * kmap, byte * keyname_ex, rpmb_keycontainer key)
{
	byte *tkskey_flat = NULL;
	int offset = 0;
	int total_num = 0;

	if (kmap->type == KEYTYPE_AES) {
		tkskey_flat = (byte *) malloc(RPMB_AES_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_AES_LEN);
	} else {
		tkskey_flat = (byte *) malloc(RPMB_RSA_LEN);
		memset(tkskey_flat, rpmb_emptybyte, RPMB_RSA_LEN);
	}

//begin flat the data in rpmb_keycontainer
	memcpy(tkskey_flat, keyname_ex, strlen((char *)keyname_ex) + 1);	//name
	offset += RPMB_KEYNAME_LEN;

	memcpy(tkskey_flat + offset, (byte *) & (key.option.flags), 2);	//opt flags
	offset += 2;

	memcpy(tkskey_flat + offset, (byte *) & (key.option.locklimit), 4);	//opt locklimit
	offset += 4;

	long tmlong = mktime(&(key.option.timelimit));
	memcpy(tkskey_flat + offset, (byte *) & tmlong, sizeof(long));	//opt timelimit
	offset += 8;

	tmlong = mktime(&(key.keyinfo.timestored));
	memcpy(tkskey_flat + offset, (byte *) & tmlong, sizeof(long));	//keyinfo.timestored
	offset += 8;

	tmlong = mktime(&(key.keyinfo.timeaccessed));
	memcpy(tkskey_flat + offset, (byte *) & tmlong, sizeof(long));	//keyinfo.timeaccessed
	offset += 8;

	tmlong = mktime(&(key.keyinfo.timefailed));
	memcpy(tkskey_flat + offset, (byte *) & tmlong, sizeof(long));	//keyinfo.timefailed
	offset += 8;

	memcpy(tkskey_flat + offset, (byte *) & (key.keyinfo.timesfailed), 4);	//keyinfo.timesfailed
	offset += 4;

	memcpy(tkskey_flat + offset, (byte *) & (key.keyinfo.flags), 4);	//keyinfo.flags
	offset += 4;

	memcpy(tkskey_flat + offset, (byte *) & (key.key.key_type), 4);	//tks_key type
	offset += 4;

	memcpy(tkskey_flat + offset, (byte *) & (key.key.keylen), 4);	//tks_key keylen
	offset += 4;

	memcpy(tkskey_flat + offset, key.key.keydata, key.key.keylen);	//tks_key keydata
//end flat the data in rpmb_keycontainer

	if (kmap->type == KEYTYPE_AES) {
		//byte name_exist[RPMB_KEYNAME_LEN];
		//read_rpmb_pos(fd_rpmbdev,RPMB_AES_START+RPMB_AES_LEN*(kmap->id),SEEK_SET,(byte *)name_exist,RPMB_KEYNAME_LEN);

		write_rpmb_pos(fd_rpmbdev,
			       RPMB_AES_START + RPMB_AES_LEN * (kmap->id),
			       SEEK_SET, (byte *) tkskey_flat, RPMB_AES_LEN);

		//update the total num of aes key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
			      (byte *) & total_num, 4);
		if (kmap->name[0] == rpmb_emptybyte)	//if (name_exist[0]==rpmb_emptybyte)//new key space need to update the number;otherwise is to overwrite a key space which is already have the named key,so no need to update the number
		{
			if ((*((byte *) & total_num) == rpmb_emptybyte) && (*(((byte *) & total_num) + 1) == rpmb_emptybyte) && (*(((byte *) & total_num) + 2) == rpmb_emptybyte) && (*(((byte *) & total_num) + 3) == rpmb_emptybyte))	//still not initialize
				total_num = 0;
			total_num += 1;
			write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN, SEEK_SET,
				       (byte *) & total_num, 4);
		}
	} else {
		write_rpmb_pos(fd_rpmbdev, ~(RPMB_RSA_LEN * (kmap->id + 1)) + 1,
			       SEEK_END, (byte *) tkskey_flat, RPMB_RSA_LEN);

		//update the total num of rsa key
		read_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4, SEEK_SET,
			      (byte *) & total_num, 4);
		if (kmap->name[0] == rpmb_emptybyte)	//if (name_exist[0]==rpmb_emptybyte)//new key space need to update the number;otherwise is to overwrite a key space which is already have the named key,so no need to update the number
		{
			if ((*((byte *) & total_num) == rpmb_emptybyte) && (*(((byte *) & total_num) + 1) == rpmb_emptybyte) && (*(((byte *) & total_num) + 2) == rpmb_emptybyte) && (*(((byte *) & total_num) + 3) == rpmb_emptybyte))	//still not initialize
				total_num = 0;
			total_num += 1;
			write_rpmb_pos(fd_rpmbdev, RPMB_SYSINFO_LEN + 4,
				       SEEK_SET, (byte *) & total_num, 4);
		}
	}
	free(tkskey_flat);
	tkskey_flat = NULL;
	memcpy(kmap->name, keyname_ex, RPMB_KEYNAME_LEN);

	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
新增某个密钥
参数定义：
[in] char *keyname:密钥名称
[in] rpmb_keycontainer key:密钥
return:
0:succeed
1:failed
*/
int rpmb_write_key(char *keyname, rpmb_keycontainer key)
{
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_write_key() >>> rpmb_initialize() failed! \n");
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL != kmap)	//the named userpass already exist
	{
		if ((kmap->type == KEYTYPE_AES) && ((key.key.key_type == KEY_AES_128) || (key.key.key_type == KEY_AES_256)))	//key type is the same with old:aes
		{

			write_key(kmap, keyname_ex, key);
			return ERROR_RPMB_SUCCESS;
		} else if ((kmap->type == KEYTYPE_RSA) && ((key.key.key_type == KEY_RSA_1024_PUB) || (key.key.key_type == KEY_RSA_1024_PRV) || (key.key.key_type == KEY_RSA_2048_PUB) || (key.key.key_type == KEY_RSA_2048_PRV)))	//key type is the same with old:rsa
		{
			write_key(kmap, keyname_ex, key);
			return ERROR_RPMB_SUCCESS;
		} else
			clearkey(kmap);
	}

	if ((key.key.key_type == KEY_AES_128)
	    || (key.key.key_type == KEY_AES_256))
		kmap = search_keymap_null(KEYTYPE_AES);
	else if ((key.key.key_type == KEY_RSA_1024_PUB)
		 || (key.key.key_type == KEY_RSA_1024_PRV)
		 || (key.key.key_type == KEY_RSA_2048_PUB)
		 || (key.key.key_type == KEY_RSA_2048_PRV))
		kmap = search_keymap_null(KEYTYPE_RSA);
	else {
		ALOGE("rpmb_write_key() error: key type %d error!\n",
		      key.key.key_type);
		return ERROR_RPMB_FAILED;
	}
	if (NULL == kmap) {
		ALOGE("rpmb_write_key() error: find NULL keymap error!\n");
		return ERROR_RPMB_FAILED;
	} else {
		write_key(kmap, keyname_ex, key);
	}
	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
更新密钥信息
参数定义：
[in] char* keyname:密钥名称
[in] key_info keyinfo:密钥信息
return:
0:succeed
1:failed
*/
int rpmb_write_keyinfo(char *keyname, key_info keyinfo)
{
	byte *keyinfo_flat = NULL;
	int offset = 0;
	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_write_keyinfo() >>> rpmb_initialize() failed! \n");
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);

	if (NULL == kmap)	//the named userpass already exist
	{
		ALOGE
		    ("rpmb_write_keyinfo() error: not found the named key %s!\n",
		     keyname_ex);
		return ERROR_RPMB_FAILED;
	}

	keyinfo_flat = (byte *) malloc(RPMB_KEYINFO_LEN);
	memset(keyinfo_flat, rpmb_emptybyte, RPMB_KEYINFO_LEN);
//begin flat the keyinfo
	long tmlong = mktime(&(keyinfo.timestored));
	memcpy(keyinfo_flat, (byte *) & tmlong, sizeof(long));	//keyinfo.timestored
	offset += 8;

	tmlong = mktime(&(keyinfo.timeaccessed));
	memcpy(keyinfo_flat + offset, (byte *) & tmlong, sizeof(long));	//keyinfo.timeaccessed
	offset += 8;

	tmlong = mktime(&(keyinfo.timefailed));
	memcpy(keyinfo_flat + offset, (byte *) & tmlong, sizeof(long));	//keyinfo.timefailed
	offset += 8;

	memcpy(keyinfo_flat + offset, (byte *) & (keyinfo.timesfailed), 4);	//keyinfo.timesfailed
	offset += 4;

	memcpy(keyinfo_flat + offset, (byte *) & (keyinfo.flags), 4);	//keyinfo.flags
//end flat the keyinfo

	offset = RPMB_KEYNAME_LEN + RPMB_OPT_LEN;

	if (kmap->type == KEYTYPE_AES) {
		write_rpmb_pos(fd_rpmbdev,
			       RPMB_AES_START + RPMB_AES_LEN * (kmap->id) +
			       offset, SEEK_SET, (byte *) keyinfo_flat,
			       RPMB_KEYINFO_LEN);
	} else {
		write_rpmb_pos(fd_rpmbdev,
			       ~(RPMB_RSA_LEN * (kmap->id + 1)) + 1 + offset,
			       SEEK_END, (byte *) keyinfo_flat,
			       RPMB_KEYINFO_LEN);
	}
	free(keyinfo_flat);
	keyinfo_flat = NULL;

	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
更新安全系统信息
return:
0:succeed
1:failed

*/
int rpmb_write_sysinfo(sys_info sysinfo)
{
	byte *sysinfo_flat = NULL;
	int offset = 0;

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_write_sysinfo() >>> rpmb_initialize() failed! \n");

	sysinfo_flat = (byte *) malloc(RPMB_SYSINFO_LEN);
	memset(sysinfo_flat, rpmb_emptybyte, RPMB_SYSINFO_LEN);

//begin flat the sysinfo
	long tmlong = mktime(&(sysinfo.timeinit));
	memcpy(sysinfo_flat, (byte *) & tmlong, sizeof(long));	//sysinfo.timestored
	offset += 8;

	tmlong = mktime(&(sysinfo.timeaccessed));
	memcpy(sysinfo_flat + offset, (byte *) & tmlong, sizeof(long));	//sysinfo.timeaccessed
	offset += 8;

	tmlong = mktime(&(sysinfo.timefailed));
	memcpy(sysinfo_flat + offset, (byte *) & tmlong, sizeof(long));	//sysinfo.timefailed
	offset += 8;

	memcpy(sysinfo_flat + offset, (byte *) & (sysinfo.timesfailed), 4);	//sysinfo.timesfailed
	offset += 4;

	memcpy(sysinfo_flat + offset, (byte *) & (sysinfo.codefailed), 4);	//sysinfo.codefailed
	offset += 4;

	memcpy(sysinfo_flat + offset, sysinfo.keyname, sizeof(sysinfo.keyname));	//sysinfo.keyname
	offset += 30;

	memcpy(sysinfo_flat + offset, (byte *) & (sysinfo.devcert.pubkey.exp), 4);	//sysinfo.devcert.pubkey.exp
	offset += 4;

	memcpy(sysinfo_flat + offset, sysinfo.devcert.pubkey.mod, 256);	//sysinfo.devcert.pubkey.mod
	offset += 256;

	memcpy(sysinfo_flat + offset, sysinfo.devcert.devid, 15);	//sysinfo.devcert.devid
	offset += 15;

	memcpy(sysinfo_flat + offset, sysinfo.devcert.signature, 256);	//sysinfo.devcert.signature

//end flat the sysinfo

	write_rpmb_pos(fd_rpmbdev, 0, SEEK_SET, (byte *) sysinfo_flat,
		       RPMB_SYSINFO_LEN);

	free(sysinfo_flat);
	sysinfo_flat = NULL;

	return ERROR_RPMB_SUCCESS;
}

/*
功    能：
删除某个密钥（名字为keyname）及其对应的userpass
return:
0:succeed
1:failed
*/
int rpmb_del_key(char *keyname)
{

	byte keyname_ex[RPMB_KEYNAME_LEN] = { rpmb_emptybyte };

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_del_key() >>> rpmb_initialize() failed! \n");
//construct userpass name *.u
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_USERPASS, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	keymap *kmap = search_keymap(keyname_ex);
	if (NULL == kmap) {
		ALOGD("rpmb_del_key() error: %s not found\n", keyname_ex);
//              return ERROR_RPMB_FAILED;
	} else {
		clearkey(kmap);
	}
///
//construct userpass name *.k
	if (ERROR_RPMB_FAILED ==
	    reconstruct_keyname(KEYTYPE_KEY, keyname_ex, keyname))
		return ERROR_RPMB_FAILED;
	kmap = search_keymap(keyname_ex);
	if (NULL == kmap) {
		ALOGD("rpmb_del_key() error: %s not found\n", keyname_ex);
//              return ERROR_RPMB_FAILED;
	} else {
		clearkey(kmap);
	}

	return ERROR_RPMB_SUCCESS;
}

int rpmb_getall_keyname(keyname_list_t * list, int *num)	//(char (*keynamelist)[RPMB_KEYNAME_LEN-2],int *num)
{
	//char *kname=NULL;
	//char (*keynamelist)[RPMB_KEYNAME_LEN-2];
	keymap *kmap = aes_key_list;
	int listnum = 0;
	*num = 0;
	int totalnum = aes_key_list_len + rsa_key_list_len;
	*list = NULL;

	if (ERROR_RPMB_SUCCESS != rpmb_initialize())
		ALOGE("rpmb_getall_keyname() >>> rpmb_initialize() failed! \n");

	if (totalnum > 0)
		*list = (keyname_list_t) malloc((RPMB_KEYNAME_LEN - 2) * totalnum);	//malloc for the max length
	else
		return ERROR_RPMB_SUCCESS;

	while (listnum < 2) {
		while (kmap != NULL) {
			if ((kmap->name[0] != rpmb_emptybyte) && (kmap->name[strlen((char *)(kmap->name)) - 1] == 'k'))	//valid key name
			{
				// kname=(char *)malloc(RPMB_KEYNAME_LEN-2);
				memset((*list)[*num], '\0',
				       RPMB_KEYNAME_LEN - 2);
				memcpy((*list)[*num], kmap->name,
				       strlen((char *)(kmap->name)) - 2);
				*num += 1;
			}
			kmap = kmap->next;
		}
		kmap = rsa_key_list;
		listnum += 1;
	}

	if (*num == 0) {
		free(*list);
		*list = NULL;
	}
	//*list=(long)keynamelist;
	//*list=keynamelist;

	return ERROR_RPMB_SUCCESS;
}

int rpmb_deinitialize(void)
{
	keymap *kmap = NULL;
	keymap *key_list = NULL;
	int listnum = 0;

	if (0 == is_initialized)
		ALOGE
		    ("rpmb_deinitialize() >>> still not initialized,so no need to deinitialize! \n");
	if (fd_rpmbdev > 0)
		close(fd_rpmbdev);
	key_list = aes_key_list;
	while (listnum < 2) {
		while (key_list != NULL) {
			kmap = key_list;
			key_list = key_list->next;
			free(kmap);
		}
		listnum += 1;
		if (listnum == 1)
			key_list = rsa_key_list;
	}
	aes_key_list = NULL;
	rsa_key_list = NULL;
	aes_key_list_len = 0;
	rsa_key_list_len = 0;
	is_initialized = 0;
	return ERROR_RPMB_SUCCESS;
}

/////////////////////////
/*
test case for read_rpmb_pos() and write_rpmb_pos()
*/

void clearRPMB( )
{
       byte fillbuf[RPMB_DATA_SIZE];
	 int i,totalblk;
	 	for (i=0;i<RPMB_DATA_SIZE;i++)
		fillbuf[i]=0;

//initialize the full file with rpmb_emptybyte
#ifdef TEST_FILE
		lseek(fd_rpmbdev, 0, SEEK_SET);
		i=0;
		totalblk=SIZE_RPMB/RPMB_DATA_SIZE;
		while (i++ <totalblk) {
			write(fd_rpmbdev, fillbuf, RPMB_DATA_SIZE);
		}
#else
 rpmb_get_size(&sizeRPMB,&blksizeRPMB);
   // rpmb_program_key( "0123456789abcdef0123456789abcdef", 32);

    for (i=0;i<SIZE_RPMB/RPMB_DATA_SIZE;i++)
         rpmb_blk_write(fillbuf,i);
#endif
}

void test_rw_rpmb_pos()
{
   int pos,pos1;
   byte datablk[256<<2];
   int  passed=1;


///clear rpmb to 1,2,3,....,0xff,1,2,3,...,0xff,...
       byte fillbuf[RPMB_DATA_SIZE];
	 int i;

      
	//fd_rpmbdev = open(rpmb_dev_path, O_RDWR);

	for (i=0;i<RPMB_DATA_SIZE;i++)
		fillbuf[i]=i;
         clearRPMB();
//test case 1:read in one block
         pos=10;
         memset(datablk,0,256<<2);
#if 1
         read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 4);

	  for (i=0;i<4;i++)
	  	if (datablk[i]!=fillbuf[pos+i])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case1 failed!datablk[%d]=%d,fillbuf[%d+%d]=%d \n",i,datablk[i],pos,i,fillbuf[pos+i]);
	  	}
        if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case1  passed \n");
//test case 2:read accross blocks
         passed=1;
         pos1=0;
         memset(datablk,0,256<<2);

         read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 256);

	  for (i=0;i<256;i++)
	  {
	       if (pos+i>=256)
		   	pos1=pos+i-256;
		 else
		 	pos1=pos+i;

	  	if (datablk[i]!=fillbuf[pos1])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case2 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],pos1,fillbuf[pos1]);
	  	}

	  }
	   if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case2  passed \n");

//test case 3:read across serveral whole blocks
         passed=1;
         pos1=0;
         memset(datablk,0,256<<2);
         read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 256<<1);

	  for (i=0;i<512;i++)
	  {
	       if (pos+i>=512)
		   	pos1=pos+i-512;
	       else
		 if (pos+i>=256)
		   	pos1=pos+i-256;
		 else
		 	pos1=pos+i;

	  	if (datablk[i]!=fillbuf[pos1])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case3 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],pos1,fillbuf[pos1]);
	  	}

	  }
	   if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case3  passed \n");


//test case 4:read in one block from tail
         passed=1;
         pos1=0;
         memset(datablk,0,256<<2);
         read_rpmb_pos(fd_rpmbdev, pos, SEEK_END,
		      datablk, 4);

	  for (i=0;i<4;i++)
	  {
	      pos1=255-pos+i;
	  	if (datablk[i]!=fillbuf[pos1])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case4 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],pos1,fillbuf[pos1]);
	  	}
	  }
        if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case4  passed \n");

//test case 5:read accross blocks from tail

         passed=1;
         pos1=0;
         memset(datablk,0,256<<2);
         read_rpmb_pos(fd_rpmbdev, 255+pos, SEEK_END,
		      datablk, 256);
	  pos=255-pos+1;

	  for (i=0;i<256;i++)
	  {
	       if (pos+i>=256)
		   	pos1=pos+i-256;
		 else
		 	pos1=pos+i;

	  	if (datablk[i]!=fillbuf[pos1])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case5 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],pos1,fillbuf[pos1]);
	  	}

	  }
	   if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case5  passed \n");
#endif
//test case 6:read across serveral whole blocks from tail
         passed=1;
         pos1=0;
	  pos=10;
         memset(datablk,0,256<<2);
         read_rpmb_pos(fd_rpmbdev, 511+pos, SEEK_END,
		      datablk, 256<<1);

	  pos=255-pos+1;

	  for (i=0;i<512;i++)
	  {
	       if (pos+i>=512)
		   	pos1=pos+i-512;
	       else
		 if (pos+i>=256)
		   	pos1=pos+i-256;
		 else
		 	pos1=pos+i;

	  	if (datablk[i]!=fillbuf[pos1])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case6 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],pos1,fillbuf[pos1]);
	  	}

	  }
	   if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case6  passed \n");


//test case 7:write in one block
         passed=1;
     	  pos=10;
         memset(datablk,0,256<<2);
            write_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		     fillbuf , 4);
	  read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 4);
	  for (i=0;i<4;i++)
	  	if (datablk[i]!=fillbuf[i])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case7 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i,fillbuf[i]);
	  	}
if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case7  passed \n");
//test case 8:write accross blocks
         passed=1;
     	  pos=10;
         memset(datablk,0,256<<2);
        write_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		     fillbuf , 256);
	  read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 256);
	  for (i=0;i<256;i++)
	  	if (datablk[i]!=fillbuf[i])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case8 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i,fillbuf[i]);
	  	}
         if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case8  passed \n");
//test case 9:write across serveral whole blocks
         passed=1;
     	  pos=10;

	   memcpy(datablk,fillbuf,256);
	   memcpy(datablk+256,fillbuf,256);
         //
        write_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		     datablk , 512);
        memset(datablk,0,256<<2);
	  read_rpmb_pos(fd_rpmbdev, pos, SEEK_SET,
		      datablk, 512);
	  for (i=0;i<512;i++)
	  	if (datablk[i]!=fillbuf[i%256])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case9 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i%256,fillbuf[i%256]);
	  	}
         if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case9  passed \n");

//test case 10:write in one block from tail
         passed=1;
     	  pos=10;
         memset(datablk,0,256<<2);
         write_rpmb_pos(fd_rpmbdev, pos, SEEK_END,
		     fillbuf , 4);
	  read_rpmb_pos(fd_rpmbdev, pos, SEEK_END,
		      datablk, 4);
	  for (i=0;i<4;i++)
	  	if (datablk[i]!=fillbuf[i])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case10 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i,fillbuf[i]);
	  	}
        if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case10  passed \n");

//test case 11:write accross blocks from tail
         passed=1;
     	  pos=10;
         memset(datablk,0,256<<2);
        write_rpmb_pos(fd_rpmbdev, 256+pos, SEEK_END,
		     fillbuf , 256);
	  read_rpmb_pos(fd_rpmbdev, 256+pos, SEEK_END,
		      datablk, 256);
	  for (i=0;i<256;i++)
	  	if (datablk[i]!=fillbuf[i])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case11 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i,fillbuf[i]);
	  	}
         if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case11  passed \n");

//test case 12:write across serveral whole blocks from tail
         passed=1;
     	  pos=10;

	   memcpy(datablk,fillbuf,256);
	   memcpy(datablk+256,fillbuf,256);
         //
        write_rpmb_pos(fd_rpmbdev, 512+pos, SEEK_END,
		     datablk , 512);
        memset(datablk,0,256<<2);
	  read_rpmb_pos(fd_rpmbdev, 512+pos, SEEK_END,
		      datablk, 512);
	  for (i=0;i<512;i++)
	  	if (datablk[i]!=fillbuf[i%256])
	  	{
	  	     passed=0;
		     ALOGE
		    ("test_rw_rpmb_pos() test case12 failed!datablk[%d]=%d,fillbuf[%d]=%d \n",i,datablk[i],i%256,fillbuf[i%256]);
	  	}
         if (passed==1)
                ALOGE
		    ("test_rw_rpmb_pos() test case12  passed \n");


}
///////////////rpmb.api.c end
