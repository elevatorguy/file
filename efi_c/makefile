# Aiming for (mostly) POSIX.1-2024 compliance
.POSIX:

# Set architecture and machine type if wanted
# Can override on command line with 'ARCH=<arch> MACHINE=<machine> make'
ARCH 	?= x86_64
MACHINE ?= unknown

ifeq ($(OS), Windows_NT)
HOST_OS     = Windows
QEMU_SCRIPT = qemu_$(ARCH).bat
else
HOST_OS     = Linux
QEMU_SCRIPT = ./qemu_$(ARCH).sh
endif

# Uncomment disk image program & shell script 
DISK_IMG_PGM = file.exe

DISK_IMG_FOLDER = ../UEFI-GPT-image-creator/

ifeq ($(ARCH), x86_64)
OVMF = $(DISK_IMG_FOLDER)bios64.bin
endif
ifeq ($(ARCH), aarch64)
OVMF = QEMU_EFI_AARCH64.raw
endif

# Uncomment CC/LDFLAGS for EFI object - gcc
#EFICC       = $(ARCH)-w64-mingw32-gcc
#EFI_LDFLAGS = \
	-nostdlib \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment CC/LDFLAGS for EFI object - clang
EFICC       = icx -target $(ARCH)-$(MACHINE)-windows
EFI_LDFLAGS = \
	-nostdlib \
	-fuse-ld=lld-link
EFI_LDFLAGS2 = \
	-link \
	/subsystem:efi_application \
	/entry:efi_main

# PE32+ files
PECC  = $(ARCH)-w64-mingw32-gcc
PELD  = $(ARCH)-w64-mingw32-ld

KERNEL_SRC     = kernel.c

EFISRC  = file.c
EFIOBJ  = $(EFISRC:%.c=%_$(ARCH).o)
DEPENDS = $(EFIOBJ:.o=.d) $(KERNEL_SRC:.c=.d)

EFI_APP = FILE.EFI

# Uncomment kernel binary format to build
KERNEL  = kernel.elf        # ELF64 PIE kernel binary

FONT = ter-132n.psf	# PSF2 Bitmapped Font: Terminus 16x32 ISO8859-1

# Add kernel binary to new disk image
ADD_KERNEL = \
	cd $(DISK_IMG_FOLDER) && $(DISK_IMG_PGM) -ds 95 -es 1000 -ae /EFI/BOOT/ ../efi_c/$(EFI_APP) /EFI/ DRIVER -ad ../efi_c/kernel ../efi_c/$(FONT) && move file.img K: && del FILE.TXT

all: $(EFI_APP) $(KERNEL)

$(DISK_IMG_FOLDER)/$(DISK_IMG_PGM):
	cd $(DISK_IMG_FOLDER) && $(MAKE) 

# Common CFLAGS
#todo: verify that the Q the same as not Q
QFLAGS = \
	-Qstd=c17 \
	-QMMD \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-Qfreestanding \
	-fno-stack-protector	# Freestanding programs do not have libc stack protector functions

CFLAGS = \
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
QFLAGS += -D ARCH=$(ARCH) -D MACHINE=$(MACHINE) -I include

KERNEL_CFLAGS  = $(CFLAGS) -fPIE
KERNEL_LDFLAGS = -e kmain -nostdlib -pie

# ELF files
#ELFCC = icx -target $(ARCH)-$(MACHINE)-elf 
ELFCC = clang -target $(ARCH)-$(MACHINE)-elf
ELFLD = ld.lld

$(EFI_APP): $(EFIOBJ)
	$(EFICC) $(EFI_LDFLAGS) $< $(EFI_LDFLAGS2) -out:$@

$(EFIOBJ): $(EFISRC)
	$(EFICC) $(QFLAGS) -c -o $@ $<

kernel.elf: $(KERNEL_SRC)
	$(ELFCC) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ $< && move $@ kernel
	$(ADD_KERNEL)

-include $(DEPENDS)

clean:
	rm -rf $(EFI_APP) $(KERNEL) [!bios]*.bin* *.d *.efi *.EFI *.elf *.o *.obj *.pe

