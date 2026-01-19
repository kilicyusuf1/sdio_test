#include <stdio.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "sdiodrv.h"  // Kendi header dosyamız
#include <string.h>
#include "xil_cache.h"

// IP Adresi (xparameters.h'dan kontrol edin)

#define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR

// 32-byte aligned buffers (cacheline aligned)
static unsigned char wr_buf[512] __attribute__((aligned(32)));
static unsigned char rd_buf[512] __attribute__((aligned(32)));

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


    // --- WRITE + READBACK TEST ---
    uint32_t test_lba;
    if (sd_card->d_sector_count > 4096)
        test_lba = sd_card->d_sector_count - 2048; // sondan güvenli bir yer
    else
        test_lba = 1024;

    xil_printf("Yazma testi: LBA=%lu (Dikkat: veri overwrite!)\r\n", (unsigned long)test_lba);

    // Pattern hazırla
    memset(wr_buf, 0, sizeof(wr_buf));
    for (int i = 0; i < 512; i++)
        wr_buf[i] = (unsigned char)(i ^ 0xA5);

    wr_buf[0]   = 'S';
    wr_buf[1]   = 'D';
    wr_buf[2]   = 'W';
    wr_buf[3]   = 'R';
    wr_buf[508] = 0xDE;
    wr_buf[509] = 0xAD;
    wr_buf[510] = 0xBE;
    wr_buf[511] = 0xEF;

    // Cache flush (yazmadan önce)
    Xil_DCacheFlushRange((UINTPTR)wr_buf, 512);

    // Yaz
    int wstat = sdio_write(sd_card, test_lba, 1, (char*)wr_buf);
    if (wstat != RES_OK) {
        xil_printf("HATA: Yazma basarisiz! Kod=%d\r\n", wstat);
        return -1;
    }
    xil_printf("Yazma OK.\r\n");

    // Oku
    memset(rd_buf, 0, sizeof(rd_buf));
    Xil_DCacheFlushRange((UINTPTR)rd_buf, 512);

    int rstat = sdio_read(sd_card, test_lba, 1, (char*)rd_buf);
    if (rstat != RES_OK) {
        xil_printf("HATA: Readback basarisiz! Kod=%d\r\n", rstat);
        return -1;
    }

    // Cache invalidate (okumadan sonra CPU doğru veriyi görsün)
    Xil_DCacheInvalidateRange((UINTPTR)rd_buf, 512);

    // Karşılaştır
    int diff = memcmp(wr_buf, rd_buf, 512);
    if (diff == 0) {
        xil_printf("READBACK OK: Yazilan veri birebir dogrulandi.\r\n");
        xil_printf("Imza son 4 byte: %02X %02X %02X %02X\r\n",
            rd_buf[508], rd_buf[509], rd_buf[510], rd_buf[511]);
    } else {
        xil_printf("READBACK FAIL: memcmp farkli!\r\n");
        xil_printf("Ilk 16 byte:\r\n");
        for (int i = 0; i < 16; i++) xil_printf("%02X ", rd_buf[i]);
        xil_printf("\r\nSon 16 byte:\r\n");
        for (int i = 496; i < 512; i++) xil_printf("%02X ", rd_buf[i]);
        xil_printf("\r\n");
    }


    return 0;
}