#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xil_printf.h"
#include "ff.h"         
#include "diskio.h"
#include "xil_cache.h"

#define AXI_RAM_BASE    0x00050000

// FatFs objesi hata vermesin diye bir yere koyuyoruz ama bu testte işimiz yok.
FATFS *fs_ptr = (FATFS *)(AXI_RAM_BASE + 0x4000); 

int main() {
    // 1. Cache'i kökten kapatıyoruz
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    
    xil_printf("\r\n==========================================\r\n");
    xil_printf("--- DMA HEDEF ADRES BULMA (SWEEP) TESTI ---\r\n");
    xil_printf("==========================================\r\n");

    // 2. RAM'in ilk 20 KB'ını tamamen SIFIRLA (İzleri sil)
    xil_printf("RAM '00' ile temizleniyor...\r\n");
    memset((void*)AXI_RAM_BASE, 0, 0x5000);

    // 3. Kartı Uyandır
    // Not: disk_read içindeki memcpy'yi sildiğimiz için FatFs veriyi göremeyecek
    // ve f_mount doğal olarak Hata 13 dönecek. Bu tamamen beklenen bir durum!
    xil_printf("Kart init ediliyor (Hata 13 gelmesi normaldir)...\r\n");
    f_mount(fs_ptr, "0:", 1); 

    // 4. Gerçek Test: DMA'yı Manuel Tetikle (Sektör 0)
    xil_printf("\r\nDMA 0x3000 adresine yazıyor...\r\n");
    disk_read(0, NULL, 0, 1); 

    // 5. HAFIZA TARAMASI (RÖNTGEN)
    // Şüpheli adreslere işaretçiler (pointer) atıyoruz
    uint32_t *hedef_0 = (uint32_t *)0x00050000; // Eger DMA her seyi 0'a maskeliyorsa
    uint32_t *hedef_1 = (uint32_t *)0x00051000; // Eger her sey olmasi gerektigi gibiyse
    uint32_t *hedef_2 = (uint32_t *)0x00052000; // Rastgele bir adres (Bos olmali)
    uint32_t *hedef_3 = (uint32_t *)0x00053000;

    xil_printf("\r\n--- RONTGEN SONUCLARI (Ilk 4 Byte) ---\r\n");
    xil_printf("0x50000 (DMA = 0)      : %08X\r\n", hedef_0[0]);
    xil_printf("0x51000 (DMA = 0x1000) : %08X\r\n", hedef_1[0]);
    xil_printf("0x52000 (Rastgele)     : %08X\r\n", hedef_2[0]);
    xil_printf("0x53000                : %08X\r\n", hedef_3[0]);

    xil_printf("\r\nTest tamamlandi.\r\n");
    while(1); 
    return 0;
}