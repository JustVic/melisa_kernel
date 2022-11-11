
#include <stdint.h>

unsigned char port_inb (unsigned short port);
void port_outb (unsigned short port, unsigned char data);

uint16_t port_inw (uint16_t port);
void port_outw (unsigned short port, unsigned short data);

uint32_t port_inl(uint32_t port);
void port_outl(uint32_t port, uint32_t value);
