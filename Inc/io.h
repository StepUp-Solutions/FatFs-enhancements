/** @file io.h
 *
 * @brief This file contains an API to interact efficiently with FAT file system
 */

#ifndef INC_IO_H_
#define INC_IO_H_

#include "fatfs.h"
#include <stdint.h>


#define BUF_MULTIPLIER 1
/* Buffer size, in number of sectors.
 * During the tests, BUF_MULTIPLIER was 1. Increasing its value will consume more memory
 */

#define MAX_BUFFER_SIZE 16384
/*
 * Limit the buffer size.
 * When trying to read or write more than MAX_BUFFER_SIZE bytes, io_write will return FR_NOT_ENOUGH_CORE
 */

typedef struct {
	uint8_t isOpen;            /* Bool telling if the file is opened or not */
	UINT ssize;                /* Sector size * BUF_MULTIPLIER (in bytes) */
	UINT bufferBegin;          /* Buffer beginning (in ssize bytes) */
	UINT bufferSize;           /* Buffer size (in bytes) */
	UINT actualSize;           /* Only useful when increasing file size : actualSize is the number of bytes that have the correct value */
	uint8_t* buffer;           /* Buffer */
	FIL* file;                 /* FATFS File object */
	uint8_t unsavedData;       /* Bool telling if the buffer has been modified */
	FSIZE_t actualFileSize;    /* Actual file size, knowing buffer modifications */
	FSIZE_t rwPointer;         /* Position of the read/write pointer */
} IO_FileDescriptor;


/* File creation, opening and closing */
IO_FileDescriptor* io_create_contiguous(const TCHAR* path, BYTE mode, FSIZE_t size);
IO_FileDescriptor* io_open(const TCHAR* path, BYTE mode);
FRESULT io_close(IO_FileDescriptor* fp);

/* Managing metadata */
FRESULT io_size(IO_FileDescriptor* fp, FSIZE_t* size);
FRESULT io_error(IO_FileDescriptor* fp);
FRESULT io_set_timestamp(const TCHAR* path, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

/* Editing a file contents */
FRESULT io_write(IO_FileDescriptor* fp, const void* buff, UINT position, UINT btw, UINT* bw);
void* io_read(IO_FileDescriptor* fp, UINT position, UINT btr, UINT* br);
FRESULT io_sync(IO_FileDescriptor* fp);
FRESULT io_truncate(IO_FileDescriptor* fp, FSIZE_t newSize);
FRESULT io_tell(IO_FileDescriptor* fp, FSIZE_t* rwPointer);
FRESULT io_lseek(IO_FileDescriptor* fp, FSIZE_t rwPointer);

#endif /* INC_IO_H_ */
