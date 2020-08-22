/** @file IO_tests.h
 *
 * @brief This file contains a set of functions to test file system on input and output
 */

#ifndef INC_IO_TESTS_H_
#define INC_IO_TESTS_H_

#include <stdint.h>
#include "fatfs.h"

typedef enum {
	IO_TESTS_OK = 0,		/* (0) Succeeded */
	IO_TESTS_ERR,			/* (1) An error occurred  */
	IO_TESTS_SYS_ERR		/* (2) An error occurred that don't implies the filesystem */
} IO_TESTS_ErrorCode;

IO_TESTS_ErrorCode IO_fatfs_simple_Test(void);
IO_TESTS_ErrorCode IO_ioapi_simple_Test(void);
IO_TESTS_ErrorCode IO_ioapi_speed_Test(UINT bytes, UINT total_size, uint32_t* write_time, uint32_t* read_time);
IO_TESTS_ErrorCode IO_fatfs_speed_Test(UINT bytes, UINT total_size, uint32_t* write_time, uint32_t* read_time);
IO_TESTS_ErrorCode IO_contiguous_speed_Test(FSIZE_t size, uint32_t* time);

#endif /* INC_IO_TESTS_H_ */
