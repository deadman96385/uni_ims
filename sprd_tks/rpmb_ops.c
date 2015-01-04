#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <unistd.h>


#include <dirent.h>
#include <limits.h>

#include <sys/types.h>

#include <errno.h>
#include <utils/Log.h>

#include "tks_lib.h"


#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)
#define MMC_CMD_BCR	(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)		/* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

#define SEC_DRIVER_DEV  "/dev/sprdtks"
#define SEC_IOC_HMAC               _IOWR('S', 6, request_hmac)

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

uint8_t rpmb_program_key(uint8_t *key,uint8_t len);
struct mmc_ioc_cmd {
	/* Implies direction of data.  true = write, false = read */
	int write_flag;

	/* Application-specific command.  true = precede with CMD55 */
	int is_acmd;

	__u32 opcode;
	__u32 arg;
	__u32 response[4];  /* CMD response */
	unsigned int flags;
	unsigned int blksz;
	unsigned int blocks;

	/*
	 * Sleep at least postsleep_min_us useconds, and at most
	 * postsleep_max_us useconds *after* issuing command.  Needed for
	 * some read commands for which cards have no other way of indicating
	 * they're ready for the next command (i.e. there is no equivalent of
	 * a "busy" indicator for read operations).
	 */
	unsigned int postsleep_min_us;
	unsigned int postsleep_max_us;

	/*
	 * Override driver-computed timeouts.  Note the difference in units!
	 */
	unsigned int data_timeout_ns;
	unsigned int cmd_timeout_ms;

	/*
	 * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
	 * be 8-byte aligned.  Make sure this struct is the same size when
	 * built for 32-bit.
	 */
	__u32 __pad;

	/* DAT buffer */
	__u64 data_ptr;
};


#define MMC_BLOCK_MAJOR		179
#define rpmb_dev_path  "/dev/block/mmcblk0rpmb"

#define  rpmb_size_mult  "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/raw_rpmb_size_mult"

#define RPMB_MSG_TYPE_REQ_AUTH_KEY_PROGRAM          0x0001
#define RPMB_MSG_TYPE_REQ_WRITE_COUNTER_VAL_READ    0x0002
#define RPMB_MSG_TYPE_REQ_AUTH_DATA_WRITE           0x0003
#define RPMB_MSG_TYPE_REQ_AUTH_DATA_READ            0x0004
#define RPMB_MSG_TYPE_REQ_RESULT_READ               0x0005

#define RPMB_MSG_TYPE_RESP_AUTH_KEY_PROGRAM         0x0100
#define RPMB_MSG_TYPE_RESP_WRITE_COUNTER_VAL_READ   0x0200
#define RPMB_MSG_TYPE_RESP_AUTH_DATA_WRITE          0x0300
#define RPMB_MSG_TYPE_RESP_AUTH_DATA_READ           0x0400

#define RPMB_STUFF_DATA_SIZE                        196
#define RPMB_KEY_MAC_SIZE                           32
#define RPMB_DATA_SIZE                              256
#define RPMB_NONCE_SIZE                             16
#define RPMB_DATA_FRAME_SIZE                        512

#define RPMB_RESULT_OK                              0x00

struct rpmb_data_frame {
	uint8_t stuff_bytes[RPMB_STUFF_DATA_SIZE];
	uint8_t key_mac[RPMB_KEY_MAC_SIZE];
	uint8_t data[RPMB_DATA_SIZE];
	uint8_t nonce[RPMB_NONCE_SIZE];
	uint8_t write_counter[4];
	uint8_t address[2];
	uint8_t block_count[2];
	uint8_t op_result[2];
	uint8_t msg_type[2];
};


#define MMC_IOC_CMD _IOWR(MMC_BLOCK_MAJOR, 0, struct mmc_ioc_cmd)

uint32_t rpmb_read_writecount(void);

static void u16_to_bytes(uint16_t u16, uint8_t *bytes)
{
	*bytes = (uint8_t) (u16 >> 8);
	*(bytes + 1) = (uint8_t) u16;
}

static void bytes_to_u16(uint8_t *bytes, uint16_t *u16)
{
	*u16 = (uint16_t) ((*bytes << 8) + *(bytes + 1));
}

