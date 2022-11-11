CC=gcc
SHARED_FLAGS = -fno-stack-protector -fno-builtin -nostdlib -ffreestanding -g \
               -I. -MMD -mno-red-zone -mcmodel=large -fno-pie
CFLAGS = $(SHARED_FLAGS)
ASFLAGS = $(SHARED_FLAGS) -Wa,--divide

OBJS := boot32.o
OBJS += boot64.o
OBJS += lib.o
OBJS += liba.o
OBJS += print.o
OBJS += keyboard.o
OBJS += trapa.o
OBJS += trap.o
OBJS += debug.o
OBJS += memory.o
OBJS += proc.o
OBJS += syscall.o
OBJS += file.o
OBJS += ports.o
OBJS += ata.o
OBJS += kernel.o

DFILES = $(patsubst %.o,%.d,$(OBJS))

all: kernel

kernel: $(OBJS) kernel.ld Makefile
	$(CC) $(CFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o $@ $(OBJS)

clean:
	find -name "*~" -delete
	rm -rf $(OBJS) $(DFILES) kernel

$(OBJS): Makefile
-include $(DFILES)
