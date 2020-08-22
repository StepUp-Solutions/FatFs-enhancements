/** @file io.c
 *
 * @brief This file contains source codes of IO API
 */

#include "io.h"
#include <stdlib.h>
#include <string.h>

const uint64_t MAX_FILE_SIZE = 4294967294;

static FRESULT buffer_specs(IO_FileDescriptor* fp, UINT bytes, UINT start, UINT* begin, UINT* end, UINT* size);
static uint8_t buffer_exists(IO_FileDescriptor* fp, UINT begin, UINT size);
static FRESULT free_buffer(IO_FileDescriptor* fp, uint8_t ignoreWriteErrors);
static FRESULT alloc_buffer(IO_FileDescriptor* fp, UINT begin, UINT size);
static FRESULT write_cache(IO_FileDescriptor* fp);
static FRESULT modif_cache(IO_FileDescriptor* fp, const void* data, UINT position, UINT btw, UINT* bw);
static FRESULT seek(IO_FileDescriptor* fp, FSIZE_t ofs);
static FRESULT preallocate(IO_FileDescriptor* fp, FSIZE_t size);
static IO_FileDescriptor* allocFileDescriptor();

/**
  * @brief Create and open a contiguous file
  * @param path[IN] File name
  * @param mode[IN] FATFS mode flags
  * @param size[IN] file size
  * @retval Pointer to a IO_FileDescriptor structure, or NULL in case of error
  * @warning if a file with specified name already exists, it will be deleted
  * @note No need to call io_open after. But don't forget to call io_close !
  */
IO_FileDescriptor* io_create_contiguous(const TCHAR* path, BYTE mode, FSIZE_t size)
{
	FRESULT res;
	IO_FileDescriptor* fp = NULL;

	if ((_USE_EXPAND != 1) || (_FS_READONLY != 0))
	{
		return NULL;
	}

	mode |= FA_WRITE|FA_CREATE_ALWAYS;

	fp = io_open(path, mode);
	if (fp == NULL)
	{
		return NULL;
	}

	res = f_expand(fp->file, size, 1);
	if (res != FR_OK)
	{
		io_close(fp);
		if (_FS_MINIMIZE == 0)
		{
			f_unlink(path);
		}
		return NULL;
	}

	fp->actualFileSize = size;
	return fp;
}

/**
  * @brief Open/create a file
  * @param path[IN] File name
  * @param mode[IN] FATFS mode flags
  * @retval Pointer to a IO_FileDescriptor structure, or NULL in case of error
  */
IO_FileDescriptor* io_open(const TCHAR* path, BYTE mode)
{
	FRESULT res;
	WORD sectorSize;
	IO_FileDescriptor* fp = NULL;
	
	fp = allocFileDescriptor();
	if (fp == NULL)
	{
		return NULL;
	}
	
	// Adapt mode if necessary :
	if (mode & FA_WRITE)
	{
		mode |= FA_READ;
	}

	// Actually open the file
	res = f_open(fp->file, path, mode);
	if (res != FR_OK)
	{
		io_close(fp);
		return NULL;
	}
	
	// Updating file descriptor fields value
	fp->isOpen = 1;
	fp->actualFileSize = f_size(fp->file);
	fp->rwPointer =  f_tell(fp->file);

	// Getting sector size
#if (_MAX_SS == _MIN_SS)
		sectorSize = _MAX_SS;
#else
		sectorSize = fp->file->obj.fs->ssize;
#endif
	fp->ssize = sectorSize * BUF_MULTIPLIER;
	
	return fp;
}

/**
  * @brief Reads data from a file.
  * @param fp[IN] IO_FileDescriptor* object
  * @param number[IN] Number of bytes to read
  * @param position[IN] Position of the first byte to read in the file
  * @param btr[IN] Number of bytes to read
  * @param br[OUT] Number of read bytes
  * @retval Buffer containing data, NULL in case of error
  */
