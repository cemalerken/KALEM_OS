; KALEM OS Bootloader
; GRUB Multiboot özellikli önyükleyici

[BITS 32]
[global mboot]
[global _start]
[extern kernel_main]

; Multiboot başlık bilgileri
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .text
align 4
mboot:
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM
    dd mboot
    dd _start
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

_start:
    ; GRUB tarafından verilen multiboot bilgileri eax'da
    push ebx        ; multiboot bilgisi

    ; Yığın işaretçisini oluştur
    mov esp, stack_top

    ; Çekirdek ana fonksiyonunu çağır
    call kernel_main

    ; Çekirdek dönerse, sonsuz döngüye gir
hang:
    cli             ; Kesmeleri devre dışı bırak
    hlt             ; İşlemciyi durdur
    jmp hang

section .bss
align 16
stack_bottom:
    resb 16384      ; 16 KB'lık yığın
stack_top: 