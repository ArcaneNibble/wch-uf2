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

const uint8_t USB_DEV_DESC[18] __attribute__((aligned(16))) = {
    18,             // bLength
    1,              // bDescriptorType
    0x00, 0x02,     // bcdUSB
    0,              // bDeviceClass
    0,              // bDeviceSubClass
    0,              // bDeviceProtocol
    8,              // bMaxPacketSize0
    0x55, 0xf0,     // idVendor
    0x00, 0x00,     // idProduct
    0x00, 0x00,     // bcdDevice
    1,              // iManufacturer
    2,              // iProduct
    3,              // iSerialNumber
    1,              // bNumConfigurations
};

const uint8_t USB_CONF_DESC[0x20] __attribute__((aligned(16))) = {
    9,              // bLength
    2,              // bDescriptorType
    0x20, 0x00,     // wTotalLength
    1,              // bNumInterfaces
    1,              // bConfigurationValue
    0,              // iConfiguration
    0x80,           // bmAttributes
    250,            // bMaxPower

    9,              // bLength
    4,              // bDescriptorType
    0,              // bInterfaceNumber
    0,              // bAlternateSetting
    2,              // bNumEndpoints
    0x08,           // bInterfaceClass
    0x05,           // bInterfaceSubClass
    0x50,           // bInterfaceProtocol
    0,              // iInterface

    7,              // bLength
    5,              // bDescriptorType
    0x81,           // bEndpointAddress
    2,              // bmAttributes
    0x40, 0x00,     // wMaxPacketSize
    0,              // bInterval

    7,              // bLength
    5,              // bDescriptorType
    0x01,           // bEndpointAddress
    2,              // bmAttributes
    0x40, 0x00,     // wMaxPacketSize
    0,              // bInterval
};

const uint8_t USB_MANUF[12] = "ArcaneNibble";
const uint8_t USB_PRODUCT[14] = "CH32V UF2 Boot";
const uint8_t HEXLUT[16] = "0123456789ABCDEF";

#define ESIG_UNIID(x)       (*(volatile uint8_t*)(0x1FFFF7E8 + (x)))
// XXX manual claims 96 bits but only 64 bits seem to actually be programmed?

#define USB_DESCS           ((volatile USBD_descriptor*)0x40006000)
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

// state[7:0] = addr
#define STATE_SET_ADDR          0x00
// state[15:8] = byte pos
// state[7:0] = bytes left
#define STATE_GET_DESC          0x01
// state[15:8] = str pos *ascii*
// state[7:0] = bytes left *unicode*
#define STATE_GET_STR_STATIC    0x02
#define STATE_GET_STR_SERIAL    0x03
#define STATE_CTRL_STATUS_OUT   0x04
#define STATE_CTRL_SIMPLE_IN    0x05

__attribute__((always_inline)) static inline void set_ep_mode(uint8_t ep, uint8_t stat_rx, uint8_t stat_tx, uint16_t xtra) {
    uint16_t val = R16_USBD_EPR(ep);
    uint8_t cur_stat_rx = (val >> 12) & 0b11;
    uint8_t cur_stat_tx = (val >> 4) & 0b11;
    R16_USBD_EPR(ep) = (val & 0b0000011000001111) | xtra | ((stat_rx ^ cur_stat_rx) << 12) | ((stat_tx ^ cur_stat_tx) << 4);
}

__attribute__((always_inline)) static inline uint32_t min(uint32_t a, uint32_t b) {
    if (a <= b)
        return a;
    return b;
}

