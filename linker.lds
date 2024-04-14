ENTRY( _start )

MEMORY
{
    /* Try to squeeze down to this target size */
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4K
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

    PROVIDE( R16_USBD_EPR       = 0x40005C00 );
    PROVIDE( R16_USBD_CNTR      = 0x40005C40 );
    PROVIDE( R16_USBD_ISTR      = 0x40005C44 );
    PROVIDE( R16_USBD_DADDR     = 0x40005C4C );
    PROVIDE( __global_pointer$  = 0x40006000 );
    PROVIDE( USB_DESCS          = 0x40006000 );
    PROVIDE( USB_EP0_OUT        = 0x40006020 );
    PROVIDE( USB_EP0_IN         = 0x40006030 );
    PROVIDE( USB_EP1_OUT        = 0x40006040 );
    PROVIDE( USB_EP1_IN         = 0x400060c0 );

    PROVIDE( UF2_GOT_BLOCKS     = 0x40006140 );

    PROVIDE( SCSI_XFER_CUR_LBA  = 0x400061d0 );
    PROVIDE( SCSI_XFER_BLK_LEFT = 0x400061d4 );
    PROVIDE( BLOCKNUM_LO        = 0x400061d8 );
    PROVIDE( TOTBLOCKS_LO       = 0x400061dc );
    PROVIDE( CSWTAG_LO          = 0x400061e0 );
    PROVIDE( CSWTAG_HI          = 0x400061e4 );
    PROVIDE( ADDRESS_LO         = 0x400061e8 );
    PROVIDE( ADDRESS_HI         = 0x400061ec );
    PROVIDE( ACTIVE_CONFIG      = 0x400061f0 );
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
