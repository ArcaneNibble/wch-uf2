// CH32V UF2 bootloader, size-optimized (target: <= 4096 bytes)

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
//      ram for checking download completion
// +0x1d0
//      ram for stashing variables
// +0x200
//      <256-byte flash page buffer>
// In addition to the expected USB buffers, this RAM is used to store program state.
// This allows the *entire* SRAM to be used when downloading to SRAM.

// Note that every buffer that needs to be sent over USB must be 16-bit aligned.
// This is assumed by copying code (which will copy in 16-bit chunks).

// Device descriptor
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
    // Configuration descriptor
    9,              // bLength
    2,              // bDescriptorType
    0x20, 0x00,     // wTotalLength
    1,              // bNumInterfaces
    1,              // bConfigurationValue
    0,              // iConfiguration
    0x80,           // bmAttributes
    250,            // bMaxPower

    // Interface descriptor
    9,              // bLength
    4,              // bDescriptorType
    0,              // bInterfaceNumber
    0,              // bAlternateSetting
    2,              // bNumEndpoints
    0x08,           // bInterfaceClass
    0x05,           // bInterfaceSubClass
    0x50,           // bInterfaceProtocol
    0,              // iInterface

    // EP1 IN endpoint descriptor
    7,              // bLength
    5,              // bDescriptorType
    0x81,           // bEndpointAddress
    2,              // bmAttributes
    0x40, 0x00,     // wMaxPacketSize
    0,              // bInterval

    // EP1 OUT endpoint descriptor
    7,              // bLength
    5,              // bDescriptorType
    0x01,           // bEndpointAddress
    2,              // bmAttributes
    0x40, 0x00,     // wMaxPacketSize
    0,              // bInterval
};

// Must be UTF-16, and the first character is a *manually-calculated* total length
// (combined with a 0x03 string descriptor type)
const uint16_t USB_MANUF[13] = u"\u031aArcaneNibble";
const uint16_t USB_PRODUCT[15] = u"\u031eCH32V UF2 Boot";
// Used for outputting serial number from electronic signature bytes
const uint8_t HEXLUT[16] = "0123456789ABCDEF";

// SCSI INQUIRY standard response
const uint8_t INQUIRY_RESPONSE[36] __attribute__((aligned(2))) = {
    0x00,   // direct access block device
    0x80,   // RMB removable media
    0x04,   // SPC-2
    0x02,   // fixed, response data format
    0x1F,   // additional length
    0x00,   // xxx mostly obsolete junk
    0x00,
    0x00,
    // Vendor
    'A', 'r', 'c', 'a', 'n', 'e', 'N', 'b',
    // Product
    'C', 'H', '3', '2', 'V', ' ', 'U', 'F', '2', ' ', 'B', 'o', 'o', 't', ' ', ' ',
    // Revision
    ' ', ' ', ' ', ' ',
};

// FAT16 boot sector
// Most of these parameters *cannot* be changed,
// as synthesize_block assumes a particular layout
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

// Change here to change UF2 data files
const uint8_t INFO_UF2[70] __attribute__((aligned(2))) = "UF2 Bootloader v0.0.0\nModel: CH32V Generic\nBoard-ID: CH32Vxxx-Generic\n";
const uint8_t INDEX_HTM[119] __attribute__((aligned(2))) = "<!doctype html>\n<html><body><script>location.replace(\"https://github.com/ArcaneNibble/wch-uf2\")</script></body></html>\n";
// Change here for different chips
#define THIS_CHIP_FLASH_MAX_SZ_BYTES    (224 * 1024)
#define THIS_CHIP_RAM_MAX_SZ_BYTES      (20 * 1024)

#define BOOTLOADER_RESERVED_SZ_BYTES    (4 * 1024)
#define FAMILY_ID                       0x699b62ec