static void bytes_to_u32(uint8_t *bytes, uint32_t *u32)
{
	*u32 = (uint32_t) ((*(bytes) << 24) +
			   (*(bytes + 1) << 16) +
			   (*(bytes + 2) << 8) + (*(bytes + 3)));
}

static void u32_to_bytes(uint32_t u32, uint8_t *bytes)
{
	*bytes = (uint8_t) (u32 >> 24);
	*(bytes + 1) = (uint8_t) (u32 >> 16);
	*(bytes + 2) = (uint8_t) (u32 >> 8);
	*(bytes + 3) = (uint8_t) u32;
}

/*
 get the rpmb size info ;
 sizeRPMB:total bytes num of rpmb
 blksizeRPMB:bytes num in one block
 */
void  rpmb_get_size(int *sizeRPMB,int *blksizeRPMB)
{
   int count=0;
   FILE *fp;
   char buff[10];
   int i=0;
   char cmd[128]={0};

    sprintf(cmd,"cat %s",rpmb_size_mult);
    ALOGE( "cmd=%s",cmd);

    fp=popen(cmd,"r");
    if(fp==NULL)
    {
                ALOGE( "%s popen error",rpmb_size_mult);
                return;
    }
    fgets(buff,10,fp);

    sscanf(buff, "%i", &count);
    ALOGE( "count=%d",count);

    *blksizeRPMB=RPMB_DATA_SIZE;
    *sizeRPMB=count*1024*128;
}

/*
blk_data: for save read data;
blk_index: the block index for read;
block_count: the read count;
success return 0 ;
*/
uint8_t  rpmb_blk_read(char *blk_data,uint16_t blk_index,uint8_t block_count)
{
    struct mmc_ioc_cmd *rpmb_read_cmd;
    struct rpmb_data_frame *data_frame;
     struct rpmb_data_frame * resp_buf;
    uint16_t msg_type;
    uint16_t op_result;
    int fd=0;
	int ret=-1;

       if(blk_data == NULL) {
	   	ALOGE("rpmb_blk_read null \n");
		return 1;
       }
     fd= open(rpmb_dev_path, O_RDWR);
    if(fd < 0) {
        ALOGE(" open rpmb dev fail \n");
        return 1;
    }

      rpmb_read_cmd=(struct mmc_ioc_cmd *)malloc(sizeof(struct mmc_ioc_cmd ));

      /*for write rpmb key req */
    rpmb_read_cmd->write_flag=1;
    rpmb_read_cmd->opcode=25;
    rpmb_read_cmd->blksz=512;
    rpmb_read_cmd->blocks=1;
    rpmb_read_cmd->data_timeout_ns=1000000000;
    rpmb_read_cmd->flags=MMC_RSP_R1;
    msg_type=RPMB_MSG_TYPE_REQ_AUTH_DATA_READ;
    data_frame=(uint8_t *)malloc(RPMB_DATA_FRAME_SIZE);
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);

    u16_to_bytes(msg_type,data_frame->msg_type);
    u16_to_bytes(blk_index, data_frame->address);

    rpmb_read_cmd->data_ptr=data_frame;
    rpmb_read_cmd->is_acmd=0;
	ALOGE( " ioctl for read blk, blk_index=%d,block_count=%d \n",blk_index,block_count);

    // while (ret==-1 && i++<10)
	ret=ioctl(fd, MMC_IOC_CMD,rpmb_read_cmd);
     if (ret){
        ALOGE( " ioctl for read blk req fail =%d\n",ret);
	   close(fd);
	   free(data_frame);
        return 1;
    }


     rpmb_read_cmd->write_flag=0;
     rpmb_read_cmd->opcode=18;
      rpmb_read_cmd->blocks=block_count;
      resp_buf=(uint8_t *)malloc(RPMB_DATA_FRAME_SIZE*block_count);
     memset(resp_buf,0,RPMB_DATA_FRAME_SIZE*block_count);
     rpmb_read_cmd->data_ptr=resp_buf;
     if (ioctl(fd, MMC_IOC_CMD, rpmb_read_cmd)) {
	   close(fd);
	    free(data_frame);
        return 1;
    }


