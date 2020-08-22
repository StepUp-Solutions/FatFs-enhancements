/** @file IO_tests.c
 *
 * @brief This file contains source codes of IO tests
 */

#include <stdlib.h>
#include <string.h>

#include "IO_tests.h"
#include "fatfs.h"
#include "io.h"
#include "timer.h"

static IO_TESTS_ErrorCode randomString(uint16_t length, uint8_t * str);


/**
  * @brief Performs a speed test with IO API to create a contiguous file.
  * @param size[IN] Size of the file to create
  * @param time[OUT] Milliseconds to create the file
  * @retval IO_TESTS_ErrorCode
  */
IO_TESTS_ErrorCode IO_contiguous_speed_Test(FSIZE_t size, uint32_t* time)
{
	uint8_t filename[8];
	FRESULT res;
	IO_FileDescriptor* io_file;

	// Generating a name for the file
	do {
		if (randomString(sizeof(filename), filename) != IO_TESTS_OK)
		{
			return IO_TESTS_SYS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);

		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			return IO_TESTS_ERR;
		}
	} while (res != FR_NO_FILE);

	// TEST: BEGIN
	timer_chronoStart();

	io_file = io_create_contiguous((const TCHAR*)filename, FA_WRITE | FA_READ | FA_CREATE_ALWAYS, size);
	if (io_file == NULL)
	{
		return IO_TESTS_ERR;
	}

    res = io_close(io_file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	*time = timer_getTime();
	// TEST: END

	// Tests finished. Delete TEST file
	do {
		res = f_unlink((const TCHAR*)filename);
		if (res != FR_OK)
		{
			return IO_TESTS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);
	}while (res != FR_NO_FILE);

	return IO_TESTS_OK;
}

/**
  * @brief Performs a speed test with IO API
  * @param bytes[IN] Number of bytes to write at one time (max: 512 bytes)
  * @param total_size[IN] Size of the file to test on (in bytes)
  * @param write_time[OUT] Milliseconds to do the write process
  * @param read_time[OUT] Milliseconds to do the read process
  * @retval IO_TESTS_ErrorCode
  */