// FAT16 root directory entries
// Most of these parameters *cannot* be changed,
// as synthesize_block assumes a particular layout
const uint8_t ROOT_DIR[32 * 3] __attribute__((aligned(2))) = {
    'C', 'H', '3', '2', 'V', ' ', 'U', 'F', '2', ' ', ' ',      // name
    0x08,                                                       // attributes (volume label)
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // timestamps
    0x00, 0x00,                                                 // reserved
    0x00, 0x00, 0x00, 0x00,                                     // timestamps
    0x00, 0x00,                                                 // start cluster
    0x00, 0x00, 0x00, 0x00,                                     // file size

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

// Note that all of this stuff is declared *extern*
// A significant amount of code size is being saved because the gp register
// is pointed at 0x40006000, right at the beginning of USBD RAM
// (and within reach of the USBD registers).
// The linker is then being instructed to perform gp-relative linker relaxation.
// Actual addresses for these variables are in the linker.lds file.
extern volatile USBD_descriptor     USB_DESCS[2];
extern volatile uint32_t            USB_EP0_OUT[4];
extern volatile uint32_t            USB_EP0_IN[4];
extern volatile uint32_t            USB_EP1_OUT[32];
extern volatile uint32_t            USB_EP1_IN[32];

extern uint32_t SCSI_XFER_CUR_LBA;
extern uint32_t SCSI_XFER_BLK_LEFT;
extern uint32_t BLOCKNUM_LO;
extern uint32_t TOTBLOCKS_LO;
extern uint32_t CSWTAG_LO;
extern uint32_t CSWTAG_HI;
extern uint32_t ADDRESS_LO;
extern uint32_t ADDRESS_HI;
extern uint32_t ACTIVE_CONFIG;
extern uint32_t CTRL_XFER_STATE;
extern uint32_t CTRL_XFER_STATE_X;
extern uint32_t CTRL_XFER_DESC_SZ;
extern uint32_t USB_SECTOR_STASH[128];

// Files larger than this won't cause an auto-reboot
// (there is not enough USBD RAM to mark which blocks have been received)
#define AUTO_BOOT_BITMAP_NUM_HWORDS     36
#define MAX_AUTO_BOOT_BLOCKS            (AUTO_BOOT_BITMAP_NUM_HWORDS * 16 - 1)
extern uint32_t UF2_GOT_BLOCKS[AUTO_BOOT_BITMAP_NUM_HWORDS];

// Constants for USBD registers
#define USB_EPTYPE_BULK     0b00
#define USB_EPTYPE_CONTROL  0b01

#define USB_STAT_DISABLED   0b00
#define USB_STAT_STALL      0b01
#define USB_STAT_NAK        0b10
#define USB_STAT_ACK        0b11

// Other registers we need
#define R16_BKP_DATAR10     (*(volatile uint32_t*)0x40006C28)

#define R32_PWR_CTLR        (*(volatile uint32_t*)0x40007000)

#define R32_RCC_CTLR        (*(volatile uint32_t*)0x40021000)
#define R32_RCC_CFGR0       (*(volatile uint32_t*)0x40021004)
#define R32_RCC_APB2PCENR   (*(volatile uint32_t*)0x40021018)
#define R32_RCC_APB1PCENR   (*(volatile uint32_t*)0x4002101C)
#define R32_RCC_RSTSCKR     (*(volatile uint32_t*)0x40021024)

#define R32_GPIOA_CFGLR     (*(volatile uint32_t*)0x40010800)
#define R32_GPIOA_CFGHR     (*(volatile uint32_t*)0x40010804)
#define R32_GPIOA_BSHR      (*(volatile uint32_t*)0x40010810)

// USBD registers are within range to perform gp relaxation
// XXX wrong access size
extern volatile uint32_t R16_USBD_EPR[16];
extern volatile uint16_t R16_USBD_CNTR;
extern volatile uint16_t R16_USBD_ISTR;
extern volatile uint16_t R16_USBD_DADDR;

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

// Everything in this code is controlled by various state machines

// USB device stack control transfer state handling
// state_x[7:0] = addr
#define STATE_SET_ADDR          0x00
// state_x[15:8] = byte pos
// state_x[7:0] = bytes left
#define STATE_GET_DESC          0x01
#define STATE_GET_STR_SERIAL    0x02
#define STATE_CTRL_SIMPLE_IN    0x03

// USB MSC state handling
// msc_state contains both the state *and* request sense:
//  state[31:24] = additional sense code
//  state[23:20] = sense key
//  state[7:0] = state
#define STATE_WANT_CBW          0x00
#define STATE_SENT_CSW          0x01
#define STATE_SENT_CSW_REBOOT   0x02
#define STATE_SENT_DATA_IN      0x03
//  state[10:8] = sector fragment
#define STATE_SEND_MORE_READ    0x04
//  state[12] = uf2 good so far
//  state[10:8] = sector fragment
#define STATE_WAITING_FOR_WRITE 0x05

// Because this code is running completely RAM-less
// (other than explicit usage of USBD RAM),
// there is no stack, and thus no space to spill registers.
// WARNING: This means that only one level of
// non-inline subroutine calls are possible.
// This is checked by manually searching the assembly listing
// for access to the sp register.

// Just to be entirely sure, we've marked some functions as
// __attribute__((always_inline)). Other functions are
// explicitly *outlined* in order to save code size.
// This is not entirely stable and relies on hand-checking
// the generated assembly.

__attribute__((always_inline)) static inline void set_ep_mode(uint32_t epidx, uint32_t epaddr, uint32_t eptype, uint32_t stat_rx, uint32_t stat_tx, uint32_t xtra, int clear_dtog) {
    uint32_t val = R16_USBD_EPR[epidx];
    uint32_t cur_stats;
    if (clear_dtog)
        cur_stats = val & 0x7070;
    else
        cur_stats = val & 0x3030;
    uint32_t want_stats = (stat_rx << 12) | (stat_tx) << 4;
    R16_USBD_EPR[epidx] = epaddr | (eptype << 9) | xtra | (cur_stats ^ want_stats);
}
static void set_ep0_ack_in() {
    set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
}
static void set_ep0_stall() {
    set_ep_mode(0, 0, USB_EPTYPE_CONTROL, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
}
static void set_ep1_ack_in() {
    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_ACK, 0, 0);
}
static void set_ep1_ack_out() {
    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 0);
}