/*result check*/
	bytes_to_u16((resp_buf+block_count-1)->op_result,&op_result);
      bytes_to_u16((resp_buf+block_count-1)->msg_type,&msg_type);
	if((op_result==RPMB_RESULT_OK)&&(msg_type==RPMB_MSG_TYPE_RESP_AUTH_DATA_READ))
	{
	    uint8_t i=0;
               for(i=0;i<block_count;i++)
			   	memcpy((blk_data+i*RPMB_DATA_SIZE),((uint8_t *)(resp_buf+i)+RPMB_STUFF_DATA_SIZE+RPMB_KEY_MAC_SIZE),RPMB_DATA_SIZE);
  		  ALOGE("read  successed\n");
		   close(fd);
		   free(resp_buf);
		   free(data_frame);
		  return 0;
	}
	else
		ALOGE(" rpmb_blk_read() failed,result:0x%x ,msg_type:0x%x\n",op_result,msg_type);

	 free(resp_buf);
	free(data_frame);
       close(fd);
	return 1;   //should not run here;


}
/**
write_data: data for write
writecount: the rpmb write count;
blk_index: the block will be write to;
success return 0
*/
uint8_t rpmb_blk_write(char *write_data,uint16_t blk_index)
{
       struct mmc_ioc_cmd *rpmb_write_cmd;
    struct rpmb_data_frame *data_frame;
    uint16_t msg_type;
    uint16_t op_result;
    uint16_t block_count=1;
    int fd=0;
    uint32_t writecount=rpmb_read_writecount();
    int  key_len=32;
    int  mac_data_len;
	        ALOGE(" rpmb_blk_write() start,count=%d \n",writecount);
  //  rpmb_program_key( "0123456789abcdef0123456789abcdef", key_len);
//	sleep(60);
    if (write_data == NULL)// || (RPMB_DATA_SIZE != strlen(write_data)))
		return 1;

    fd= open(rpmb_dev_path, O_RDWR);
    if(fd < 0) {
        ALOGE(" open rpmb dev fail \n");
        return 1;
    }
    rpmb_write_cmd=(struct mmc_ioc_cmd *)malloc(sizeof(struct mmc_ioc_cmd ));

      /*for write rpmb key req */
    rpmb_write_cmd->write_flag=1;
    rpmb_write_cmd->opcode=25;
    rpmb_write_cmd->blksz=512;
    rpmb_write_cmd->blocks=1;
    rpmb_write_cmd->is_acmd=0;
     rpmb_write_cmd->data_timeout_ns=1000000000;
    rpmb_write_cmd->flags=MMC_RSP_R1;
    msg_type=RPMB_MSG_TYPE_REQ_AUTH_DATA_WRITE;
    data_frame=(uint8_t *)malloc(RPMB_DATA_FRAME_SIZE);
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);

    u16_to_bytes(msg_type,data_frame->msg_type);
    u16_to_bytes(blk_index, data_frame->address);
    u16_to_bytes(block_count, data_frame->block_count);
    u32_to_bytes(writecount,data_frame->write_counter);
   memcpy(data_frame->data,write_data,RPMB_DATA_SIZE);
	 /**/
	//for key_mac calc;
	 key_len=32;
	 mac_data_len=284;
     #if 1
    // 打开sec设备节点
    int sec_fd = 0;
    sec_fd = open(SEC_DRIVER_DEV, O_RDWR, 0);
    if (sec_fd < 0) {
        ALOGD("%s: open sec driver failed!\n", __FUNCTION__);
        return 1;
    }
    packdata mydata;
    uint8_t databuf[512];
    memcpy(databuf,data_frame->data,mac_data_len);
    mydata.len = mac_data_len;
    mydata.data = databuf;
    request_hmac myhmac;
    myhmac.message = mydata;

    //struct timeval tv1;
    //struct timeval tv2;
    //gettimeofday(&tv1, NULL);
    //printf("start: %d\t%d\n", tv1.tv_usec, tv1.tv_sec);
    if (ioctl(sec_fd, SEC_IOC_HMAC, &myhmac) < 0) {
        //gettimeofday(&tv2, NULL);
        //printf("end: %d\t%d\n", tv2.tv_usec, tv2.tv_sec);
        ALOGD("%s: ioctl SEC_IOC_HMAC failed!\n", __FUNCTION__);
        ALOGD("%s: error = %d\n", __FUNCTION__, errno);
        return 1;
    }
    if(RPMB_KEY_MAC_SIZE == myhmac.message.len)
        memcpy(data_frame->key_mac,myhmac.message.data,myhmac.message.len);
    else
    {
        ALOGD("%s: ioctl SEC_IOC_HMAC failed 2!\n", __FUNCTION__);
        return 1;
    }
    close(sec_fd);
     #else
     hmac_sha256 hmac;
     hmac_sha256_initialize(&hmac, "0123456789abcdef0123456789abcdef", key_len);
     hmac_sha256_update(&hmac, data_frame->data, mac_data_len);
	     /* Finalize the HMAC-SHA256 digest and output its value. */
     hmac_sha256_finalize(&hmac, NULL, 0);
     memcpy(data_frame->key_mac,hmac.digest,RPMB_KEY_MAC_SIZE);
	#endif

	 /**/
    rpmb_write_cmd->data_ptr=data_frame;
    if (ioctl(fd, MMC_IOC_CMD,rpmb_write_cmd)) {
        ALOGE( " ioctl for write data req fail \n");
	   close(fd);
	   free(data_frame);
        return 1;
    }

   /*for read result req*/
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
     msg_type=RPMB_MSG_TYPE_REQ_RESULT_READ;
     u16_to_bytes(msg_type,data_frame->msg_type);
     if (ioctl(fd, MMC_IOC_CMD,rpmb_write_cmd)) {
	   close(fd);
	   free(data_frame);
	   ALOGE( " ioctl for write data req fail1 \n");
        return 1;
    }

     rpmb_write_cmd->write_flag=0;
     rpmb_write_cmd->opcode=18;
     memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
     rpmb_write_cmd->data_ptr=data_frame;
     if (ioctl(fd, MMC_IOC_CMD, rpmb_write_cmd)) {
	   close(fd);
	    free(data_frame);
		ALOGE( " ioctl for write data req fail2 \n");
        return 1;
    }

