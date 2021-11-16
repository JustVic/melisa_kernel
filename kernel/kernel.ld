ENTRY(_start)

KERNEL_VMA = 0xffff800000000000;

SECTIONS
{
  . = 2M;
  _kernel_physical_start = .;

  .boottext :
    {
      boot32.o (.multiboot)
      boot32.o (.text)
    }
  .bootrodata :
    {
      boot32.o (.rodata)
    }
  .bootdata :
    {
      boot32.o (.data)
    }
  .bootbss :
    {
      boot32.o (.bss)
      boot32.o (COMMON)
    }

  . = ALIGN(0x1000);
  _boot_end = .;

  . += KERNEL_VMA;

  .text : AT(ADDR(.text) - KERNEL_VMA)
    {
       *(.multiboot)
       *(.text)
    }
  .rodata : AT(ADDR(.rodata) - KERNEL_VMA)
    {
       *(.rodata)
    }
  .data : AT(ADDR(.data) - KERNEL_VMA)
    {
       *(.data)
    }
  .bss : AT(ADDR(.bss) - KERNEL_VMA)
    {
		PROVIDE(bss_start = .);
       *(.bss)
		PROVIDE(bss_end = .);
       *(COMMON)
    }

	PROVIDE(_end = .);
}