void* io_read(IO_FileDescriptor* fp, UINT position, UINT btr, UINT* br)
{
	UINT begin = 0;
	UINT end = 0;
	UINT size = 0;
	UINT offset = 0;
	UINT bytesread = 0;
	FRESULT res;
	uint8_t buf_exist = 0;

	*br = 0;

	if ((btr == 0) || (fp == NULL))
	{
		return NULL;
	}
	
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return NULL;
	}
	
	if (fp->ssize == 0)
	{
		return NULL;
	}

	// Check that the file isn't too small (start of reading block)
	if (position >= f_size(fp->file))
	{
		// Maybe there is cached data that goes over current eof
		if (position >= fp->actualSize + fp->bufferBegin * fp->ssize)
		{
			return NULL;
		}
	}

	// Check that the file isn't too small (end of reading block)
	if (position + btr > f_size(fp->file))
	{
		// Maybe there is cached data that goes over current eof
		if (position + btr > fp->actualSize + fp->bufferBegin * fp->ssize)
		{
			btr = f_size(fp->file) - position;
		}
	}

	// Compute the begin and end sectors of the buffer to use, and its size:
	res = buffer_specs(fp, btr, position, &begin, &end, &size);
	if (res != FR_OK)
	{
		return NULL;
	}

	// Check if the buffer already exists
	buf_exist = buffer_exists(fp, begin, size);

	if (buf_exist == 2)
	{
		// Buffer ready !
		offset = position - fp->bufferBegin * fp->ssize;
		*br = btr;
		fp->rwPointer = *br + position;
		return fp->buffer + offset;
	}
	else if (buf_exist == 0)
	{
		// We have to completely change the buffer.
		res = free_buffer(fp, 0);
		if (res != FR_OK)
		{
			return NULL;
		}

		res = alloc_buffer(fp, begin, size);
		if ((fp->buffer == NULL) || (res != FR_OK))
		{
			return NULL;
		}
	}

	/* offset variable is the difference between the position of a byte
	 *  in the buffer and in the file
	 */
	offset = position - fp->bufferBegin * fp->ssize;

	// Move the read/write pointer to the correct place
	res = seek(fp, fp->bufferBegin * fp->ssize);
	if (res != FR_OK)
	{
		return NULL;
	}

	res = f_read(fp->file, fp->buffer, fp->bufferSize, &bytesread);

	// Update actualSize
	if (fp->actualSize < bytesread)
	{
		fp->actualSize = bytesread;
		if (fp->actualSize + fp->bufferBegin * fp->ssize > fp->actualFileSize)
		{
			fp->actualFileSize = fp->actualSize + fp->bufferBegin * fp->ssize;
		}
	}

	*br = bytesread - offset;
	if (*br > btr)
	{
		*br = btr;
	}
	fp->rwPointer = *br + position;

	if (res != FR_OK)
	{
		return NULL;
	}

	return fp->buffer + offset;
}

/**
  * @brief Writes data to a file.
  * @param fp[IN] IO_FileDescriptor* object
  * @param buff[IN] Pointer to the data to be written
  * @param position[IN] Position of the first byte to write in the file
  * If the position is over file size, the difference will be filled with zeroes.
  * @param btw[IN] Number of bytes to write
  * @param bw[OUT] Number of bytes written
  * @retval FRESULT, same as f_write
  * @warning This function needs that FF_FS_MINIMIZE == 0 when expanding file size
  */
