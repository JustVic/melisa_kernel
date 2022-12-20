#include "multiboot2.h"
//#include "stdarg.h"
#include "print.h"
#include "trap.h"
#include "memory.h"
#include "proc.h"
#include "syscall.h"

#include "fs/pparser.h"
#include "fs/files.h"
#include "disk/disc.h"

extern char bss_start;
extern char bss_end;


int parse_multiboot(unsigned long addr, unsigned long magic, struct
					 FreeMemRegion free_mem_region[50], int *free_region_count,
					 int* filesystem_address, int* user_process_address)
{
	int module_number = 0;

	addr += 0xffff800000000000;
	if ((int)magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		printk ("Invalid magic number: 0x%x\n", (unsigned) magic);
		return;
	}

	if (addr & 7)
    {
		printk ("Unaligned mbi: 0x%x\n", addr);
		return;
    }

	struct multiboot_tag *tag;
	unsigned size;

	size = *(unsigned *) (addr);

	printk ("Announced mbi size 0x%d\n", size);


	  for (tag = (struct multiboot_tag *) (addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag 
                                       + ((tag->size + 7) & ~7)))
    {
      //printk ("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
      switch (tag->type)
      {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
          /*printk ("Command line = %s\n",
                  ((struct multiboot_tag_string *) tag)->string);*/
          break;
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
          /*printk ("Boot loader name = %s\n",
                  ((struct multiboot_tag_string *) tag)->string);*/
          break;
        case MULTIBOOT_TAG_TYPE_MODULE:
          printk ("Module at 0x%x-0x%x. Command line %s\n",
                  ((struct multiboot_tag_module *) tag)->mod_start,
                  ((struct multiboot_tag_module *) tag)->mod_end,
                  ((struct multiboot_tag_module *) tag)->cmdline);

		  if(module_number == 0)
			  *user_process_address = ((struct multiboot_tag_module *) tag)->mod_start;
		  if(module_number == 1)
			  *filesystem_address = ((struct multiboot_tag_module *) tag)->mod_start;

		  module_number++;

          break;
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
          /*printk ("mem_lower = %uKB, mem_upper = %uKB\n",
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
                  ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);*/
          break;
        case MULTIBOOT_TAG_TYPE_BOOTDEV:
          /*printk ("Boot device 0x%x,%u,%u\n",
                  ((struct multiboot_tag_bootdev *) tag)->biosdev,
                  ((struct multiboot_tag_bootdev *) tag)->slice,
                  ((struct multiboot_tag_bootdev *) tag)->part);*/
          break;
        case MULTIBOOT_TAG_TYPE_MMAP:
          {
            multiboot_memory_map_t *mmap;

            printk ("mmap\n");

			int i = 0;
            for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
                 (multiboot_uint8_t *) mmap 
                   < (multiboot_uint8_t *) tag + tag->size;
                 mmap = (multiboot_memory_map_t *) 
                   ((unsigned long) mmap
                    + ((struct multiboot_tag_mmap *) tag)->entry_size))
			{
				printk (" base_addr = 0x%x,"
                      " length = 0x%x, type = %d, i = %d\n",
                      (unsigned) mmap->addr,
                      (unsigned) mmap->len,
                      (unsigned) mmap->type,
					  i);

				free_mem_region[*free_region_count].address = (unsigned)mmap->addr;

				free_mem_region[*free_region_count].length = (unsigned)mmap->len;

				*free_region_count += 1;

				i++;
			}

          }

          break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
          {
               break;
          }

        }
    }

  tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag 
                                  + ((tag->size + 7) & ~7));
  printk ("Total mbi size 0x%x\n", (unsigned) tag - addr);

}

int cmain(unsigned long addr, unsigned long magic)
{
	struct FreeMemRegion free_mem_region[50];
	int free_region_count = 0;
	int mod_address = 0;
	int user_process_address = 0;
	int filesystem_address = 0;

	parse_multiboot(addr, magic, free_mem_region, &free_region_count,
					&filesystem_address, &user_process_address);

	uint64_t size = (uint64_t)&bss_end-(uint64_t)&bss_start;
	memset(&bss_start, 0, size);

	init_idt();

	init_memory(free_mem_region, free_region_count);

	init_kvm();

	init_system_call();

	//init old in RAM fat filesystem
	init_fs(&filesystem_address);

	init_process(&user_process_address);

	printk("\nAll Systems Online\n");

    	// Initialize new filesystems
	fs_init();

	disk_search_and_init();

	int fd = fopen("0:/LS", "r");

	if(fd)
	{
		struct file_stat s;
		fstat(fd, &s);
		printk("FILESIZE: %d\n", s.filesize);

		fclose(fd);
	}
}
