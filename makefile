.POSIX:
.PHONY: all clean

SOURCE = src/efi.c
TARGET = FILE.EFI

# Uncomment for gcc, or move to OS check above, etc.
CC = x86_64-w64-mingw32-gcc \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment for clang, or move to OS check above, etc.
#CC = clang \
	-target x86_64-unknown-windows \
	-fuse-ld=lld-link \
	-Wl,-subsystem:efi_application \
	-Wl,-entry:efi_main

CFLAGS = \
	-std=c17 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-ffreestanding \
	-nostdlib \
	-I include

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET)
