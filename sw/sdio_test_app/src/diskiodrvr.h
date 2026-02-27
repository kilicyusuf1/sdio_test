#ifndef	DISKIODRVR_H
#define	DISKIODRVR_H

#include <stddef.h>
#include "sdiodrv.h" // Sadece SDIO sürücün kalsın

typedef	struct DISKIODRVR_S *(*DIO_INIT_FN)(void *io_addr);
typedef	int (*DIO_WRITE_FN)(void *, const unsigned, const unsigned, const char *);
typedef	int (*DIO_READ_FN)(void *, const unsigned, const unsigned, char *);
typedef	int (*DIO_IOCTL_FN)(void *, const char, char *);

typedef	struct	DISKIODRVR_S {
	DIO_INIT_FN	dio_init;
	DIO_WRITE_FN	dio_write;
	DIO_READ_FN	dio_read;
	DIO_IOCTL_FN	dio_ioctl;
} DISKIODRVR;

// Sadece SDIO sürücüsünü tanımlıyoruz
static const DISKIODRVR	SDIODRVR  = { 
    (DIO_INIT_FN)&sdio_init, 
    (DIO_WRITE_FN)&sdio_write, 
    (DIO_READ_FN)&sdio_read, 
    (DIO_IOCTL_FN)&sdio_ioctl  
};

typedef	struct	FATDRIVE_S {
	void		*fd_addr;
	const DISKIODRVR	*fd_driver;
	void		*fd_data;
} FATDRIVE;

// MicroBlaze ve Xparameters uyumu için burayı manuel set ediyoruz
#include "xparameters.h"
#define SDIO_ADDR (void *)(XPAR_AXI_RAM_SDIO_WRAPPER_0_BASEADDR)

#define	MAX_DRIVES	1 // Sadece 1 tane SD kartımız var
static FATDRIVE	DRIVES[MAX_DRIVES] = {
	{ SDIO_ADDR, &SDIODRVR, NULL }
};

#endif