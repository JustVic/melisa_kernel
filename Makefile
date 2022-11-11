ISO_FILE=kernel.iso

.PHONY: all
.PHONY: kernel
.PHONY: qemu
.PHONY: iso
.PHONY: clean

ifneq (, $(shell which grub2-mkrescue 2> /dev/null))
  GRUB_MKRESCUE = grub2-mkrescue
else ifneq (, $(shell which grub-mkrescue 2> /dev/null))
  GRUB_MKRESCUE = grub-mkrescue
else
    $(error "Cannot find grub-mkrescue or grub2-mkrescue")
endif

all: kernel

kernel:
	make -C kernel all


qemu: $(ISO_FILE)
	qemu-system-x86_64 -S -s -cdrom $(ISO_FILE) -serial stdio -m 1024 -d int -drive file=hdd.img,format=raw
#1024M


clean:
	make -C kernel clean
	rm -rf iso
	rm -rf $(ISO_FILE)

iso: $(ISO_FILE)

$(ISO_FILE): kernel
	mkdir -p iso/boot/grub
	cd ./user_programs/user2\(shell\)/; \
	bash ./build.sh; \
	cp ./user2 ../../iso/boot; \
	cd ../../
	cp grub.cfg iso/boot/grub/
	cp kernel/kernel iso/boot/
	cp ./disk.img ./iso/boot
	$(GRUB_MKRESCUE) -o $(ISO_FILE) iso
