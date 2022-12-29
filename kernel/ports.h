
#include <stdint.h>

unsigned char inb (unsigned short port);
void outb (unsigned short port, unsigned char data);

uint16_t inw (uint16_t port);
void outw (unsigned short port, unsigned short data);

uint32_t inl(uint32_t port);
void outl(uint32_t port, uint32_t value);
