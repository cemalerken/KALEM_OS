# Derleme bayrakları
CFLAGS = -Wall -Wextra -Werror -nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32 -I src/include

# Hedef mimari ve çıktı formatı
ARCH = i386
FORMAT = elf32

# Kaynak dosyaları
KERNEL_SOURCES = src/kernel/main.c \
                 src/kernel/process.c \
                 src/kernel/syscall.c \
                 src/kernel/memory.c

DRIVER_SOURCES = src/drivers/vga.c \
                 src/drivers/keyboard.c \
                 src/drivers/mouse.c \
                 src/drivers/disk.c \
                 src/drivers/pci.c \
                 src/drivers/gui.c

LIB_SOURCES = src/libs/string.c \
              src/libs/math.c \
              src/libs/stdlib.c

USERSPACE_SOURCES = src/userspace/launcher.c \
                   src/userspace/terminal.c \
                   src/userspace/browser.c \
                   src/userspace/text_editor.c \
                   src/userspace/clock.c \
                   src/userspace/media_player.c \
                   src/userspace/paint.c \
                   src/userspace/calculator.c \
                   src/userspace/settings.c \
                   src/userspace/file_manager.c \
                   src/userspace/html5_player.c \
                   src/userspace/kalem_shell.c \
                   src/userspace/context_menu.c \
                   src/userspace/desktop_menu.c \
                   src/userspace/app_context_menu.c \
                   src/userspace/desktop.c \
                   src/userspace/taskbar.c \
                   src/userspace/android_settings.c

# Android desteği için dosyalar
ANDROID_SOURCES = src/android/android.c \
                src/android/runtime/art_main.c \
                src/android/container/container.c \
                src/android/bridge/bridge.c \
                src/android/manager/manager.c

# Tüm kaynakları birleştir
SOURCES = $(KERNEL_SOURCES) $(DRIVER_SOURCES) $(LIB_SOURCES) $(USERSPACE_SOURCES) $(ANDROID_SOURCES)

# Nesne dosyaları
OBJECTS = $(SOURCES:.c=.o)

# Hedef dosya
TARGET = kalemos.bin

# Varsayılan hedef
all: $(TARGET)

# Nesne dosyalarını derle
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Hedef dosyayı oluştur
$(TARGET): $(OBJECTS)
	$(LD) -m $(FORMAT) -T src/link.ld $(OBJECTS) -o $(TARGET)

# Temizleme
clean:
	rm -f $(OBJECTS) $(TARGET)

# QEMU ile sanal makinede çalıştır
run: $(TARGET)
	qemu-system-$(ARCH) -kernel $(TARGET)

# Disk görüntüsü oluştur
disk: $(TARGET)
	dd if=/dev/zero of=kalemos.img bs=1M count=10
	mkfs.fat -F 32 kalemos.img
	mkdir -p mnt
	mount kalemos.img mnt
	cp $(TARGET) mnt/
	umount mnt
	rmdir mnt

# Disk görüntüsü ile çalıştır
run-disk: disk
	qemu-system-$(ARCH) -hda kalemos.img

.PHONY: all clean run disk run-disk 