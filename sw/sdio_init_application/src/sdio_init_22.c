#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"

int main()
{
    // 1. Platformu ve UART'ı hazırla
    init_platform();

    // 2. Açılış Mesajı (TX Testi)
    xil_printf("\n\r");
    xil_printf("==========================================\n\r");
    xil_printf("   UART Test Programi Basladi (Nexys A7)  \n\r");
    xil_printf("   Klavyeden bir tusa basin (Echo Test)   \n\r");
    xil_printf("   Cikmak icin 'q' tusuna basin.          \n\r");
    xil_printf("==========================================\n\r");

    char received_char;

    // 3. Sonsuz Döngü (RX ve Echo Testi)
    while (1) {
        // Klavyeden veri gelmesini bekle (Bloklayıcı fonksiyon)
        received_char = inbyte();

        // Gelen karakteri ekrana geri bas (Echo)
        outbyte(received_char);

        // Eğer 'Enter' tuşuna basıldıysa (\r), alt satıra geç (\n)
        if (received_char == '\r') {
            outbyte('\n');
        }

        // 'q' tuşuna basılırsa döngüden çık
        if (received_char == 'q') {
            break;
        }
    }

    xil_printf("\n\rTest Bitti. Gule gule!\n\r");

    cleanup_platform();
    return 0;
}