/*result check*/
	bytes_to_u16(data_frame->op_result,&op_result);
      bytes_to_u16(data_frame->msg_type,&msg_type);
	if((op_result==RPMB_RESULT_OK)&&(msg_type==RPMB_MSG_TYPE_RESP_AUTH_DATA_WRITE))
	{
		  ALOGE(" data  write successed\n");
		   close(fd);
		   free(data_frame);
		  return 0;
	}
	else
	{
		ALOGE( " data write fail op_result:0x%x ,msg_type:0x%x\n",op_result,msg_type);
	}

	free(data_frame);
       close(fd);
	return 1;   //should not run here;

}

/*get rpmb write count
   return write count number*/
uint32_t rpmb_read_writecount(void)
{
    struct mmc_ioc_cmd *read_writecount_cmd;
    struct rpmb_data_frame *data_frame;
    uint16_t msg_type;
    uint16_t op_result;
    uint32_t writecount;
    int fd=0;

    fd= open(rpmb_dev_path, O_RDWR);
    if(fd < 0) {
        ALOGE(" open rpmb dev fail \n");
        return 1;
    }
    read_writecount_cmd=(struct mmc_ioc_cmd *)malloc(sizeof(struct mmc_ioc_cmd ));
   /*for write rpmb key req */
    read_writecount_cmd->write_flag=1;
    read_writecount_cmd->opcode=25;
    read_writecount_cmd->blksz=512;
    read_writecount_cmd->blocks=1;
    read_writecount_cmd->is_acmd=0;
     read_writecount_cmd->data_timeout_ns=1000000000;
    read_writecount_cmd->flags=MMC_RSP_R1;
    msg_type=RPMB_MSG_TYPE_REQ_WRITE_COUNTER_VAL_READ;
    data_frame=(uint8_t *)malloc(RPMB_DATA_FRAME_SIZE);
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
    u16_to_bytes(msg_type,data_frame->msg_type);

    read_writecount_cmd->data_ptr=data_frame;
    if (ioctl(fd, MMC_IOC_CMD,read_writecount_cmd)) {
        ALOGE( " ioctl for read writecount req fail \n");
	   close(fd);
	   free(data_frame);
        return 1;
    }

        read_writecount_cmd->write_flag=0;
     read_writecount_cmd->opcode=18;
     memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
     read_writecount_cmd->data_ptr=data_frame;
     if (ioctl(fd, MMC_IOC_CMD, read_writecount_cmd)) {
	   close(fd);
	    free(data_frame);
        return 1;
    }


/*result check*/
	bytes_to_u16(data_frame->op_result,&op_result);
      bytes_to_u16(data_frame->msg_type,&msg_type);
	if((op_result==RPMB_RESULT_OK)&&(msg_type==RPMB_MSG_TYPE_RESP_WRITE_COUNTER_VAL_READ))
	{
		  bytes_to_u32(data_frame->write_counter,&writecount);
  		  ALOGE("read write count successed\n");
		   close(fd);
		   free(data_frame);
		  return writecount;
	}
	else
		{
		ALOGE(" read write count:0x%x ,msg_type:0x%x\n",op_result,msg_type);
		}
	free(data_frame);
       close(fd);
	return 1;   //should not run here;


}

