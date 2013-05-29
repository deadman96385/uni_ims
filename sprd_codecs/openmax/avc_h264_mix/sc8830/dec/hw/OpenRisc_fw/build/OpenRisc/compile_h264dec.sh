######################################################################
export PATH=$PATH:/opt/or32-new/bin:/opt/or1ksim/bin
sudo ln -s -f /usr/lib/or32_lib/libgmp.so.10.0.1 /usr/lib/libgmp.so.10
sudo ln -s -f /usr/lib/or32_lib/libmpc.so.2.0.0 /usr/lib/libmpc.so.2
sudo ln -s -f /usr/lib/or32_lib/libmpfr.so.4.0.1 /usr/lib/libmpfr.so.4
ldd /opt/or32-new/libexec/gcc/or32-elf/4.5.1-or32-1.0rc4/cc1

######################################################################
# Machine flags - uncomment one or create custom combination of flags
# All hardware flags
#MARCH_FLAGS ?=-mhard-mul -mhard-div -mhard-float
# Hardware integer arith, soft float
#MARCH_FLAGS ?=-mhard-mul -mhard-div -msoft-float
# FPGA default - only hardware multiply
#MARCH_FLAGS ?=-mhard-mul -msoft-div -msoft-float
# All software div, mul and FPU
#MARCH_FLAGS ?=-msoft-mul -msoft-div -msoft-float

#R32_CFLAGS ?=-ggdb -nostdlib -mnewlib -O2 $(MARCH_FLAGS) $(INCLUDE_FLAGS) -I$(SW_ROOT)/lib/include
#OR32_LDFLAGS ?=-L$(SW_ROOT)/lib -lorpsoc -lgcc -T ./link.ld -e 0x100 

#COMPILE_SRC =
ORSC_LDFLAGS="-L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lgcc -L/opt/or32-new/or32-elf/include/ -L/opt/or32-new/or32-elf/lib/ -lg -lm"

OUT=h264dec
SRC=../../source
DST=../../../include

or32-elf-gcc -g -O2 -nostdlib -T ${SRC}/link_main.ld -e 0x00000000 -Wl,-Map,mem_main.map -v \
	${SRC}/main.c \
	${SRC}/h264dec_main.c \
	${SRC}/h264dec_table.c \
	${SRC}/h264dec_slice.c \
	${SRC}/h264dec_parset.c \
	${SRC}/h264dec_malloc.c \
	${SRC}/h264dec_interface.c \
	${SRC}/h264dec_init.c \
	${SRC}/h264dec_image.c \
	${SRC}/h264dec_header.c \
	${SRC}/h264dec_global.c \
	${SRC}/h264dec_ctx_table.c \
	${SRC}/h264dec_context_init.c \
	${SRC}/h264dec_buffer.c \
	${SRC}/h264dec_bitstream.c \
	${SRC}/h264dec_biaridecod.c \
	${ORSC_LDFLAGS} -o ${OUT}.elf 2>&1 | tee log_main

or32-elf-objcopy -O binary ${OUT}.elf ${OUT}.bin
#or32-elf-objdump -D h264dec.elf > h264dec.dis

######################################################################
sudo ln -s -f /usr/lib/libgmp.so.3.5.2 /usr/lib/libgmp.so.10
sudo ln -s -f /usr/lib/libmpc.so.2.0.0 /usr/lib/libmpc.so.2
sudo ln -s -f /usr/lib/libmpfr.so.4.0.0 /usr/lib/libmpfr.so.4

./bin2txt -i ${OUT}.bin > tmp1.txt
sed -e '/0x/d' -e '/static/r tmp1.txt' ${DST}/${OUT}_or_fw.h > tmp2.txt
mv tmp2.txt ${DST}/${OUT}_or_fw.h

rm tmp1.txt
rm log_main
rm mem_main.map
rm h264dec.bin
rm h264dec.elf

######################################################################