FRESULT io_write(IO_FileDescriptor* fp, const void* buff, UINT position, UINT btw, UINT* bw)
{
	UINT begin = 0;
	UINT end = 0;
	UINT size = 0;
	UINT bytesread = 0;
	FRESULT res;
	void* io_read_ret_val = NULL;

	*bw = 0;

	if (btw == 0)
	{
		return FR_OK;
	}
	
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}

	if (fp->ssize == 0)
	{
		return FR_INT_ERR;
	}
	
	if (position >= MAX_FILE_SIZE)
	{
		// We are over max. file size !
		return FR_INVALID_PARAMETER;
	}
	
	// We have to cast everything into uint64 because by default it's int32, and this whould cause an overflow 
	if ((uint64_t)((uint64_t)position + (uint64_t)btw) > MAX_FILE_SIZE)
	{
		btw = (UINT)(MAX_FILE_SIZE - (uint64_t)position);
	}

	if (btw == 0)
	{
		return FR_OK;
	}

	// Compute the begin and end sectors of the buffer to use, and its size:
	res = buffer_specs(fp, btw, position, &begin, &end, &size);
	if (res != FR_OK)
	{
		return res;
	}

	// Check if the buffer already exists
	if (buffer_exists(fp, begin, size))
	{
		res = modif_cache(fp, buff, position, btw, bw);
		return res;
	}

	res = free_buffer(fp, 0);
	if (res != FR_OK)
	{
		return res;
	}

	res = alloc_buffer(fp, begin, size);
	if (res != FR_OK)
	{
		return res;
	}
	if (fp->buffer == NULL)
	{
		return FR_INT_ERR;
	}

	// Read file to fill the buffer (only read what's necessary)
	if ((position != begin * fp->ssize) && (begin * fp->ssize < f_size(fp->file)))
	{
		io_read_ret_val = io_read(fp, begin * fp->ssize, position - begin * fp->ssize, &bytesread);
		if (io_read_ret_val == NULL)
		{
			return FR_INT_ERR;
		}
	}

	// Write data on the buffer
	res = modif_cache(fp, buff, position, btw, bw);
	return res;
}

/**
  * @brief Saves every cached data
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @retval FRESULT
  */
FRESULT io_sync(IO_FileDescriptor* fp)
{
	FRESULT res = FR_OK;
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}
	
	// First: synchronize FatFs with the buffer
	res = write_cache(fp);
	if (res != FR_OK)
	{
		return res;
	}

	// Synchronize FatFs with the mass storage
	if (_FS_READONLY != 0)
	{
		return FR_DENIED;
	}
	res = f_sync(fp->file);
	return res;
}

/**
  * @brief Returns the size of the file taking in consideration unsaved changes
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param size[out] Pointer to the variable that will contain the size of the file in unit of byte
  * @retval FRESULT error code
  */
FRESULT io_size(IO_FileDescriptor* fp, FSIZE_t* size)
{
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}

	*size = fp->actualFileSize;
	return FR_OK;
}

/**
  * @brief Gives the current position of read/write pointer
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param rwPointer[out] Pointer to the variable to store read/write pointer
  * @retval FRESULT error code
  */
FRESULT io_tell(IO_FileDescriptor* fp, FSIZE_t* rwPointer)
{
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}

	*rwPointer = fp->rwPointer;
	return FR_OK;
}

/**
  * @brief Moves the current position of read/write pointer
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param rwPointer[in] Byte offset from beginning of the file to set read/write pointer.
  * @retval FRESULT error code
  * @note if you set the new pointer over current file size, space will be preallocated in the disk
  */
FRESULT io_lseek(IO_FileDescriptor* fp, FSIZE_t rwPointer)
{
	FRESULT res;
	FSIZE_t size;
	
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}
	
	res = io_size(fp, &size);
	if (res != FR_OK)
	{
		return res;
	}
	
	if (rwPointer > size)
	{
		res = preallocate(fp, rwPointer);
	}
	if (res != FR_OK)
	{
		return res;
	}
	
	// Update actual file size value
	res = io_size(fp, &size);
	if (res != FR_OK)
	{
		return res;
	}
	
	if (rwPointer > size)
	{
		rwPointer = size;
	}
	if (rwPointer != fp->rwPointer)
	{
		fp->rwPointer = rwPointer;
	}
	return FR_OK;
}

/**
  * @brief Changes a file size
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param newSize[in] new file size
  * @retval FRESULT: FR_OK if there is no error.
  * @note When expanding the file, space will be automatically pre allocated
  */
