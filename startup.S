#define R16_BKP_DATAR10                 0x40006C28
#define BOOT_MAGIC_APP_IMMEDIATELY      0x4170
#define BOOT_MAGIC_BOOTLOADER           0x624c

#define R32_PWR_CTLR            0x40007000

#define RCC_BASE                0x40021000
#define R32_RCC_APB1PCENR       0x4002101C
#define R32_RCC_RSTSCKR         0x40021024

#define SYSTICK_BASE            0xE000F000
#define STK_CTLR                0xE000F000
#define STK_SR                  0xE000F004
#define STK_CMPLR               0xE000F010
#define WAIT_TIME   (500 /* ms */ * 1000 /* assume 8 MHz system clock, div8 */)

.section .vector,"ax",@progbits
.align 1

.global _start
_start:
    // Use Adafruit(?)-inspired boot entry, namely "double-reset in short succession to enter bootloader"
    // 1. If the reset wasn't triggered by the nRST pin, go to application
    // 2. On nRST, if not first reset, go to bootloader
    // 3. On nRST, if *is* first reset, set magic and wait
    // 4a. If wait times out, go to application
    // 4b. If wait doesn't time out but a reset happens again, detect "not first" reset and go to bootloader
    la a0, RCC_BASE
    lw a1, (R32_RCC_RSTSCKR-RCC_BASE)(a0)
    slli a1, a1, (31 - 26)
    bgez a1, _bootloader_limit      // if PINRSTF == 0

    // Enable BKP and PWR clock
    lw a1, (R32_RCC_APB1PCENR-RCC_BASE)(a0)
    la a2, (1 << 27) | (1 << 28)
    or a1, a1, a2
    sw a1, (R32_RCC_APB1PCENR-RCC_BASE)(a0)
    // Enable BKP write
    la a0, R32_PWR_CTLR
    lw a1, (a0)
    ori a1, a1, (1 << 8)
    sw a1, (a0)

    // Check for magic values
    la a3, R16_BKP_DATAR10
    lhu a1, (a3)
    la a2, BOOT_MAGIC_APP_IMMEDIATELY
    beq a1, a2, _enter_application_code_magic
    la a2, BOOT_MAGIC_BOOTLOADER
    beq a1, a2, _enter_bootloader

    // Here we're on the first reset
    sh a2, (a3)
    la a0, SYSTICK_BASE
    la a1, WAIT_TIME
    sw a1, (STK_CMPLR-SYSTICK_BASE)(a0)
    la a1, 0b111000
    sw a1, (STK_CTLR-SYSTICK_BASE)(a0)
    // Seems to spuriously fire immediately if start bit is set concurrently
    la a1, 0b111001
    sw a1, (STK_CTLR-SYSTICK_BASE)(a0)
1:
    lw a1, (STK_SR-SYSTICK_BASE)(a0)
    slli a1, a1, (31 - 0)
    bgez a1, 1b
    // Delay done, clear systick and magic
    sw zero, (STK_CTLR-SYSTICK_BASE)(a0)
    sw zero, (STK_SR-SYSTICK_BASE)(a0)
    // If the delay expires, boot the application after all

_enter_application_code_magic:
    sh zero, (a3)
_enter_application_code:
    // Clear all of these enables that we had set
    la a0, R32_PWR_CTLR
    lw a1, (a0)
    xori a1, a1, (1 << 8)
    sw a1, (a0)
    la a0, R32_RCC_APB1PCENR
    lw a1, (a0)
    la a2, (1 << 27) | (1 << 28)
    xor a1, a1, a2
    sw a1, (a0)

    j _bootloader_limit

_enter_bootloader:
    sh zero, (a3)
    la gp, 0
    la sp, 0

    li t0, 0x1f
    csrw 0xbc0, t0

    // Invoke main
    la t0, main
    csrw mepc, t0
    mret
