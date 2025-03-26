# KALEM OS İşletim Sistemi Makefile

# Derleyici ve bağlayıcı ayarları
CC = gcc
AS = nasm
LD = ld
QEMU = qemu-system-i386

# Derleyici bayrakları
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
         -Wall -Wextra -Werror -c -I./src/include

# Assembly bayrakları
ASFLAGS = -f elf32

# Bağlayıcı bayrakları
LDFLAGS = -m elf_i386 -T src/linker.ld

# Kaynak ve çıktı dizinleri
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

# Kaynak dosyaları
C_SOURCES = $(wildcard $(SRC_DIR)/kernel/*.c) \
            $(wildcard $(SRC_DIR)/kernel/*/*.c)
ASM_SOURCES = $(wildcard $(SRC_DIR)/bootloader/*.asm)

# Nesne dosyaları
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
          $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# Ana hedefler
.PHONY: all clean run iso

all: $(BUILD_DIR)/kernel

# Dizin yapısını kontrol et
$(BUILD_DIR):
	@echo "Dizinler oluşturuluyor..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/kernel/mm
	@mkdir -p $(BUILD_DIR)/bootloader

# Derle: .c -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Derleniyor: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -o $@

# Derle: .asm -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@echo "Assembly derleniyor: $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

# Bağla: .o -> kernel
$(BUILD_DIR)/kernel: $(BUILD_DIR) $(OBJECTS)
	@echo "Bağlanıyor: $@"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@
	@echo "Kernel oluşturuldu!"

# ISO oluştur
iso: $(BUILD_DIR)/kernel
	@echo "ISO oluşturuluyor..."
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/kernel $(ISO_DIR)/boot/
	@cp $(SRC_DIR)/grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o kalemos.iso $(ISO_DIR)
	@echo "ISO oluşturuldu: kalemos.iso"

# QEMU ile çalıştır
run: iso
	@echo "QEMU başlatılıyor..."
	@$(QEMU) -cdrom kalemos.iso

# Temizle
clean:
	@echo "Temizleniyor..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)
	@rm -f kalemos.iso
	@echo "Temizlendi!" 