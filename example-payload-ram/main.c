#include <stdint.h>

#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((constructor(0))) static void setup_qingke() {
    asm volatile(
        // Undocumented "Configure pipelining and instruction prediction"
        "li t0, 0x1f\n"
        "csrw 0xbc0, t0\n"
    );
}

#define R32_RCC_APB2PCENR   (*(volatile uint32_t*)0x40021018)
#define R32_GPIOA_CFGLR     (*(volatile uint32_t*)0x40010800)
#define R32_GPIOA_BSHR      (*(volatile uint32_t*)0x40010810)

int main(void) {
    R32_RCC_APB2PCENR |= 1 << 2;
    R32_GPIOA_CFGLR = (R32_GPIOA_CFGLR & ~0xf) | (0b0010 << 0);
    while (1) {
        R32_GPIOA_BSHR = 1 << 0;
        for (int i = 0; i < 250000; i++)
            asm volatile("");
        R32_GPIOA_BSHR = 1 << 16;
        for (int i = 0; i < 250000; i++)
            asm volatile("");
    }
}
