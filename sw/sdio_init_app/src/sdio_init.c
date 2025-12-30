#include <stdio.h>
#include <stdint.h>
#include "xparameters.h"
#include "xil_printf.h"

// --- IP Adresi ---
// xparameters.h'daki IP isminizi kontrol edin
#ifndef XPAR_SDIO_TOP_0_BASEADDR
    #define SDIO_BASE_ADDR 0x44A00000 
#else
    #define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR
#endif

// --- Register Yapısı ---
typedef struct {
    volatile uint32_t sd_cmd;       // 0x00
    volatile uint32_t sd_data;      // 0x01
    volatile uint32_t sd_fifa;      // 0x02
    volatile uint32_t sd_fifb;      // 0x03
    volatile uint32_t sd_phy;       // 0x04
} sdio_regs_t;

sdio_regs_t *sdio = (sdio_regs_t *)SDIO_BASE_ADDR;

// --- Bit Maskeleri ve Sabitler ---
#define SDIO_PRESENTN   0x00080000
#define SDIO_REMOVED    0x00040000
#define SDIO_NULLCMD    0x00000080
#define SDIO_CMD_FLAG   0x00000040
#define SDIO_RNONE      0x00000000
#define SDIO_R1         0x00000100  // 48-bit yanıt (CRC var)
#define SDIO_ERR_CLR    0x00008000
#define SDIO_BUSY       0x00106800
#define SDIO_ERR        0x00008000

#define PHY_100KHZ_512B 0x090000FC

// CMD Argümanları
#define CMD8_ARG        0x000001AA 
// ACMD41: HCS(Bit 30)=1 (SDHC destekle), Voltage(Bit 23-8)=Tüm aralık
#define ACMD41_ARG      0x40FF8000 

void delay_loops(volatile int count) {
    while (count > 0) count--;
}

int sd_card_init(void) {
    uint32_t reg_val, response;
    int timeout, init_tries;

    xil_printf("\r\n--- SDIO Baslatma (CMD0 -> CMD8 -> ACMD41) ---\r\n");

    // ==========================================
    // ADIM 1-4: ÖNCEKİ KODUN AYNISI (Özetlendi)
    // ==========================================
    
    // 1. Kart Bekle
    timeout = 10000000;
    do {
        if (--timeout == 0) { xil_printf("HATA: Kart Yok.\r\n"); return -1; }
    } while (sdio->sd_cmd & SDIO_PRESENTN);
    
    // 2. Bayrak Temizle & PHY Ayarla
    sdio->sd_cmd = SDIO_REMOVED | SDIO_NULLCMD;
    sdio->sd_phy = PHY_100KHZ_512B;
    delay_loops(10000); 

    // 3. CMD0 (Reset)
    xil_printf("[CMD0] Reset... ");
    sdio->sd_data = 0;
    sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_RNONE | SDIO_ERR_CLR | 0;
    
    timeout = 100000;
    do { if (--timeout == 0) return -3; } while (sdio->sd_cmd & SDIO_BUSY);
    xil_printf("OK.\r\n");

    // 4. CMD8 (Voltaj Kontrol)
    xil_printf("[CMD8] Voltaj (0x1AA)... ");
    sdio->sd_data = CMD8_ARG;
    sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_R1 | SDIO_ERR_CLR | 8;

    timeout = 100000;
    do { if (--timeout == 0) return -4; } while (sdio->sd_cmd & SDIO_BUSY);

    if (sdio->sd_cmd & SDIO_ERR) {
        xil_printf("HATA: CMD8 Cevap vermedi!\r\n");
        return -5;
    }
    
    response = sdio->sd_data;
    if ((response & 0xFF) == 0xAA) {
        xil_printf("OK (Yanit: 0x%08x)\r\n", response);
    } else {
        xil_printf("HATA: Voltaj/Desen hatasi!\r\n");
        return -6;
    }

    // ==========================================
    // ADIM 5: ACMD41 (Kart Hazirlama Dongusu)
    // ==========================================
    
    xil_printf("[ACMD41] Kart baslatiliyor (Busy Loop)...\r\n");
    
    // Gecikme: CMD8'den hemen sonra kartı yormamak için biraz bekle
    delay_loops(50000); 
    
    init_tries = 200; // Deneme sayısını artırdık
    
    while (init_tries > 0) {
        
        // --- A) CMD55 Gönder (APP_CMD) ---
        sdio->sd_data = 0;
        sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_R1 | SDIO_ERR_CLR | 55;
        
        // Bekle
        do { reg_val = sdio->sd_cmd; } while (reg_val & SDIO_BUSY);
        
        // CMD55 Hata kontrolü
        if (reg_val & SDIO_ERR) {
            // HATA DETAYI YAZDIRMA
            // Hata olsa bile "continue" diyerek tekrar deneyeceğiz.
            // SD kartlar ilk açılışta bazen komut kaçırabilir.
            xil_printf("UYARI: CMD55 Hatasi (Reg: 0x%08x). Tekrar deneniyor...\r\n", reg_val);
            
            init_tries--;
            delay_loops(10000); 
            continue; // Döngünün başına dön ve tekrar dene
        }

        // --- B) ACMD41 Gönder ---
        sdio->sd_data = ACMD41_ARG; // HCS=1
        sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_R1 | SDIO_ERR_CLR | 41;
        
        // Bekle
        do { reg_val = sdio->sd_cmd; } while (reg_val & SDIO_BUSY);
        
        // ACMD41 için hata kontrolü (Opsiyonel, R3 yanıtı bazen CRC hatası gibi görünebilir)
        // Biz doğrudan içeriğe bakacağız.

        response = sdio->sd_data;
        
        // Bit 31 (Busy) kontrolü
        if (response & 0x80000000) {
            xil_printf("TAMAMLANDI! Kart Hazir.\r\n");
            xil_printf("OCR: 0x%08x ", response);
            if (response & 0x40000000) {
                xil_printf("(High Capacity - SDHC)\r\n");
            } else {
                xil_printf("(Standard Capacity - SDSC)\r\n");
            }
            return 0; // Başarı!
        }
        
        // Kart henüz hazır değilse biraz bekle
        init_tries--;
        delay_loops(5000); 
    }

    xil_printf("\r\nHATA: Kart hazir olamadi (Timeout)!\r\n");
    return -8;
}
int main() {
    int status = sd_card_init();
    if (status == 0) {
        xil_printf("\r\n--- INIT BASARILI! Kart IDLE durumunda. ---\r\n");
    }
    return 0;
}