#include "io/io.h"

int disk_read_sector(int lba, int total, void* buf)
{
    if (buf == 0 || total <= 0) {
        return -1;
    }

    unsigned short* ptr = (unsigned short*)buf;

    for (int sector = 0; sector < total; sector++) {
        unsigned int current_lba = lba + sector;

        outb(0x1F6, (unsigned char)((current_lba >> 24) | 0xE0));
        outb(0x1F2, 1);
        outb(0x1F3, (unsigned char)(current_lba & 0xFF));
        outb(0x1F4, (unsigned char)((current_lba >> 8) & 0xFF));
        outb(0x1F5, (unsigned char)((current_lba >> 16) & 0xFF));
        outb(0x1F7, 0x20);

        int timeout = 100000;
        while (timeout > 0) {
            unsigned char status = insb(0x1F7);
            if (status & 0x08) {
                break;
            }
            if (status & 0x21) {
                return -1;
            }
            timeout--;
        }

        if (timeout == 0) {
            return -1;
        }

        for (int i = 0; i < 256; i++) {
            *ptr = insw(0x1F0);
            ptr++;
        }
    }

    return 0;
}
