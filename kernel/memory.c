#include "memory.h"
#include "print.h"
#include "debug.h"
#include "lib.h"
#include "stddef.h"
#include "stdbool.h"

static void free_region(uint64_t v, uint64_t e);

static struct FreeMemRegion free_mem_region[50];
static struct Page free_memory;
static uint64_t memory_end;
uint64_t page_map;
extern char _end;
uint64_t total_mem = 0;

void init_memory(struct FreeMemRegion free_mem_region[50], int free_region_count)
{

    for (int i = 0; i < free_region_count; i++) {
        uint64_t vstart = P2V(free_mem_region[i].address);
        uint64_t vend = vstart + free_mem_region[i].length;

        if (vstart > (uint64_t)&_end) {
            free_region(vstart, vend);
        }
        else if (vend > (uint64_t)&_end) {
            free_region((uint64_t)&_end, vend);
        }

		total_mem += free_mem_region[i].length;
    }

    memory_end = (uint64_t)free_memory.next+PAGE_SIZE;
}

uint64_t get_total_memory(void)
{
	return total_mem/1024/1024;
}

static void free_region(uint64_t v, uint64_t e)
{
    for (uint64_t start = PA_UP(v); start+PAGE_SIZE <= e; start += PAGE_SIZE) { 

        if (start+PAGE_SIZE <= 0xffff800030000000) { //0xffff800040000000)
           kfree(start);
        }
    }
}

void kfree(uint64_t v)
{
    ASSERT(v % PAGE_SIZE == 0);
    ASSERT(v >= (uint64_t) & _end);
    ASSERT(v+PAGE_SIZE <= 0xffff800030000000); //0xffff800040000000)

    struct Page *page_address = (struct Page*)v;
    page_address->next = free_memory.next;
    free_memory.next = page_address;
}

void* kalloc(void)
{
    struct Page *page_address = free_memory.next;

    if (page_address != NULL) {
        ASSERT((uint64_t)page_address % PAGE_SIZE == 0);
        ASSERT((uint64_t)page_address >= (uint64_t)&_end);
        ASSERT((uint64_t)page_address+PAGE_SIZE <= 0xffff800030000000); //0xffff800040000000)
	/*	printk("Free_memory: %x\n", free_memory);
		printk("Free_memory.next: %x\n", free_memory.next);
		printk("Page_address: %x\n", page_address);
		printk("Page_address->next: %x\n", page_address->next);*/

        free_memory.next = page_address->next;
    }

	return page_address;
}

static PDPTR find_pml4t_entry(uint64_t map, uint64_t v, int alloc, uint32_t attribute)
{
    PDPTR *map_entry = (PDPTR*)map;
    PDPTR pdptr = NULL;
    unsigned int index = (v >> 39) & 0x1FF;

    if ((uint64_t)map_entry[index] & PTE_P) {
        pdptr = (PDPTR)P2V(PDE_ADDR(map_entry[index]));
    }
    else if (alloc == 1) {
        pdptr = (PDPTR)kalloc();
        if (pdptr != NULL) {
            memset(pdptr, 0, PAGE_SIZE);
            map_entry[index] = (PDPTR)(V2P(pdptr) | attribute);
        }
    }

    return pdptr;
}

static PD find_pdpt_entry(uint64_t map, uint64_t v, int alloc, uint32_t attribute)
{
    PDPTR pdptr = NULL;
    PD pd = NULL;
    unsigned int index = (v >> 30) & 0x1FF;

    pdptr = find_pml4t_entry(map, v, alloc, attribute);
    if (pdptr == NULL)
        return NULL;

    if ((uint64_t)pdptr[index] & PTE_P) {
        pd = (PD)P2V(PDE_ADDR(pdptr[index]));
    }
    else if (alloc == 1) {
        pd = (PD)kalloc();
        if (pd != NULL) {
            memset(pd, 0, PAGE_SIZE);
            pdptr[index] = (PD)(V2P(pd) | attribute);
        }
    }

    return pd;
}

bool map_pages(uint64_t map, uint64_t v, uint64_t e, uint64_t pa, uint32_t attribute)
{
    uint64_t vstart = PA_DOWN(v);
    uint64_t vend = PA_UP(e);
    PD pd = NULL;
    unsigned int index;

    ASSERT(v < e);
    ASSERT(pa % PAGE_SIZE == 0);
    ASSERT(pa+vend-vstart <= 1024*1024*1024);

    do {
        pd = find_pdpt_entry(map, vstart, 1, attribute);
        if (pd == NULL) {
            return false;
        }

        index = (vstart >> 21) & 0x1FF;
        ASSERT(((uint64_t)pd[index] & PTE_P) == 0);

        pd[index] = (PDE)(pa | attribute | PTE_ENTRY);

        vstart += PAGE_SIZE;
        pa += PAGE_SIZE;
    } while (vstart + PAGE_SIZE <= vend);

    return true;
}