FRESULT io_truncate(IO_FileDescriptor* fp, FSIZE_t newSize)
{
	FRESULT res = FR_OK;
	FSIZE_t currentSize;
	
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}
	
	res = io_size(fp, &currentSize);
	if (res != FR_OK)
	{
		return res;
	}
	
	if (newSize == currentSize)
	{
		// No changes
		return FR_OK;
	}
	else if (newSize > currentSize)
	{
		// Expanding file size
		return preallocate(fp, newSize);
	}
	else if ((newSize < currentSize) && (newSize >= fp->bufferBegin * fp->ssize))
	{
		// Reducing file size, but still using the same buffer
		fp->actualFileSize = newSize;
		fp->actualSize = (UINT)(fp->actualFileSize - fp->bufferBegin * fp->ssize);
	}
	else if ((newSize < currentSize) && (newSize < fp->bufferBegin * fp->ssize))
	{
		// Reducing file size enough to make the buffer useless
		fp->actualFileSize = newSize;
		fp->actualSize = 0;
		fp->unsavedData = 0;
		res = free_buffer(fp, 1);
		if (res != FR_OK)
		{
			return res;
		}
	}

	if (newSize < f_size(fp->file))
	{
		// Updating actual file size on disk (only if size was reduced)
		if ((_FS_READONLY != 0) || (_FS_MINIMIZE != 0))
		{
			return FR_DENIED;
		}

		res = seek(fp, newSize);
		if (res != FR_OK)
		{
			return res;
		}

		res = f_truncate(fp->file);
		if (res != FR_OK)
		{
			return res;
		}
	}

	return FR_OK;
}

/**
  * @brief Tests for an error in a file
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @retval FRESULT: FR_OK if there is no error.
  */
FRESULT io_error(IO_FileDescriptor* fp)
{
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}
	if (f_error(fp->file) == 0)
	{
		return FR_OK;
	}
	return FR_DISK_ERR;
}

/**
  * @brief Changes the timestamp of a file
  * @param path[IN] File name
  * @param year[IN] File creation year
  * @param month[IN] File creation month
  * @param day[IN] File creation day
  * @param hour[IN] File creation hour
  * @param min[IN] File creation minutes
  * @param sec[IN] File creation seconcs
  * @retval FRESULT error code
  */
FRESULT io_set_timestamp(const TCHAR* path, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	FILINFO fno;

	if ((_FS_READONLY != 0) || (_USE_CHMOD != 1))
	{
		// f_utime unavailable !
		return FR_DENIED;
	}

	if ((year < 1970) || (month > 12) || (month == 0) || (day > 31) || (day == 0) || (hour > 24) || (min > 60) || (sec > 60))
	{
		return FR_INVALID_PARAMETER;
	}
	
	fno.fdate = (WORD)(((uint32_t)(year - 1980) * 512U) | month * 32U | day);
	fno.ftime = (WORD)(hour * 2048U | min * 32U | sec / 2U);

	return f_utime(path, &fno);
}

/**
  * @brief Close a file
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @retval FRESULT, same as f_close
  */
FRESULT io_close(IO_FileDescriptor* fp)
{
	FRESULT res;
	FRESULT res_return = FR_OK;
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}
	
	res = free_buffer(fp, 1);
	if (res != FR_OK)
	{
		res_return = res;
	}
	
	res = f_close(fp->file);
	if (res != FR_OK)
	{
		res_return = res;
	}
	
	free(fp->file);
	free(fp);
	fp = NULL;
	
	return res_return;
}

/**
  * @brief Calls malloc to allocate the buffer
  * @param fp[IN] IO_FileDescriptor* object
  * @param begin[IN] First sector
  * @param size[IN] Size of the buffer (in bytes)
  * @retval FRESULT
  */
static FRESULT alloc_buffer(IO_FileDescriptor* fp, UINT begin, UINT size)
{
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}

	fp->bufferSize = size;
	fp->actualSize = 0;
	fp->bufferBegin = begin;
	
	if (size == 0)
	{
		return FR_INVALID_PARAMETER;
	}
	
	fp->buffer = malloc(size * sizeof(uint8_t));
	
	return FR_OK;
}

