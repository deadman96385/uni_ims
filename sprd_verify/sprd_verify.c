#include "stdio.h"
#include "sec_boot.h"
#include "fcntl.h"

void usage0(void)
{
        fprintf(stderr,
"\n"
"Usage: verify [-f filename] \n");
}

int verify(char* name1,char* name2)
{
	char* data1;
	char* data2;
	int len1,len2;
	int fd1,fd2;
	char *puk_adr;
	vlr_info_t*  vlr_info;

	printf("vlr=%d,key=%d\n",VLR_INFO_SIZ,KEY_INFO_SIZ);
	printf("name1=%s,name2=%s\n",name1,name2);

	//read fd1 puk
	fd1 = open(name1,O_RDONLY);
	len1 = lseek(fd1,0,SEEK_END);
	data1=malloc(len1);
	lseek(fd1,0,SEEK_SET);
	read(fd1,data1,len1);
	printf("name1=%s len=%d\n",name1,len1);
	close(fd1);
	puk_adr = data1 + len1 - KEY_INFO_SIZ - VLR_INFO_SIZ;

	//read fd2 vlr
	fd2 = open(name2,O_RDONLY);
	len2 = lseek(fd2,0,SEEK_END);
	data2=malloc(len2);
	lseek(fd2,0,SEEK_SET);
	read(fd2,data2,len2);
	printf("name2=%s len=%d\n",name2,len2);
	close(fd2);

	//verify
	vlr_info = (vlr_info_t*)(data2 + 512);
	printf("vlr=%x,%x,%x,%x\n",*(char*)vlr_info,*((char*)vlr_info+1),*((char*)vlr_info+2),*((char*)vlr_info+3));
	printf("name2=%s magic=%x,singedlen=%d\n",name2,vlr_info->magic,vlr_info->length);
    secure_check(data2+1024,vlr_info->length, vlr_info, puk_adr);

	free(data1);
	free(data2);
	return 0;
}

int main(int argc, char **argv)
{
#if 0
	int opt;
	char *op = "f:h";
	void (*usage)(void);

	usage = usage0;

	while ((opt = getopt(argc, argv, op)) != -1) {
		switch (opt) {
			case 'f':
				break;
			case 'h':
				usage();
				exit(0);
			default:
				break;
		}
		return 0;
	};
#endif
	char* file1;
        char* file2;

	file1 = argv[1];
        file2 = argv[2];
	verify(file1,file2);

	printf("verify ok\n");

	return 0;
}
