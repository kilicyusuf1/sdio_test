#include <stdio.h>
#include "xparameters.h"
#include "sdiodrv.h"
#include <string.h>
#include "xil_io.h" // Doğrudan okuma için
#include "xil_cache.h"

#define SDIO_BASE      XPAR_AXI_RAM_SDIO_WRAPPER_0_BASEADDR
#define DP_RAM_MB_SIDE 0x00050000 
#define TEST_LBA       400 
#define TEST_COUNT     5 

void dump_ram(uint32_t *ptr, int words) {
    xil_printf("--- RAM ICERIGI (0x%08X) ---\r\n", (uint32_t)ptr);
    for(int i = 0; i < words; i++) {
        xil_printf("[%02d]: 0x%08X  ", i, ptr[i]);
        if((i+1)%4 == 0) xil_printf("\r\n");
    }
    xil_printf("-------------------------------\r\n");
}

int main() {
    xil_printf("\r\n--- SDIO DMA DEBUG MODU ---\r\n");

    SDIO *hw_ptr = (SDIO *)SDIO_BASE;
    SDIODRV *sd_card = sdio_init(hw_ptr);

    if (sd_card == NULL) {
        xil_printf("HATA: Init basarisiz!\r\n");
        return -1;
    }

    uint32_t *ram_ptr = (uint32_t *)DP_RAM_MB_SIDE;

    // 1. RAM'i doldur ve kontrol et
    for(int i = 0; i < 128; i++) {
        ram_ptr[i] = 0xAABBCCDD + i;
    }
    xil_printf("1. RAM MicroBlaze tarafından dolduruldu.\r\n");
    dump_ram(ram_ptr, 8); // İlk 8 word'ü gör

    // 2. Yazma Testi
    xil_printf("2. DMA Yazma baslatiliyor...\r\n");
    int ws = sdio_write(sd_card, TEST_LBA, TEST_COUNT, (char *)0);
    
    // YAZMA SONRASI KRITIK KONTROL:
    // Eğer SDIO IP'si RAM'den veriyi ÇEKTİYSE, RAM içeriği değişmez.
    // Ama IP hata yapıp RAM'e bir şeyler yazdıysa burada görürüz.
    
    // 3. RAM'i temizle
    memset(ram_ptr, 0x00, 512);
    xil_printf("3. RAM Sifirlandi (Temizlik).\r\n");

    // 4. Okuma Testi
    xil_printf("4. DMA Okuma (SD -> RAM) baslatiliyor...\r\n");
    int rs = sdio_read(sd_card, TEST_LBA, TEST_COUNT, (char *)0);
    
    if (rs != RES_OK) {
        xil_printf("HATA: Okuma basarisiz! CMD status: 0x%08X\r\n", hw_ptr->sd_cmd);
    }

    // 5. SONUC GORUNTULEME
    xil_printf("5. Okuma Sonrasi RAM Durumu:\r\n");
    dump_ram(ram_ptr, 16); // İlk 16 word'ü bas

    xil_printf("--- GENISLETILMIS RAM TARAMASI ---\r\n");
    for(int i = 0; i < 256; i++) {
    if(ram_ptr[i] != 0) {
        xil_printf("VERI BULUNDU! Adres: 0x%08X, Deger: 0x%08X\r\n", (uint32_t)&ram_ptr[i], ram_ptr[i]);
    }
    }
    return 0;
}