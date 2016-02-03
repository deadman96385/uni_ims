
common:=server/ylog.c server/file.c server/ylog.h server/command.c server/filter.c \
	client/ylog_cli.c benchmark/ylog_benchmark.c server/popen2.c server/socket.c server/common.c

C_FLAGS:=-Wall -Wunused-parameter -Werror -Wstrict-prototypes -Wclobbered \
	-Wmissing-field-initializers -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type \
	-Werror=non-virtual-dtor -Werror=address -Werror=sequence-point \
	-Werror=int-to-pointer-cast -Werror=pointer-to-int-cast \
	-g
ubuntu: $(common) server/os/ubuntu.c
	gcc $(C_FLAGS) -o ylog -pthread -lrt server/ylog.c -ldl
	gcc $(C_FLAGS) -o ylog_cli client/ylog_cli.c
	gcc $(C_FLAGS) -o ylog_benchmark benchmark/ylog_benchmark.c

android: $(common) server/os/android.c
	gcc -DANDROID $(C_FLAGS) -o ylog -pthread -lrt server/ylog.c
	gcc -DANDROID $(C_FLAGS) -o ylog_cli client/ylog_cli.c
	gcc -DANDROID $(C_FLAGS) -o ylog_benchmark benchmark/ylog_benchmark.c

clean:
	rm -rf ylog ylog_cli ylog_benchmark data/* sdcard/*
