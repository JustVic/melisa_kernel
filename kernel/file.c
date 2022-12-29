#include "proc.h"
#include "file.h"
#include "memory.h"
#include "print.h"
#include "lib.h"
#include "debug.h"

static int FS_BASE  = 0;//0x212000;
static struct FCB *fcb_table;
static struct FileDesc *file_desc_table;

static struct BPB* get_fs_bpb(void)
{
    uint32_t lba = *(uint32_t*)(P2V(FS_BASE) + 0x1be + 8);

    return (struct BPB*)P2V(FS_BASE + lba * 512);
}

static uint16_t *get_fat_table(void)
{
    struct BPB* p = get_fs_bpb();
    uint32_t offset = (uint32_t)p->reserved_sector_count * p->bytes_per_sector;
    printk(" sECTOR_SIZE:%d; ",  p->bytes_per_sector);
    printk(" rESERVED_SECTOR:%d; ",  (uint32_t)p->reserved_sector_count);




    return (uint16_t *)((uint8_t*)p + offset);
}

static uint16_t get_cluster_value(uint32_t cluster_index)
{
    uint16_t * fat_table = get_fat_table();

    return fat_table[cluster_index];
}

static uint32_t get_cluster_size(void)
{
    struct BPB* bpb = get_fs_bpb();

    return (uint32_t)bpb->bytes_per_sector * bpb->sectors_per_cluster;
}

static uint32_t get_cluster_offset(uint32_t index)
{
    uint32_t res_size;
    uint32_t fat_size;
    uint32_t dir_size;

    ASSERT(index >= 2);

    struct BPB* p = get_fs_bpb();

    res_size = (uint32_t)p->reserved_sector_count * p->bytes_per_sector;
    fat_size = (uint32_t)p->fat_count * p->sectors_per_fat * p->bytes_per_sector;
    dir_size = p->root_entry_count * sizeof(struct DirEntry);

    return res_size + fat_size + dir_size +
        (index - 2) * ((uint32_t)p->sectors_per_cluster * p->bytes_per_sector);
}

static uint32_t get_root_directory_count(void)
{
    struct BPB* bpb = get_fs_bpb();

    return bpb->root_entry_count;
}

static struct DirEntry *get_root_directory(void)
{
    struct BPB* p; 
    uint32_t offset; 

    p = get_fs_bpb();
    offset = (p->reserved_sector_count + (uint32_t)p->fat_count * p->sectors_per_fat) * p->bytes_per_sector;

    return (struct DirEntry *)((uint8_t*)p + offset);
}

static bool is_file_name_equal(struct DirEntry *dir_entry, char *name, char *ext)
{
    bool status = false;

    if (memcmp(dir_entry->name, name, 8) == 0 &&
        memcmp(dir_entry->ext, ext, 3) == 0) {
        status = true;
    }

    return status;
}

static bool split_path(char *path, char *name, char *ext)
{
    int i;

    for (i = 0; i < 8 && path[i] != '.' && path[i] != '\0'; i++) {
        if (path[i] == '/') {
            return false;
        }

        name[i] = path[i];
    }

    if (path[i] == '.') {
        i++;
        
        for (int j = 0; j < 3 && path[i] != '\0'; i++, j++) {
            if (path[i] == '/') {
                return false;
            }

            ext[j] = path[i];
        }
    }

    if (path[i] != '\0') {        
        return false;
    }

    return true;
}

static uint32_t search_file(char *path)
{
    char name[8] = {"        "};
    char ext[3] =  {"   "};
    uint32_t root_entry_count;
    struct DirEntry *dir_entry; 

    bool status = split_path(path, name, ext);

    if (status == true) {
        root_entry_count = get_root_directory_count();
        dir_entry = get_root_directory();
        
        for (uint32_t i = 0; i < root_entry_count; i++) {
            if (dir_entry[i].name[0] == ENTRY_EMPTY || dir_entry[i].name[0] == ENTRY_DELETED)
                continue;

            if (dir_entry[i].attributes == 0xf) {
                continue;
            }

            if (is_file_name_equal(&dir_entry[i], name, ext)) {
                return i;
            }
        }
    }

    return 0xffffffff;
}

static uint32_t get_fcb(uint32_t index)
{
    struct DirEntry *dir_table;

    if (fcb_table[index].count == 0) {        
        dir_table = get_root_directory();
        fcb_table[index].dir_index = index;
        fcb_table[index].file_size = dir_table[index].file_size;
        fcb_table[index].cluster_index = dir_table[index].cluster_index;
        memcpy(&fcb_table[index].name, &dir_table[index].name, 8);
        memcpy(&fcb_table[index].ext, &dir_table[index].ext, 3);
    }
    
    fcb_table[index].count++;

    return index;
}

