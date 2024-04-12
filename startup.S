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

    // Call preinit array functions
    la s0, __preinit_array_start
    la s1, __preinit_array_end
    bgeu s0, s1, 2f
1:
    lw t0, (s0)
    jalr t0
    addi s0, s0, 4
    bltu s0, s1, 1b
2:

    // Call init array functions
    la s0, __init_array_start
    la s1, __init_array_end
    bgeu s0, s1, 2f
1:
    lw t0, (s0)
    jalr t0
    addi s0, s0, 4
    bltu s0, s1, 1b
2:

    // Invoke main with a possible mode switch
    la ra, 1f
    la t0, main
    csrw mepc, t0
    mret

1:
    // Return from main, call fini array functions
    la s0, __fini_array_start
    la s1, __fini_array_end
    bgeu s0, s1, 2f
1:
    lw t0, (s0)
    jalr t0
    addi s0, s0, 4
    bltu s0, s1, 1b
2:

    // Finally, loop infinitely
    wfi
    j 2b
