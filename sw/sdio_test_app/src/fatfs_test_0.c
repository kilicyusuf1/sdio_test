#include <stdio.h>
#include "xil_printf.h"
#include "ff.h"
#include "diskio.h"
#include "xparameters.h"
#include "sleep.h"

// FatFs Nesneleri
FATFS fs;           // Filesystem object
FIL fil;            // File object
FRESULT res;        // FatFs return code
UINT bw;            // Bytes written
BYTE work[FF_MAX_SS]; // Working buffer for f_mkfs

int main() {
    xil_printf("\r\n--- SDIO FatFs Test Başlatılıyor ---\r\n");

    // 1. Kartı Mount Et (Sisteme Tanıt)
    // "0:" sürücü numarasını temsil eder (diskio.c'de MAX_DRIVES 1 yapmıştık)
    res = f_mount(&fs, "0:", 1);
    
    if (res == FR_NO_FILESYSTEM) {
        xil_printf("Dosya sistemi bulunamadı, Formatlanıyor...\r\n");
        // Eğer kart boşsa veya formatı bozuksa FAT32 olarak formatla
        res = f_mkfs("0:", 0, work, sizeof(work));
        if (res != FR_OK) {
            xil_printf("HATA: Formatlama başarısız! Kod: %d\r\n", res);
        } else {
            xil_printf("Formatlama başarılı. Tekrar mount ediliyor...\r\n");
            res = f_mount(&fs, "0:", 1);
        }
    }

    if (res != FR_OK) {
        xil_printf("HATA: f_mount başarısız! Hata kodu: %d\r\n", res);
        // Hata 3 (FR_NOT_READY) ise SD kart init olamamıştır, bağlantıları/voltajı kontrol et.
    } else {
        xil_printf("Başarılı: SD Kart mount edildi!\r\n");

        // 2. Bir Dosya Oluştur ve Yaz
        xil_printf("test.txt dosyası oluşturuluyor...\r\n");
        res = f_open(&fil, "test.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
        
        if (res == FR_OK) {
            char *test_data = "ZipCPU SDIO Kontrolcu ve FatFs Testi Basarili!";
            res = f_write(&fil, test_data, strlen(test_data), &bw);
            
            if (res == FR_OK && bw == strlen(test_data)) {
                xil_printf("Başarılı: Dosyaya %d byte yazıldı.\r\n", bw);
            } else {
                xil_printf("HATA: Yazma işlemi başarısız. Kod: %d\r\n", res);
            }

            // Dosyayı kapat
            f_close(&fil);
        } else {
            xil_printf("HATA: Dosya açılamadı! Kod: %d\r\n", res);
        }
    }

    // 3. Kartı Unmount Et
    f_mount(NULL, "0:", 0);
    xil_printf("Test tamamlandı.\r\n");

    return 0;
}
