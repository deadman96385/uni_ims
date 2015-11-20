#!/bin/bash

ROOTDIR=$(pwd)
CERTPATH=$ROOTDIR/../bin

if [ ! -f $CERTPATH/primary_debug.cert ] ;
	then echo -e "\033[31m primary_debug.cert is not exist,pls execute the script in mkprimarycert document  first! \033[0m "
fi

if [ ! -f $CERTPATH/u-boot-spl-16k-sign.bin ] ;
	then echo -e "\033[31m u-boot-spl-16k-sign.bin is not exist,pls execute make chipram  first! \033[0m "
fi

if [ ! -f $CERTPATH/fdl1-sign.bin ] ;
	then echo -e "\033[31m fdl1-sign.bin is not exist, pls execute make chipram  first! \033[0m "
fi

if [ "$1" = "" -o "$2" = "" -o "$3" = "" ] ;
	then echo -e "\033[31m pls input command: mkdbimg 0x1234 0x5678 0xffff eg.\033[0m "
	    exit
fi

IMAGE_FDL1=$CERTPATH/fdl1-sign.bin
IMAGE_SPL=$CERTPATH/u-boot-spl-16k-sign.bin
KEY_PATH=$ROOTDIR/../config
CERT=$CERTPATH/primary_debug.cert
UID0=$1
UID1=$2
MASK=$3

dosprdmkdbimg()
{

if [ -f $IMAGE_FDL1 ] ;
	then $ROOTDIR/../bin/mkdbimg $IMAGE_FDL1 $CERT $KEY_PATH $UID0 $UID1 $MASK
	echo -e "\033[33m mkdbimg fdl1-sign.bin ok!\033[0m "
fi
if [ -f $IMAGE_SPL ] ;
	then $ROOTDIR/../bin/mkdbimg $IMAGE_SPL $CERT $KEY_PATH $UID0 $UID1 $MASK
		echo -e "\033[33m mkdbimg spl-sign.bin ok! \033[0m "
fi

}

dosprdmkdbimg
