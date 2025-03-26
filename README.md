# KALEM OS - Türkiye'nin İşletim Sistemi

KALEM OS, sıfırdan geliştirilmiş, x86/x64 mimarisi üzerinde çalışan, Türkçe odaklı bir işletim sistemidir. Sistem, modüler tasarım, yüksek performans ve Türkçe dil desteği temel ilkeleriyle geliştirilmektedir.

## Özellikler

- x86/x64 mimarisine yönelik tasarım
- Mikro çekirdek mimarisi
- Türkçe dil desteği
- Yapay zeka entegrasyonu (planlanan)
- Modern arayüz (planlanan)

## Sistem Gereksinimleri

### Geliştirme Ortamı
- Linux veya Windows (WSL önerilir)
- GCC çapraz derleyici (cross-compiler)
- NASM assembler
- GNU Make
- QEMU emülatör
- XorrISO veya grub-mkrescue

### Çalıştırma Gereksinimleri
- En az 32MB RAM
- x86 veya x64 işlemci
- GRUB önyükleyici

## Derleme

Sistemi derlemek için:

```bash
make
```

ISO dosyası oluşturmak için:

```bash
make iso
```

QEMU üzerinde çalıştırmak için:

```bash
make run
```

## Proje Yapısı

- `src/bootloader/`: Önyükleyici kodları
- `src/kernel/`: Çekirdek kodları
- `src/kernel/mm/`: Bellek yönetimi
- `src/include/`: Başlık dosyaları
- `src/drivers/`: Cihaz sürücüleri
- `src/libs/`: Genel kütüphaneler
- `src/userspace/`: Kullanıcı alanı programları

## Lisans

Bu proje [GPLv3 lisansı](LICENSE) altında yayınlanmaktadır.

## Katkıda Bulunma

Projeye katkıda bulunmak istiyorsanız, lütfen bir çatal (fork) oluşturun ve değişikliklerinizi bir çekme isteği (pull request) ile gönderin. Her türlü katkı memnuniyetle karşılanmaktadır.

## İletişim

 