/**
  * @brief Compute specs of the buffer that will be used to be aligned with sectors
  * @param fp[IN] IO_FileDescriptor* object
  * @param bytes[IN] Number of bytes to read/write
  * @param start[IN] Position of the first byte to be read/written
  * @param begin[OUT] First sector
  * @param size[OUT] Size of the buffer (in bytes)
  * @retval FRESULT
  */
static FRESULT buffer_specs(IO_FileDescriptor* fp, UINT bytes, UINT start, UINT* begin, UINT* end, UINT* size)
{
	uint64_t begin_tmp = 0;
	uint64_t end_tmp = 0;
	uint64_t size_tmp = 0;
	
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}

	if (fp->ssize == 0)
	{
		return FR_INVALID_PARAMETER;
	}
	// Compute the begin and end sectors of the buffer to use:
	if (((start / fp->ssize) * fp->ssize) > fp->actualFileSize)
	{
		// If buffer starts after eof
		begin_tmp = (uint64_t)(fp->actualFileSize / fp->ssize);
	}
	else
	{
		begin_tmp = start / fp->ssize;
	}

	end_tmp = (uint64_t)((uint64_t)start + (uint64_t)bytes - (uint64_t)1) / fp->ssize;
	
	/* It's not a issue if the buffer ends after the file ending thanks
	 * to actualSize field in IO_FileDescriptor
	 */
	size_tmp = (end_tmp - begin_tmp + (uint64_t)1) * fp->ssize;
	
	if (size_tmp > MAX_BUFFER_SIZE)
	{
		return FR_NOT_ENOUGH_CORE;
	}
	
	*begin = (UINT)begin_tmp;
	*end = (UINT)end_tmp;
	*size = (UINT)size_tmp;
	
	return FR_OK;
}

/**
  * @brief Checks if the buffer with appropriate specs already exists
  * @param fp[IN] IO_FileDescriptor* object
  * @param begin[IN] First sector
  * @param size[IN] Size of the buffer (in bytes)
  * @retval 2 if the perfect buffer exists, 1 if the buffer exists but its contents end too soon, 0 else
  */
static uint8_t buffer_exists(IO_FileDescriptor* fp, UINT begin, UINT size)
{
	if (fp->buffer == NULL)
	{
		// No buffer allocated
		return 0;
	}
	if (begin < fp->bufferBegin)
	{
		// Buffer allocated but its contents start too far in the file system
		return 0;
	}
	if (size + begin * fp->ssize > fp->bufferSize + fp->bufferBegin * fp->ssize)
	{
		// Buffer allocated but it ends too soon in the file system
		return 0;
	}
	if (size + begin * fp->ssize > fp->actualSize + fp->bufferBegin * fp->ssize)
	{
		// Buffer allocated but its actual contents end too soon in the file system
		return 1;
	}
	return 2;
}

/**
  * @brief Calls f_write on cached data
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @retval FRESULT
  */
static FRESULT write_cache(IO_FileDescriptor* fp)
{
	uint32_t byteswritten = 0;
	FRESULT res = FR_OK;

	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}

	if (fp->unsavedData)
	{
		FSIZE_t newSize;
		
		res = io_size(fp, &newSize);
		if (res != FR_OK)
		{
			return res;
		}
		
		// Pre-allocate space (and automatically stop if disk is full)
		res = preallocate(fp, newSize);

		// Reposition the read/write pointer
		res = seek(fp, fp->bufferBegin * fp->ssize);
		if (res != FR_OK)
		{
			return res;
		}

		// Check that f_write is available:
		if (_FS_READONLY != 0)
		{
			return FR_DENIED;
		}

		// Call FatFs API:
		res = f_write(fp->file, fp->buffer, fp->actualSize, (UINT*)&byteswritten);
		if (res != FR_OK)
		{
			return res;
		}
		else if (byteswritten != fp->actualSize)
		{
			return FR_INT_ERR;
		}
		fp->unsavedData = 0;
	}
	return res;
}

