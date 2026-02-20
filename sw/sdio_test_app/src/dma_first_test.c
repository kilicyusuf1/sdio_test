#include <stdio.h>
#include "xparameters.h"
#include "xil_printf.h"
#include <string.h>

// Address Editor'deki fiziksel adres
#define DP_RAM_MB_SIDE   0x50000000 

int main() {
    xil_printf("\r\n--- MicroBlaze - DP-RAM Erisim Testi ---\r\n");

    // RAM portunu 32-bit pointer olarak tanımlıyoruz
    volatile uint32_t *ram_ptr = (volatile uint32_t *)DP_RAM_MB_SIDE;
    int error_count = 0;

    xil_printf("1. RAM'e veri yaziliyor...\r\n");
    // Birinci test: Ardışık sayılar yaz
    for(int i = 0; i < 64; i++) {
        ram_ptr[i] = 0x12345670 + i;
    }

    xil_printf("2. Veriler geri okunup dogrulanıyor...\r\n");
    for(int i = 0; i < 64; i++) {
        uint32_t read_val = ram_ptr[i];
        if(read_val != (0x12345670 + i)) {
            xil_printf("HATA! Adres: 0x%08X | Yazilan: 0x%08X | Okunan: 0x%08X\r\n", 
                        &ram_ptr[i], (0x12345670 + i), read_val);
            error_count++;
        }
    }

    // İkinci test: Bit pattern testi
    xil_printf("3. Bit pattern testi (0xAAAAAAAA)...\r\n");
    ram_ptr[100] = 0xAAAAAAAA;
    if(ram_ptr[100] != 0xAAAAAAAA) {
        xil_printf("HATA! Bit pattern yazilamadi. Okunan: 0x%08X\r\n", ram_ptr[100]);
        error_count++;
    }

    if(error_count == 0) {
        xil_printf("--- SONUC: MICROBLAZE RAM ERISIMI BASARILI! ---\r\n");
    } else {
        xil_printf("--- SONUC: %d ADET HATA BULUNDU! ---\r\n", error_count);
    }

    return 0;
}