static void put_fcb(struct FCB *fcb)
{
    ASSERT(fcb->count > 0);
    fcb->count--;
}

int open_file(struct Process *proc, char *path_name)
{
    int fd = -1;
    int file_desc_index = -1;
    uint32_t entry_index;
    uint32_t fcb_index;
    
    for (int i = 0; i < 100; i++) {
        if (proc->file[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        return -1;
    }

    for (int i = 0; i < PAGE_SIZE / sizeof(struct FileDesc); i++) {
        if (file_desc_table[i].fcb == NULL) {
            file_desc_index = i;
            break;
        }
    }

    if (file_desc_index == -1) {
        return -1;
    }

    entry_index = search_file(path_name);
    if (entry_index == 0xffffffff) {
        return -1;
    }
    
    fcb_index = get_fcb(entry_index);
    
    memset(&file_desc_table[file_desc_index], 0, sizeof(struct FileDesc));
    file_desc_table[file_desc_index].fcb = &fcb_table[fcb_index];
    file_desc_table[file_desc_index].count = 1;
    proc->file[fd] = &file_desc_table[file_desc_index];
    
    return fd;
}

static uint32_t read_raw_data(uint32_t cluster_index, char *buffer, uint32_t position, uint32_t size)
{
    uint32_t read_size = 0;
    uint32_t index = cluster_index;
    uint32_t cluster_size = get_cluster_size(); 
    uint32_t count = position / cluster_size;
    printk("Position:%d; ", position);
    uint32_t offset = position % cluster_size;

    for (uint32_t i = 0; i < count; i++) {
        index = get_cluster_value(index);
        ASSERT(index < 0xfff7);
    }

    struct BPB* bpb =  get_fs_bpb();
    char *data;

    if (offset != 0) {
        read_size = (offset + size) <= cluster_size ? size : (cluster_size - offset);
        data = (char*)((uint64_t)bpb + get_cluster_offset(index));
        memcpy(buffer, data + offset, read_size);
        buffer += read_size;
        index = get_cluster_value(index);
    }

    while (read_size < size && index < 0xfff7) {
	printk("WHILE");
	printk(" read_size: %d; ", read_size);
	printk(" size: %d; ", size);
	printk(" index: %d; ", index);

        data = (char*)((uint64_t)bpb + get_cluster_offset(index));

        if (read_size + cluster_size >= size) {
            memcpy(buffer, data, size - read_size);
            read_size = size;
            break;     
        }   
              
        memcpy(buffer, data, cluster_size);
        buffer += cluster_size;
        read_size += cluster_size;
        index = get_cluster_value(index);
    }
    
    return read_size;
}

int read_file(struct Process *proc, int fd, void *buffer, uint32_t size)
{
    uint32_t position = proc->file[fd]->position;
    uint32_t file_size = proc->file[fd]->fcb->file_size;
    uint32_t read_size;

    if (position + size > file_size) {
        return -1;
    }

    read_size = read_raw_data(proc->file[fd]->fcb->cluster_index, buffer, position, size);
    proc->file[fd]->position += read_size;
    
    return read_size;
}

void close_file(struct Process *proc, int fd)
{
    put_fcb(proc->file[fd]->fcb);
    proc->file[fd]->count--;

    if (proc->file[fd]->count == 0) {
        proc->file[fd]->fcb = NULL;
    }

    proc->file[fd] = NULL;
}

uint32_t get_file_size(struct Process *proc, int fd)
{
    return proc->file[fd]->fcb->file_size;
}

int read_root_directory(char *buffer)
{
	struct DirEntry *dir_entry = get_root_directory();
	uint32_t count = get_root_directory_count();

	memcpy(buffer, dir_entry, count * sizeof(struct DirEntry));

	return count;
}

static bool init_fcb(void)
{
    fcb_table = (struct FCB*)kalloc();

    if (fcb_table == NULL) {
	    return false;
    }

    memset(fcb_table, 0, PAGE_SIZE);

    return true;
}

static bool init_file_desc(void)
{
    file_desc_table = (struct FileDesc*)kalloc();

    if (file_desc_table == NULL) {
	    return false;
    }

    memset(file_desc_table, 0, PAGE_SIZE);

    return true;
}

void init_fs(int* filesystem_address)
{
	FS_BASE = *filesystem_address;

    uint8_t *p = (uint8_t*)get_fs_bpb();

    if (p[0x1fe] != 0x55 || p[0x1ff] != 0xaa) {
        printk("invalid signature\n");
        ASSERT(0);
    }

    ASSERT(init_fcb());
    ASSERT(init_file_desc());
}