void switch_vm(uint64_t map)
{
    load_cr3(V2P(map));
}

uint64_t setup_kvm(void)
{
    uint64_t page_map = (uint64_t)kalloc();


    if (page_map != 0) {
        memset((void*)page_map, 0, PAGE_SIZE);
        if (!map_pages(page_map, KERNEL_BASE, /*P2V(0x40000000)*/memory_end, V2P(KERNEL_BASE), PTE_P|PTE_W)) {
            free_vm(page_map);
            page_map = 0;
        }
    }
    return page_map;

}

void init_kvm(void)
{
	uint64_t page_map = setup_kvm();
    ASSERT(page_map != 0);
    switch_vm(page_map);
    printk("memory manager is working now\n");
}

bool setup_uvm(uint64_t map, uint64_t start, int size)
{
    bool status = false;
    void *page = kalloc();

    if (page != NULL) {
        memset(page, 0, PAGE_SIZE);
        status = map_pages(map, 0x400000, 0x400000+PAGE_SIZE, V2P(page), PTE_P|PTE_W|PTE_U);
        if (status == true) {

            memcpy(page, (void*)start, size);

        }
        else {
            kfree((uint64_t)page);
            free_vm(map);
        }
    }

    return status;
}

void free_pages(uint64_t map, uint64_t vstart, uint64_t vend)
{
    unsigned int index; 

    ASSERT(vstart % PAGE_SIZE == 0);
    ASSERT(vend % PAGE_SIZE == 0);

    do {
        PD pd = find_pdpt_entry(map, vstart, 0, 0);

        if (pd != NULL) {
            index = (vstart >> 21) & 0x1FF;
            if (pd[index] & PTE_P) {        
                kfree(P2V(PTE_ADDR(pd[index])));
                pd[index] = 0;
            }
        }

        vstart += PAGE_SIZE;
    } while (vstart+PAGE_SIZE <= vend);
}

static void free_pdt(uint64_t map)
{
    PDPTR *map_entry = (PDPTR*)map;

    for (int i = 0; i < 512; i++) {
        if ((uint64_t)map_entry[i] & PTE_P) {
            PD *pdptr = (PD*)P2V(PDE_ADDR(map_entry[i]));

            for (int j = 0; j < 512; j++) {
                if ((uint64_t)pdptr[j] & PTE_P) {
                    kfree(P2V(PDE_ADDR(pdptr[j])));
                    pdptr[j] = 0;
                }
            }
        }
    }
}

static void free_pdpt(uint64_t map)
{
    PDPTR *map_entry = (PDPTR*)map;

    for (int i = 0; i < 512; i++) {
        if ((uint64_t)map_entry[i] & PTE_P) {
            kfree(P2V(PDE_ADDR(map_entry[i])));
            map_entry[i] = 0;
        }
    }
}

static void free_pml4t(uint64_t map)
{
    kfree(map);
}

void free_vm(uint64_t map)
{
    free_pages(map, 0x400000, 0x400000+PAGE_SIZE);
    free_pdt(map);
    free_pdpt(map);
    free_pml4t(map);
}

bool copy_uvm(uint64_t dst_map, uint64_t src_map, int size)
{
    bool status = false;
    unsigned int index;
    PD pd = NULL;
    uint64_t start;

    void *page = kalloc();
    if (page != NULL) {
        memset(page, 0, PAGE_SIZE);
        status = map_pages(dst_map, 0x400000, 0x400000+PAGE_SIZE, V2P(page), PTE_P|PTE_W|PTE_U);

        if (status == true) {
            pd = find_pdpt_entry(src_map, 0x400000, 0, 0);
            if (pd == NULL) {
                free_vm(dst_map);
                return false;
            }

            index = (0x400000U >> 21) & 0x1FF;
            ASSERT(((uint64_t)pd[index] & PTE_P) == 1);
            start = P2V(PTE_ADDR(pd[index]));
            memcpy(page, (void*)start, size);
        }
        else {
            kfree((uint64_t)page);
            free_vm(dst_map);
        }
    }

    return status;
}
