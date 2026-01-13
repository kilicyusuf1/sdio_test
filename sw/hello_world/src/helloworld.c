#include <stdio.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "sdiodrv.h"  // Kendi header dosyamız

// IP Adresi (xparameters.h'dan kontrol edin)

#define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR

// Veri okumak için RAM'de yer ayıralım
char test_buffer[512];

int main() {
    xil_printf("\r\n--- SDIO Sürücü Testi ---\r\n");

volatile uint32_t *sd = (volatile uint32_t*)SDIO_BASE_ADDR;

    // 1. Donanım işaretçisini oluştur
    SDIO *hw_ptr = (SDIO *)SDIO_BASE_ADDR;

    // 2. Sürücüyü Başlat (Init)
    // Bu fonksiyon ACMD41, Voltaj, Bus Width gibi ayarları otomatik yapar.
    xil_printf("Kart Init baslatiliyor...\r\n");
    SDIODRV *sd_card = sdio_init(hw_ptr);

    if (sd_card == NULL) {
        xil_printf("HATA: Init basarisiz! (Kablo, Güç veya Bitstream kontrolü yapin)\r\n");
        return -1;
    }

    // 3. Kart Bilgilerini Göster
    xil_printf("INIT BASARILI!\r\n");
    xil_printf("Kapasite: %d Sektor\r\n", sd_card->d_sector_count);
    
    // Kapasite hesabı (MB cinsinden)
    // Sektör Sayısı * 512 Byte / 1024 / 1024
    int size_mb = (sd_card->d_sector_count / 2048); 
    xil_printf("Boyut: ~%d MB\r\n", size_mb);


    // 4. Okuma Testi (Sektör 0 - MBR/Boot Sector)
    xil_printf("Sektor 0 okunuyor...\r\n");
    
    // sdio_read(Driver, SektörNo, Adet, Buffer)
    int status = sdio_read(sd_card, 0, 1, test_buffer);

    if (status == RES_OK) {
        xil_printf("Okuma Tamamlandi. İlk 16 Byte:\r\n");
        for(int i=0; i<16; i++) {
            xil_printf("%02X ", (unsigned char)test_buffer[i]);
        }
        xil_printf("\r\n");
        
        // Son iki byte kontrolü (Genelde 0x55 0xAA olur)
        xil_printf("Sektor Sonu İmzasi: %02X %02X\r\n", 
                   (unsigned char)test_buffer[510], 
                   (unsigned char)test_buffer[511]);
                   
    } else {
        xil_printf("HATA: Okuma yapilamadi! Hata Kodu: %d\r\n", status);
    }

    return 0;
}