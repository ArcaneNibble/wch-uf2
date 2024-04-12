.PHONY: all clean

RV_ARCH = rv32imacxw

CC = riscv-none-elf-gcc
OBJDUMP = riscv-none-elf-objdump
CFLAGS = -ggdb3 -Os -march=$(RV_ARCH) -ffunction-sections -fdata-sections
LDFLAGS = -ggdb3 -march=$(RV_ARCH) -Wl,--gc-sections --specs=nosys.specs

all: main.elf

main.elf: startup.o main.o

%.elf: linker.lds
	$(CC) -Xlinker -Map=$(@:.elf=.map) -Wl,--script=linker.lds -nostartfiles -o $@ $(filter-out linker.lds,$+)
	$(OBJDUMP) -xdsS $@ >$(@:.elf=.dump)

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf *.map *.dump
