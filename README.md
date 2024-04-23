# WCH RISC-V UF2 Bootloader

- Triggered by a double reset within 0.5 seconds (like Adafruit bootloaders)
- Needs usage of `R16_BKP_DATAR10`
    - Write 0x4170 ('Ap') to boot into the application immediately without delay
    - Write 0x624c ('bL') to unconditionally enter the bootloader
- Fits in $\leq$ 4096 bytes
- Tested and built with MounRiver GCC V1.91
    - Code size is improved through use of "XW" instructions, which are not upstream
- Supports USBD peripheral *only* (i.e. not USBFS)
    - USBD and USBFS are completely different, and the QFN28 package (which is available in largest quantities on LCSC) only bonds out USBD
- Runs off of internal 8 MHz oscillator (despite stability concerns)
    - The HSI oscillator's tolerance (-1.0% to +1.6%) is well outside of the range specified for USB full speed ($\pm$ 0.25%), but it seems to work in practice n=1 ¯\\\_(ツ)\_/¯
- Should work across most of the CH32V2xx and CH32V3xx family with appropriate changes to hardcoded constants.
- Allows both download to flash and to SRAM with the "not main flash" flag (this is how the RP2040 bootrom works)
    - Flash download address must be 08xxxxxx (i.e. not starting at 0)
    - SRAM download address must be 20xxxxxx
    - The entire size of the SRAM can be used, as USBD contains its own buffer memory independent of the main SRAM
- Auto-reboot on complete download, _but_ **only** works if the download is sufficiently small. Larger downloads will of course still flash, but they will not trigger auto-reboot and a manual reboot will be required.
- Lots of nasty code golfing tricks -- see comments in [bootloader.c](bootloader.c)
