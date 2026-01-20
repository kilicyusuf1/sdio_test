#ifndef SDIODRV_H
#define SDIODRV_H

#include <stdint.h>

// --- FATFS Uyumluluk Tanımları (diskio.h yerine) ---
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD, LBA_t, UINT;

// Dönüş Değerleri
#define RES_OK      0
#define RES_ERROR   1
#define RES_WRPRT   2
#define RES_NOTRDY  3
#define RES_PARERR  4

// --- FATFS Sabitleri (diskio.h yerine) ---
#define CTRL_SYNC           0
#define GET_SECTOR_COUNT    1
#define GET_SECTOR_SIZE     2
#define GET_BLOCK_SIZE      3
#define MMC_GET_SDSTAT      13

// --- Register Haritası (SDIO IP Core) ---
typedef struct {
    volatile uint32_t sd_cmd;
    volatile uint32_t sd_data;
    volatile uint32_t sd_fifa;
    volatile uint32_t sd_fifb;
    volatile uint32_t sd_phy;
    volatile uint32_t sd_dma_addr_l;
    volatile uint32_t sd_dma_addr_h;
    volatile uint32_t sd_dma_len;
} SDIO;

// --- Sürücü Kontrol Yapısı ---
typedef struct SDIODRV_S {
    SDIO        *d_dev;           // Donanım adresi
    uint32_t    d_CID[4], d_OCR;  // Kart Kimlik Bilgileri
    char        d_SCR[8], d_CSD[16];
    uint16_t    d_RCA;
    uint32_t    d_sector_count;   // Toplam Sektör Sayısı
    uint32_t    d_block_size;     // Blok Boyutu (512)
} SDIODRV;

// --- Fonksiyon Prototipleri ---
SDIODRV *sdio_init(SDIO *dev);
int sdio_read(SDIODRV *dev, const unsigned sector, const unsigned count, char *buf);
int sdio_write(SDIODRV *dev, const unsigned sector, const unsigned count, const char *buf);

#endif