######################################################################
export PATH=$PATH:/opt/or32-new/bin:/opt/or1ksim/bin
sudo ln -s -f /usr/lib/or32_lib/libgmp.so.10.0.1 /usr/lib/libgmp.so.10
sudo ln -s -f /usr/lib/or32_lib/libmpc.so.2.0.0 /usr/lib/libmpc.so.2
sudo ln -s -f /usr/lib/or32_lib/libmpfr.so.4.0.1 /usr/lib/libmpfr.so.4
ldd /opt/or32-new/libexec/gcc/or32-elf/4.5.1-or32-1.0rc4/cc1

cd ../h264e
######################################################################
# Include Headers
ORSC_INC=./openrisc/inc/
VSP_DRV_INC=./vsp_drv/
H264E_COMMON_INC=./h264_encoder/code/inc/common/
H264E_ENC_INC=./h264_encoder/code/inc/encoder/
# Source Code
ORSC_SRC=./openrisc/src/*.c
ORSC_ROM= #./openrisc/src/*.S
H264E_ENC_SRC=./h264_encoder/code/src/encoder/*.c
H264E_MAIN=./h264_encoder/simulation/vc/WinPrj/h264enc_main.c
MAIN=./main.c
COMPILE_SRC="${ORSC_SRC} ${ORSC_ROM} ${H264E_ENC_SRC} ${MAIN}"
# Memory Link Script
LINK_SCRIPT="-T ./link.ld"
# Compile Flags
ORSC_CFLAGS="-g -O2 -DH264_ENC -DORSC_FW -nostdlib -Wl,-Map,mem.map -e 0x000 -I${ORSC_INC} -I${VSP_DRV_INC} -I${H264E_COMMON_INC} -I${H264E_ENC_INC}"
# ORSC_LDFLAGS="-L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lgcc -L/opt/or32-new/or32-elf/include/ -L/opt/or32-new/or32-elf/lib/ -lg -lm"
ORSC_LDFLAGS="-L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lgcc -L/opt/or32-new/or32-elf/lib/ -lg"
# Output File
OUT=h264enc



# Compile & Link
rm ${OUT}
or32-elf-gcc ${ORSC_CFLAGS} ${LINK_SCRIPT} ${COMPILE_SRC} ${ORSC_LDFLAGS} -o ${OUT} 2>&1 | tee log
or32-elf-objcopy -O binary ${OUT} ${OUT}.bin
or32-elf-objdump -t -d ${OUT} > ${OUT}.txt

#or32-elf-gcc -g -O0 -DNUM_RUNS=10000 -nostdlib -T ./link_rom.ld -e 0x100 -v -Wl,-Map,mem.map -v int.c or1200-utils.c printf.c uart.c lib-utils.c exceptions.c dhry.c crt0.S cache.S -o dhry_test -L/opt/or32-new/or32-elf/lib/boards/ordb2a/ -L/opt/or32-new/or32-elf/lib/ -L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lg -lgcc -lor32 -lboard -lg 2>&1 | tee log

#or32-elf-gcc -g -O0 -DNUM_RUNS=10000 -nostdlib -T ./link_rom.ld -e 0x100 -v -Wl,-Map,mem.map -v dhry.c crt0.S -o dhry_test -L/opt/or32-new/or32-elf/lib/boards/ordb2a/ -L/opt/or32-new/or32-elf/lib/ -L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -L/home/openrisc/soc-design/orpsocv2/sw/lib -lorpsoc -lg -lgcc -lor32 -lboard -lg 2>&1 | tee log
#or32-elf-gcc -g -O0 -DNUM_RUNS=100 -mboard=ordb2a -e 0x100 -v -Wl,-Map,mem.map -v int.c or1200-utils.c printf.c uart.c dhry.c -o dhry_test 2>&1 | tee log


######################################################################
sudo ln -s -f /usr/lib/libgmp.so.3.5.2 /usr/lib/libgmp.so.10
sudo ln -s -f /usr/lib/libmpc.so.2.0.0 /usr/lib/libmpc.so.2
sudo ln -s -f /usr/lib/libmpfr.so.4.0.0 /usr/lib/libmpfr.so.4

./../build/bin2txt -i ${OUT}.bin > tmp1.txt
sed -e '/0x/d' -e '/static/r tmp1.txt' ../build/${OUT}_or_fw.h > tmp2.txt
rm tmp1.txt
mv tmp2.txt ../build/${OUT}_or_fw.h


cd ../build/

######################################################################
