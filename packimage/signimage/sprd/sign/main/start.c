
#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/err.h"

#include "sprdsec_header.h"
#include "pk1.h"
#include "rsa_sprd.h"
#include "sprdsha.h"
#include "sprd_verify.h"
#include "sec_string.h"

#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

static unsigned char padding[512] = { 0 };

#define NAME_MAX_LEN 2048
#define KEYCERT_VERSION 1
#define CONTENTCERT_VERSION 1
static void *load_file(const char *fn, unsigned *_sz)
{
	char *data;
	int sz;
	int fd;

	data = 0;
	fd = open(fn, O_RDONLY);

	if (fd < 0)
		return 0;

	sz = lseek(fd, 0, SEEK_END);

	if (sz < 0)
		goto oops;

	if (lseek(fd, 0, SEEK_SET) != 0)
		goto oops;

	data = (char *)malloc(sz);

	if (data == 0)
		goto oops;

	if (read(fd, data, sz) != sz)
		goto oops;

	close(fd);

	if (_sz)
		*_sz = sz;

	return data;

oops:
	close(fd);
	if (data != 0)
		free(data);
	return 0;
}

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
	unsigned pagemask = pagesize - 1;
	unsigned int count;
	memset(padding, 0xff, sizeof(padding));
	if ((itemsize & pagemask) == 0) {
		return 0;
	}

	count = pagesize - (itemsize & pagemask);
	//printf("need to padding %d byte,%d,%d\n",count,itemsize%8,(itemsize & pagemask));
	if (write(fd, padding, count) != count) {
		return -1;
	} else {
		return 0;
	}
}

int usage(void)
{
	printf("usage:sign the image\n");
	printf("--filename,      the image which to signed \n");
	printf("--config_path,   the config path keep the config file for signture \n");

}

/*
*  this function only sign the img
*/

