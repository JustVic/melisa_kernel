#ifndef _LIB_H_
#define _LIB_H_

#include "stdint.h"

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

#define ENTRY_AVAILABLE 0
#define ENTRY_DELETED 0xe5

int printf(const char *format, ...);
void sleepu(uint64_t ticks);
void exitu(void);
void waitu(int pid);
int open_file(char *name);
int read_file(int fd, void *buffer, uint32_t size);
void close_file(int fd);
int get_file_size(int fd);
int fork(void);
void exec(char *name);
int read_root_directory(void *buffer);

unsigned char keyboard_readu(void);
void memset(void* buffer, char value, int size);
void memmove(void* dst, void* src, int size);
void memcpy(void* dst, void* src, int size);
int memcmp(void* src1, void* src2, int size);
int get_total_memoryu(void);

#endif
