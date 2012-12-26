#define LOG_TAG "SRIL"

#include <utils/Log.h>
#include "samsung_nv_flash.h"
#include <fcntl.h>

#define RF_BACKUP_BUFFER_SIZE			16*1024*3 // unknown define
#define FAIL -1
#define SUCCESS 1
#define TRUE		1
#define FALSE      0

#ifndef FEATURE_SAMSUNG_BOOT
extern unsigned short Get_GSMChecksumSysparm(unsigned short* Checksum); //rfcal.c
#endif
nvi_factory_test_check_type TestData;
nv_backup_type_t		rfnv_backup_data;
//static unsigned char	rf_back_up_rw_buffer[RF_BACKUP_BUFFER_SIZE]; /* 1 page Buffer to rf_back_up */

const nv_backup_item_info_type nv_backup_item_info_table[] = {
/*============================================================================================
                                       COMMON NV ITEM
=============================================================================================*/
	{ NV_SWVERSION, sizeof(nvi_SW_version_t), 0, 0 },		// 16
	{ NV_HWVERSION, sizeof(nvi_HW_version_t), 0, 1 },		// 16
	{ NV_PRDINFO, sizeof(nvi_CalDate_t), 0, 2 },			// 16
	{ NV_BLUETOOTH, sizeof(nvi_bluetooth_ID_t), 0, 3 },	// 8
	{ NV_WLAN, sizeof(nvi_wlan_mac_address_type), 0, 4},	// 6
	{ NV_SERIAL_NUMBER, sizeof(nvi_SerialNum_t), 0, 5},	// 16
	{ NV_CSC_VER, sizeof(nvi_CSCVer_t), 0, 6},			// 40
	{ NV_SETBAND, sizeof(nvi_Setband_t), 0, 7 },			// 4
	{ NV_AUTO_ATTACH_I, sizeof(nvi_Auto_Attach_type), 0, 8 },	// 2

	{ NV_SELLOUT_PRODUCT_CODE_I, sizeof(nvi_product_code_type), 0, 9 },	// 20
	{ NV_SELLOUT_PRODUCT_INFO_I, sizeof(nvi_product_info_type), 0, 10 }, 	// 2
	{ NV_SELLOUT_SMS_MODE_I, sizeof(unsigned int), 0, 11},	// 4
	{ NV_SELLOUT_OPR_MODE_I, sizeof(unsigned int), 0, 12},	// 4

	{ NV_FACTORY_TEST_CHECK_ITEM, sizeof(nvi_factory_test_check_type), 0, 13},	// 220
	{ NV_FACTORY_TEST_HISTORY, sizeof(nvi_factory_history_type), 0, 14},		// 120
	{ NV_FACTORY_TEST_FAIL_CHECK, sizeof(nvi_factory_fail_record_type), 0, 15},	// 4
	{ NV_FACTORY_DUMMY, sizeof(nvi_factory_dummy_t), 0, 16},	// 200

	{ NV_SENSOR_CAL_DONE, sizeof(nvi_SensorCalDone_type), 0, 17 },	// 8
	{ NV_SENSOR_CAL_DATA, sizeof(nvi_SensorCalData_type), 0, 18},	// 12

	{ NV_SYSPARM_CHECKSUM, sizeof(nvi_sysparm_checksum_t), 0, 19 },		// 4
	{ NV_SYSPARM_DEP_I, sizeof(nvi_Sysparm_dep_t), 0, 20 },				// 8480
	{ NV_NVRAM8_CHECKSUM_I, sizeof(nvi_NVRAM8_checksum_t), 0, 21 },		// 4
	{ NV_NVRAM8_I, sizeof(nvi_NVRAM8_t), 0, 22 },						// 3404 + 36

	{ NV_AMR_CODEC, sizeof(nvi_amr_codec_t), 0, 23 },						//wannatea
	{ NV_TOTAL_CALLTIME, sizeof(unsigned long), 0, 24 },
	{ NV_AR_TDF_PROD_FLAG, sizeof(unsigned char), 0, 25 }, // LTN_SW1_SYSTEM kwanguk.kim 2012.05.21 - TDF Call Block Flag

	{ NV_DUMMY, sizeof(nvi_dummy_t), 0, 26},	// 200
};

