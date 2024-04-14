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
// +0x1d4
//      ram for stashing data
// +0x200
//      <256-byte flash page buffer>

const uint8_t USB_DEV_DESC[18] __attribute__((aligned(2))) = {
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

const uint8_t USB_CONF_DESC[0x20] __attribute__((aligned(2))) = {
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

const uint16_t USB_MANUF[13] = u"\u031aArcaneNibble";
const uint16_t USB_PRODUCT[15] = u"\u031eCH32V UF2 Boot";
const uint8_t HEXLUT[16] = "0123456789ABCDEF";

const uint8_t INQUIRY_RESPONSE[36] __attribute__((aligned(2))) = {
    0x00,
    0x80,
    0x04,
    0x02,
    0x1F,
    0x00,
    0x00,
    0x00,
    'A', 'r', 'c', 'a', 'n', 'e', 'N', 'b',
    'C', 'H', '3', '2', 'V', ' ', 'U', 'F', '2', ' ', 'B', 'o', 'o', 't', ' ', ' ',
    ' ', ' ', ' ', ' ',
};

const uint8_t BOOT_SECTOR[0x3e] __attribute__((aligned(2))) = {
    0xeb, 0x3c, 0x90,                           // jump
    'A', 'r', 'c', 'a', 'n', 'e', 'N', 'b',     // oem name
    0x00, 0x02,                                 // 512 bytes/sector
    0x01,                                       // 1 sector/cluster
    0x01, 0x00,                                 // 1 reserved sector
    0x01,                                       // 1 fat
    0x10, 0x00,                                 // 16 root dir entries
    0x00, 0x40,                                 // num sectors
    0xf8,                                       // media descriptor
    0x40, 0x00,                                 // 16384 sectors/clusters -> 64 FAT sectors
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         // useless stuff
    0x80,                                       // "drive number"
    0x00,                                       // reserved
    0x29,                                       // magic
    0xde, 0xad, 0xbe, 0xef,                     // serial number
    'C', 'H', '3', '2', 'V', ' ', 'U', 'F', '2', ' ', ' ',      // volume label
    'F', 'A', 'T', '1', '6', ' ', ' ', ' ',     // fs type
};

const uint8_t INFO_UF2[70] __attribute__((aligned(2))) = "UF2 Bootloader v0.0.0\nModel: CH32V Generic\nBoard-ID: CH32Vxxx-Generic\n";
const uint8_t INDEX_HTM[119] __attribute__((aligned(2))) = "<!doctype html>\n<html><body><script>location.replace(\"https://github.com/ArcaneNibble/wch-uf2\")</script></body></html>\n";
#define THIS_CHIP_FLASH_MAX_SZ_BYTES    (224 * 1024)
#define FAMILY_ID                       0xdeadbeef

const uint8_t ROOT_DIR[32 * 3] __attribute__((aligned(2))) = {
    'C', 'H', '3', '2', 'V', ' ', 'U', 'F', '2', ' ', ' ',      // name
    0x08,                                                       // attributes (volume label)
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // timestamps
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00,                                     // timestamps
    0x00, 0x00,                                                 // start cluster
    0x00, 0x00, 0x00, 0x00,                                     // file size]

    'I', 'N', 'F', 'O', '_', 'U', 'F', '2', 'T', 'X', 'T',      // name
    0x01,                                                       // attributes (RO)
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // timestamps
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00,                                     // timestamps
    0x02, 0x00,                                                 // start cluster
    sizeof(INFO_UF2), sizeof(INFO_UF2) >> 8, sizeof(INFO_UF2) >> 16, sizeof(INFO_UF2) >> 24,

    'I', 'N', 'D', 'E', 'X', ' ', ' ', ' ', 'H', 'T', 'M',      // name
    0x01,                                                       // attributes (RO)
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // timestamps
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00,                                     // timestamps
    0x03, 0x00,                                                 // start cluster
    sizeof(INDEX_HTM), sizeof(INDEX_HTM) >> 8, sizeof(INDEX_HTM) >> 16, sizeof(INDEX_HTM) >> 24,
};

#define ESIG_UNIID(x)       (*(volatile uint8_t*)(0x1FFFF7E8 + (x)))
// XXX manual claims 96 bits but only 64 bits seem to actually be programmed?

#define USB_DESCS           ((volatile USBD_descriptor*)0x40006000)
#define USB_EP0_OUT(offs)   (*(volatile uint32_t *)(0x40006020 + 2 * (offs)))
#define USB_EP0_IN(offs)    (*(volatile uint32_t *)(0x40006030 + 2 * (offs)))
#define USB_EP1_OUT(offs)   (*(volatile uint32_t *)(0x40006040 + 2 * (offs)))
#define USB_EP1_IN(offs)    (*(volatile uint32_t *)(0x400060C0 + 2 * (offs)))

#define USB_UF2_FIELDS_STASH(offs)  (*(uint32_t *)(0x400061d4 + 2 * (offs)))
#define ACTIVE_CONFIG       USB_UF2_FIELDS_STASH(12)
#define UF2_BLOCKS_LEFT     USB_UF2_FIELDS_STASH(14)
#define CTRL_XFER_STATE     USB_UF2_FIELDS_STASH(16)
#define CTRL_XFER_STATE_X   USB_UF2_FIELDS_STASH(18)
#define CTRL_XFER_DESC_SZ   USB_UF2_FIELDS_STASH(20)
#define USB_SECTOR_STASH(offs)      (*(uint32_t *)(0x40006200 + 2 * (offs)))

#define USB_EPTYPE_BULK     0b00
#define USB_EPTYPE_CONTROL  0b01

#define USB_STAT_DISABLED   0b00
#define USB_STAT_STALL      0b01
#define USB_STAT_ACK        0b11

#define R16_BKP_DATAR10     (*(volatile uint32_t*)0x40006C28)

#define R32_RCC_CTLR        (*(volatile uint32_t*)0x40021000)
#define R32_RCC_CFGR0       (*(volatile uint32_t*)0x40021004)
#define R32_RCC_APB2PCENR   (*(volatile uint32_t*)0x40021018)
#define R32_RCC_APB1PCENR   (*(volatile uint32_t*)0x4002101C)
#define R32_RCC_RSTSCKR     (*(volatile uint32_t*)0x40021024)

#define R32_GPIOA_CFGLR     (*(volatile uint32_t*)0x40010800)
#define R32_GPIOA_CFGHR     (*(volatile uint32_t*)0x40010804)
#define R32_GPIOA_BSHR      (*(volatile uint32_t*)0x40010810)

#define R16_USBD_EPR(n)     (*(volatile uint16_t*)(0x40005C00 + 4 * (n)))
#define R16_USBD_CNTR       (*(volatile uint16_t*)0x40005C40)
#define R16_USBD_ISTR       (*(volatile uint16_t*)0x40005C44)
#define R16_USBD_DADDR      (*(volatile uint16_t*)0x40005C4C)

#define R32_FLASH_KEYR      (*(volatile uint32_t*)0x40022004)
#define R32_FLASH_STATR     (*(volatile uint32_t*)0x4002200C)
#define R32_FLASH_CTLR      (*(volatile uint32_t*)0x40022010)
#define R32_FLASH_ADDR      (*(volatile uint32_t*)0x40022014)
#define R32_FLASH_MODEKEYR  (*(volatile uint32_t*)0x40022024)

#define R32_EXTEN_CTR       (*(volatile uint32_t*)0x40023800)

#define STK_CTLR            (*(volatile uint32_t*)0xE000F000)
#define STK_SR              (*(volatile uint32_t*)0xE000F004)
#define STK_CMPLR           (*(volatile uint32_t*)0xE000F010)

#define PFIC_CFGR           (*(volatile uint32_t*)0xE000E048)

// state_x[7:0] = addr
#define STATE_SET_ADDR          0x00
// state_x[15:8] = byte pos
// state_x[7:0] = bytes left
#define STATE_GET_DESC          0x01
#define STATE_GET_STR_SERIAL    0x02
#define STATE_CTRL_SIMPLE_IN    0x03

#define STATE_WANT_CBW          0x00
#define STATE_SENT_CSW          0x01
#define STATE_SENT_CSW_REBOOT   0x02
#define STATE_SENT_DATA_IN      0x03
// state[10:8] = sector fragment
#define STATE_SEND_MORE_READ    0x04
// state[12] = uf2 good so far
// state[10:8] = sector fragment
#define STATE_WAITING_FOR_WRITE 0x05

__attribute__((always_inline)) static inline void set_ep_mode(uint32_t epidx, uint32_t epaddr, uint32_t eptype, uint32_t stat_rx, uint32_t stat_tx, uint32_t xtra, int clear_dtog) {
    uint32_t val = R16_USBD_EPR(epidx);
    uint32_t cur_stats;
    if (clear_dtog)
        cur_stats = val & 0x7070;
    else
        cur_stats = val & 0x3030;
    uint32_t want_stats = (stat_rx << 12) | (stat_tx) << 4;
    R16_USBD_EPR(epidx) = epaddr | (eptype << 9) | xtra | (cur_stats ^ want_stats);
}

__attribute__((always_inline)) static inline uint32_t min(uint32_t a, uint32_t b) {
    if (a <= b)
        return a;
    return b;
}

__attribute__((always_inline)) static inline void synthesize_block(uint32_t block, uint32_t piece) {
    if (block == 0) {
        if (piece == 0) {
            for (int i = 0; i < sizeof(BOOT_SECTOR) / 2; i++)
                USB_EP1_IN(i * 2) = ((uint16_t*)BOOT_SECTOR)[i];
            for (int i = sizeof(BOOT_SECTOR) / 2; i < 32; i++)
                USB_EP1_IN(i * 2) = 0;
        } else {
            for (int i = 0; i < 32; i++)
                USB_EP1_IN(i * 2) = 0;
        }

        if (piece == 3) {
            USB_EP1_IN(62) = 0xaa55;
        }
    } else if (block == 1 && piece == 0) {
        USB_EP1_IN(0) = 0xfff8;
        USB_EP1_IN(2) = 0xffff;
        USB_EP1_IN(4) = 0xfff8;
        USB_EP1_IN(6) = 0xfff8;
        for (int i = 4; i < 32; i++)
            USB_EP1_IN(i * 2) = 0;
    } else if (block == 65 && piece == 0) {
        for (int i = 0; i < 32; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)ROOT_DIR)[i];
    } else if (block == 65 && piece == 1) {
        for (int i = 0; i < 16; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)ROOT_DIR)[32+ i];
        for (int i = 16; i < 32; i++)
            USB_EP1_IN(i * 2) = 0;
    } else if (block == 66 && piece == 0) {
        for (int i = 0; i < 32; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)INFO_UF2)[i];
    } else if (block == 66 && piece == 1) {
        for (int i = 0; i < (sizeof(INFO_UF2) - 64 + 1) / 2; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)INFO_UF2)[32 + i];
        for (int i = (sizeof(INFO_UF2) - 64 + 1) / 2; i < 32; i++)
            USB_EP1_IN(i * 2) = 0;
    } else if (block == 67 && piece == 0) {
        for (int i = 0; i < 32; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)INDEX_HTM)[i];
    } else if (block == 67 && piece == 1) {
        for (int i = 0; i < (sizeof(INDEX_HTM) - 64 + 1) / 2; i++)
            USB_EP1_IN(i * 2) = ((uint16_t*)INDEX_HTM)[32 + i];
        for (int i = (sizeof(INDEX_HTM) - 64 + 1) / 2; i < 32; i++)
            USB_EP1_IN(i * 2) = 0;
    } else {
        for (int i = 0; i < 32; i++)
            USB_EP1_IN(i * 2) = 0;
    }

    USB_DESCS[1].count_tx = 64;
    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
}

