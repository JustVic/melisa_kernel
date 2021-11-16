as -o syscall.o syscall.S
as -o lib.o liba.S
ar -rcs lib.a syscall.o lib.o print.o