const SEC_ITEM_info_type nv_secure_item_info_table[] = {
/*============================================================================================
                                       SECURE NV ITEM
=============================================================================================*/
	{ SEC_IMEI, sizeof(SEC_IMEI_data_t), 0, 0 },
	{ SEC_MSL, sizeof(SEC_MSL16_Data_t), 0, 1 },
	{ SEC_LOCK, sizeof(SEC_LOCK_Data_t), 0, 2 },
};

#define FLASH_RF_NV_PARTITION_NAME	"cal"
#define FLASH_RF_NV_BACKUP_ADDR	0x00000000
#define FLASH_RF_NV_BACKUP_ADDR_2	0x00040000
#define FLASH_RF_NV_BACKUP_ADDR_3	0x00080000
#define FLASH_RF_NV_BACKUP_ADDR_4	0x000C0000

#define DEV_PATH_LEN	128
#define PART_NAME_LEN	128

static char	sa_Dev_Path[DEV_PATH_LEN]	= { 0, };
static char	sa_Part_Name[PART_NAME_LEN]	= { 0, };

static int param_get_dev_path(char* p_partition_name)
{
#define DEFAULT_GPT_ENTRIES			128
#define MMCBLK_PART_INFO_PATH_LEN	128

	int		p_file;
	char		a_part_info_path[MMCBLK_PART_INFO_PATH_LEN]	= { 0, };
	char		a_part_name[PART_NAME_LEN]					= { 0, };
	int		v_index;

	if( (sa_Dev_Path[0] != 0x0) && !strcmp( p_partition_name, sa_Part_Name ) )
	{
		return	NULL;
	}

	memset( sa_Dev_Path, 0x0, DEV_PATH_LEN );
	memset( sa_Part_Name, 0x0, PART_NAME_LEN );

	for( v_index = 0; v_index < DEFAULT_GPT_ENTRIES; v_index++ )
	{
		memset( a_part_info_path, 0x0, MMCBLK_PART_INFO_PATH_LEN );
		snprintf( a_part_info_path, MMCBLK_PART_INFO_PATH_LEN, "/sys/block/mmcblk0/mmcblk0p%d/partition_name", v_index + 1 );

		p_file	= open( a_part_info_path, O_RDONLY | O_SYNC );
		if( p_file < 0 )
		{
			ALOGE( "[%s] %s file open was failed...", __FUNCTION__, a_part_info_path );
		}
		else
		{
			memset( a_part_name, 0x0, PART_NAME_LEN );
			if( read( p_file, a_part_name, PART_NAME_LEN) == (-1) )
			{
				ALOGE( "[%s] %s partition name read was failed...\n", __FUNCTION__, a_part_info_path );
			}
			close( p_file );

			/***
				Use the "strncmp" function to avoid following garbage character
			***/
			if( !strncmp( p_partition_name, a_part_name, strlen(p_partition_name) ) )
			{
				snprintf( sa_Dev_Path, DEV_PATH_LEN, "/dev/block/mmcblk0p%d", v_index + 1 );
				strncpy( sa_Part_Name, p_partition_name, PART_NAME_LEN - 1 );
				ALOGD( "NV : %s(%s) device was found\n", sa_Dev_Path, sa_Part_Name );

				break;
			}
		}
	}

	if( sa_Dev_Path[0] != 0x0 )
	{
		return	NULL;
	}
	else
	{
		return	-1;
	}
}

static int Flash_Read_Data(unsigned char* buffer, int offset, unsigned long nbytes)
{
	int fd, len;

	ALOGD("Flash_Read_Data start\n");


	if( param_get_dev_path( FLASH_RF_NV_PARTITION_NAME ) )
	{
		ALOGE("NV device was not found\n");
		return 0;
	}


	fd = open( sa_Dev_Path, O_RDONLY | O_SYNC );
	if(fd<0)
	{
		ALOGE("%s device open was failed for read\n", sa_Dev_Path);
		return 0;
	}


	if(lseek(fd, offset, SEEK_SET) <0)
	{
		ALOGE("Flash_Read_Data lseek fail");
		close(fd);
		return 0;
	}


	len = read(fd, buffer, nbytes);
	if(len != nbytes)
	{
		ALOGE("Flash_Read_Data read fail");
		close(fd);
		return 0;
	}


	close(fd);


	ALOGD("Flash_Read_Data success");


	return 1;
}