__attribute__((always_inline)) static inline uint32_t min(uint32_t a, uint32_t b) {
    if (a <= b)
        return a;
    return b;
}

const uint8_t MODE_SENSE_6[4] __attribute__((aligned(2))) = {
    0x03, 0x00, 0x00, 0x00,
};
const uint8_t MODE_SENSE_10[8] __attribute__((aligned(2))) = {
    0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const uint8_t READ_FORMAT_CAPACITY[12] __attribute__((aligned(2))) = {
    0x00, 0x00, 0x00,
    0x08,
    0x00, 0x00, 0x40, 0x00,     // 8 MiB
    0x02,                       // formatted media
    0x00, 0x02, 0x00,           // 512 bytes / sector
};
const uint8_t READ_CAPACITY[8] __attribute__((aligned(2))) = {
    0x00, 0x00, 0x3f, 0xff,     // 8 MiB
    0x00, 0x00, 0x02, 0x00,
};
static void ep1_send_hardcoded_response(const uint16_t *data, uint32_t len) {
    for (uint32_t i = 0; i < (len + 1) / 2; i++)
        USB_EP1_IN[i] = data[i];
    USB_DESCS[1].count_tx = len;
    set_ep1_ack_in();
}

static void synthesize_block(uint32_t block, uint32_t piece) {
    if (block == 0 || block == 65 || block == 66 || block == 67) {
        const uint16_t *sector_ptr;
        uint32_t sector_sz_16bits;
        if (block == 0) {
            sector_ptr = (uint16_t*)BOOT_SECTOR;
            sector_sz_16bits = (sizeof(BOOT_SECTOR) + 1) / 2;
        } else if (block == 65) {
            sector_ptr = (uint16_t*)ROOT_DIR;
            sector_sz_16bits = (sizeof(ROOT_DIR) + 1) / 2;
        } else if (block == 66) {
            sector_ptr = (uint16_t*)INFO_UF2;
            sector_sz_16bits = (sizeof(INFO_UF2) + 1) / 2;
        } else if (block == 67) {
            sector_ptr = (uint16_t*)INDEX_HTM;
            sector_sz_16bits = (sizeof(INDEX_HTM) + 1) / 2;
        }

        uint32_t cur_offset_16bits = piece * 32;

        int usbofs, secofs;
        for (usbofs = 0, secofs = cur_offset_16bits; usbofs < 32 && secofs < sector_sz_16bits; usbofs++, secofs++)
            USB_EP1_IN[usbofs] = sector_ptr[secofs];
        for (; usbofs < 32; usbofs++)
            USB_EP1_IN[usbofs] = 0;

        if (block == 0 && piece == 7)
            USB_EP1_IN[31] = 0xaa55;
    } else if (block == 1 && piece == 0) {
        USB_EP1_IN[0] = 0xfff8;
        USB_EP1_IN[1] = 0xffff;
        USB_EP1_IN[2] = 0xfff8;
        USB_EP1_IN[3] = 0xfff8;
        for (int i = 4; i < 32; i++)
            USB_EP1_IN[i] = 0;
    } else {
        for (int i = 0; i < 32; i++)
            USB_EP1_IN[i] = 0;
    }

    USB_DESCS[1].count_tx = 64;
    set_ep1_ack_in();
}

static void make_msc_csw(uint32_t dCSWTag, uint32_t error) {
    USB_EP1_IN[0] = 0x5355;
    USB_EP1_IN[1] = 0x5342;
    USB_EP1_IN[2] = dCSWTag;
    USB_EP1_IN[3] = dCSWTag >> 16;
    USB_EP1_IN[4] = 0;      // xxx this isn't very right
    USB_EP1_IN[5] = 0;
    USB_EP1_IN[6] = error;
    USB_DESCS[1].count_tx = 13;
    set_ep1_ack_in();
}

__attribute__((naked)) int main(void) {
    // Make sure this stuff is enabled
    R32_RCC_APB1PCENR |= (1 << 27) | (1 << 28);
    R32_PWR_CTLR |= 1 << 8;
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
    ADDRESS_HI = 0;

    uint32_t msc_state = STATE_WANT_CBW;

    UF2_GOT_BLOCKS[AUTO_BOOT_BITMAP_NUM_HWORDS - 1] = 0;


    while (1) {
        uint32_t usb_int_status = R16_USBD_ISTR;
        if (usb_int_status & (1 << 10)) {
            // RESET
            // set all to STALL, SETUP will come in nonetheless
            set_ep0_stall();
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
            uint32_t ep_status = R16_USBD_EPR[epidx];
            if ((ep_status & (1 << 15)) && (epidx == 0)) {
                if (ep_status & (1 << 11)) {
                    // setup, STAT_RX and STAT_TX both now NAK
                    uint16_t bRequest_bmRequestType = USB_EP0_OUT[0];
                    uint32_t wLength = USB_EP0_OUT[3];
                    if (bRequest_bmRequestType == 0x0080 || bRequest_bmRequestType == 0x0081) {
                        // GET_STATUS
                        USB_EP0_IN[0] = 0;
                        USB_DESCS[0].count_tx = min(2, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep0_ack_in();
                    } else if (bRequest_bmRequestType == 0x0102) {
                        // CLEAR_FEATURE
                        // XXX how is this supposed to work?
                        uint32_t wIndex = USB_EP0_OUT[2];
                        if (wIndex == 0x81) {
                            uint32_t dCSWTag = CSWTAG_LO | (CSWTAG_HI << 16);
                            make_msc_csw(dCSWTag, 1);
                            msc_state = (msc_state & 0xffffff00) | STATE_SENT_CSW;
                            set_ep0_ack_in();
                        } else if (wIndex == 0x01) {
                            set_ep1_ack_out();
                            msc_state = (msc_state & 0xffffff00) | STATE_WANT_CBW;
                            set_ep0_ack_in();
                        } else {
                            set_ep0_stall();
                        }
                    } else if (bRequest_bmRequestType == 0x0500) {
                        // SET_ADDRESS
                        CTRL_XFER_STATE_X = USB_EP0_OUT[1];
                        CTRL_XFER_STATE = STATE_SET_ADDR;
                        USB_DESCS[0].count_tx = 0;
                        set_ep0_ack_in();
                    } else if (bRequest_bmRequestType == 0x0680) {
                        // GET_DESCRIPTOR
                        uint32_t wValue = USB_EP0_OUT[1];
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
                                USB_EP0_IN[i / 2] = *((uint16_t*)(outputting_desc + i));
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                CTRL_XFER_STATE_X = 0;
                                CTRL_XFER_STATE = STATE_GET_DESC;
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                CTRL_XFER_STATE_X = (8 << 8) | (wLength - 8);
                                CTRL_XFER_STATE = STATE_GET_DESC;
                            }
                            set_ep0_ack_in();
                        } else if (wValue == 0x0300) {
                            // string LANGIDs
                            USB_EP0_IN[0] = 0x0304;
                            USB_EP0_IN[1] = 0x0409;
                            USB_DESCS[0].count_tx = min(4, wLength);
                            CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                            set_ep0_ack_in();
                        } else if (wValue == 0x0303) {
                            USB_EP0_IN[0] = 0x0300 | (24 * 2 + 2);
                            USB_EP0_IN[1] = HEXLUT[ESIG_UNIID(0) >> 4];
                            USB_EP0_IN[2] = HEXLUT[ESIG_UNIID(0) & 0xf];
                            USB_EP0_IN[3] = HEXLUT[ESIG_UNIID(1) >> 4];
                            if (wLength < 8) {
                                USB_DESCS[0].count_tx = wLength;
                                CTRL_XFER_STATE_X = 0;
                                CTRL_XFER_STATE = STATE_GET_STR_SERIAL;
                            } else {
                                USB_DESCS[0].count_tx = 8;
                                CTRL_XFER_STATE_X = (8 << 8) | (wLength - 8);
                                CTRL_XFER_STATE = STATE_GET_STR_SERIAL;
                            }
                            set_ep0_ack_in();
                        } else {
                            // bad descriptor
                            set_ep0_stall();
                        }
                    } else if (bRequest_bmRequestType == 0x0880) {
                        // GET_CONFIGURATION
                        USB_EP0_IN[0] = ACTIVE_CONFIG;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep0_ack_in();
                    } else if (bRequest_bmRequestType == 0x0900) {
                        // SET_CONFIGURATION
                        uint32_t wValue = USB_EP0_OUT[1];
                        if (wValue == 0 || wValue == 1) {
                            ACTIVE_CONFIG = wValue;
                            if (wValue) {
                                // activate, allow OUT
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_STALL, 0, 1);
                                msc_state = STATE_WANT_CBW;
                            } else {
                                // deactivate
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_DISABLED, USB_STAT_DISABLED, 0, 0);
                            }
                            USB_DESCS[0].count_tx = 0;
                            set_ep0_ack_in();
                        } else {
                            set_ep0_stall();
                        }
                    } else if (bRequest_bmRequestType == 0x0A81) {
                        // GET_INTERFACE
                        USB_EP0_IN[0] = 0;
                        USB_DESCS[0].count_tx = min(1, wLength);
                        CTRL_XFER_STATE = STATE_CTRL_SIMPLE_IN;
                        set_ep0_ack_in();
                    } else {
                        // unknown
                        set_ep0_stall();
                    }
                } else {
                    // back to stall for everything, expect SETUP
                    set_ep0_stall();
                }
            } else if ((ep_status & (1 << 7)) && (epidx == 0)) {
                switch (CTRL_XFER_STATE) {
                    case STATE_SET_ADDR:
                        R16_USBD_DADDR = 0x80 | (CTRL_XFER_STATE_X & 0x7f);
                        // back to stall for everything, expect SETUP
                        set_ep0_stall();
                        break;
                    case STATE_GET_DESC:
                    case STATE_GET_STR_SERIAL:
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
                                if (CTRL_XFER_STATE == STATE_GET_DESC) {
                                    if (byte_pos + bytes > CTRL_XFER_DESC_SZ)
                                        bytes = CTRL_XFER_DESC_SZ - byte_pos;
                                    for (int i = 0; i < bytes; i+=2)
                                        USB_EP0_IN[i / 2] = *((uint16_t*)(outputting_desc + byte_pos + i));
                                } else {
                                    uint32_t serial_no_pos = (byte_pos - 2) / 2;
                                    uint32_t ascii_bytes = (bytes + 1) / 2;
                                    if (serial_no_pos + ascii_bytes > 24) {
                                        ascii_bytes = 24 - serial_no_pos;
                                        // xxx this should be fine, reads 8 byte chunks
                                        bytes = ascii_bytes * 2;
                                    }
                                    for (int i = 0; i < ascii_bytes; i++)
                                        USB_EP0_IN[i] = HEXLUT[(ESIG_UNIID((serial_no_pos + i) / 2) >> ((1 - ((serial_no_pos + i) % 2)) * 4)) & 0xf];
                                }
                                if (bytes < 8) {
                                    USB_DESCS[0].count_tx = bytes;
                                    CTRL_XFER_STATE_X = 0;
                                } else {
                                    USB_DESCS[0].count_tx = 8;
                                    CTRL_XFER_STATE_X = ((byte_pos + 8) << 8) | (bytes_left - 8);
                                }
                                set_ep0_ack_in();
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
                        if ((USB_DESCS[1].count_rx & 0x3f) != 0x1f || USB_EP1_OUT[0] != 0x5355 || USB_EP1_OUT[1] != 0x4342) {
                            set_ep1_ack_out();
                        } else {
                            CSWTAG_LO = USB_EP1_OUT[2];
                            CSWTAG_HI = USB_EP1_OUT[3];
                            uint32_t dCSWTag = CSWTAG_LO | (CSWTAG_HI << 16);
                            uint32_t dCBWDataTransferLength = USB_EP1_OUT[4] | (USB_EP1_OUT[5] << 16);
                            uint32_t operation_code = USB_EP1_OUT[7] >> 8;

                            switch (operation_code) {
                                case 0x00:
                                    // test unit ready
                                    make_msc_csw(dCSWTag, 0);
                                    msc_state = STATE_SENT_CSW;
                                    break;
                                case 0x03:
                                    // request sense
                                    USB_EP1_IN[0] = 0x0070;
                                    USB_EP1_IN[1] = (msc_state >> 20) & 0xf;
                                    USB_EP1_IN[2] = 0;
                                    USB_EP1_IN[3] = 0;
                                    USB_EP1_IN[4] = 0;
                                    USB_EP1_IN[5] = 0;
                                    USB_EP1_IN[6] = (msc_state >> 24) & 0xff;
                                    USB_EP1_IN[7] = 0;
                                    USB_EP1_IN[8] = 0;
                                    USB_DESCS[1].count_tx = 18;
                                    set_ep1_ack_in();
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x12:
                                    // inquiry
                                    if (USB_EP1_OUT[8] == 0) {
                                        ep1_send_hardcoded_response((uint16_t*)INQUIRY_RESPONSE, sizeof(INQUIRY_RESPONSE));
                                        msc_state = STATE_SENT_DATA_IN;
                                        break;
                                    }
                                    msc_state = STATE_SENT_CSW | (5 << 20) | (0x24 << 24);
                                    set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                    break;
                                case 0x1a:
                                    // mode sense (6)
                                    ep1_send_hardcoded_response((uint16_t*)MODE_SENSE_6, sizeof(MODE_SENSE_6));
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x1b:
                                    // start/stop unit
                                    uint32_t param = USB_EP1_OUT[9] >> 8;
                                    make_msc_csw(dCSWTag, 0);
                                    if (param == 0x02)
                                        msc_state = STATE_SENT_CSW_REBOOT;
                                    else
                                        msc_state = STATE_SENT_CSW;
                                    break;
                                case 0x23:
                                    // read format capacity
                                    ep1_send_hardcoded_response((uint16_t*)READ_FORMAT_CAPACITY, sizeof(READ_FORMAT_CAPACITY));
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x5a:
                                    // mode sense (10)
                                    ep1_send_hardcoded_response((uint16_t*)MODE_SENSE_10, sizeof(MODE_SENSE_10));
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x25:
                                    // READ CAPACITY (10)
                                    // xxx don't bother checking the silly fields
                                    ep1_send_hardcoded_response((uint16_t*)READ_CAPACITY, sizeof(READ_CAPACITY));
                                    msc_state = STATE_SENT_DATA_IN;
                                    break;
                                case 0x28:
                                case 0x2a:
                                    {
                                        // READ (10) / WRITE (10)
                                        // xxx also don't bother checking the flags
                                        // @ 16: flags lba3
                                        // @ 18: lba2 lba1
                                        // @ 20: lba0 group
                                        // @ 22: len1 len0
                                        uint32_t lba = (USB_EP1_OUT[8] << 16) & 0xff000000;
                                        uint32_t tmp = USB_EP1_OUT[9];
                                        lba |= (tmp & 0xff) << 24;
                                        lba |= (tmp & 0xff00);
                                        tmp = USB_EP1_OUT[10];
                                        lba |= (tmp & 0xff);
                                        tmp = USB_EP1_OUT[11];
                                        uint32_t blocks = (tmp >> 8) | ((tmp & 0xff) << 8);

                                        if (blocks > 0x4000 || lba >= 0x4000 || (blocks + lba) > 0x4000) {
                                            msc_state = STATE_SENT_CSW | (5 << 20) | (0x24 << 24);
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_STALL, USB_STAT_STALL, 0, 0);
                                            break;
                                        }

                                        if (blocks == 0) {
                                            make_msc_csw(dCSWTag, 0);
                                            msc_state = STATE_SENT_CSW;
                                            break;
                                        }

                                        SCSI_XFER_BLK_LEFT = blocks;

                                        if (operation_code == 0x28) {
                                            // READ
                                            SCSI_XFER_CUR_LBA = lba;
                                            synthesize_block(lba, 0);
                                            msc_state = STATE_SEND_MORE_READ;
                                        } else {
                                            // WRITE
                                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_NAK, 0, 0);
                                            msc_state = STATE_WAITING_FOR_WRITE;
                                        }
                                        break;
                                    }
                                default:
                                    // illegal command
                                    msc_state = STATE_SENT_CSW | (5 << 20);
                                    if (dCBWDataTransferLength == 0) {
                                        make_msc_csw(dCSWTag, 1);
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
                            if ((USB_EP1_OUT[0] == 0x4655) &&
                                (USB_EP1_OUT[1] == 0x0A32) &&
                                (USB_EP1_OUT[2] == 0x5157) &&
                                (USB_EP1_OUT[3] == 0x9E5D)) {
                                // UF2 magic okay
                                uint32_t flags_lo = USB_EP1_OUT[4];
                                uint32_t address_lo = USB_EP1_OUT[6];
                                uint32_t address_hi = USB_EP1_OUT[7];
                                uint32_t bytes_lo = USB_EP1_OUT[8];
                                uint32_t bytes_hi = USB_EP1_OUT[9];
                                uint32_t familyid = USB_EP1_OUT[14] | (USB_EP1_OUT[15] << 16);
                                uint32_t blocknum_hi = USB_EP1_OUT[11];
                                uint32_t totblocks_hi = USB_EP1_OUT[13];

                                if (bytes_lo == 256 && bytes_hi == 0 && (address_lo & 0xff) == 0 && blocknum_hi == 0 && totblocks_hi == 0) {
                                    if (flags_lo & (0x2000) && familyid == FAMILY_ID) {
                                        // uf2 good so far!

                                        // *preliminary* bounds check
                                        if ((!(flags_lo & 1) && (address_hi >> 8) == 0x08) || ((flags_lo & 1) && (address_hi >> 8) == 0x20)) {
                                            ADDRESS_LO = address_lo;
                                            ADDRESS_HI = address_hi;
                                            BLOCKNUM_LO = USB_EP1_OUT[10];
                                            TOTBLOCKS_LO = USB_EP1_OUT[12];

                                            for (int i = 0; i < 16; i++)
                                                USB_SECTOR_STASH[i] = USB_EP1_OUT[16 + i];

                                            msc_state += 0x1000;
                                        }
                                    }
                                }
                            }
                        } else if (piece >= 1 && piece <= 4) {
                            for (int i = 0; i < (piece != 4 ? 32 : 16); i++)
                                USB_SECTOR_STASH[16 + (piece - 1) * 32 + i] = USB_EP1_OUT[i];
                        }

                        if (piece != 7) {
                            set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_NAK, 0, 0);
                            msc_state += 0x100;
                        } else {
                            // full sector is done
                            if (msc_state & 0x1000) {
                                if (USB_EP1_OUT[30] == 0x6F30 && USB_EP1_OUT[31] == 0x0AB1) {
                                    // uf2 all magics are good!
                                    uint32_t address = ADDRESS_LO | (ADDRESS_HI << 16);
                                    uint32_t blocknum = BLOCKNUM_LO;
                                    uint32_t totblocks = TOTBLOCKS_LO;

                                    if (UF2_GOT_BLOCKS[AUTO_BOOT_BITMAP_NUM_HWORDS - 1] & 0x8000) {
                                        // not first uf2 block
                                        UF2_GOT_BLOCKS[blocknum / 16] |= 1 << (blocknum % 16);
                                    } else {
                                        // first uf2 block, or too big
                                        if (totblocks <= MAX_AUTO_BOOT_BLOCKS) {
                                            for (int i = 0; i < AUTO_BOOT_BITMAP_NUM_HWORDS; i++)
                                                UF2_GOT_BLOCKS[i] = 0;
                                            for (int i = totblocks; i <= MAX_AUTO_BOOT_BLOCKS; i++) {
                                                // deliberate off-by-1 to set high bit
                                                UF2_GOT_BLOCKS[i / 16] |= 1 << (i % 16);
                                            }
                                            UF2_GOT_BLOCKS[blocknum / 16] |= 1 << (blocknum % 16);
                                        }
                                    }

                                    if (address >= 0x08000000 + BOOTLOADER_RESERVED_SZ_BYTES &&
                                        address <= 0x08000000 + THIS_CHIP_FLASH_MAX_SZ_BYTES - 256) {
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
                                            uint32_t val = USB_SECTOR_STASH[i * 2] | (USB_SECTOR_STASH[i * 2 + 1] << 16);
                                            *addr = val;
                                            while (R32_FLASH_STATR & 2) {}
                                        }
                                        R32_FLASH_CTLR = (1 << 16) | (1 << 21);
                                        while (R32_FLASH_STATR & 1) {}
                                        R32_FLASH_CTLR = (1 << 15) | (1 << 7);
                                    }
                                    if (address >= 0x20000000 && address <= 0x20000000 + THIS_CHIP_RAM_MAX_SZ_BYTES - 256) {
                                        for (int i = 0; i < 64; i++) {
                                            volatile uint32_t *addr = (volatile uint32_t *)(address + i * 4);
                                            uint32_t val = USB_SECTOR_STASH[i * 2] | (USB_SECTOR_STASH[i * 2 + 1] << 16);
                                            *addr = val;
                                        }
                                    }
                                }
                            }

                            if (SCSI_XFER_BLK_LEFT == 1) {
                                uint32_t dCSWTag = CSWTAG_LO | (CSWTAG_HI << 16);
                                make_msc_csw(dCSWTag, 0);
                                uint32_t all_blocks = 0xffff;
                                for (int i = 0; i < AUTO_BOOT_BITMAP_NUM_HWORDS; i++)
                                    all_blocks &= UF2_GOT_BLOCKS[i];
                                if (all_blocks == 0xffff)
                                    msc_state = STATE_SENT_CSW_REBOOT;
                                else
                                    msc_state = STATE_SENT_CSW;
                            } else {
                                SCSI_XFER_BLK_LEFT--;
                                set_ep_mode(1, 1, USB_EPTYPE_BULK, USB_STAT_ACK, USB_STAT_NAK, 0, 0);
                                msc_state = STATE_WAITING_FOR_WRITE;
                            }
                        }
                        break;
                }
            } else if ((ep_status & (1 << 7)) && (epidx == 1)) {
                // ep 1 in
                uint32_t dCSWTag = CSWTAG_LO | (CSWTAG_HI << 16);
                switch (msc_state & 0xff) {
                    case STATE_SENT_CSW:
                        set_ep1_ack_out();
                        msc_state = (msc_state & 0xffffff00) | STATE_WANT_CBW;
                        break;
                    case STATE_SENT_CSW_REBOOT:
                        STK_CMPLR = (50 /* ms */ * 12000 /* assume 96 MHz system clock, div8 */);
                        STK_CTLR = 0b111000;
                        STK_CTLR = 0b111001;
                        while (!(STK_SR & 1)) {}
                        STK_CTLR = 0;
                        STK_SR = 0;
                        R16_USBD_CNTR = 0b11;
                        R32_EXTEN_CTR &= ~(1 << 1);
                        if (ADDRESS_HI >> 8 == 0x20) {
                            // ram boot
                            R32_RCC_CFGR0 = (R32_RCC_CFGR0 & ~0b11) | 0b00;
                            while ((R32_RCC_CFGR0 & 0b1100) != 0b0000) {}
                            asm volatile("la t0, 0x20000000\njr t0\n1:\nj 1b\n");
                        } else {
                            // wtf why does this work and the other stuff doesn't?
                            R16_BKP_DATAR10 = 0x4170;
                            PFIC_CFGR = 0xbeef0080;
                            while (1) { asm volatile(""); }
                        }
                        break;
                    case STATE_SENT_DATA_IN:
                        make_msc_csw(dCSWTag, 0);
                        msc_state = STATE_SENT_CSW;
                        break;
                    case STATE_SEND_MORE_READ:
                        uint32_t piece = (msc_state >> 8) & 0b111;
                        if (piece != 7) {
                            synthesize_block(SCSI_XFER_CUR_LBA, piece + 1);
                            msc_state += 0x100;
                        } else {
                            if (SCSI_XFER_BLK_LEFT == 1) {
                                make_msc_csw(dCSWTag, 0);
                                msc_state = STATE_SENT_CSW;
                            } else {
                                synthesize_block(SCSI_XFER_CUR_LBA + 1, 0);
                                SCSI_XFER_CUR_LBA++;
                                SCSI_XFER_BLK_LEFT--;
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
