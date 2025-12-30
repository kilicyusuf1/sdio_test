#include <stdio.h>
#include <stdint.h>
#include "xparameters.h"  // Donanım adreslerini içerir
#include "xil_printf.h"   // Xilinx UART çıktı fonksiyonu

#define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR

// ==========================================
// REGISTER VE MASK TANIMLARI (Dokümantasyona Göre)
// ==========================================

// Register Yapısı (Tablo 5.1)
typedef struct {
    volatile uint32_t sd_cmd;       // Offset: 0x00 - Komut ve Durum
    volatile uint32_t sd_data;      // Offset: 0x01 - Argüman/Veri
    volatile uint32_t sd_fifa;      // Offset: 0x02 - FIFO A
    volatile uint32_t sd_fifb;      // Offset: 0x03 - FIFO B
    volatile uint32_t sd_phy;       // Offset: 0x04 - Fiziksel Katman
} sdio_regs_t;

// Pointer ile donanıma erişim
sdio_regs_t *sdio = (sdio_regs_t *)SDIO_BASE_ADDR;

// CMD Register Bitleri (Şekil 5.1 ve Tablo 5.2/5.3)
#define SDIO_PRESENTN   0x00080000
#define SDIO_REMOVED    0x00040000
#define SDIO_NULLCMD    0x00000080
#define SDIO_CMD_FLAG   0x00000040
#define SDIO_RNONE      0x00000000  // Yanıt Yok
#define SDIO_R1         0x00000100  // R1 Yanıtı (48-bit) 
#define SDIO_ERR_CLR    0x00008000
#define SDIO_BUSY       0x00106800
#define SDIO_ERR        0x00008000  // Hata bayrağı [cite: 664]

// PHY Register Ayarları (Tablo 5.8)
// 100kHz Hız (0xFC) ve 512 Byte Sektör (0x09..)
#define PHY_100KHZ_512B 0x090000FC
// CMD8 Argümanı: 2.7-3.6V (1) + Check Pattern (0xAA)
#define CMD8_ARG        0x000001AA

// ==========================================
// YARDIMCI FONKSİYONLAR
// ==========================================

// Basit gecikme fonksiyonu (işlemci hızına göre süre değişir)
void delay_loops(volatile int count) {
    while (count > 0) count--;
}

