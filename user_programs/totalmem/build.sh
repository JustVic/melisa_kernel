
as -o start.o start.S
gcc -std=c99 -mcmodel=large -ffreestanding -fno-stack-protector -mno-red-zone -c main.c
ld -nostdlib -Tlink.lds -o user_tmp start.o main.o lib.a
objcopy -O binary user_tmp totalmem
