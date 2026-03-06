////////////////////////////////////////////////////////////////////////////////
//
// Filename:	sw/diskio.c
// {{{
// Project:	SD-Card controller
//
// Purpose:	This file contains the low-level SD-Card I/O wrappers for use
//		with the FAT-FS file-system library.  This low-level wrappers
//	are specific to systems having either an SDSPI or an SDIO device within
//	them.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// }}}
// Copyright (C) 2016-2025, Gisselquist Technology, LLC
// {{{
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
// }}}
// License:	GPL, v3, as defined and found on www.gnu.org,
// {{{
//		http://www.gnu.org/licenses/gpl.html
//
////////////////////////////////////////////////////////////////////////////////
//
// }}}
#include "ff.h"		// From FATFS
#include "diskio.h"	// From FATFS as well
#include "xparameters.h"	// Defines associated with the driver
//#include "sdspidrv.h"
#include "sdiodrv.h"
//#include "emmcdrv.h"
#include "diskiodrvr.h"

#include <string.h> // memcpy icin

// #define	STDIO_DEBUG
//#include "zipcpu.h"

#ifdef	STDIO_DEBUG
#include <stdio.h>
#define	DBGPRINTF	printf
#else
#define	DBGPRINTF	null
#endif

#define BOUNCE_DMA_ADDR 0x00003000 // DMA'nın göreceği offset adresi (0 tabanlı)
#define BOUNCE_CPU_ADDR 0x00053000 // İşlemcinin (CPU) göreceği mutlak fiziksel adres

static inline	void	null(char *s,...) {}

DSTATUS	disk_status(
	BYTE pdrv
	) {
	// {{{
	unsigned	stat = 0;

	if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr
			|| NULL == DRIVES[pdrv].fd_driver)
		return STA_NODISK;
	if (NULL == DRIVES[pdrv].fd_data)
		DRIVES[pdrv].fd_data= (DRIVES[pdrv].fd_driver->dio_init)(
					DRIVES[pdrv].fd_data);
	if (NULL == DRIVES[pdrv].fd_data
		|| RES_OK != (*DRIVES[pdrv].fd_driver->dio_ioctl)(
			DRIVES[pdrv].fd_data,
					MMC_GET_SDSTAT, (char *)&stat))
		stat = STA_NODISK;

	return	stat;
}
// }}}

DSTATUS	disk_initialize(
	BYTE pdrv
	) {
	// {{{
	if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr
			|| NULL == DRIVES[pdrv].fd_driver) {
		return STA_NODISK;
	} else if (NULL != DRIVES[pdrv].fd_data
		|| NULL != (DRIVES[pdrv].fd_data
				= (*DRIVES[pdrv].fd_driver->dio_init)(
					DRIVES[pdrv].fd_addr))) {
		return RES_OK;
	} else
		return STA_NODISK;
}
// }}}

DRESULT disk_ioctl(
	BYTE pdrv,	// [IN] Drive number
	BYTE cmd,	// [IN] Control command code
	void *buff	// [I/O parameter and data buffer
	) {
	// {{{
	if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr
			|| NULL == DRIVES[pdrv].fd_driver)
		return RES_ERROR;
	return (*DRIVES[pdrv].fd_driver->dio_ioctl)(DRIVES[pdrv].fd_data,
						cmd, buff);
}
// }}}

DWORD	get_fattime(void) {
	// {{{
	DWORD	result;
	unsigned	thedate, clocktime;

#ifdef	_BOARD_HAS_VERSION
	thedate   = *_version;
#else
	thedate = 0x20191001;
#endif
#ifdef	_BOARD_HAS_BUILDTIME
	clocktime = *_buildtime;
#else
	clocktime = 0x0; // Midnight
#endif

#ifdef	_BOARD_HAS_RTC
	clocktime = _rtc->r_clock;
#endif

	unsigned year, month, day, hrs, mns, sec;

	year =  ((thedate & 0xf0000000)>>28)*1000 +
		((thedate & 0x0f000000)>>24)*100 +
		((thedate & 0x00f00000)>>20)*10 +
		((thedate & 0x000f0000)>>16);
	year -= 1980;

	month = ((thedate & 0x00f000)>>12)*10 +
		((thedate & 0x000f00)>> 8);

	day   = ((thedate & 0x00f0)>> 4)*10 +
		((thedate & 0x000f)    );

	hrs   = ((clocktime & 0xf00000)>>20)*10 +
		((clocktime & 0x0f0000)>>16);

	mns   = ((clocktime & 0xf000)>>12)*10 +
		((clocktime & 0x0f00)>> 8);

	sec   = ((clocktime & 0xf0)>> 4)*10 +
		((clocktime & 0x0f));

	result = (sec & 0x01f) | ((mns & 0x3f)<<5)
		| ((hrs & 0x01f)<<11)
		| ((day & 0x01f)<<16)
		| ((month & 0x0f)<<21)
		| ((year & 0x0f)<<21);

	return result;
}
// }}}