// ==========================================
// BAŞLATMA FONKSİYONU
// ==========================================
int sd_card_init_debug(void) {
    uint32_t reg_val;
    int timeout;

    xil_printf("\r\n--- SDIO Baslatma Testi Basladi ---\r\n");
    xil_printf("Base Address: 0x%08x\r\n", SDIO_BASE_ADDR);

    // ---------------------------------------------------------
    // ADIM 1: Kartın Fiziksel Varlığını Kontrol Et
    // ---------------------------------------------------------
    xil_printf("[1] Kart bekleniyor... ");
    
    // SDIO_PRESENTN biti 0 olana kadar bekle (0 = Kart Var) 
    timeout = 10000000;
    do {
        reg_val = sdio->sd_cmd;
        timeout--;
        if (timeout == 0) {
            xil_printf("\r\nHATA: Zaman asimi! Kart takili degil.\r\n");
            return -1;
        }
    } while (reg_val & SDIO_PRESENTN);

    xil_printf("OK! Kart algilandi.\r\n");

    // ---------------------------------------------------------
    // ADIM 2: 'Kart Çıkarıldı' Bayrağını Temizle
    // ---------------------------------------------------------
    // Kart yeni takıldığı için sistem onu "Removed" olarak işaretlemiş olabilir.
    // Bunu temizliyoruz.
    sdio->sd_cmd = SDIO_REMOVED | SDIO_NULLCMD;
    xil_printf("[2] 'Removed' bayragi temizlendi.\r\n");

    // ---------------------------------------------------------
    // ADIM 3: PHY (Hız ve Sektör) Ayarları
    // ---------------------------------------------------------
    // Başlangıç için güvenli hız: 100 kHz, Open-Drain Modu.
    xil_printf("[3] PHY ayarlaniyor (100kHz)... ");
    sdio->sd_phy = PHY_100KHZ_512B;

    // Ayarın donanımda aktif olmasını bekle (Saat hızı değişimi zaman alır) 
    timeout = 100000;
    while ((sdio->sd_phy & 0xFF) != (PHY_100KHZ_512B & 0xFF)) {
        timeout--;
        if (timeout == 0) {
            xil_printf("\r\nHATA: PHY hizi ayarlanamadi!\r\n");
            return -2;
        }
    }
    xil_printf("OK! PHY Kilitlendi.\r\n");

    // Küçük bir bekleme (Elektriksel kararlılık için)
    delay_loops(10000);

    // ---------------------------------------------------------
    // ADIM 4: CMD0 (GO_IDLE_STATE) Gönder
    // ---------------------------------------------------------
    // Kartı resetler ve IDLE moduna sokar. Argüman = 0.
    xil_printf("[4] CMD0 (Reset) gonderiliyor... ");

    sdio->sd_data = 0; // Argüman [cite: 670]
    
    // Komutu yaz: CMD bayrağı (0x40) | Yanıt Yok | Hata Temizle
    // CMD0 kodu 0 olduğu için index eklemeye gerek yok (+0)
    sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_RNONE | SDIO_ERR_CLR; // [cite: 673]

    // ---------------------------------------------------------
    // ADIM 5: İşlemin Bitmesini Bekle (Busy Check)
    // ---------------------------------------------------------
    timeout = 100000;
    do {
        reg_val = sdio->sd_cmd;
        timeout--;
        if (timeout == 0) {
            xil_printf("\r\nHATA: Komut zaman asimi (Controller Busy)!\r\n");
            return -3;
        }
    } while (reg_val & SDIO_BUSY); // 

    xil_printf("OK! Komut tamamlandi.\r\n");

    delay_loops(1000);

    // ==========================================
    // ADIM 5: CMD8 (SEND_IF_COND) GÖNDERİMİ
    // ==========================================
    // Referans: Doküman Sayfa 17, Bölüm 4.5 [cite: 699]
    
    xil_printf("[CMD8] Voltaj kontrolu (0x1AA)... ");

    // 1. Argümanı ayarla (sd_data register'ına yazılır)
    sdio->sd_data = CMD8_ARG; // 0x1AA

    // 2. Komutu gönder
    // CMD Flag | R1 Yanıtı Bekle | Hata Temizle | Komut Index 8
    sdio->sd_cmd = SDIO_CMD_FLAG | SDIO_R1 | SDIO_ERR_CLR | 8;

    // 3. Bitmesini bekle
    timeout = 100000;
    do {
        reg_val = sdio->sd_cmd;
        if (--timeout == 0) {
            xil_printf("\r\nHATA: CMD8 zaman asimi!\r\n");
            return -4;
        }
    } while (reg_val & SDIO_BUSY);

    // 4. Hata Kontrolü (sd_cmd içindeki ERR biti)
    if (sdio->sd_cmd & SDIO_ERR) {
        xil_printf("\r\nHATA: CMD8 sirasinda hata olustu (CRC veya Timeout).\r\n");
        // Eski kartlar CMD8'i desteklemeyebilir, ama modern kartlar destekler.
        return -5;
    }

    // 5. Yanıtı Kontrol Et
    // Doküman diyor ki: "The next 32-bits may be read from the sd_data register" 
    uint32_t response = sdio->sd_data;
    
    xil_printf("Yanit: 0x%08x\r\n", response);

    // Yanıtın son 8 biti bizim gönderdiğimiz 0xAA deseni ile aynı olmalı
    if ((response & 0xFF) == 0xAA) {
        xil_printf("BASARILI: Kart v2.0+ ve voltaj uygun.\r\n");
    } else {
        xil_printf("HATA: Voltaj uyusmazligi veya hatali desen!\r\n");
        return -6;
    }

    return 0;
}

int main() {
    int status = sd_card_init_debug();
    if (status == 0) {
        xil_printf("Init Sekansi (CMD0 -> CMD8) Tamamlandi.\r\n");
    }
    return 0;
}