static int Flash_Write_Data(unsigned char* buffer, int offset, unsigned long nbytes)
{
	int fd, len;

	ALOGD("Flash_Write_Data start");


	if( param_get_dev_path( FLASH_RF_NV_PARTITION_NAME ) )
	{
		ALOGE("NV device was not found\n");
		return 0;
	}


	fd = open( sa_Dev_Path, O_WRONLY | O_SYNC );
	if(fd<0)
	{
		ALOGE("%s device open was failed for write\n", sa_Dev_Path);
		return 0;
	}


	if(lseek(fd, offset, SEEK_SET) <0)
	{
		ALOGE("Flash_Write_Data lseek fail");
		close(fd);
		return 0;
	}


	len = write(fd, buffer, nbytes);
	if(len != nbytes)
	{
		ALOGE("Flash_Write_Data write fail");
		close(fd);
		return 0;
	}


	close(fd);


	ALOGD("Flash_Write_Data success");


	return 1;
}



int Get_All_NV_Data(nv_backup_type_t *data_ptr)
{
	nv_backup_type_t*		flash_read_data;
//	nv_backup_type_t		flash_read_data;

	flash_read_data = (nv_backup_type_t*)malloc(sizeof(nv_backup_type_t));
	if(flash_read_data == 0)
		return FAIL;

// read data from flash
	if (Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
	{
		free(flash_read_data);
		return FAIL;
	}

// copy request data
	memcpy((unsigned char*)data_ptr, (unsigned char*)flash_read_data, sizeof(nv_backup_type_t));

	free(flash_read_data);
	return SUCCESS;
}

#if 0
#ifndef FEATURE_SAMSUNG_BOOT
int Get_Sysparm_writen(void)
{
	unsigned short	Sysparm_Dep_Checksum = 0;
	unsigned short	SAMSUNG_NV_Checksum = 0;

	if(SUCCESS != Get_GSMChecksumSysparm(&Sysparm_Dep_Checksum))
		return FAIL;
	if( SUCCESS != Flash_Read_NV_Data(&SAMSUNG_NV_Checksum, NV_SYSPARM_CHECKSUM))
		return FAIL;

	if(Sysparm_Dep_Checksum == SAMSUNG_NV_Checksum)
		return SUCCESS;
	else
		return FAIL;
}
#endif
#endif

int Remove_All_NV_Data(void)
{
	// flash erease
//	if(Flash_Erase((FLASH_RF_NV_BACKUP_ADDR >> NAND_FLASH_BLOCK_SIZE_BIT)) == TRUE)
	if(1)
		return SUCCESS;
	else
		return FAIL;
}


int GetMagicCodeWriten(void)
{
	nv_backup_header_t temp_header;

	temp_header.backup_flag = 0;

	if (Flash_Read_Data((unsigned char*)&temp_header, FLASH_RF_NV_BACKUP_ADDR, sizeof(nv_backup_header_t)) != TRUE)
		return FAIL;

	if(temp_header.backup_flag== BACKUP_MAGIC_CODE)
		return SUCCESS;
	else
		return FAIL;
}


int GetBackupFlagActive(int index)
{
	nv_backup_header_t temp_header;

	memset((void*)&temp_header, 0, sizeof(nv_backup_header_t ));

	if (Flash_Read_Data((unsigned char*)&temp_header, FLASH_RF_NV_BACKUP_ADDR, sizeof(nv_backup_header_t)) != TRUE)
		return FAIL;

	if ((temp_header.active_flag[nv_backup_item_info_table[index].table_index] & ((unsigned	long) 1 << nv_backup_item_info_table[index].active_bit)))
		return FAIL;
	else
		return SUCCESS;
}


int Flash_Write_NV_Data(void *data_ptr, int index)
{
	nv_backup_type_t*		flash_read_data;
	//nv_backup_header_t	temp_header;
	unsigned long			Address;//	= FLASH_RF_NV_BACKUP_ADDR ;
	unsigned long			Address_offset = 0;

	int i;
// check magic code

	flash_read_data = (nv_backup_type_t*)malloc(sizeof(nv_backup_type_t));
	if(flash_read_data == 0)
		return FAIL;

	if(GetMagicCodeWriten() != SUCCESS)
	{
		// set Magic code
		flash_read_data->nv_backup_header.backup_flag = BACKUP_MAGIC_CODE;

		// clear header flags.
		for(i = 0; i < sizeof(flash_read_data->nv_backup_header.active_flag)/sizeof(flash_read_data->nv_backup_header.active_flag[0]); i++)
		{
			flash_read_data->nv_backup_header.active_flag[i] = 0xffffffff;
		}
	}
	else
	{
		if(Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
		{
			free(flash_read_data);
			return FAIL;
		}
	}

// write header flag
	flash_read_data->nv_backup_header.active_flag[nv_backup_item_info_table[index].table_index] &= ~((unsigned	long) 1 << nv_backup_item_info_table[index].active_bit);

// calcurate address offet.
	Address_offset += sizeof(nv_backup_header_t);

	for(i = 0;(i < index) && (i < NV_MAX_ITEM)  ; i++)
	{
		Address_offset += nv_backup_item_info_table[i].item_size;
	}

	Address =(unsigned long) flash_read_data + Address_offset;
	memcpy((unsigned char*)Address, (unsigned char*)data_ptr, nv_backup_item_info_table[index].item_size);

	// flash erease
//	Flash_Erase((FLASH_RF_NV_BACKUP_ADDR >> NAND_FLASH_BLOCK_SIZE_BIT));

	// flash write
	if (Flash_Write_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
	{
		free(flash_read_data);
		return FAIL;
	}
	else
	{
		free(flash_read_data);
		return SUCCESS;
	}

}


int Flash_Read_NV_Data(void *data_ptr, int index)
{
	nv_backup_type_t*		flash_read_data;
	unsigned long Address=0;
	unsigned	long Address_offset=0;
	unsigned	long size;

	nv_backup_header_t temp_header;
	unsigned	long Position = 0;
	unsigned	long i;

	flash_read_data = (nv_backup_type_t*)malloc(sizeof(nv_backup_type_t));
	if(flash_read_data == 0)
		return FAIL;
#if 0
// check magic code
	if(GetMagicCodeWriten() != SUCCESS)
	{
		ALOGD("Flash_Read_NV_Data: GetMagicCodeWriten fail");
		free(flash_read_data);
		return FAIL;
	}

// check backup header flag
	if(GetBackupFlagActive(index) != SUCCESS)
	{
		ALOGD("Flash_Read_NV_Data: GetBackupFlagActive fail");
		free(flash_read_data);
		return FAIL;
	}
#endif
// read data from flash
	if (Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
	{
		ALOGD("Flash_Read_NV_Data: Flash_Read_Data fail");
		if(rfnv_backup_data.nv_backup_header.backup_flag != BACKUP_MAGIC_CODE)
		{
			free(flash_read_data);
			return FAIL;
		}
		else
			memcpy((unsigned char*)flash_read_data, (unsigned char*)&rfnv_backup_data, sizeof(nv_backup_type_t));
	}
	if(flash_read_data->nv_backup_header.backup_flag != BACKUP_MAGIC_CODE)
	{
		ALOGD("Flash_Read_NV_Data: GetMagicCodeWriten fail");
		free(flash_read_data);
		return FAIL;
	}
	memcpy((unsigned char*)&rfnv_backup_data, (unsigned char*)flash_read_data, sizeof(nv_backup_type_t));
	if ((flash_read_data->nv_backup_header.active_flag[nv_backup_item_info_table[index].table_index] & ((unsigned	long) 1 << nv_backup_item_info_table[index].active_bit)))
	{
		ALOGD("Flash_Read_NV_Data: GetBackupFlagActive fail");
		free(flash_read_data);
		return FAIL;
	}


// calcurate address

	Address_offset += sizeof(nv_backup_header_t);

	for(i = 0;(i != index) && (i < NV_MAX_ITEM)  ; i++)
	{
		Address_offset += nv_backup_item_info_table[i].item_size;
	}
	Address =(unsigned long) flash_read_data + Address_offset;

// copy request data
	memcpy((unsigned char*)data_ptr, (unsigned char*)Address, nv_backup_item_info_table[index].item_size);

	free(flash_read_data);
	ALOGD("Flash_Read_NV_Data success");
	return SUCCESS;
}


int Flash_Remove_NV_Data(int index)
{
	nv_backup_type_t*		flash_read_data;
	//nv_backup_header_t	temp_header;
	unsigned long			Address;//	= FLASH_RF_NV_BACKUP_ADDR ;
	unsigned long			Address_offset = 0;

	int i;

	flash_read_data = (nv_backup_type_t*)malloc(sizeof(nv_backup_type_t));
	if(flash_read_data == 0)
		return FAIL;

// check magic code
	if(GetMagicCodeWriten() == SUCCESS)
	{
		if(Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
		{
			free(flash_read_data);
			return FAIL;
		}
	}
	else
	{
		free(flash_read_data);
		return SUCCESS;
	}

// Remove header flag
	flash_read_data->nv_backup_header.active_flag[nv_backup_item_info_table[index].table_index] |= ((unsigned	long) 1 << nv_backup_item_info_table[index].active_bit);

// flash erease
//	Flash_Erase((FLASH_RF_NV_BACKUP_ADDR >> NAND_FLASH_BLOCK_SIZE_BIT));

	// flash write
	if (Flash_Write_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR,sizeof(nv_backup_type_t)) != TRUE)
	{
		free(flash_read_data);
		return FAIL;
	}
	else
	{

		free(flash_read_data);
		return SUCCESS;
	}

}


void Factory_Process_Check(unsigned char TestId, unsigned char Result)
{
	unsigned char i;
	nvi_factory_test_check_type TestData;
	nvi_factory_history_type HistoryData;
	unsigned char log_index;

	memset(&TestData, 0, sizeof(nvi_factory_test_check_type));
	memset(&HistoryData, 0, sizeof(nvi_factory_history_type));

	Flash_Read_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
	Flash_Read_NV_Data(&HistoryData, NV_FACTORY_TEST_HISTORY);

	if(TestId <= TEST_ITEM_NUM)
	{
		TestData.factory_item[TestId-1].test_id = TestId;
		TestData.factory_item[TestId-1].result = Result;
	}

	log_index	 = 0;
	while(HistoryData.factory_item[log_index].test_id != 0x00)
	{
		log_index++;
		if(log_index==HISTORY_ITEM_NUM)
			break;
	}

	if(log_index < HISTORY_ITEM_NUM)
	{
		HistoryData.factory_item[log_index].test_id  = TestId;
		HistoryData.factory_item[log_index].result  = Result;
	}
	else
	{
		for(i = 1; i < HISTORY_ITEM_NUM; i++)
		{
			HistoryData.factory_item[i-1].test_id  = HistoryData.factory_item[i].test_id;
			HistoryData.factory_item[i-1].result  = HistoryData.factory_item[i].result;
		}
		HistoryData.factory_item[HISTORY_ITEM_NUM-1].test_id  = TestId;
		HistoryData.factory_item[HISTORY_ITEM_NUM-1].result  = Result;
	}

	Flash_Write_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
	Flash_Write_NV_Data(&HistoryData, NV_FACTORY_TEST_HISTORY);
}


unsigned char Factory_Process_Check_Read(unsigned char TestId)
{
	nvi_factory_test_check_type TestData;

	memset(&TestData, 0, sizeof(nvi_factory_test_check_type));

	if(TestId > TEST_ITEM_NUM)
		return 0;

	Flash_Read_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);

	return TestData.factory_item[TestId-1].result;
}


void InitFactoryTestNV(void)
{
	int i;

	nvi_factory_history_type 	HistoryData;
	nvi_factory_test_check_type TestData;
	nvi_factory_fail_record_type FailRecode;


	for(i = 0; i < TEST_ITEM_NUM ; i++)
	{
		// write default value 'N'
		TestData.factory_item[i].test_id= i+1;
		TestData.factory_item[i].result = 'N';
	}

	for(i = 0; i < HISTORY_ITEM_NUM ; i++)
	{
		// write default value 'N'
		HistoryData.factory_item[i].test_id= 0;
		HistoryData.factory_item[i].result = 'N';
	}

	FailRecode.fail_record = 'N';
	FailRecode.retry = 'N';
	FailRecode.dummy1 = 'N';
	FailRecode.dummy2 = 'N';


	Flash_Write_NV_Data(&TestData, NV_FACTORY_TEST_CHECK_ITEM);
	Flash_Write_NV_Data(&HistoryData, NV_FACTORY_TEST_HISTORY);
	Flash_Write_NV_Data(&FailRecode, NV_FACTORY_TEST_FAIL_CHECK);


}


void FullEreaseHistoryNV(void)
{
	int i;
	nvi_factory_history_type HistoryData;

	for(i = 0; i < HISTORY_ITEM_NUM ; i++)
	{
		// write default value 'N'
		HistoryData.factory_item[i].test_id= 0;
		HistoryData.factory_item[i].result = 'N';
	}
	Flash_Write_NV_Data(&HistoryData, NV_FACTORY_TEST_HISTORY);

}


int ReadSerialNumber(char *Serialptr)											// js0809.kim 2009.12.08 Serial Number Test
{
	if(!Flash_Read_NV_Data(Serialptr, NV_SERIAL_NUMBER))
	{
		return 0;
	}
	return 1;
}


int WriteSerialNumber(char *Serialptr)											// js0809.kim 2009.12.08 Serial Number Test
{
	if(!Flash_Write_NV_Data(Serialptr, NV_SERIAL_NUMBER))
	{
		return 0;
	}
	return 1;
}


int ReadCSCVer(char *CSCVer)
{
	if(!Flash_Read_NV_Data(CSCVer, NV_CSC_VER))
	{
		return 0;
	}
	return 1;
}


int WriteCSCVer(char *CSCVer)
{
	if(!Flash_Write_NV_Data(CSCVer, NV_CSC_VER))
	{
		return 0;
	}
	return 1;
}




/* --------------------------------------------------------------------- */
int Flash_Write_Secure_Data(void *data_ptr, int index)
{
	SEC_NORMAL_t*		flash_read_data;
	//nv_backup_header_t	temp_header;
	unsigned long			Address;//	= FLASH_RF_NV_BACKUP_ADDR ;
	unsigned long			Address_offset = 0;

	int i;
// check magic code

	flash_read_data = (SEC_NORMAL_t*)malloc(sizeof(SEC_NORMAL_t));
	if(flash_read_data == 0)
		return FAIL;

	if(Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR_3,sizeof(SEC_NORMAL_t)) != TRUE)
		{
			free(flash_read_data);
			return FAIL;
		}

// calcurate address offet.
//	Address_offset += sizeof(nv_backup_header_t);

	for(i = 0;(i < index) && (i < SEC_MAX_ITEM)  ; i++)
	{
		Address_offset += nv_secure_item_info_table[i].item_size;
	}

	Address =(unsigned long) flash_read_data + Address_offset;
	memcpy((unsigned char*)Address, (unsigned char*)data_ptr, nv_secure_item_info_table[index].item_size);

	// flash write
	if (Flash_Write_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR_3, sizeof(SEC_NORMAL_t)) != TRUE)
	{
		free(flash_read_data);
		return FAIL;
	}
	else
	{
		free(flash_read_data);
		return SUCCESS;
	}

}


int Flash_Read_Secure_Data(void *data_ptr, int index)
{
	SEC_NORMAL_t*		flash_read_data;
	unsigned long Address=0;
	unsigned	long Address_offset=0;
	unsigned	long size;

//	nv_backup_header_t temp_header;
	unsigned	long Position = 0;
	unsigned	long i;

	flash_read_data = (SEC_NORMAL_t*)malloc(sizeof(SEC_NORMAL_t));
	if(flash_read_data == 0)
		return FAIL;

// read data from flash
	if (Flash_Read_Data((unsigned char*)flash_read_data, FLASH_RF_NV_BACKUP_ADDR_3, sizeof(SEC_NORMAL_t)) != TRUE)
	{
		free(flash_read_data);
		return FAIL;
	}

// calcurate address

//	Address_offset += sizeof(nv_backup_header_t);

	for(i = 0;(i != index) && (i < SEC_MAX_ITEM)  ; i++)
	{
		Address_offset += nv_secure_item_info_table[i].item_size;
	}
	Address =(unsigned long) flash_read_data + Address_offset;

// copy request data
	memcpy((unsigned char*)data_ptr, (unsigned char*)Address, nv_secure_item_info_table[index].item_size);

	free(flash_read_data);
	return SUCCESS;
}

#define u8 unsigned char

int sec_atc3_040_MSLSECUR_EncodingField(char * pp_Incomming_Field, char * pp_EncodedField, int vp_size)
{
	int vl_Error=0;
	int i=0;
	char * pl_ReceivedField;
	char * pl_Splited_Field;

	pl_ReceivedField = (char *)malloc(vp_size);
	pl_Splited_Field = (char *)malloc(vp_size * 2);

	memcpy(pl_ReceivedField, pp_Incomming_Field, vp_size);

	for(i=0 ; i<vp_size; i++)
	{
		/*Split Field Num*/
		pl_Splited_Field[2*i] = (pl_ReceivedField[i] & 0xF0)>>4;
		pl_Splited_Field[2*i+1] = pl_ReceivedField[i] & 0x0F;
	}
	/*Rebuild Field Numbers*/
	for(i=0; i < vp_size ; i++)
	{
		*(u8 *)(pp_EncodedField+i) = (u8)((pl_Splited_Field[i] & 0x0F)<<4 | (pl_Splited_Field[i+vp_size] & 0x0F)) +( 0x16+i);
	}

	free(pl_ReceivedField);
	free(pl_Splited_Field);

	return vl_Error;
}

int sec_atc3_040_MSLSECUR_DecodingField(char * pp_EncodedField, char * pp_DecodedField, int vp_size)
{
	int vl_Error=0;
	int i=0;
	char * pl_ReceivedField;
	char * pl_Rebuild_Field_Total;
	char * pl_Rebuild_Field_1st;
	char * pl_Rebuild_Field_2nd;

	pl_ReceivedField = (u8 *)malloc(vp_size);
	pl_Rebuild_Field_Total = (u8 *)malloc(vp_size * 2);
	pl_Rebuild_Field_1st = (u8 *)malloc(vp_size);
	pl_Rebuild_Field_2nd = (u8 *)malloc(vp_size);

	memcpy(pl_ReceivedField, (u8 *)pp_EncodedField, vp_size);

	for(i=0; i < vp_size ; i++)
	{
		pl_ReceivedField[i] = pl_ReceivedField[i] - (0x16+i) ;
		pl_Rebuild_Field_1st[i] = (pl_ReceivedField[i] & 0xF0)>>4;
		pl_Rebuild_Field_2nd[i] = pl_ReceivedField[i] & 0x0F;
	}

	for(i=0; i<vp_size;i++)
	{
		pl_Rebuild_Field_Total[i] = pl_Rebuild_Field_1st[i];
		pl_Rebuild_Field_Total[i+vp_size] = pl_Rebuild_Field_2nd[i];
	}

	for(i=0 ; i<vp_size; i++)
	{
		pl_ReceivedField[i] = (pl_Rebuild_Field_Total[i*2]<<4) |(pl_Rebuild_Field_Total[i*2+1]);
	}

	memcpy((u8 *)pp_DecodedField,(u8 *)pl_ReceivedField, vp_size);

	free(pl_ReceivedField);
	free(pl_Rebuild_Field_Total);
	free(pl_Rebuild_Field_1st);
	free(pl_Rebuild_Field_2nd);

	return vl_Error;
}