__attribute__((naked)) int main(void) {
    // Don't reenter bootloader again
    R16_BKP_DATAR10 = 0;

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
    R16_USBD_CNTR = 1;
    USB_DESCS[0].addr_tx = 0x30 / 2;
    USB_DESCS[0].addr_rx = 0x20 / 2;
    USB_DESCS[0].count_rx = (8 << 10);
    USB_DESCS[1].addr_tx = 0xc0 / 2;
    USB_DESCS[1].addr_rx = 0x40 / 2;
    USB_DESCS[1].count_rx = (2 << 10) | (1 << 15);
    // XXX because the table is at offset 0 don't bother writing this

    // Attach USB
    R16_USBD_CNTR = 0;
    R32_EXTEN_CTR |= (1 << 1);

    CTRL_XFER_STATE = 0;
    ACTIVE_CONFIG = 0;
    const uint8_t *outputting_desc = 0;
    CTRL_XFER_DESC_SZ = 0;

    // bits [31:24] = additional sense code
    // bits [23:20] = sense key
    // bits [7:0] = state
    uint32_t msc_state = STATE_WANT_CBW;
    uint32_t dCSWTag = 0;
    // when reading:
    // [31:0] = current lba
    // [15:0] = blocks left
    // when writing:
    // [15:0] = blocks left
    uint32_t scsi_xfer_lba_blocks = 0;

    UF2_BLOCKS_LEFT = 0;

    while (1) {
        uint32_t usb_int_status = R16_USBD_ISTR;
        if (usb_int_status & (1 << 10)) {
            // RESET
            // set all to STALL, SETUP will come in nonetheless
            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_DISABLED, USB_STAT_DISABLED, 0, 0);
            R16_USBD_DADDR = 0x80;
            R16_USBD_CNTR = 0;
        } else if (usb_int_status & (1 << 11)) {
            // suspend
            R16_USBD_CNTR |= 1 << 3;
            R16_USBD_CNTR |= 1 << 2;
        } else if (usb_int_status & (1 << 12)) {
            // wakeup
            R16_USBD_CNTR = 0;
        } else if (usb_int_status & (1 << 15)) {
            uint32_t epidx = usb_int_status & 0xf;
            uint32_t ep_status = R16_USBD_EPR(epidx);
            if ((ep_status & (1 << 15)) && (epidx == 0)) {
                if (ep_status & (1 << 11)) {
                    // setup, STAT_RX and STAT_TX both now NAK
                    uint16_t bRequest_bmRequestType = USB_EP0_OUT(0);
                    uint32_t wLength = USB_EP0_OUT(6);
                    if (bRequest_bmRequestType == 0x0080 || bRequest_bmRequestType == 0x0081) {
                        // GET_STATUS
                        USB_EP0_IN(0) = 0;
                        USB_DESCS[0].count_tx = min(2, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                    } else if (bRequest_bmRequestType == 0x0102) {
                        // CLEAR_FEATURE
                        // XXX how is this supposed to work?
                        uint32_t wIndex = USB_EP0_OUT(4);
                        if (wIndex == 0x81) {
                            USB_EP1_IN(0) = 0x5355;
                            USB_EP1_IN(2) = 0x5342;
                            USB_EP1_IN(4) = dCSWTag;
                            USB_EP1_IN(6) = dCSWTag >> 16;
                            USB_EP1_IN(8) = 0;      // xxx this isn't very right
                            USB_EP1_IN(10) = 0;
                            USB_EP1_IN(12) = 1;
                            USB_DESCS[1].count_tx = 13;
                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                            msc_state = (msc_state & 0xffffff00) | STATE_SENT_CSW;
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        } else if (wIndex == 0x01) {
                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                            msc_state = (msc_state & 0xffffff00) | STATE_WANT_CBW;
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        } else {
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                        }
                    } else if (bRequest_bmRequestType == 0x0500) {
                        // SET_ADDRESS
                        CTRL_XFER_STATE_X = USB_EP0_OUT(2);
                        CTRL_XFER_STATE = STATE_SET_ADDR;
                        USB_DESCS[0].count_tx = 0;
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                    } else if (bRequest_bmRequestType == 0x0680) {
                        // GET_DESCRIPTOR
                        uint32_t wValue = USB_EP0_OUT(2);
                        if (wValue == 0x0100 || wValue == 0x0200 || wValue == 0x0301 || wValue == 0x0302) {
                            if (wValue == 0x0100) {
                                outputting_desc = USB_DEV_DESC;
                                CTRL_XFER_DESC_SZ = sizeof(USB_DEV_DESC);
                            } else if (wValue == 0x0200) {
                                outputting_desc = USB_CONF_DESC;
                                CTRL_XFER_DESC_SZ = sizeof(USB_CONF_DESC);
                            } else if (wValue == 0x0301) {
                                outputting_desc = (uint8_t*)USB_MANUF;
                                CTRL_XFER_DESC_SZ = sizeof(USB_MANUF);
                            } else if (wValue == 0x0302) {
                                outputting_desc = (uint8_t*)USB_PRODUCT;
                                CTRL_XFER_DESC_SZ = sizeof(USB_PRODUCT);
                            }

                            for (int i = 0; i < 8; i+=2)
                                USB_EP0_IN(i) = *((uint16_t*)(outputting_desc + i));
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                CTRL_XFER_STATE_X = 0;
                                CTRL_XFER_STATE = STATE_GET_DESC;
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                CTRL_XFER_STATE_X = (8 << 8) | (wLength - 8);
                                CTRL_XFER_STATE = STATE_GET_DESC;
                            }
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        } else if (wValue == 0x0300) {
                            // string LANGIDs
                            USB_EP0_IN(0) = 0x0304;
                            USB_EP0_IN(2) = 0x0409;
                            USB_DESCS[0].count_tx = min(4, wLength);
                            CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        } else if (wValue == 0x0303) {
                            USB_EP0_IN(0) = 0x0300 | (24 * 2 + 2);
                            USB_EP0_IN(2) = HEXLUT[ESIG_UNIID(0) >> 4];
                            USB_EP0_IN(4) = HEXLUT[ESIG_UNIID(0) & 0xf];
                            USB_EP0_IN(6) = HEXLUT[ESIG_UNIID(1) >> 4];
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                CTRL_XFER_STATE_X = 0;
                                CTRL_XFER_STATE = STATE_GET_STR_SERIAL;
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                CTRL_XFER_STATE_X = (3 << 8) | (wLength - 8);
                                CTRL_XFER_STATE = STATE_GET_STR_SERIAL;
                            }
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        } else {
                            // bad descriptor
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                        }
                    } else if (bRequest_bmRequestType == 0x0880) {
                        // GET_CONFIGURATION
                        USB_EP0_IN(0) = ACTIVE_CONFIG;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                    } else if (bRequest_bmRequestType == 0x0900) {
                        // SET_CONFIGURATION
                        uint32_t wValue = USB_EP0_OUT(2);
                        if (wValue == 0 || wValue == 1) {
                            ACTIVE_CONFIG = wValue;
                            USB_DESCS[0].count_tx = 0;
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                            if (wValue) {
                                // activate, allow OUT
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 1);
                                msc_state = STATE_WANT_CBW;
                            } else {
                                // deactivate
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_DISABLED, USB_STAT_DISABLED, 0, 0);
                            }
                        } else {
                            set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                        }
                    } else if (bRequest_bmRequestType == 0x0A81) {
                        // GET_INTERFACE
                        USB_EP0_IN(0) = 0;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                    } else {
                        // unknown
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                    }
                } else {
                    // back to stall for everything, expect SETUP
                    set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                }
            } else if ((ep_status & (1 << 7)) && (epidx == 0)) {
                switch (CTRL_XFER_STATE) {
                    case STATE_SET_ADDR:
                        R16_USBD_DADDR = 0x80 | (CTRL_XFER_STATE_X & 0x7f);
                        // back to stall for everything, expect SETUP
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                        break;
                    case STATE_GET_DESC:
                        {
                            uint32_t x = CTRL_XFER_STATE_X;
                            uint32_t byte_pos = (x >> 8) & 0xff;
                            uint32_t bytes_left = x & 0xff;
                            if (bytes_left == 0) {
                                // ACK for OUT ZLP, STALL for IN
                                set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_ACK, USB_STAT_STALL, 1 << 8, 0);
                            } else {
                                uint32_t bytes = bytes_left;
                                if (bytes > 8) bytes = 8;
                                if (byte_pos + bytes > CTRL_XFER_DESC_SZ)
                                    bytes = CTRL_XFER_DESC_SZ - byte_pos;
                                for (int i = 0; i < bytes; i+=2)
                                    USB_EP0_IN(i) = *((uint16_t*)(outputting_desc + byte_pos + i));
                                if (bytes < 8) {
                                    USB_DESCS[0].count_tx = bytes;
                                    CTRL_XFER_STATE_X = 0;
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    CTRL_XFER_STATE_X = ((byte_pos + 8) << 8) | (bytes_left - 8);
                                }
                                set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                            }
                        }
                        break;
                    case STATE_GET_STR_SERIAL:
                        {
                            uint32_t x = CTRL_XFER_STATE_X;
                            uint32_t str_pos = (x >> 8) & 0xff;
                            uint32_t bytes_left = x & 0xff;
                            if (bytes_left == 0) {
                                // ACK for OUT ZLP, STALL for IN
                                set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_ACK, USB_STAT_STALL, 1 << 8, 0);
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
                                    CTRL_XFER_STATE_X = 0;
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    CTRL_XFER_STATE_X = ((str_pos + 4) << 8) | (bytes_left - 8);
                                }
                                set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                            }
                        }
                        break;
                    case STATE_CTRL_SIMPLE_IN:
                        // ACK for OUT ZLP, STALL for IN
                        set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_ACK, USB_STAT_STALL, 1 << 8, 0);
                        break;
                }
            } else if ((ep_status & (1 << 15)) && (epidx == 1)) {
                // ep 1 out
                switch (msc_state & 0xff) {
                    case STATE_WANT_CBW:
                        if ((USB_DESCS[1].count_rx & 0x3f) != 0x1f || USB_EP1_OUT(0) != 0x5355 || USB_EP1_OUT(2) != 0x4342) {
                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                        } else {
                            dCSWTag = USB_EP1_OUT(4) | (USB_EP1_OUT(6) << 16);
                            uint32_t dCBWDataTransferLength = USB_EP1_OUT(8) | (USB_EP1_OUT(10) << 16);
                            uint32_t bmCBWFlags = USB_EP1_OUT(12) & 0xff;
                            uint32_t operation_code = USB_EP1_OUT(14) >> 8;

                            switch (operation_code) {
                                case 0x00:
                                    // test unit ready
                                    USB_EP1_IN(0) = 0x5355;
                                    USB_EP1_IN(2) = 0x5342;
                                    USB_EP1_IN(4) = dCSWTag;
                                    USB_EP1_IN(6) = dCSWTag >> 16;
                                    USB_EP1_IN(8) = 0;
                                    USB_EP1_IN(10) = 0;
                                    USB_EP1_IN(12) = 0;
                                    USB_DESCS[1].count_tx = 13;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_CSW;
                                    break;
                                case 0x03:
                                    // request sense
                                    USB_EP1_IN(0) = 0x0070;
                                    USB_EP1_IN(2) = (msc_state >> 20) & 0xf;
                                    USB_EP1_IN(4) = 0;
                                    USB_EP1_IN(6) = 0;
                                    USB_EP1_IN(8) = 0;
                                    USB_EP1_IN(10) = 0;
                                    USB_EP1_IN(12) = (msc_state >> 24) & 0xff;
                                    USB_EP1_IN(14) = 0;
                                    USB_EP1_IN(16) = 0;
                                    USB_DESCS[1].count_tx = 18;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x12:
                                    // inquiry
                                    if (USB_EP1_OUT(16) == 0) {
                                        for (int i = 0; i < 36 / 2; i++) {
                                            USB_EP1_IN(i * 2) = ((uint16_t*)INQUIRY_RESPONSE)[i];
                                        }
                                        USB_DESCS[1].count_tx = 36;
                                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                        msc_state = STATE_SENT_DATA_IN;
                                        break;
                                    }
                                    msc_state = STATE_SENT_CSW | (5 << 20) | (0x24 << 24);
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                    break;
                                case 0x1a:
                                    // mode sense (6)
                                    USB_EP1_IN(0) = 0x0003;
                                    USB_EP1_IN(2) = 0x0000;
                                    USB_DESCS[1].count_tx = 4;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x1b:
                                    // start/stop unit
                                    uint32_t param = USB_EP1_OUT(18) >> 8;
                                    USB_EP1_IN(0) = 0x5355;
                                    USB_EP1_IN(2) = 0x5342;
                                    USB_EP1_IN(4) = dCSWTag;
                                    USB_EP1_IN(6) = dCSWTag >> 16;
                                    USB_EP1_IN(8) = 0;
                                    USB_EP1_IN(10) = 0;
                                    USB_EP1_IN(12) = 0;
                                    USB_DESCS[1].count_tx = 13;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    if (param == 0x02)
                                        msc_state = STATE_SENT_CSW_REBOOT;
                                    else
                                        msc_state = STATE_SENT_CSW;
                                    break;
                                case 0x23:
                                    // read format capacity
                                    USB_EP1_IN(0) = 0x0000;
                                    USB_EP1_IN(2) = 0x0800;
                                    USB_EP1_IN(4) = 0x0000;     // 8 MiB
                                    USB_EP1_IN(6) = 0x0040;
                                    USB_EP1_IN(8) = 0x0002;     // formatted media, 512 byte sectors
                                    USB_EP1_IN(10) = 0x0002;
                                    USB_DESCS[1].count_tx = 12;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x5a:
                                    // mode sense (10)
                                    USB_EP1_IN(0) = 0x0800;
                                    USB_EP1_IN(2) = 0x0000;
                                    USB_EP1_IN(4) = 0x0000;
                                    USB_EP1_IN(6) = 0x0000;
                                    USB_DESCS[1].count_tx = 8;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x25:
                                    // READ CAPACITY (10)
                                    // xxx don't bother checking the silly fields
                                    USB_EP1_IN(0) = 0x0000;     // 8 MiB
                                    USB_EP1_IN(2) = 0xff3f;
                                    USB_EP1_IN(4) = 0x0000;     // 512 byte sectors
                                    USB_EP1_IN(6) = 0x0002;
                                    USB_DESCS[1].count_tx = 8;
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x28:
                                    {
                                        // READ (10)
                                        // xxx also don't bother checking the flags
                                        // @ 16: flags lba3
                                        // @ 18: lba2 lba1
                                        // @ 20: lba0 group
                                        // @ 22: len1 len0
                                        uint32_t lba = (USB_EP1_OUT(16) << 16) & 0xff000000;
                                        uint32_t tmp = USB_EP1_OUT(18);
                                        lba |= (tmp & 0xff) << 24;
                                        lba |= (tmp & 0xff00);
                                        tmp = USB_EP1_OUT(20);
                                        lba |= (tmp & 0xff);
                                        tmp = USB_EP1_OUT(22);
                                        uint32_t blocks = (tmp >> 8) | ((tmp & 0xff) << 8);

                                        if (blocks > 0x4000 || lba >= 0x4000 || (blocks + lba) > 0x4000) {
                                            msc_state = STATE_SENT_CSW | (5 << 20) | (0x24 << 24);
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                            break;
                                        }

                                        if (blocks == 0) {
                                            USB_EP1_IN(0) = 0x5355;
                                            USB_EP1_IN(2) = 0x5342;
                                            USB_EP1_IN(4) = dCSWTag;
                                            USB_EP1_IN(6) = dCSWTag >> 16;
                                            USB_EP1_IN(8) = 0;
                                            USB_EP1_IN(10) = 0;
                                            USB_EP1_IN(12) = 0;
                                            USB_DESCS[1].count_tx = 13;
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                            msc_state = STATE_SENT_CSW;
                                        }

                                        synthesize_block(lba, 0);
                                        scsi_xfer_lba_blocks = (lba << 16) | blocks;
                                        msc_state = STATE_SEND_MORE_READ;
                                        break;
                                    }
                                case 0x2a:
                                    {
                                        // WRITE (10)
                                        // xxx also don't bother checking the flags
                                        // @ 16: flags lba3
                                        // @ 18: lba2 lba1
                                        // @ 20: lba0 group
                                        // @ 22: len1 len0
                                        uint32_t lba = (USB_EP1_OUT(16) << 16) & 0xff000000;
                                        uint32_t tmp = USB_EP1_OUT(18);
                                        lba |= (tmp & 0xff) << 24;
                                        lba |= (tmp & 0xff00);
                                        tmp = USB_EP1_OUT(20);
                                        lba |= (tmp & 0xff);
                                        tmp = USB_EP1_OUT(22);
                                        uint32_t blocks = (tmp >> 8) | ((tmp & 0xff) << 8);

                                        if (blocks > 0x4000 || lba >= 0x4000 || (blocks + lba) > 0x4000) {
                                            msc_state = STATE_SENT_CSW | (5 << 20) | (0x24 << 24);
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                            break;
                                        }

                                        if (blocks == 0) {
                                            USB_EP1_IN(0) = 0x5355;
                                            USB_EP1_IN(2) = 0x5342;
                                            USB_EP1_IN(4) = dCSWTag;
                                            USB_EP1_IN(6) = dCSWTag >> 16;
                                            USB_EP1_IN(8) = 0;
                                            USB_EP1_IN(10) = 0;
                                            USB_EP1_IN(12) = 0;
                                            USB_DESCS[1].count_tx = 13;
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                            msc_state = STATE_SENT_CSW;
                                        }

                                        scsi_xfer_lba_blocks = blocks;
                                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                                        msc_state = STATE_WAITING_FOR_WRITE;
                                        break;
                                    }
                                default:
                                    // illegal command
                                    msc_state = STATE_SENT_CSW | (5 << 20);
                                    if (dCBWDataTransferLength == 0) {
                                        USB_EP1_IN(0) = 0x5355;
                                        USB_EP1_IN(2) = 0x5342;
                                        USB_EP1_IN(4) = dCSWTag;
                                        USB_EP1_IN(6) = dCSWTag >> 16;
                                        USB_EP1_IN(8) = 0;
                                        USB_EP1_IN(10) = 0;
                                        USB_EP1_IN(12) = 1;
                                        USB_DESCS[1].count_tx = 13;
                                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                    } else {
                                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                    }
                                    break;
                            }
                        }
                        break;
                    case STATE_WAITING_FOR_WRITE:
                        uint32_t piece = (msc_state >> 8) & 0b111;

                        if (piece == 0) {
                            if ((USB_EP1_OUT(0) == 0x4655) &&
                                (USB_EP1_OUT(2) == 0x0A32) &&
                                (USB_EP1_OUT(4) == 0x5157) &&
                                (USB_EP1_OUT(6) == 0x9E5D)) {
                                // UF2 magic okay
                                uint32_t flags = USB_EP1_OUT(8) | (USB_EP1_OUT(10) << 16);
                                uint32_t address = USB_EP1_OUT(12) | (USB_EP1_OUT(14) << 16);
                                uint32_t bytes = USB_EP1_OUT(16) | (USB_EP1_OUT(18) << 16);
                                uint32_t familyid = USB_EP1_OUT(28) | (USB_EP1_OUT(30) << 16);

                                if (!(flags & 1) && bytes == 256 && (address & 0xff) == 0) {
                                    if (flags & (0x2000) && familyid == FAMILY_ID) {
                                        // uf2 good so far!
                                        USB_UF2_FIELDS_STASH(0) = USB_EP1_OUT(20);
                                        USB_UF2_FIELDS_STASH(2) = USB_EP1_OUT(22);
                                        USB_UF2_FIELDS_STASH(4) = USB_EP1_OUT(24);
                                        USB_UF2_FIELDS_STASH(6) = USB_EP1_OUT(26);
                                        USB_UF2_FIELDS_STASH(8) = USB_EP1_OUT(12);
                                        USB_UF2_FIELDS_STASH(10) = USB_EP1_OUT(14);

                                        for (int i = 0; i < 16; i++)
                                            USB_SECTOR_STASH(i * 2) = USB_EP1_OUT(32 + i * 2);

                                        msc_state += 0x1000;
                                    }
                                }
                            }
                        } else if (piece >= 1 && piece <= 3) {
                            for (int i = 0; i < 32; i++)
                                USB_SECTOR_STASH(32 + (piece - 1) * 64 + i * 2) = USB_EP1_OUT(i * 2);
                        } else if (piece == 4) {
                            for (int i = 0; i < 16; i++)
                                USB_SECTOR_STASH(224 + i * 2) = USB_EP1_OUT(i * 2);
                        }

                        if (piece != 7) {
                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                            msc_state += 0x100;
                        } else {
                            // full sector is done
                            if (msc_state & 0x1000) {
                                if (USB_EP1_OUT(60) == 0x6F30 && USB_EP1_OUT(62) == 0x0AB1) {
                                    // uf2 all magics are good!
                                    uint32_t blocknum = USB_UF2_FIELDS_STASH(0) | (USB_UF2_FIELDS_STASH(2) << 16);
                                    uint32_t totblocks = USB_UF2_FIELDS_STASH(4) | (USB_UF2_FIELDS_STASH(6) << 16);
                                    uint32_t address = USB_UF2_FIELDS_STASH(8) | (USB_UF2_FIELDS_STASH(10) << 16);

                                    if (UF2_BLOCKS_LEFT == 0)
                                        UF2_BLOCKS_LEFT = (totblocks - 1) | 0x8000;
                                    else
                                        UF2_BLOCKS_LEFT--;

                                    // todo flash the file
                                    // todo ram download mode
                                    R32_FLASH_KEYR = 0x45670123;
                                    R32_FLASH_KEYR = 0xCDEF89AB;
                                    R32_FLASH_MODEKEYR = 0x45670123;
                                    R32_FLASH_MODEKEYR = 0xCDEF89AB;
                                    R32_FLASH_CTLR = 1 << 17;
                                    R32_FLASH_ADDR = address;
                                    R32_FLASH_CTLR = (1 << 17) | (1 << 6);
                                    while (R32_FLASH_STATR & 1) {}
                                    R32_FLASH_CTLR = 1 << 16;
                                    for (int i = 0; i < 64; i++) {
                                        volatile uint32_t *addr = (volatile uint32_t *)(address + i * 4);
                                        uint32_t val = USB_SECTOR_STASH(i * 4) | (USB_SECTOR_STASH(i * 4 + 2) << 16);
                                        *addr = val;
                                        while (R32_FLASH_STATR & 2) {}
                                    }
                                    R32_FLASH_CTLR = (1 << 16) | (1 << 21);
                                    while (R32_FLASH_STATR & 1) {}
                                    R32_FLASH_CTLR = (1 << 15) | (1 << 7);
                                }
                            }

                            uint32_t blocks_left = scsi_xfer_lba_blocks & 0xffff;
                            blocks_left--;
                            if (blocks_left == 0) {
                                USB_EP1_IN(0) = 0x5355;
                                USB_EP1_IN(2) = 0x5342;
                                USB_EP1_IN(4) = dCSWTag;
                                USB_EP1_IN(6) = dCSWTag >> 16;
                                USB_EP1_IN(8) = 0;
                                USB_EP1_IN(10) = 0;
                                USB_EP1_IN(12) = 0;
                                USB_DESCS[1].count_tx = 13;
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                if (UF2_BLOCKS_LEFT == 0x8000)
                                    msc_state = STATE_SENT_CSW_REBOOT;
                                else
                                    msc_state = STATE_SENT_CSW;
                            } else {
                                scsi_xfer_lba_blocks = blocks_left;
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                                msc_state = STATE_WAITING_FOR_WRITE;
                            }
                        }
                        break;
                }
            } else if ((ep_status & (1 << 7)) && (epidx == 1)) {
                // ep 1 in
                switch (msc_state & 0xff) {
                    case STATE_SENT_CSW:
                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
                        msc_state = (msc_state & 0xffffff00) | STATE_WANT_CBW;
                        break;
                    case STATE_SENT_CSW_REBOOT:
                        // todo reboot into ram
                        STK_CMPLR = (50 /* ms */ * 12000 /* assume 96 MHz system clock, div8 */);
                        STK_CTLR = 0b111000;
                        STK_CTLR = 0b111001;
                        while (!(STK_SR & 1)) {}
                        STK_CTLR = 0;
                        STK_SR = 0;
                        R16_USBD_CNTR = 0b11;
                        R32_EXTEN_CTR &= ~(1 << 1);
                        // wtf why does this work and the other stuff doesn't?
                        R16_BKP_DATAR10 = 0x4170;
                        PFIC_CFGR = 0xbeef0080;
                        while (1) { asm volatile(""); }
                        break;
                    case STATE_SENT_DATA_IN:
                        USB_EP1_IN(0) = 0x5355;
                        USB_EP1_IN(2) = 0x5342;
                        USB_EP1_IN(4) = dCSWTag;
                        USB_EP1_IN(6) = dCSWTag >> 16;
                        USB_EP1_IN(8) = 0;
                        USB_EP1_IN(10) = 0;
                        USB_EP1_IN(12) = 0;
                        USB_DESCS[1].count_tx = 13;
                        set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                        msc_state = STATE_SENT_CSW;
                        break;
                    case STATE_SEND_MORE_READ:
                        uint32_t piece = (msc_state >> 8) & 0b111;
                        if (piece != 7) {
                            synthesize_block(scsi_xfer_lba_blocks >> 16, piece + 1);
                            msc_state += 0x100;
                        } else {
                            uint32_t lba = scsi_xfer_lba_blocks >> 16;
                            uint32_t blocks_left = scsi_xfer_lba_blocks & 0xffff;
                            blocks_left--;
                            if (blocks_left == 0) {
                                USB_EP1_IN(0) = 0x5355;
                                USB_EP1_IN(2) = 0x5342;
                                USB_EP1_IN(4) = dCSWTag;
                                USB_EP1_IN(6) = dCSWTag >> 16;
                                USB_EP1_IN(8) = 0;
                                USB_EP1_IN(10) = 0;
                                USB_EP1_IN(12) = 0;
                                USB_DESCS[1].count_tx = 13;
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
                                msc_state = STATE_SENT_CSW;
                            } else {
                                lba++;
                                synthesize_block(lba, 0);
                                scsi_xfer_lba_blocks = (lba << 16) | blocks_left;
                                msc_state = STATE_SEND_MORE_READ;
                            }
                        }
                        break;
                }
            }
        }
        R16_USBD_ISTR = 0;
    }
}