int sprd_signimg(char *img, char *key_path)
{

	int i, j;
	int fd;
	int img_len;
	char *key[6] = { 0 };
	unsigned pagesize = 512;
	char *input_data = NULL;
	char *output_data = NULL;

	char *img_name = NULL;
	char *payload_addr = NULL;

	output_data = img;
	char *basec = strdup(img);
	img_name = basename(basec);
	//printf("input name is:%s\n",img_name);
	for (i = 0; i < 6; i++) {
		key[i] = (char *)malloc(NAME_MAX_LEN);
		if (key[i] == 0)
			goto fail;
		memset(key[i], 0, NAME_MAX_LEN);
		strcpy(key[i], key_path);
		if (key_path[strlen(key_path) - 1] != '\/')
			key[i][strlen(key_path)] = '/';
		//printf("key[%d]= %s\n", i, key[i]);

	}

	strcat(key[0], "rsa2048_0_pub.pem");
	strcat(key[1], "rsa2048_1_pub.pem");
	strcat(key[2], "rsa2048_2_pub.pem");
	strcat(key[3], "rsa2048_0.pem");
	strcat(key[4], "rsa2048_1.pem");
	strcat(key[5], "rsa2048_2.pem");

	sprdsignedimageheader sign_hdr;
	sprd_keycert keycert;
	sprd_contentcert contentcert;
	memset(&sign_hdr, 0, sizeof(sprdsignedimageheader));
	memset(&keycert, 0, sizeof(sprd_keycert));
	memset(&contentcert, 0, sizeof(sprd_contentcert));

	input_data = load_file(img, &img_len);
	if (input_data == 0) {
		printf("error:could not load img\n");
		return 0;
	}
	printf("img_len = %d\n", img_len);

	payload_addr = input_data + sizeof(sys_img_header);
	sign_hdr.payload_size = img_len - sizeof(sys_img_header);
	sign_hdr.payload_offset = sizeof(sys_img_header);
	sign_hdr.cert_offset = img_len + sizeof(sprdsignedimageheader);

	sprd_rsapubkey nextpubk;

	fd = open(output_data, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == 0) {
		printf("error:could create '%s'\n", output_data);
		return 0;
	}

	if (write(fd, input_data, img_len) != img_len)
		goto fail;

	if ((0 == memcmp("fdl1-sign.bin", img_name, strlen("fdl1-sign.bin")))
	    || (0 == memcmp("u-boot-spl-16k-sign.bin", img_name, strlen("u-boot-spl-16k-sign.bin")))) {
		printf("sign fdl1/spl: %s\n", img_name);
		keycert.certtype = CERTTYPE_KEY;
		//keycert.version = 0x1;
		keycert.type = 0x1;
		keycert.version = 0xFFFF;//tshark3 only
		printf("keycert version is: %d\n", keycert.version);
		sign_hdr.cert_size = sizeof(sprd_keycert);
		getpubkeyfrmPEM(&keycert.pubkey, key[0]);	/*pubk0 */
		getpubkeyfrmPEM(&nextpubk, key[1]);	/*pubk1 */
		printf("current pubk is: %s\n", key[0]);
		printf("nextpubk is: %s\n", key[1]);
		//dumpHex("payload:",payload_addr,512);
		cal_sha256(payload_addr, sign_hdr.payload_size, keycert.hash_data);
		cal_sha256(&nextpubk, SPRD_RSAPUBKLEN, keycert.hash_key);
		calcSignature(keycert.hash_data, ((HASH_BYTE_LEN << 1) + 8), keycert.signature, key[3]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &keycert, sizeof(sprd_keycert)) != sizeof(sprd_keycert))
			goto fail;

	} else if ((0 == memcmp("fdl2-sign.bin", img_name, strlen("fdl2-sign.bin")))
		   || (0 == memcmp("u-boot-sign.bin", img_name, strlen("u-boot-sign.bin")))) {
		printf("sign fdl2/uboot: %s\n", img_name);
		keycert.certtype = CERTTYPE_KEY;
		keycert.version = KEYCERT_VERSION;
		printf("keycert version is: %d\n", keycert.version);
		sign_hdr.cert_size = sizeof(sprd_keycert);
		getpubkeyfrmPEM(&keycert.pubkey, key[1]);	/*pubk1 */
		getpubkeyfrmPEM(&nextpubk, key[2]);	/*pubk2 */
		printf("current pubk is: %s\n", key[1]);
		printf("next pubk is: %s\n", key[2]);
		cal_sha256(payload_addr, sign_hdr.payload_size, keycert.hash_data);
		cal_sha256(&nextpubk, SPRD_RSAPUBKLEN, keycert.hash_key);
		calcSignature(keycert.hash_data, ((HASH_BYTE_LEN << 1) + 8), keycert.signature, key[4]);
		/*
		if(write_padding(fd,pagesize,img_len))
			goto fail;
		*/
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &keycert, sizeof(sprd_keycert)) != sizeof(sprd_keycert))
			goto fail;

	} else if ((0 == memcmp("tos-sign.bin", img_name, strlen("tos-sign.bin"))) || (0 == memcmp("sml-sign.bin", img_name, strlen("sml-sign.bin")))) {
		printf("sign tos/sml: %s\n", img_name);
		contentcert.certtype = CERTTYPE_CONTENT;
		sign_hdr.cert_size = sizeof(sprd_contentcert);
		getpubkeyfrmPEM(&contentcert.pubkey, key[1]);	/*pubk1 */
		printf("current pubk is: %s\n", key[1]);
		cal_sha256(payload_addr, sign_hdr.payload_size, contentcert.hash_data);
		calcSignature(contentcert.hash_data, (HASH_BYTE_LEN + 8), contentcert.signature, key[4]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &contentcert, sizeof(sprd_contentcert)) != sizeof(sprd_contentcert))
			goto fail;
	} else {
		printf("sign boot/modem: %s\n", img_name);
		contentcert.certtype = CERTTYPE_CONTENT;
		contentcert.version = CONTENTCERT_VERSION;
		printf("contentcert version is: %d\n", contentcert.version);
		sign_hdr.cert_size = sizeof(sprd_contentcert);
		getpubkeyfrmPEM(&contentcert.pubkey, key[2]);	/*pubk2 */
		printf("current pubk is: %s\n", key[2]);
		cal_sha256(payload_addr, sign_hdr.payload_size, contentcert.hash_data);
		calcSignature(contentcert.hash_data, (HASH_BYTE_LEN + 8), contentcert.signature, key[5]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &contentcert, sizeof(sprd_contentcert)) != sizeof(sprd_contentcert))
			goto fail;

	}

	return 1;

fail:
	printf("sign failed!!!\n");
	unlink(output_data);
	close(fd);
	for (i = 0; i < 5; i++) {
		if (key[i] != 0)
			free(key[i]);
	}

	if (basec != 0)
		free(basec);
	return 0;

}

int main(int argc, char **argv)
{
	if (argc != 3) {
		usage();
		return 0;
	}
	char *cmd1 = argv[1];	// img name
	char *cmd2 = argv[2];	//key documount
	sprd_signimg(cmd1, cmd2);

}