__attribute__((naked)) int main(void) {
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

    uint32_t master_state = 0;
    uint32_t active_config = 0;
    const uint8_t *outputting_desc = 0;
    uint32_t outputting_desc_sz = 0;

    while (1) {
        uint32_t usb_int_status = R16_USBD_ISTR;
        if (usb_int_status & (1 << 10)) {
            // RESET
            // set all to STALL, SETUP will come in nonetheless
            set_ep_mode(0, 0b01, 0b01, 0b01 << 9);
            R16_USBD_DADDR = 0x80;
        } else if (usb_int_status & (1 << 15)) {
            if ((usb_int_status & 0x1f) == 0x10) {
                uint32_t ep0_status = R16_USBD_EPR(0);
                if (ep0_status & (1 << 11)) {
                    // setup, STAT_RX and STAT_TX both now NAK
                    uint16_t bRequest_bmRequestType = USB_EP0_OUT(0);
                    uint32_t wLength = USB_EP0_OUT(6);
                    if (bRequest_bmRequestType == 0x0080 || bRequest_bmRequestType == 0x0081) {
                        // GET_STATUS
                        USB_EP0_IN(0) = 0;
                        USB_DESCS[0].count_tx = min(2, wLength);
                        master_state = (STATE_CTRL_SIMPLE_IN << 24);
                        set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                    } else if (bRequest_bmRequestType == 0x0500) {
                        // SET_ADDRESS
                        master_state = (STATE_SET_ADDR << 24) | USB_EP0_OUT(2);
                        USB_DESCS[0].count_tx = 0;
                        set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                    } else if (bRequest_bmRequestType == 0x0680) {
                        // GET_DESCRIPTOR
                        uint32_t wValue = USB_EP0_OUT(2);
                        if (wValue == 0x0100 || wValue == 0x0200) {
                            if (wValue == 0x0100) {
                                outputting_desc = USB_DEV_DESC;
                                outputting_desc_sz = sizeof(USB_DEV_DESC);
                            } else {
                                outputting_desc = USB_CONF_DESC;
                                outputting_desc_sz = sizeof(USB_CONF_DESC);
                            }
                            for (int i = 0; i < 8; i+=2)
                                USB_EP0_IN(i) = *((uint16_t*)(outputting_desc + i));
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                master_state = (STATE_GET_DESC << 24);
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                master_state = (STATE_GET_DESC << 24) | (8 << 8) | (wLength - 8);
                            }
                            set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                        } else if (wValue == 0x0300) {
                            // string LANGIDs
                            USB_EP0_IN(0) = 0x0304;
                            USB_EP0_IN(2) = 0x0409;
                            USB_DESCS[0].count_tx = min(4, wLength);
                            master_state = (STATE_CTRL_SIMPLE_IN << 24);
                            set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                        } else if (wValue == 0x0301 || wValue == 0x0302) {
                            if (wValue == 0x0301) {
                                outputting_desc = USB_MANUF;
                                outputting_desc_sz = sizeof(USB_MANUF);
                            } else {
                                outputting_desc = USB_PRODUCT;
                                outputting_desc_sz = sizeof(USB_PRODUCT);
                            }
                            USB_EP0_IN(0) = 0x0300 | (outputting_desc_sz * 2 + 2);
                            for (int i = 0; i < 3; i++)
                                USB_EP0_IN(i * 2 + 2) = outputting_desc[i];
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                master_state = (STATE_GET_STR_STATIC << 24);
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                master_state = (STATE_GET_STR_STATIC << 24) | (3 << 8) | (wLength - 8);
                            }
                            set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                        } else if (wValue == 0x0303) {
                            USB_EP0_IN(0) = 0x0300 | (24 * 2 + 2);
                            USB_EP0_IN(2) = HEXLUT[ESIG_UNIID(0) >> 4];
                            USB_EP0_IN(4) = HEXLUT[ESIG_UNIID(0) & 0xf];
                            USB_EP0_IN(6) = HEXLUT[ESIG_UNIID(1) >> 4];
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                master_state = (STATE_GET_STR_SERIAL << 24);
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                master_state = (STATE_GET_STR_SERIAL << 24) | (3 << 8) | (wLength - 8);
                            }
                            set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                        } else {
                            // bad descriptor
                            set_ep_mode(0, 0b01, 0b01, 0);  // STALL for both
                        }
                    } else if (bRequest_bmRequestType == 0x0880) {
                        // GET_CONFIGURATION
                        USB_EP0_IN(0) = active_config;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        master_state = (STATE_CTRL_SIMPLE_IN << 24);
                        set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                    } else if (bRequest_bmRequestType == 0x0900) {
                        // SET_CONFIGURATION
                        uint32_t wValue = USB_EP0_OUT(2);
                        if (wValue == 0 || wValue == 1) {
                            active_config = wValue;
                            USB_DESCS[0].count_tx = 0;
                            set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                            if (wValue) {
                                // activate, expect DATA0
                                R16_USBD_EPR(1) = ((R16_USBD_EPR(1) & 0x7070) ^ 0b0011000000010000) | 1;
                            } else {
                                // deactivate
                                R16_USBD_EPR(1) = (R16_USBD_EPR(1) & 0x7070) | 1;
                            }
                        } else {
                            set_ep_mode(0, 0b01, 0b01, 0);  // STALL for both
                        }
                    } else if (bRequest_bmRequestType == 0x0A81) {
                        // GET_INTERFACE
                        USB_EP0_IN(0) = 0;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        master_state = (STATE_CTRL_SIMPLE_IN << 24);
                        set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                    } else {
                        // unknown
                        set_ep_mode(0, 0b01, 0b01, 0);  // STALL for both
                    }
                } else {
                    // back to stall for everything, expect SETUP
                    set_ep_mode(0, 0b01, 0b01, 0);  // STALL for both
                }
            }
            if ((usb_int_status & 0x1f) == 0x00) {
                switch (master_state >> 24) {
                    case STATE_SET_ADDR:
                        R16_USBD_DADDR = 0x80 | (master_state & 0x7f);
                        master_state = 0;
                        // back to stall for everything, expect SETUP
                        set_ep_mode(0, 0b01, 0b01, 0);  // STALL for both
                        break;
                    case STATE_GET_DESC:
                        {
                            uint32_t byte_pos = (master_state >> 8) & 0xff;
                            uint32_t bytes_left = master_state & 0xff;
                            if (bytes_left == 0) {
                                master_state = (STATE_CTRL_STATUS_OUT << 24);
                                set_ep_mode(0, 0b11, 0b01, 1 << 8);     // ACK for OUT ZLP, STALL for IN
                            } else {
                                uint32_t bytes = bytes_left;
                                if (bytes > 8) bytes = 8;
                                if (byte_pos + bytes > outputting_desc_sz)
                                    bytes = outputting_desc_sz - byte_pos;
                                for (int i = 0; i < bytes; i+=2)
                                    USB_EP0_IN(i) = *((uint16_t*)(outputting_desc + byte_pos + i));
                                if (bytes < 8) {
                                    USB_DESCS[0].count_tx = bytes;
                                    master_state = (STATE_GET_DESC << 24);
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    master_state = (STATE_GET_DESC << 24) | ((byte_pos + 8) << 8) | (bytes_left - 8);
                                }
                                set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                            }
                        }
                        break;
                    case STATE_GET_STR_STATIC:
                        {
                            uint32_t str_pos = (master_state >> 8) & 0xff;
                            uint32_t bytes_left = master_state & 0xff;
                            if (bytes_left == 0) {
                                master_state = (STATE_CTRL_STATUS_OUT << 24);
                                set_ep_mode(0, 0b11, 0b01, 1 << 8);     // ACK for OUT ZLP, STALL for IN
                            } else {
                                // XXX this is confusing
                                uint32_t ascii_bytes = (bytes_left + 1) / 2;
                                if (ascii_bytes > 4) ascii_bytes = 4;
                                if (str_pos + ascii_bytes > outputting_desc_sz)
                                    ascii_bytes = outputting_desc_sz - str_pos;
                                for (int i = 0; i < ascii_bytes; i++)
                                    USB_EP0_IN(i * 2) = outputting_desc[str_pos + i];
                                if (ascii_bytes < 4) {
                                    USB_DESCS[0].count_tx = bytes_left;
                                    master_state = (STATE_GET_STR_STATIC << 24);
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    master_state = (STATE_GET_STR_STATIC << 24) | ((str_pos + 4) << 8) | (bytes_left - 8);
                                }
                                set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                            }
                        }
                        break;
                    case STATE_GET_STR_SERIAL:
                        {
                            uint32_t str_pos = (master_state >> 8) & 0xff;
                            uint32_t bytes_left = master_state & 0xff;
                            if (bytes_left == 0) {
                                master_state = (STATE_CTRL_STATUS_OUT << 24);
                                set_ep_mode(0, 0b11, 0b01, 1 << 8);     // ACK for OUT ZLP, STALL for IN
                            } else {
                                // XXX this is confusing
                                uint32_t ascii_bytes = (bytes_left + 1) / 2;
                                if (ascii_bytes > 4) ascii_bytes = 4;
                                if (str_pos + ascii_bytes > 24)
                                    ascii_bytes = 24 - str_pos;
                                for (int i = 0; i < ascii_bytes; i++)
                                    USB_EP0_IN(i * 2) = HEXLUT[(ESIG_UNIID((str_pos + i) / 2) >> ((1 - ((str_pos + i) % 2)) * 4)) & 0xf];
                                if (ascii_bytes < 4) {
                                    USB_DESCS[0].count_tx = bytes_left;
                                    master_state = (STATE_GET_STR_SERIAL << 24);
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    master_state = (STATE_GET_STR_SERIAL << 24) | ((str_pos + 4) << 8) | (bytes_left - 8);
                                }
                                set_ep_mode(0, 0b01, 0b11, 0);  // STALL for OUT, ACK for IN
                            }
                        }
                        break;
                    case STATE_CTRL_SIMPLE_IN:
                        master_state = (STATE_CTRL_STATUS_OUT << 24);
                        set_ep_mode(0, 0b11, 0b01, 1 << 8);     // ACK for OUT ZLP, STALL for IN
                        break;
                }
            }
            if ((usb_int_status & 0x1f) == 0x01) {
                // ep 1 in
                // while (1) {}
            }
            if ((usb_int_status & 0x1f) == 0x11) {
                // ep 1 out
                while (1) {}
            }
        }
        R16_USBD_ISTR = 0;
    }
}