/*
  key: the key will write must 32 len;
  len :must be 32;
  reutrn 0 success
*/
uint8_t rpmb_program_key(uint8_t *key,uint8_t len)
{
    struct mmc_ioc_cmd *write_key_cmd;
    struct rpmb_data_frame *data_frame;
    uint16_t msg_type;
    uint16_t op_result;
    int fd=0;
        ALOGE( "rpmb_program_key() start \n");
    if (key == NULL || RPMB_KEY_MAC_SIZE != len)
    	{
    	ALOGE( " rpmb_program_key()  fail,key=0x%x,len=%d \n",key,len);
		return 1;
    	}
    fd= open(rpmb_dev_path, O_RDWR);
    if(fd < 0) {
        ALOGE( " open rpmb dev fail \n");
        return 1;
    }
      write_key_cmd=(struct mmc_ioc_cmd *)malloc(sizeof(struct mmc_ioc_cmd ));

/*for write rpmb key req */
    write_key_cmd->write_flag=1;
    write_key_cmd->opcode=25;
    write_key_cmd->blksz=512;
    write_key_cmd->blocks=1;
    write_key_cmd->is_acmd=0;
    write_key_cmd->data_timeout_ns=1000000000;
    write_key_cmd->flags=MMC_RSP_R1;
    msg_type=RPMB_MSG_TYPE_REQ_AUTH_KEY_PROGRAM;
    data_frame=(uint8_t *)malloc(RPMB_DATA_FRAME_SIZE);
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
    u16_to_bytes(msg_type,data_frame->msg_type);
   memcpy(data_frame->key_mac,key,RPMB_KEY_MAC_SIZE);
    write_key_cmd->data_ptr=data_frame;
    if (ioctl(fd, MMC_IOC_CMD,write_key_cmd)) {
        ALOGE(" ioctl for write key req fail \n");
	   close(fd);
	   free(data_frame);
        return 1;
    }

/*for read result req*/
    memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
     msg_type=RPMB_MSG_TYPE_REQ_RESULT_READ;
     u16_to_bytes(msg_type,data_frame->msg_type);
     if (ioctl(fd, MMC_IOC_CMD,write_key_cmd)) {
	   close(fd);
	   free(data_frame);
	           ALOGE(" ioctl for write key req fail1 \n");
        return 1;
    }

     write_key_cmd->write_flag=0;
     write_key_cmd->opcode=18;
     memset(data_frame,0,RPMB_DATA_FRAME_SIZE);
     write_key_cmd->data_ptr=data_frame;
     if (ioctl(fd, MMC_IOC_CMD, write_key_cmd)) {
	   close(fd);
	    free(data_frame);
		        ALOGE(" ioctl for write key req fail2 \n");
        return 1;
    }

/*result check*/
	bytes_to_u16(data_frame->op_result,&op_result);
      bytes_to_u16(data_frame->msg_type,&msg_type);
	if((op_result==RPMB_RESULT_OK)&&(msg_type==RPMB_MSG_TYPE_RESP_AUTH_KEY_PROGRAM))
	{
		  ALOGE(" key write successed\n");
		   close(fd);
		   free(data_frame);
		  return 0;
	}
	else
		ALOGE( " key write fail op_result:0x%x ,msg_type:0x%x\n",op_result,msg_type);

	free(data_frame);
       close(fd);
	           ALOGE(" ioctl for write key req fail3 \n");
	return 1;   //should not run here;

}