IO_TESTS_ErrorCode IO_ioapi_speed_Test(UINT bytes, UINT total_size, uint32_t* write_time, uint32_t* read_time)
{
	uint8_t text[512];
	uint8_t filename[8];
	UINT bytesrw = 0;
	UINT number = 0; // For the loop
	void* rtext;
	FRESULT res;
	IO_FileDescriptor* io_file;

	if (bytes > 512)
	{
		bytes = 512;
	}
	if (bytes == 0)
	{
		return IO_TESTS_SYS_ERR;
	}

	// Generating a name for the file
	do {
		if (randomString(sizeof(filename), filename) != IO_TESTS_OK)
		{
			return IO_TESTS_SYS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);

		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			return IO_TESTS_ERR;
		}
	} while (res != FR_NO_FILE);

	if (randomString(sizeof(text), text) != IO_TESTS_OK) {
		return IO_TESTS_SYS_ERR;
	}

	// WRITE TEST: BEGIN
	timer_chronoStart();

	io_file = io_open((const TCHAR*)filename, FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
	if (io_file == NULL)
	{
		return IO_TESTS_ERR;
	}

	for(number = 0; number < total_size/bytes; number++)
	{
		res = io_write(io_file, text, number*bytes, bytes, (UINT*)&bytesrw);
		if ((bytesrw != bytes) || (res != FR_OK))
		{
			return IO_TESTS_ERR;
		}
	}

    res = io_close(io_file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	*write_time = timer_getTime();
	// WRITE TEST: END

	// READ TEST: BEGIN
	timer_chronoStart();

	io_file = io_open((const TCHAR*)filename, FA_READ | FA_WRITE);
	if (io_file == NULL)
	{
		return IO_TESTS_ERR;
	}

	for(number = 0; number < total_size/bytes; number++)
	{
		rtext = io_read(io_file, number*bytes, bytes, &bytesrw);
		if ((bytesrw == 0) || (rtext == NULL))
		{
			return IO_TESTS_ERR;
		}
	}

	res = io_close(io_file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	*read_time = timer_getTime();
	// READ TEST: END

	// Tests finished. Delete TEST file
	do {
		res = f_unlink((const TCHAR*)filename);
		if (res != FR_OK)
		{
			return IO_TESTS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);
	}while (res != FR_NO_FILE);

	return IO_TESTS_OK;
}


/**
  * @brief Performs a speed test with FatFs
  * @param bytes[IN] Number of bytes to write at one time (max: 512 bytes)
  * @param total_size[IN] Size of the file to test on (in bytes)
  * @param write_time[OUT] Milliseconds to do the write process
  * @param read_time[OUT] Milliseconds to do the read process
  * @retval IO_TESTS_ErrorCode
  */
IO_TESTS_ErrorCode IO_fatfs_speed_Test(UINT bytes, UINT total_size, uint32_t* write_time, uint32_t* read_time)
{
	uint8_t text[512];
	uint8_t filename[8];
	UINT bytesrw = 0;
	UINT number = 0; // For the loop
	FRESULT res;
	FIL file;

	if (bytes > 512)
	{
		bytes = 512;
	}
	if (bytes == 0)
	{
		return IO_TESTS_SYS_ERR;
	}

	// Generating a name for the file
	do {
		if (randomString(sizeof(filename), filename) != IO_TESTS_OK)
		{
			return IO_TESTS_SYS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);

		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			return IO_TESTS_ERR;
		}
	} while (res != FR_NO_FILE);

	if (randomString(sizeof(text), text) != IO_TESTS_OK) {
		return IO_TESTS_SYS_ERR;
	}

	// WRITE TEST: BEGIN
	timer_chronoStart();

	res = f_open(&file, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	for(number = 0; number < total_size/bytes; number++)
	{
		res = f_write(&file, text, bytes, (UINT*)&bytesrw);
		if ((bytesrw != bytes) || (res != FR_OK))
		{
			return IO_TESTS_ERR;
		}
	}

    res = f_close(&file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	*write_time = timer_getTime();
	// WRITE TEST: END

	// READ TEST: BEGIN
	timer_chronoStart();

	res = f_open(&file, (const TCHAR*)filename, FA_READ);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	for(number = 0; number < total_size/bytes; number++)
	{
		res = f_read(&file, text, bytes, &bytesrw);
		if ((bytesrw != bytes) || (res != FR_OK))
		{
			return IO_TESTS_ERR;
		}
	}

	res = f_close(&file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	*read_time = timer_getTime();
	// READ TEST: END

	// Tests finished. Delete TEST file
	do {
		res = f_unlink((const TCHAR*)filename);
		if (res != FR_OK)
		{
			return IO_TESTS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);
	}while (res != FR_NO_FILE);

	return IO_TESTS_OK;
}


/**
  * @brief Test a mounted file system (write and read in a temporary file using IO API)
  * @param None
  * @retval IO_TESTS_ErrorCode
  * @warning This function has a bug that don't know the cause :
  * Sometimes the test file don't want to disappear when deleted
  */
IO_TESTS_ErrorCode IO_ioapi_simple_Test(void)
{
	uint8_t wtext[2048];
	void* rtext;
	uint8_t filename[8];
	uint32_t byteswritten = 0;
	UINT bytesread = 0;
	FRESULT res;
	IO_FileDescriptor* io_file;

	// Generating a name for the file
	do {
		if (randomString(sizeof(filename), filename) != IO_TESTS_OK)
		{
			return IO_TESTS_SYS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);

		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			return IO_TESTS_ERR;
		}
	} while (res != FR_NO_FILE);

	// Generating random data
	if (randomString(sizeof(wtext), wtext) != IO_TESTS_OK)
	{
		return IO_TESTS_SYS_ERR;
	}

	// Let's create a TEST file to write and read in
	io_file = io_open((const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (io_file == NULL)
	{
		return IO_TESTS_ERR;
	}

    res = io_write(io_file, wtext, 0, sizeof(wtext), (UINT*)&byteswritten);
    if ((byteswritten == 0) || (res != FR_OK))
    {
    	return IO_TESTS_ERR;
    }

    res = io_close(io_file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	io_file = io_open((const TCHAR*)filename, FA_READ);
	if (io_file == NULL)
	{
		return IO_TESTS_ERR;
	}

	rtext = io_read(io_file, 0, sizeof(wtext), &bytesread);
    if ((bytesread == 0) || (rtext == NULL))
    {
    	return IO_TESTS_ERR;
    }

    /* Comparison of written string and read string.
     * It's better to use strncmp than strcmp to avoid buffer overflow, as rtext may
     * contain unexpected data
     */
    if (strncmp((const char *)wtext, (const char *)rtext, strlen((const char *)wtext)) != 0)
    {
    	return IO_TESTS_ERR;
    }

	res = io_close(io_file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	// Test finished. Delete TEST file
	do {
		/* I check that the file is deleted because sometimes f_unlink fails but still returns FR_OK
		 * Actually, even with that sometimes the file don't disappear I don't understand why
		 */
		res = f_unlink((const TCHAR*)filename);
		if (res != FR_OK)
		{
			return IO_TESTS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);
	}while (res != FR_NO_FILE);

	return IO_TESTS_OK;
}



/**
  * @brief Test a mounted file system (write and read in a temporary file)
  * @param None
  * @retval IO_TESTS_ErrorCode
  * @warning This function has a bug that don't know the cause :
  * Sometimes the test file don't want to disappear when deleted
  */
IO_TESTS_ErrorCode IO_fatfs_simple_Test(void)
{
	uint8_t wtext[32];
	uint8_t rtext[64];
	uint8_t filename[8];
	uint32_t byteswritten = 0;
	UINT bytesread = 0;
	FRESULT res;
	FIL file;

	// Generating a name for the file
	do {
		if (randomString(sizeof(filename), filename) != IO_TESTS_OK)
		{
			return IO_TESTS_SYS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);

		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			return IO_TESTS_ERR;
		}
	} while (res != FR_NO_FILE);

	if (randomString(sizeof(wtext), wtext) != IO_TESTS_OK) {
		return IO_TESTS_SYS_ERR;
	}

	// Let's create a TEST file to write and read in
	res = f_open(&file, (const TCHAR*)filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

    res = f_write(&file, wtext, sizeof(wtext), (UINT*)&byteswritten);
    if ((byteswritten == 0) || (res != FR_OK))
    {
    	return IO_TESTS_ERR;
    }

    res = f_close(&file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	res = f_open(&file, (const TCHAR*)filename, FA_READ);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	res = f_read(&file, rtext, sizeof(wtext), &bytesread);
    if ((bytesread == 0) || (res != FR_OK))
    {
    	return IO_TESTS_ERR;
    }

    /* Comparison of written string and read string.
     * It's better to use strncmp than strcmp to avoid buffer overflow, as rtext may
     * contain unexpected data
     */
    if (strncmp((const char *)wtext, (const char *)rtext, strlen((const char *)wtext)) != 0)
    {
    	return IO_TESTS_ERR;
    }

	res = f_close(&file);
	if (res != FR_OK)
	{
		return IO_TESTS_ERR;
	}

	// Test finished. Delete TEST file
	do {
		/* I check that the file is deleted because sometimes f_unlink fails but still returns FR_OK
		 * Actually, even with that sometimes the file don't disappear I don't understand why
		 */
		res = f_unlink((const TCHAR*)filename);
		if (res != FR_OK)
		{
			return IO_TESTS_ERR;
		}

		res = f_stat((const TCHAR*)filename, NULL);
	}while (res != FR_NO_FILE);

	return IO_TESTS_OK;
}


/**
  * @brief Generates a random string, useful for IO tests
  * @param length[in] The length of the array
  * @param str[out] a pointer to the array
  * @retval IO_TESTS_ErrorCode
  */
static IO_TESTS_ErrorCode randomString(uint16_t length, uint8_t * str) 
{
	uint8_t symbols[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	uint16_t index = 0;

	if ((length == 0) || (str == NULL))
	{
		return IO_TESTS_ERR;
	}

	for(index = 0; index < length - 1; index++)
	{
		str[index] = symbols[rand() % (int)strlen((const char *)symbols)];
	}

	str[length-1] = '\0';

	return IO_TESTS_OK;
}
