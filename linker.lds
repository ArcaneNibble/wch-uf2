ENTRY( _start )

MEMORY
{
    /* Try to squeeze down to this target size */
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 2K
    /* We have a total of 512 bytes here.
    - Need 256 bytes to buffer a flash page across USB packets
    - Need space for USB buffers
    - Tiny tiny amount of free space
    */
    USBRAM (rw) : ORIGIN = 0x400060c0, LENGTH = 64
}

SECTIONS
{
    PROVIDE(_bootloader_limit = ORIGIN(FLASH) + LENGTH(FLASH));
    PROVIDE(_bootloader_stack = ORIGIN(USBRAM) + LENGTH(USBRAM));

    .vector :
    {
        *(.vector);
        . = ALIGN(4);
    } >FLASH
    
    .text :
    {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(4);
    } >FLASH

    PROVIDE(_data_lma = .);

    .data :
    {
        PROVIDE(_data_vma = .);
        *(.gnu.linkonce.r.*)
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(.sdata .sdata.*)
        *(.sdata2.*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(4);
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
        . = ALIGN(4);
        PROVIDE( _edata = .);
    } >USBRAM AT>FLASH

    .bss :
    {
        PROVIDE( _sbss = .);
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss*)
        *(.gnu.linkonce.b.*)
        *(COMMON*)
        . = ALIGN(4);
        PROVIDE( _ebss = .);
    } >USBRAM

    PROVIDE( _end = _ebss);
}
