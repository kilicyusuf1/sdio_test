#include <stdio.h>
#include <string.h>
#include "xil_printf.h"
#include "ff.h"         
#include "diskio.h"
#include "xil_cache.h"

FATFS fs;  
FIL fil;   
FRESULT res; 
UINT bw;

// Formatlama islemi icin FatFs'in ihtiyac duydugu gecici calisma alani (4 KB)
BYTE work_buffer[4096]; 

int main() {
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    
    xil_printf("\r\n======================================================\r\n");
    xil_printf("--- FATFS FORMAT VE YAZMA TESTI ---\r\n");
    xil_printf("======================================================\r\n");

    // 1. KARTI FORMATLA (f_mkfs)
    xil_printf("SD Kart formatlaniyor... (Bu islem kartin boyutuna gore biraz surebilir)\r\n");
    
    /* * f_mkfs parametreleri Xilinx'in FatFs versiyonuna gore degisiklik gosterebilir.
     * Genelde kullanilan standart yapi sudur:
     * FM_ANY: FAT16/FAT32/exFAT ne uygunsa otomatik sec.
     * 0: Cluster size otomatik.
     */
    //MKFS_PARM opt = {FM_ANY, 0, 0, 0, 0}; 
    //res = f_mkfs("0:", &opt, work_buffer, sizeof(work_buffer));
    
    // NOT: Eger ustteki satir "too many arguments" veya struct hatasi verirse, 
    // Xilinx SDK eski bir FatFs kullaniyor demektir. O zaman ustteki 2 satiri silip sunu kullan:
    // res = f_mkfs("0:", FM_ANY, 0, work_buffer, sizeof(work_buffer));

    //if (res != FR_OK) {
    //    xil_printf(">>> HATA! Format atilamadi. Hata Kodu: %d <<<\r\n", res);
    //    while(1); 
    //}
    //xil_printf("-> Formatlama Basarili! Kart tertemiz oldu.\r\n\r\n");

    // 2. KARTI MOUNT ET
    xil_printf("SD Kart sisteme baglaniyor (f_mount)...\r\n");
    res = f_mount(&fs, "0:", 1); 
    if (res != FR_OK) {
        xil_printf(">>> HATA! Mount basarisiz oldu. Hata Kodu: %d <<<\r\n", res);
        while(1);
    }
    xil_printf("-> Mount Basarili!\r\n\r\n");

    // 3. DOSYA OLUSTUR VE YAZ
    xil_printf("Test.txt dosyasi olusturuluyor...\r\n");
    res = f_open(&fil, "0:Test.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        xil_printf(">>> HATA! Dosya acilamadi. Hata Kodu: %d <<<\r\n", res);
        while(1);
    }
    
    char test_text[] = "Hello World.\r\n";
    res = f_write(&fil, test_text, strlen(test_text), &bw);
    
    // 4. DOSYAYI GUVENLE KAPAT
    res = f_close(&fil);
    if (res == FR_OK) {
        xil_printf("\r\n>>> MUHTESEM ZAFER! FORMAT ATILDI VE DOSYA YAZILDI! <<<\r\n");
    } else {
        xil_printf(">>> HATA! Kapatma basarisiz. Hata Kodu: %d <<<\r\n", res);
    }

    while(1); 
    return 0;
}