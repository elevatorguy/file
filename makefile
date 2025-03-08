# Aiming for (mostly) POSIX.1-2024 compliance
.POSIX:

# Set architecture and machine type if wanted
# Can override on command line with 'ARCH=<arch> MACHINE=<machine> make'
ARCH 	?= x86_64
MACHINE ?= unknown

# Uncomment disk image program & shell script 
#DISK_IMG_PGM ::= write_gpt.exe		# Windows
#QEMU_SCRIPT  ::= qemu.bat			# Windows
DISK_IMG_PGM ::= write_gpt			# Linux
QEMU_SCRIPT	 ::= qemu_$(ARCH).sh	# Linux

DISK_IMG_FOLDER ::= bin

# Uncomment CC/LDFLAGS for EFI object - gcc
#EFICC       ::= $(ARCH)-w64-mingw32-gcc
#EFI_LDFLAGS ::= \
	-nostdlib \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment CC/LDFLAGS for EFI object - clang
EFICC       ::= clang -target $(ARCH)-$(MACHINE)-windows 
EFI_LDFLAGS ::= \
	-nostdlib \
	-fuse-ld=lld-link \
	-Wl,-subsystem:efi_application \
	-Wl,-entry:efi_main

# ELF files
ELFCC ::= clang -target $(ARCH)-$(MACHINE)-elf 
ELFLD ::= ld.lld

# PE32+ files
PECC  ::= $(ARCH)-w64-mingw32-gcc
PELD  ::= $(ARCH)-w64-mingw32-ld

# Common CFLAGS
CFLAGS ::= \
	-std=c17 \
	-MMD \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-ffreestanding \
	-fno-stack-protector	# Freestanding programs do not have libc stack protector functions

# Define arch/machine types for #ifdef, etc. use in source files
# -I include for "#include <arch/ARCH/ARCH.h>" or other files under top level "include" directory
CFLAGS += -D ARCH=$(ARCH) -D MACHINE=$(MACHINE) -I include

KERNEL_SRC     ::= src/kernel.c
KERNEL_CFLAGS  ::= $(CFLAGS) -fPIE
KERNEL_LDFLAGS ::= -e kmain -nostdlib -pie

EFISRC  ::= src/efi.c
EFIOBJ  ::= $(EFISRC:%.c=%_$(ARCH).o)
DEPENDS ::= $(EFIOBJ:.o=.d) $(KERNEL_SRC:.c=.d)

# EFI application to use, should automatically boot from UEFI/OVMF if in /EFI/BOOT/ folder in 
#  the EFI System Partition (ESP)
ifeq ($(ARCH), x86_64) 
EFI_APP ::= BOOTX64.EFI
else ifeq ($(ARCH), aarch64) 
EFI_APP ::= BOOTAA64.EFI
endif

# Uncomment kernel binary format to build
KERNEL ::= kernel.elf    # ELF64 PIE kernel binary
#KERNEL ::= kernel.pe     # PE32+ PIE kernel binary
#KERNEL ::= kernel.binelf # Flat binary PIE kernel from ELF file
#KERNEL ::= kernel.binpe  # Flat binary PIE kernel from PE file

FONT ::= ter-132n.psf	# PSF2 Bitmapped Font: Terminus 16x32 ISO8859-1

# Add kernel binary to new disk image
ADD_KERNEL = \
	cd $(DISK_IMG_FOLDER); \
	./$(DISK_IMG_PGM) -ae /EFI/BOOT/ ../src/$(EFI_APP) \
					  -ad ../src/$(KERNEL) ../src/$(FONT);

all: $(DISK_IMG_FOLDER)/$(DISK_IMG_PGM) $(EFI_APP)

$(DISK_IMG_FOLDER)/$(DISK_IMG_PGM):
	cd $(DISK_IMG_FOLDER) && $(MAKE) 

$(EFI_APP): $(EFIOBJ) 
	$(EFICC) $(EFI_LDFLAGS) -o $@ $<
	$(ADD_KERNEL)

$(EFIOBJ): $(EFISRC)
	$(EFICC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(KERNEL_SRC)
	$(ELFCC) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ $<
	$(ADD_KERNEL)

kernel.pe: $(KERNEL_SRC)
	$(PECC) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ $<
	$(ADD_KERNEL)

kernel.binelf: $(KERNEL_SRC)
	$(ELFCC) -c $(KERNEL_CFLAGS) -o kernel.o $<
	$(ELFLD) $(KERNEL_LDFLAGS) -Tkernel.ld --oformat binary -o $@ kernel.o 
	$(ADD_KERNEL)

kernel.binpe: $(KERNEL_SRC)
	$(PECC) -c $(KERNEL_CFLAGS) -o kernel.o $<
	$(PELD) $(KERNEL_LDFLAGS) -Tkernel.ld --image-base=0 -o kernel.obj kernel.o
	objcopy -O binary kernel.obj $@
	$(ADD_KERNEL)

-include $(DEPENDS)

clean:
	rm -rf $(EFI_APP) $(KERNEL) [!bios]*.bin* *.d *.efi *.EFI *.elf *.o *.obj *.pe

