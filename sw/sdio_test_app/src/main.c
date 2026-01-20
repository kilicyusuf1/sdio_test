#include <stdint.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_cache.h"

#define SDIO_BASE_ADDR XPAR_SDIO_TOP_0_BASEADDR

// ZipCPU sd_cmd bit maskeleri (BURAYI DOKÜMANINA GÖRE DOLDUR)
// Aşağıdakiler örnektir. Senin projedeki doğru maskeyi buraya koy.
// Eğer sadece ham sd_cmd görmek istiyorsan CARDDET_MASK'ı 0 bırak.

// Card detect/present biti hangisiyse buraya yaz.
// Örn: (1u<<??). Bilmiyorsan 0 bırak ve ham sd_cmd’u izle.
static	const	uint32_t SDIO_CARDDET_MASK = 0x00080000;
// Register offsetleri (ZipCPU SDIO core tipik haritası)
// Senin core’da farklıysa düzelt. Emin değilsen sadece [0] kullan.
#define REG_SD_CMD   0
#define REG_SD_DATA  1
#define REG_SD_PHY   2   // varsa

static inline uint32_t rd(volatile uint32_t *sd, unsigned reg)
{
    return sd[reg];
}

int main(void)
{
    xil_printf("\r\n--- SDIO CD + CMD DEBUG (NO INIT) ---\r\n");

    volatile uint32_t *sd = (volatile uint32_t *)SDIO_BASE_ADDR;

    // Cache ile ilgisi yok ama güvenlik için kapalı kalsın
    // Xil_DCacheDisable();

    xil_printf("Base=0x%08lx\r\n", (unsigned long)SDIO_BASE_ADDR);

    uint32_t last_cmd = 0xFFFFFFFFu;

    while (1) {
        uint32_t cmd  = rd(sd, REG_SD_CMD);
        uint32_t data = rd(sd, REG_SD_DATA);

        if (cmd != last_cmd) {
            last_cmd = cmd;

            xil_printf("sd_cmd=0x%08lx  sd_data=0x%08lx",
                       (unsigned long)cmd, (unsigned long)data);

            // Eğer CD bit maskesini biliyorsan:
            if (SDIO_CARDDET_MASK != 0u) {
                xil_printf("  CARD=%ld", (unsigned long)((cmd & SDIO_CARDDET_MASK) ? 1 : 0));
            }

            xil_printf("\r\n");
        }

        // Basit gecikme
        for (volatile uint32_t d = 0; d < 300000; d++) ;
    }

    // return 0; // unreachable
}
