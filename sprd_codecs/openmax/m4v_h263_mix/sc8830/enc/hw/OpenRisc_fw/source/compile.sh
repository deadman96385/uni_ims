
# Include Headers
ORSC_INC=./openrisc/inc/
VSP_DRV_INC=./vsp_drv/
MP4E_COMMON_INC=./mp4_encoder/code/inc/common/
MP4E_ENC_INC=./mp4_encoder/code/inc/encoder/
MP4E_DEC_INC=./mp4_encoder/code/inc/decoder/
# Source Code
ORSC_SRC=./openrisc/src/*.c
#ORSC_ROM=./openrisc/src/*.S
MP4E_ENC_SRC=./mp4_encoder/code/src/encoder/*.c
MP4E_DEC_SRC=./mp4_encoder/code/src/decoder/*.c
MP4E_COM_SRC=./mp4_encoder/code/src/common/*.c
#MP4E_MAIN=./mp4_encoder/simulation/vc/WinPrj/mp4enc_main.c
MAIN=./main.c
#COMPILE_SRC="${ORSC_SRC} ${ORSC_ROM} ${MP4E_ENC_SRC} ${MP4E_COM_SRC} ${MP4E_MAIN} ${MAIN}"
COMPILE_SRC="${MP4E_ENC_SRC} ${MP4E_COM_SRC} ${MAIN}"
# Memory Link Script
LINK_SCRIPT="-T ./link.ld"
# Compile Flags
ORSC_CFLAGS="-g -O2 -DMPEG4_ENC -DORSC_FW -nostdlib -Wl,-Map,mem.map -e 0x00000000 -I${ORSC_INC} -I${VSP_DRV_INC} -I${MP4E_COMMON_INC} -I${MP4E_ENC_INC} -I${MP4E_DEC_INC}"
ORSC_LDFLAGS="-L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lgcc -L/opt/or32-new/or32-elf/include/ -L/opt/or32-new/or32-elf/lib/ -lg -lm"
# Output File
OUT=mp4e


# Compile & Link
rm ${OUT}
or32-elf-gcc ${ORSC_CFLAGS} ${LINK_SCRIPT} ${COMPILE_SRC} ${ORSC_LDFLAGS} -o ${OUT} 2>&1 | tee log
or32-elf-objcopy -O binary ${OUT} ${OUT}.bin
or32-elf-objdump -t -d ${OUT} > ${OUT}.txt

#or32-elf-gcc -g -O0 -DNUM_RUNS=10000 -nostdlib -T ./link_rom.ld -e 0x100 -v -Wl,-Map,mem.map -v int.c or1200-utils.c printf.c uart.c lib-utils.c exceptions.c dhry.c crt0.S cache.S -o dhry_test -L/opt/or32-new/or32-elf/lib/boards/ordb2a/ -L/opt/or32-new/or32-elf/lib/ -L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -lg -lgcc -lor32 -lboard -lg 2>&1 | tee log

#or32-elf-gcc -g -O0 -DNUM_RUNS=10000 -nostdlib -T ./link_rom.ld -e 0x100 -v -Wl,-Map,mem.map -v dhry.c crt0.S -o dhry_test -L/opt/or32-new/or32-elf/lib/boards/ordb2a/ -L/opt/or32-new/or32-elf/lib/ -L/opt/or32-new/lib/gcc/or32-elf/4.5.1-or32-1.0rc4/ -L/home/openrisc/soc-design/orpsocv2/sw/lib -lorpsoc -lg -lgcc -lor32 -lboard -lg 2>&1 | tee log
#or32-elf-gcc -g -O0 -DNUM_RUNS=100 -mboard=ordb2a -e 0x100 -v -Wl,-Map,mem.map -v int.c or1200-utils.c printf.c uart.c dhry.c -o dhry_test 2>&1 | tee log