/*
DRESULT	disk_read(
	BYTE	pdrv,
	BYTE	*buff,
	DWORD	sector,
	UINT	count) {
	// {{{
	if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr
			|| NULL == DRIVES[pdrv].fd_driver)
		return RES_ERROR;
	return (*DRIVES[pdrv].fd_driver->dio_read)(DRIVES[pdrv].fd_data,
					sector, count, buff);
}
// }}}
*/
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr) return RES_ERROR;

    for(UINT i = 0; i < count; i++) {
        // 1. DMA ile veriyi RAM'e (0x53000) çek
        int res = (*DRIVES[pdrv].fd_driver->dio_read)(DRIVES[pdrv].fd_data, sector + i, 1, (char *)BOUNCE_DMA_ADDR);
        if(res != 0) return RES_ERROR; 

        // 2. Veriyi FatFs'in buff'ına kopyala
        memcpy(buff + (i * 512), (void *)BOUNCE_CPU_ADDR, 512);

        // =========================================================
        // 3. HAYAT KURTARAN DOKUNUŞ: 32-Bit Endianness Düzeltici
        // Donanımın ters dizdiği baytları FatFs için hizaya sokuyoruz!
        // =========================================================
        uint32_t *ptr = (uint32_t *)(buff + (i * 512));
        for(int j = 0; j < 128; j++) { // 128 kelime * 4 bayt = 512 bayt (1 Sektör)
            uint32_t val = ptr[j];
            ptr[j] = ((val & 0xFF000000) >> 24) |
                     ((val & 0x00FF0000) >>  8) |
                     ((val & 0x0000FF00) <<  8) |
                     ((val & 0x000000FF) << 24);
        }
    }
    return RES_OK;
}
/*
DRESULT	disk_write(
	BYTE		pdrv,
	const BYTE	*buff,
	DWORD		sector,
	UINT		count) {
	// {{{
	if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr
			|| NULL == DRIVES[pdrv].fd_driver)
		return RES_ERROR;
	return (*DRIVES[pdrv].fd_driver->dio_write)(DRIVES[pdrv].fd_data,
					sector, count, buff);
}
*/
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv >= MAX_DRIVES || NULL == DRIVES[pdrv].fd_addr) return RES_ERROR;

    for(UINT i = 0; i < count; i++) {
        // 1. FatFs'ten gelen veriyi BOUNCE (Sıçrama) adresine kopyala
        memcpy((void *)BOUNCE_CPU_ADDR, buff + (i * 512), 512);

        // =========================================================
        // 2. YAZMA İÇİN ENDIANNESS DÜZELTİCİ (HATA 2'NİN ÇÖZÜMÜ)
        // DMA veriyi karta ters yazmasın diye önceden biz tersliyoruz!
        // =========================================================
        uint32_t *ptr = (uint32_t *)BOUNCE_CPU_ADDR;
        for(int j = 0; j < 128; j++) {
            uint32_t val = ptr[j];
            ptr[j] = ((val & 0xFF000000) >> 24) |
                     ((val & 0x00FF0000) >>  8) |
                     ((val & 0x0000FF00) <<  8) |
                     ((val & 0x000000FF) << 24);
        }

        // 3. DMA'ya 0x1000 adresinden alıp SD karta yazmasını söyle
        int res = (*DRIVES[pdrv].fd_driver->dio_write)(DRIVES[pdrv].fd_data, sector + i, 1, (char *)BOUNCE_DMA_ADDR);
        if(res != 0) return RES_ERROR;
    }
    return RES_OK;
}
