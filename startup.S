.section .vector,"ax",@progbits
.align 1

.global _start
_start:
    // Set up global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    // Load data section from flash to RAM
    la a0, _data_lma
    la a1, _data_vma
    la a2, _edata
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b
2:

    // Clear bss section
    la a0, _sbss
    la a1, _ebss
    bgeu a0, a1, 2f
1:
    sw zero, (a0)
    addi a0, a0, 4
    bltu a0, a1, 1b
2:

    // Invoke main
    la t0, main
    csrw mepc, t0
    mret
