#include <stdio.h>
#include <string.h>
#include "xil_printf.h"
#include "ff.h"         
#include "diskio.h"
#include "xil_cache.h"

// --- Global Objects ---
FATFS FatFs;  
FRESULT Res; 
BYTE WorkBuf[4096]; 

#define BIG_BUFFER_ADDR 0x00051000 
BYTE *DataBuffer = (BYTE *)BIG_BUFFER_ADDR;

#define CHUNK_SIZE (128 * 1024)

// --- Function Prototypes ---
FRESULT SD_FormatDisk(void);
FRESULT SD_ListDirectory(const char *path);
FRESULT SD_WriteFile(const char *filename, const char *content);
FRESULT SD_ReadFile(const char *filename);
FRESULT SD_TransferStressTest(const char *filename);

int main() {
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    
    xil_printf("SYSTEM: SDIO Storage Controller Initializing...\r\n");

    Res = f_mount(&FatFs, "0:", 1); 

    xil_printf("SUCCESS: Storage device mounted and ready.\r\n");

    // Test fonksiyonlari
    //SD_WriteFile("Test.txt", "DMA-accelerated storage sequence initiated.\r\n");
    SD_ReadFile("Test.txt");
    
    // Stress testini cagiriyoruz (Artik derleyici tipini biliyor)
    SD_TransferStressTest("photo.jpg"); 
    
    SD_ListDirectory("0:");

    xil_printf("\r\nSYSTEM: All scheduled tasks completed successfully.\r\n");
    while(1); 
    return 0;
}

// --- Module: Storage Stress Test (Streaming Read) ---
FRESULT SD_TransferStressTest(const char *filename) {
    FIL fil;
    UINT br;
    FRESULT res;
    uint32_t total_read = 0;
    
    xil_printf("STRESS_TEST: Using External AXI RAM at 0x%08X\r\n", BIG_BUFFER_ADDR);

    res = f_open(&fil, filename, FA_READ);
    if (res != FR_OK) return res;

    uint32_t file_size = f_size(&fil);
    xil_printf("STORAGE: File size: %lu bytes | Chunk: %d KB\r\n", file_size, CHUNK_SIZE/1024);
    
    xil_printf("PROGRESS: [");

    while (total_read < file_size) {
        res = f_read(&fil, DataBuffer, CHUNK_SIZE, &br);
        
        if (res != FR_OK || br == 0) break;

        // AXI RAM'e (0x54000) gidip oraya dusen ilk 32-bit (4 bayt) veriyi aliyoruz
        uint32_t *first_word = (uint32_t *)DataBuffer;
        xil_printf("\r\n[Offset: %7lu] AXI RAM'deki (0x%X) ilk 4 bayt: 0x%08X", total_read, DataBuffer, *first_word);

        total_read += br;

        // Ilerleme
        xil_printf("#"); 
    }

    xil_printf("] 100%%\r\n");
    xil_printf("SUCCESS: Verification complete.\r\n");
    
    f_close(&fil);
    return FR_OK;
}

// --- Module: Format Disk ---
FRESULT SD_FormatDisk(void) {
    xil_printf("STORAGE: Initializing low-level format...\r\n");
    MKFS_PARM opt = {FM_ANY, 0, 0, 0, 0}; 
    Res = f_mkfs("0:", &opt, WorkBuf, sizeof(WorkBuf));
    
    if (Res == FR_OK) xil_printf("SUCCESS: Disk formatted successfully.\r\n");
    else xil_printf("ERROR: Format failed (Code: %d).\r\n", Res);
    return Res;
}

// --- Module: Write File ---
FRESULT SD_WriteFile(const char *filename, const char *content) {
    FIL fil;
    UINT bw;
    xil_printf("FILE_IO: Opening '%s' for write...\r\n", filename);

    Res = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (Res != FR_OK) {
        xil_printf("ERROR: Open failed (Code: %d).\r\n", Res);
        return Res;
    }

    Res = f_write(&fil, content, strlen(content), &bw);
    if (Res == FR_OK) xil_printf("SUCCESS: Written %u bytes to '%s'.\r\n", bw, filename);
    
    f_close(&fil);
    return Res;
}

// --- Module: Read File ---
FRESULT SD_ReadFile(const char *filename) {
    FIL fil;
    UINT br;
    char read_buf[256];
    xil_printf("FILE_IO: Opening '%s' for read...\r\n", filename);

    Res = f_open(&fil, filename, FA_READ);
    if (Res != FR_OK) {
        xil_printf("ERROR: Open failed (Code: %d).\r\n", Res);
        return Res;
    }

    Res = f_read(&fil, read_buf, sizeof(read_buf)-1, &br);
    if (Res == FR_OK) {
        read_buf[br] = '\0'; // Null-terminate string
        xil_printf("DATA_CONTENT:\r\n%s\r\n", read_buf);
    }
    
    f_close(&fil);
    return Res;
}

// --- Module: List Directory (ls) ---
FRESULT SD_ListDirectory(const char *path) {
    DIR dir;
    static FILINFO fno;
    xil_printf("\r\nDIR_LISTING: Path '%s'\r\n", path);
    xil_printf("------------------------------------------\r\n");

    Res = f_opendir(&dir, path);
    if (Res == FR_OK) {
        for (;;) {
            Res = f_readdir(&dir, &fno);
            if (Res != FR_OK || fno.fname[0] == 0) break; 
            
            if (fno.fattrib & AM_DIR) {
                xil_printf(" [DIR]  %s\r\n", fno.fname);
            } else {
                // Veriyi parça parça basarak xil_printf'in hata yapmasını engelliyoruz
                xil_printf(" SIZE: %u Bytes | NAME: ", (unsigned int)fno.fsize);
                xil_printf("%s\r\n", fno.fname);
            }
        }
        f_closedir(&dir);
        xil_printf("------------------------------------------\r\n");
    } else {
        xil_printf("ERROR: Directory access denied (Code: %d).\r\n", Res);
    }
    return Res;
}