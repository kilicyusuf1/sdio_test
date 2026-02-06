#include <stdio.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "sdiodrv.h"  // Kendi header dosyamız
#include <string.h>
#include "xil_cache.h"
#include "sleep.h"

// IP Adresi (xparameters.h'dan kontrol edin)

#define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR

#define NBLK 16

static unsigned char wrm[512*NBLK] __attribute__((aligned(32)));
static unsigned char rdm[512*NBLK] __attribute__((aligned(32)));


int main() {
    
xil_printf("\r\n--- SDIO Driver Test ---\r\n");

volatile uint32_t *sd = (volatile uint32_t*)SDIO_BASE_ADDR;
    SDIO *hw_ptr = (SDIO *)SDIO_BASE_ADDR;

    xil_printf("Starting Standard Init...\r\n");
    SDIODRV *sd_card = sdio_init(hw_ptr);

    if (sd_card == NULL) {
        xil_printf("HATA: Init basarisiz!\r\n");
        return -1;
    }

    xil_printf("INIT SUCCESSFUL!\r\n");
    xil_printf("Capacity: %d Sectors\r\n", (int)sd_card->d_sector_count);
    int size_mb = (int)(sd_card->d_sector_count / 2048); 
    xil_printf("Size: ~%d MB\r\n", size_mb);

    // --- MULTI-BLOCK WRITE + READBACK TEST ---
uint32_t lba = (sd_card->d_sector_count > 8192) ?
               (sd_card->d_sector_count - 4096) : 2048;

xil_printf("MB TEST: LBA=%lu count=%d\r\n", (unsigned long)lba, NBLK);

// 1) Yazma buffer'ini doldur (her blok farklı pattern + imza)
for (int b = 0; b < NBLK; b++) {
    unsigned char *p = &wrm[512*b];
    for (int i = 0; i < 512; i++)
        p[i] = (unsigned char)((i + b) ^ 0x5A);

    // Imza: ilk 4 byte blok index'i taşır
    p[0] = 'M';
    p[1] = 'B';
    p[2] = 'W';
    p[3] = (unsigned char)b;

    // Son 2 byte marker
    p[510] = 0x55;
    p[511] = 0xAA;
}

memset(rdm, 0, sizeof(rdm));

// Cache yönetimi
Xil_DCacheFlushRange((UINTPTR)wrm, 512*NBLK);
Xil_DCacheFlushRange((UINTPTR)rdm, 512*NBLK);

// 2) Multi-block yaz
int ws = sdio_write(sd_card, lba, NBLK, (char*)wrm);
xil_printf("MB write status=%d\r\n", ws);
if (ws != RES_OK) return -1;

// 3) Multi-block oku
int rs = sdio_read(sd_card, lba, NBLK, (char*)rdm);
xil_printf("MB read  status=%d\r\n", rs);
if (rs != RES_OK) return -1;

Xil_DCacheInvalidateRange((UINTPTR)rdm, 512*NBLK);

// 4) Doğrula
int diff = memcmp(wrm, rdm, 512*NBLK);
int bad = 0;

for (int b = 0; b < NBLK; b++) {
    unsigned char *p = &rdm[512*b];
    if (!(p[0]=='M' && p[1]=='B' && p[2]=='W' && p[3]==(unsigned char)b))
        bad++;
    if (!(p[510]==0x55 && p[511]==0xAA))
        bad++;
}

if (diff == 0 && bad == 0) {
    xil_printf("MB READBACK OK (%d blocks)\r\n", NBLK);
} else {
    xil_printf("MB READBACK FAIL diff=%d bad=%d\r\n", diff, bad);
}


    return 0;
}