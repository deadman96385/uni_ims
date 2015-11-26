#!/bin/sh

SECURE_BOOT=$(get_build_var PRODUCT_SECURE_BOOT)
HOST_OUT=$(get_build_var HOST_OUT_EXECUTABLES)
PRODUCT_OUT=$(get_build_var PRODUCT_OUT)
CURPATH=$(pwd)
CERTPATH=$CURPATH/vendor/sprd/proprietories-source/packimage/signimage/sansa/output
CFGPATH=$CURPATH/vendor/sprd/proprietories-source/packimage/signimage/sprd/config
DESTDIR=$CURPATH/vendor/sprd/proprietories-source/packimage/signimage/sprd/mkdbimg/bin
SPL=$PRODUCT_OUT/u-boot-spl-16k.bin
SPLSIGN=$PRODUCT_OUT/u-boot-spl-16k-sign.bin
SML=$PRODUCT_OUT/sml.bin
SMLSIGN=$PRODUCT_OUT/sml-sign.bin
TOS=$PRODUCT_OUT/tos.bin
TOSSIGN=$PRODUCT_OUT/tos-sign.bin
UBOOT=$PRODUCT_OUT/u-boot.bin
UBOOTSIGN=$PRODUCT_OUT/u-boot-sign.bin
FDL1=$PRODUCT_OUT/fdl1.bin
FDL1SIGN=$PRODUCT_OUT/fdl1-sign.bin
FDL2=$PRODUCT_OUT/fdl2.bin
FDL2SIGN=$PRODUCT_OUT/fdl2-sign.bin
BOOT=$PRODUCT_OUT/boot.img
BOOTSIGN=$PRODUCT_OUT/boot-sign.img
RECOVERY=$PRODUCT_OUT/recovery.img
RECOVERYSIGN=$PRODUCT_OUT/recovery-sign.img

getModuleName()
{
    local name="allmodules"
    if [ $# -gt 0 ]; then
        for loop in $@
        do
            case $loop in
            "chipram")
            name="chipram"
            break
            ;;
            "bootloader")
            name="bootloader"
            break
            ;;
            "bootimage")
            name="bootimage"
            break
            ;;
            "systemimage")
            name="systemimage"
            break
            ;;
            "userdataimage")
            name="userdataimage"
            break
            ;;
            "recoveryimage")
            name="recoveryimage"
            break
            ;;
            "clean")
            name="clean"
            break
            ;;
            *)
            ;;
            esac
        done
    fi
    echo $name
}

doImgHeaderInsert()
{
    local NO_SECURE_BOOT

    if [ "NONE" = $SECURE_BOOT ]; then
        NO_SECURE_BOOT=1
    else
        NO_SECURE_BOOT=0
    fi

    for loop in $@
    do
        if [ -f $loop ] ; then
            $HOST_OUT/imgheaderinsert $loop $NO_SECURE_BOOT
        else
            echo "#### no $loop,please check ####"
        fi
    done
}

dosprdcopy()
{
	if [ -f $SPLSIGN ];then
		cp $SPLSIGN $DESTDIR
		#echo -e "\033[33m copy spl-sign.bin finish!\033[0m"
	fi

	if [ -f $FDL1SIGN ]; then
		cp $FDL1SIGN $DESTDIR
		#echo -e "\033[33m copy fdl1-sign.bin finish!\033[0m"
	fi
}

doSignImage()
{
    if [ "NONE" = $SECURE_BOOT ]; then
        return
    fi
	#/*add sprd sign*/
if [ "SPRD" = $SECURE_BOOT ]; then
	for image in $@
	do
		if [ -f $image ]; then
			$HOST_OUT/sprd_sign  $image  $CFGPATH
		else
			echo -e "\033[31m ####  no $image, pls check #### \033[0m"
		fi
	done
	#call this function do copy fdl1&spl to mkdbimg/bin document
	dosprdcopy
fi


}

doPackImage()
{
    case $(getModuleName "$@") in
        "chipram")
            doImgHeaderInsert $SPL $FDL1
            doSignImage $SPLSIGN $FDL1SIGN
		rm $SPL $FDL1
            ;;
        "bootloader")
            doImgHeaderInsert $UBOOT $FDL2
            doSignImage $UBOOTSIGN $FDL2SIGN
		rm $UBOOT $FDL2
            ;;
        "bootimage")
	    if [ "NONE" = $SECURE_BOOT ]; then
		echo "secure boot not enabled, skip!"
	    else
		doImgHeaderInsert $BOOT
		doSignImage $BOOTSIGN
	    fi
            ;;
        "recoveryimage")
	    if [ "NONE" = $SECURE_BOOT ]; then
		echo "secure boot not enabled, skip!"
	    else
		doImgHeaderInsert $RECOVERY
		doSignImage $RECOVERYSIGN
	    fi
            ;;
        "allmodules")
	    if [ "NONE" = $SECURE_BOOT ]; then
		doImgHeaderInsert $SPL $UBOOT $FDL1 $FDL2
	    else
		doImgHeaderInsert $SPL $UBOOT $FDL1 $FDL2 $BOOT $RECOVERY
		doSignImage $SPLSIGN $FDL1SIGN $UBOOTSIGN $FDL2SIGN $BOOTSIGN $RECOVERYSIGN
	    fi
		rm $SPL $FDL1 $UBOOT $FDL2
            ;;
        "clean")
            #do nothing
            ;;
        *)
            ;;
    esac
}

doPackImage "$@"