/**
  * @brief Write data on the cache
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param data[IN] Pointer to the data to be written
  * @param position[IN] Position of the first byte to write in the file
  * @param btw[IN] Number of bytes to write
  * @param bw[OUT] Number of bytes written
  * @retval None
  */
static FRESULT modif_cache(IO_FileDescriptor* fp, const void* data, UINT position, UINT btw, UINT* bw)
{
	UINT offset;

	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}

	offset = position - fp->bufferBegin * fp->ssize;

	while (offset > fp->actualSize)
	{
		// We have to fill with zeros (expanding file size)
		(fp->buffer)[fp->actualSize] = '\0';
		fp->actualSize += 1;
	}

	*bw = btw;
	fp->unsavedData = 1;
	if (fp->actualSize < offset + btw)
	{
		fp->actualSize = offset + btw;
	}

	if (fp->actualSize + fp->bufferBegin * fp->ssize > fp->actualFileSize)
	{
		fp->actualFileSize = fp->actualSize + fp->bufferBegin * fp->ssize;
	}

	memcpy((void *)(fp->buffer + offset), data, btw);
	fp->rwPointer = *bw + position;
	return FR_OK;
}

/**
  * @brief Free the buffer associated with a file, and save data if necessary
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param ignoreWriteErrors[IN] if the buffer must be discarded even if there was a write error
  * @retval FRESULT
  * @note check that fp->buffer == NULL to be sure that the buffer is free
  */
static FRESULT free_buffer(IO_FileDescriptor* fp, uint8_t ignoreWriteErrors)
{
	FRESULT res = FR_OK;
	if (fp == NULL)
	{
		return FR_INVALID_OBJECT;
	}

	if (fp->buffer != NULL)
	{
		res = write_cache(fp);
		if ((ignoreWriteErrors == 0) && (res != FR_OK))
		{
			return res;
		}
		free(fp->buffer);
		fp->buffer = NULL;
	}
	fp->bufferBegin = 0;
	fp->bufferSize = 0;
	fp->unsavedData = 0;
	fp->actualSize = 0;

	return res;
}

/**
  * @brief moves the fatfs read/write pointer of an open file object
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param ofs[in] New read/write pointer
  * @retval FRESULT
  */
static FRESULT seek(IO_FileDescriptor* fp, FSIZE_t ofs)
{
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}
	if (_FS_MINIMIZE > 2)
	{
		return FR_DENIED;
	}
	return f_lseek(fp->file, ofs);
}

/**
  * @brief Creates a new file descriptor
  * @retval Pointer to allocated file descriptor
  */
static IO_FileDescriptor* allocFileDescriptor()
{
	IO_FileDescriptor* fp = NULL;
	
	// Allocate file object structures :
	fp = malloc(sizeof(IO_FileDescriptor));
	if (fp == NULL)
	{
		return NULL;
	}
	
	fp->file = malloc(sizeof(FIL));
	if (fp->file == NULL)
	{
		free(fp);
		return NULL;
	}
	
	// Initialize everything:
	fp->buffer = NULL;
	fp->bufferBegin = 0;
	fp->bufferSize = 0;
	fp->unsavedData = 0;
	fp->actualSize = 0;
	fp->actualFileSize = 0;
	fp->rwPointer = 0;
	fp->isOpen = 0;
	
	return fp;
}

/**
  * @brief Preallocates space for the file
  * @param fp[IN] Pointer to the file IO_FileDescriptor* object structure
  * @param size[in] New file size
  * @retval FRESULT
  */
static FRESULT preallocate(IO_FileDescriptor* fp, FSIZE_t size)
{
	FRESULT res;
	
	if ((fp == NULL)|| (fp->isOpen == 0))
	{
		return FR_INVALID_OBJECT;
	}

	if (size <= f_size(fp->file))
	{
		return FR_OK;
	}
	
	// FatFs pre-allocates space thanks to this line :
	res = seek(fp, size);
	if (res != FR_OK)
	{
		return res;
	}
	// Update file size (use f_tell instead of size argument in case disk if full):
	fp->actualFileSize = f_tell(fp->file);
	return FR_OK;
}
