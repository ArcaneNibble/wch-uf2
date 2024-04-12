#include <stdint.h>

typedef struct USBD_descriptor {
    uint32_t addr_tx;
    uint32_t count_tx;
    uint32_t addr_rx;
    uint32_t count_rx;
} USBD_descriptor;

// USBD memory allocation:
// WARNING THIS IS A 16-bit MEMORY LMAO
// +0x00
//      descriptors (two sets of two endpoints)
// +0x20
//      EP 0 OUT (8 bytes)
// +0x30
//      EP 0 IN (8 bytes)
// +0x40
//      EP 1 OUT (64 bytes)
// +0xc0
//      EP 1 IN (64 bytes)
// +0x140
//      <unused>
// +0x200
//      <256-byte flash page buffer>

#define USB_DESCS           ((USBD_descriptor*)0x40006000)
#define USB_EP0_OUT(offs)   (*(volatile uint32_t *)(0x40006020 + 2 * (offs)))
#define USB_EP0_IN(offs)    (*(volatile uint32_t *)(0x40006030 + 2 * (offs)))
#define USB_EP1_OUT(offs)   (*(volatile uint32_t *)(0x40006040 + 2 * (offs)))
#define USB_EP1_IN(offs)    (*(volatile uint32_t *)(0x400060C0 + 2 * (offs)))

#define R32_RCC_CTLR        (*(volatile uint32_t*)0x40021000)
#define R32_RCC_CFGR0       (*(volatile uint32_t*)0x40021004)
#define R32_RCC_APB2PCENR   (*(volatile uint32_t*)0x40021018)
#define R32_RCC_APB1PCENR   (*(volatile uint32_t*)0x4002101C)
#define R32_GPIOA_CFGLR     (*(volatile uint32_t*)0x40010800)
#define R32_GPIOA_CFGHR     (*(volatile uint32_t*)0x40010804)
#define R32_GPIOA_BSHR      (*(volatile uint32_t*)0x40010810)

#define R16_USBD_EPR(n)     (*(volatile uint16_t*)(0x40005C00 + 4 * (n)))
#define R16_USBD_CNTR       (*(volatile uint16_t*)0x40005C40)
#define R16_USBD_ISTR       (*(volatile uint16_t*)0x40005C44)
#define R16_USBD_DADDR      (*(volatile uint16_t*)0x40005C4C)

#define R32_EXTEN_CTR       (*(volatile uint32_t*)0x40023800)

int main(void) {
    // PLL setup: system clock 96 MHz
    R32_EXTEN_CTR |= (1 << 4);  // sneaky div2
    R32_RCC_CFGR0 = (R32_RCC_CFGR0 & ~0xff0000) | (0b01101000 << 16);
    R32_RCC_CTLR |= (1 << 24);
    while (!(R32_RCC_CTLR & (1 << 25))) {}
    R32_RCC_CFGR0 = (R32_RCC_CFGR0 & ~0b11) | 0b10;
    while ((R32_RCC_CFGR0 & 0b1100) != 0b1000) {}

    // As recommended in the manual, output low on these pins before enabling USB
    R32_RCC_APB2PCENR |= (1 << 2);
    R32_GPIOA_CFGHR = (R32_GPIOA_CFGHR & ~(0xff << 12)) | (0b00100010 << 12);
    R32_GPIOA_BSHR = (1 << 27) | (1 << 28);

    // Enable USB
    R32_RCC_APB1PCENR |= (1 << 23);
    R16_USBD_CNTR &= ~(1 << 1);
    USB_DESCS[0].addr_tx = 0x30 / 2;
    USB_DESCS[0].addr_rx = 0x20 / 2;
    USB_DESCS[0].count_rx = (8 << 10);
    USB_DESCS[1].addr_tx = 0xc0 / 2;
    USB_DESCS[1].addr_rx = 0x40 / 2;
    USB_DESCS[1].count_rx = (2 << 10) | (1 << 15);
    // XXX because the table is at offset 0 don't bother writing this

    // Attach USB
    R16_USBD_CNTR &= ~(1 << 0);
    R32_EXTEN_CTR |= (1 << 1);

    uint32_t need_to_set_addr = 0;

    // R32_GPIOA_CFGLR = (R32_GPIOA_CFGLR & ~0xf) | (0b0010 << 0);
    while (1) {
        uint32_t usb_int_status = R16_USBD_ISTR;
        if (usb_int_status & (1 << 10)) {
            // RESET
            R16_USBD_EPR(0) = (0b11 << 12) | (0b01 << 9) | (0b01 << 4);
            R16_USBD_DADDR = 0x80;
        } else if (usb_int_status & (1 << 15)) {
            if ((usb_int_status & 0x1f) == 0x10) {
                uint32_t ep0_status = R16_USBD_EPR(0);
                if (ep0_status & (1 << 11)) {
                    // setup, STAT_RX and STAT_TX both now NAK
                    uint16_t bRequest_bmRequestType = USB_EP0_OUT(0);
                    if (bRequest_bmRequestType == 0x0500) {
                        // SET_ADDRESS
                        need_to_set_addr = USB_EP0_OUT(2);
                        USB_DESCS[0].count_tx = 0;
                        R16_USBD_EPR(0) = (0b01 << 9) | (0b11 << 12) | (0b01 << 4);  // STALL for OUT, ACK for IN
                    } else {
                        // unknown
                        R16_USBD_EPR(0) = (0b01 << 9) | (0b11 << 12) | (0b11 << 4);   // STALL for both
                        while (1) {}
                    }
                }
            }
            if ((usb_int_status & 0x1f) == 0x00) {
                if (need_to_set_addr) {
                    R16_USBD_DADDR = 0x80 | need_to_set_addr;
                    need_to_set_addr = 0;
                    R16_USBD_EPR(0) = (0b01 << 9) | (0b11 << 4);  // STALL if another IN
                }
            }
        }
        R16_USBD_ISTR = 0;
    }
}
