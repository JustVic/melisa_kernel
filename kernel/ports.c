#include <stdint.h>
#include "ports.h"

unsigned char port_inb (unsigned short port) {
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_outb (unsigned short port, unsigned char data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

uint16_t port_inw (uint16_t port) {
    uint16_t result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_outw (unsigned short port, unsigned short data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}

void port_outl(uint32_t port, uint32_t value) {
	   __asm__ __volatile__("outl %%eax,%%dx"::"d" (port), "a" (value));
};

uint32_t port_inl(uint32_t port) {
    uint32_t result;
    __asm__ __volatile__("inl %%dx,%%eax":"=a" (result):"d"(port));
    return result;
}
