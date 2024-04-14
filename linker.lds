ENTRY( _start )

MEMORY
{
    /* Try to squeeze down to this target size */
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 8K
    LOL_NO_RAM (rw) : ORIGIN = 0x400060c0, LENGTH = 0
}

SECTIONS
{
    PROVIDE(_bootloader_limit = ORIGIN(FLASH) + LENGTH(FLASH));

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
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
        . = ALIGN(4);
    } >FLASH

    PROVIDE( __global_pointer$  = 0x40006000 );
    PROVIDE( USB_DESCS          = 0x40006000 );
    PROVIDE( USB_EP0_OUT        = 0x40006020 );
    PROVIDE( USB_EP0_IN         = 0x40006030 );
    PROVIDE( USB_EP1_OUT        = 0x40006040 );
    PROVIDE( USB_EP1_IN         = 0x400060c0 );
    PROVIDE( ACTIVE_CONFIG      = 0x400061ec );
    PROVIDE( UF2_BLOCKS_LEFT    = 0x400061f0 );
    PROVIDE( CTRL_XFER_STATE    = 0x400061f4 );
    PROVIDE( CTRL_XFER_STATE_X  = 0x400061f8 );
    PROVIDE( CTRL_XFER_DESC_SZ  = 0x400061fc );
    PROVIDE( USB_SECTOR_STASH   = 0x40006200 );

    PROVIDE(_data_lma = .);

    .data :
    {
        PROVIDE(_data_vma = .);
        *(.gnu.linkonce.r.*)
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        *(.sdata .sdata.*)
        *(.sdata2.*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(4);
        . = ALIGN(4);
        PROVIDE( _edata = .);
    } >LOL_NO_RAM AT>FLASH

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
    } >LOL_NO_RAM

    PROVIDE( _end = _ebss);
}
