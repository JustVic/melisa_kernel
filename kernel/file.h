#ifndef _FILE_H_
#define _FILE_H_

#include "stdint.h"

struct BPB {
	uint8_t jump[3];
	uint8_t oem[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t fat_count;
	uint16_t root_entry_count;
	uint16_t sector_count;
	uint8_t media_type;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t head_count;
	uint32_t hidden_sector_count;
	uint32_t large_sector_count;
	uint8_t drive_number;
	uint8_t flags;
	uint8_t signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t file_system[8];
} __attribute__((packed));

struct DirEntry {
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attributes;
	uint8_t reserved;
	uint8_t create_ms;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t attr_index;
	uint16_t m_time;
	uint16_t m_date;
	uint16_t cluster_index;
	uint32_t file_size;
} __attribute__((packed));

struct FCB {
	char name[8];
	char ext[3];
	uint32_t cluster_index;
	uint32_t dir_index;
	uint32_t file_size;
	int count;
};

struct FileDesc {
	struct FCB *fcb;
	uint32_t position;
	int count;
};

struct Process;

//#define FS_BASE 0x212000//0x30000000
#define ENTRY_EMPTY 0
#define ENTRY_DELETED 0xe5

void init_fs(int* filesystem_address);
void close_file(struct Process *proc, int fd);
int read_file(struct Process *proc, int fd, void *buffer, uint32_t size);
int open_file(struct Process *proc, char *path_name);
uint32_t get_file_size(struct Process *proc, int fd);
int read_root_file(char *buffer);
int read_root_directory(char *buffer);

#endif
