/* KALEM OS işletim sistemi bağlayıcı betiği */

ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* 1MB'tan başla */
    . = 1M;

    /* Multiboot başlığının ilk olmasını sağla */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    /* Salt okunur veri */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Başlatılmış veri */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Başlatılmamış veri */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Kernel bitiş noktası */
    kernel_start = 1M;
    kernel_end = .;
} 