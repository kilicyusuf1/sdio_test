#include <stdio.h>
#include <string.h>
#include "xil_printf.h"
#include "ff.h"         
#include "diskio.h"
#include "xil_cache.h"

// --- Global Objects ---
FATFS FatFs;  
FRESULT Res; 
BYTE WorkBuf[4096]; // Work area for f_mkfs

// --- Function Prototypes ---
FRESULT SD_FormatDisk(void);
FRESULT SD_ListDirectory(const char *path);
FRESULT SD_WriteFile(const char *filename, const char *content);
FRESULT SD_ReadFile(const char *filename);

int main() {
    // Disable CPU Caches to ensure DMA coherency with AXI RAM
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    
    xil_printf("\r\n------------------------------------------------------\r\n");
    xil_printf("SYSTEM: SDIO Controller Initializing...\r\n");
    xil_printf("------------------------------------------------------\r\n");

    // 1. Mount the Filesystem
    Res = f_mount(&FatFs, "0:", 1); 
    if (Res != FR_OK) {
        xil_printf("ERROR: Mount failed (Code: %d). Attempting format...\r\n", Res);
        
        // 2. Format if mount fails (e.g., corrupted filesystem)
        if (SD_FormatDisk() == FR_OK) {
            f_mount(&FatFs, "0:", 1);
        } else {
            xil_printf("CRITICAL: Storage media unusable.\r\n");
            while(1);
        }
    }
    xil_printf("SUCCESS: SD Card mounted.\r\n");

    // 3. File Operations
    //SD_WriteFile("Test_2.txt", "Test 2.\r\n");
    SD_ReadFile("Test_2.txt");
    SD_ListDirectory("0:");

    xil_printf("\r\nSYSTEM: Task sequence completed.\r\n");
    while(1); 
    return 0;
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