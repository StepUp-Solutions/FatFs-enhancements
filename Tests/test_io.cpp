#include <CppUTest/TestHarness.h>

#define FILE_SIZE 64
#define FAKE_SSIZE 16

extern "C"
{
    #include "io.h"
    #include "fatfs.h"
    #include "ffconf.h"
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <string.h>
	#include <fcntl.h>
	#include <stdint.h>
}

static void randomString(size_t length, char * str);
static void deleteTempFile(uint8_t * filename, uint8_t size);
static off_t getFileSize(uint8_t * filename);

TEST_GROUP(TestOpen)
{
    void setup()
    {
		// Be sure that no file named "testTmpFile" exists
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }

    void teardown()
    {
		// Delete "testTmpFile"
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }
};

TEST_GROUP(TestWrite)
{
    void setup()
    {
		// Be sure that no file named "testTmpFile" exists
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }

    void teardown()
    {
		// Delete "testTmpFile"
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }
};

TEST_GROUP(TestRead)
{
    void setup()
    {
		// Be sure that no file named "testTmpFile" exists
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }

    void teardown()
    {
		// Delete "testTmpFile"
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }
};

TEST_GROUP(ChangeFileSize)
{
    void setup()
    {
		// Be sure that no file named "testTmpFile" exists
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }

    void teardown()
    {
		// Delete "testTmpFile"
		uint8_t filename[] = "testTmpFile";
		deleteTempFile(filename, (uint8_t)strlen((const char*)filename));
    }
};

/**
 * Test: TestOpen CreateFile
 * Test case: io_open successfully creates a file
 * Preconditions: no file named "testTmpFile" exists
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to create a file
 *  - Call io_close to close the file
 * Expected result:
 *  - io_open must not return NULL
 *  - the file must exist after io_open call
 */
TEST(TestOpen, CreateFile)
{
	IO_FileDescriptor* io_file;
	uint8_t filename[] = "testTmpFile";
	struct stat statbuf;
	
	// Create a file
	io_file = io_open((const TCHAR*)filename, FA_WRITE);
	
    CHECK(io_file != NULL);
    
    // Check that the file exists
    CHECK(stat((const TCHAR*)filename, &statbuf) == 0);
    
    // Close the file
    io_close(io_file);
}

/**
 * Test: TestOpen OpenExistingFile
 * Test case: io_open successfully opens an existing file
 * Preconditions: a file named "testTmpFile" exists (test CreateFile)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open the file
 *  - Call io_close to close the file
 * Expected result:
 *  - io_open must not return NULL
 *  - the file must exist after io_open call
 */
TEST(TestOpen, OpenExistingFile)
{
	IO_FileDescriptor* io_file;
	uint8_t filename[] = "testTmpFile";
	struct stat statbuf;
	
	// Open the file
	io_file = io_open((const TCHAR*)filename, FA_WRITE);
	
    CHECK(io_file != NULL);
    
    // Check that the file still exists
    CHECK(stat((const TCHAR*)filename, &statbuf) == 0);
    
    // Close the file
    io_close(io_file);
}

/**
 * Test: TestOpen CheckSectorSize
 * Test case: io_open successfully gets the sector size (only relevant if _MAX_SS != _MIN_SS
 * Preconditions: a file named "testTmpFile" exists (test CreateFile)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open the file
 *  - Call io_close to close the file
 * Expected result:
 *  - io_open must not return NULL
 *  - fs.ssize must have the correct value (FAKE_SSIZE)
 */
TEST(TestOpen, CheckSectorSize)
{
	IO_FileDescriptor* io_file;
	uint8_t filename[] = "testTmpFile";
	
	// Open the file
	io_file = io_open((const TCHAR*)filename, FA_WRITE);
	
    CHECK(io_file != NULL);
    
    // Sector size
	#if (_MAX_SS != _MIN_SS)
		CHECK(io_file->ssize == io_file->file->obj.fs->ssize);
	#else
		CHECK(io_file->ssize == _MAX_SS);
	#endif
	
    // Close the file
    io_close(io_file);
}

/**
 * Test: TestOpen OpenInvalidFile
 * Test case: io_open fails opening an invalid file
 * Preconditions:
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open the file named ""
 * Expected result:
 *  - io_open must return NULL
 */
TEST(TestOpen, OpenInvalidFile)
{
	IO_FileDescriptor* io_file;
	uint8_t filename[] = "";
	
	// Open the file
	io_file = io_open((const TCHAR*)filename, FA_WRITE);
	
    CHECK(io_file == NULL);
}


/**
 * Test: TestWrite WriteBeyondMaxFileSize1
 * Test case: io_write doesn't write when starting after the 4GB limit of FAT filesystem
 * Preconditions: io_open and io_close must work (TestOpen)
 * Expected result:
 *  - io_write must return FR_INVALID_PARAMETER and write 0 bytes
 */
 /*
Test commented because this necessitate a 4GB buffer
TEST(TestWrite, WriteBeyondMaxFileSize1)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	
	UINT byteswritten;
	
	// Open/Create the file
	io_file = io_open(filename, FA_WRITE);
	CHECK(io_file != NULL);
	
	// Write on the file
	res = io_write(io_file, "test", MAX_FILE_SIZE, 4, &byteswritten);
	CHECK_EQUAL(FR_INVALID_PARAMETER, res);
	CHECK_EQUAL(0, byteswritten);
	
	// Close the file
	io_close(io_file);

	CHECK(remove(filename) == 0);
}
*/

/**
 * Test: TestWrite WriteBeyondMaxFileSize2
 * Test case: io_write doesn't write after the 4GB limit of FAT filesystem, but sucessfully write before
 * Preconditions: io_open and io_close must work (TestOpen)
 * Expected result:
 *  - io_write must return FR_OK and stop at the 4GB limit
 */
 /*
Test commented because this necessitate a 4GB buffer
TEST(TestWrite, WriteBeyondMaxFileSize2)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	
	UINT byteswritten;
	
	// Open/Create the file
	io_file = io_open(filename, FA_WRITE);
	CHECK(io_file != NULL);
	
	// Write on the file
	res = io_write(io_file, "test", MAX_FILE_SIZE-2, 4, &byteswritten);
	CHECK_EQUAL(FR_OK, res);
	CHECK_EQUAL(2, byteswritten);
	
	// Close the file
	io_close(io_file);

	CHECK(remove(filename) == 0);
}
*/

/**
 * Test: TestWrite WriteBeyondMaxFileSize3
 * Test case: io_write write until the 4GB limit of FAT filesystem
 * Preconditions: io_open and io_close must work (TestOpen)
 * Expected result:
 *  - io_write must return FR_OK and write 1 bytes
 */
 /*
Test commented because this necessitate a 4GB buffer
TEST(TestWrite, WriteBeyondMaxFileSize3)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	
	UINT byteswritten;
	
	// Open/Create the file
	io_file = io_open(filename, FA_WRITE);
	CHECK(io_file != NULL);
	
	// Write on the file
	res = io_write(io_file, "test", MAX_FILE_SIZE-1, 1, &byteswritten);
	CHECK_EQUAL(FR_OK, res);
	CHECK_EQUAL(1, byteswritten);
	
	// Close the file
	io_close(io_file);

	CHECK(remove(filename) == 0);
}
*/

/**
 * Test: TestWrite WriteRandomEmptyFile1
 * Test case: io_write successfully writes random data in an empty file, starting at the beginning of the file
 * Preconditions: io_open and io_close must work (TestOpen)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open/create a file
 *  - Generate random data
 *  - Call io_write to write data in the file, starting at the first byte of the file
 *  - Close the file with io_close
 *  - Check that the file contains the data
 *  - Delete the file
 *  - Loop
 * Expected result:
 *  - io_write must return FR_OK
 *  - io_write must write the expected number of bytes
 *  - The file must contain correct data
 */
TEST(TestWrite, WriteRandomEmptyFile1)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	int fd; // File descriptor
	
	ssize_t bytesread;
	UINT byteswritten;
	char data_w[FILE_SIZE];
	char data_r[FILE_SIZE];
	UINT data_length;
	
    for (data_length = 0; data_length <= FILE_SIZE; data_length++)
	{
		// Open/Create the file
		io_file = io_open(filename, FA_WRITE);
		CHECK(io_file != NULL);
		
		// Generate the data
		randomString(data_length, data_w);
		
		// Write on the file
		res = io_write(io_file, data_w, 0, data_length, &byteswritten);
		CHECK(res == FR_OK);
		CHECK(byteswritten == data_length);
		
		// Close the file
		io_close(io_file);
		
		// Check the contents of the file
		fd = open(filename, O_RDONLY);
		CHECK(fd != -1);
		
		bytesread = read(fd, data_r, data_length);
		CHECK(bytesread != -1);
		CHECK((size_t)bytesread == data_length);
		MEMCMP_EQUAL(data_w, data_r, data_length);
		
		// Close and delete the file
		close(fd);
		CHECK(remove(filename) == 0);
	}
}

/**
 * Test: TestWrite WriteRandomEmptyFile2
 * Test case: io_write successfully writes random data in an empty file, starting at a ramdom point of the file
 * Preconditions: io_open and io_close must work (TestOpen)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open/create a file
 *  - Generate random data
 *  - Call io_write to write data in the file, starting at a ramdom position in the file
 *  - Close the file with io_close
 *  - Check that the file contains the data (0000 0000 0000 data data data...)
 *  - Delete the file
 *  - Loop
 * Expected result:
 *  - io_write must return FR_OK
 *  - io_write must write the expected number of bytes
 *  - The file must contain correct data (zeroes until the position where write begins)
 */
TEST(TestWrite, WriteRandomEmptyFile2)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	int fd; // File descriptor
	
	ssize_t bytesread;
	UINT byteswritten;
	char data_w[FILE_SIZE];
	char data_r[FILE_SIZE];
	UINT data_length;
	UINT file_size;
	UINT position;
	UINT i; // For a for loop
	
	for (file_size = 0; file_size <= FILE_SIZE; file_size++)
	{
		for (position = 0; position < file_size; position++)
		{
			data_length = file_size - position;
			
			// Open/Create the file
			io_file = io_open(filename, FA_WRITE);
			CHECK(io_file != NULL);
			
			// Generate the data
			randomString(data_length, data_w);
			
			// Write on the file
			res = io_write(io_file, data_w, position, data_length, &byteswritten);
			CHECK(res == FR_OK);
			CHECK(byteswritten == data_length);
			
			// Close the file
			io_close(io_file);
			
			// Check the contents of the file
			fd = open(filename, O_RDONLY);
			CHECK(fd != -1);
			
			bytesread = read(fd, data_r, file_size);
			
			CHECK(bytesread != -1);
			CHECK((size_t)bytesread == file_size);
			MEMCMP_EQUAL(data_w, data_r + position, data_length);
			
			for (i = 0; i < position; i++)
			{
				CHECK(data_r[i] == '\0');
			}
			
			// Close and delete the file
			close(fd);
			CHECK(remove(filename) == 0);
		}
	}
}

/**
 * Test: TestWrite WriteRandom
 * Test case: io_write successfully writes random data in an existing file, starting at a ramdom point of the file
 * Preconditions: io_open and io_close must work (TestOpen)
 * Test steps: 
 *  - Initialize data structures
 *  - Generate random data to create a file, and create the file
 *  - Call io_open to open the file
 *  - Generate random data
 *  - Call io_write to write data in the file, starting at a ramdom position in the file
 *  - Close the file with io_close
 *  - Check that the file contains the data (oldData oldData newData oldData)
 *  - Delete the file
 *  - Loop
 * Expected result:
 *  - io_write must return FR_OK
 *  - io_write must write the expected number of bytes
 *  - The file must contain correct data (zeroes until the position where write begins)
 */
TEST(TestWrite, WriteRandom)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	FRESULT res;
	int fd; // File descriptor
	
	ssize_t bytes;
	UINT bytesrw;
	char data_file[FILE_SIZE];
	char data_w[FILE_SIZE];
	char data_r[FILE_SIZE];
	UINT data_length;
	UINT position;
	
	for (position = 0; position < FILE_SIZE; position++)
	{
		for (data_length = 0; data_length <= FILE_SIZE - position; data_length++)
		{
			// Create and fill the file
			fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
			CHECK(fd != -1);
			randomString(FILE_SIZE, data_file);
			bytes = write(fd, data_file, FILE_SIZE);
			CHECK(bytes == FILE_SIZE);
			close(fd);
			
			// Generate random data and use IO API to write it on the file
			randomString(data_length, data_w);
			io_file = io_open(filename, FA_WRITE | FA_READ);
			CHECK(io_file != NULL);
			
			// Write on the file
			res = io_write(io_file, data_w, position, data_length, &bytesrw);
			CHECK(res == FR_OK);
			CHECK(bytesrw == data_length);
			
			// Close the file
			io_close(io_file);
			
			// Check the contents of the file
			fd = open(filename, O_RDONLY);
			CHECK(fd != -1);
			bytes = read(fd, data_r, FILE_SIZE);
			CHECK(bytes != -1);
			CHECK((size_t)bytes == FILE_SIZE);
			
			// Check beginning of the file:
			MEMCMP_EQUAL(data_file, data_r, position);

			// Check modified portion of the file:
			MEMCMP_EQUAL(data_w, data_r + position, data_length);
			
			// Check end of the file:
			MEMCMP_EQUAL(data_file + position + data_length, data_r + position + data_length, FILE_SIZE - data_length - position);
			
			// Close and delete the file
			close(fd);
			CHECK(remove(filename) == 0);
		}
	}
}

/**
 * Test: TestRead ReadTooSmallFile1
 * Test case: io_read tries to read a file, starting after eof
 * Preconditions: io_open and io_close must work (TestOpen)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open/create a file
 *  - Call io_read with position <= size
 *  - Close the file with io_close
 *  - Delete the file
 * Expected result:
 *  - io_read must return NULL
 *  - io_read must read 0 bytes
 */
TEST(TestRead, ReadTooSmallFile1)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	int fd; // File descriptor
	
	char data_file[FILE_SIZE];
	ssize_t bytes;
	UINT bytesrw;
	void * buffer;
	UINT file_size;
	UINT position;

	for (file_size = 0; file_size <= FILE_SIZE; file_size++)
	{
		for (position = file_size; position <= FILE_SIZE+1; position++)
		{
			// Create and fill the file
			fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
			CHECK(fd != -1);
			randomString(file_size, data_file);
			bytes = write(fd, data_file, file_size);
			CHECK(bytes == file_size);
			close(fd);
			
			// Open the file
			io_file = io_open(filename, FA_WRITE | FA_READ);
			CHECK(io_file != NULL);
			
			// Read
			buffer = io_read(io_file, position, 1, &bytesrw);
			CHECK(buffer == NULL);
			CHECK(bytesrw == 0);
			
			// Close the file
			io_close(io_file);
			CHECK(remove(filename) == 0);
		}
	}
}

/**
 * Test: TestRead ReadTooSmallFile2
 * Test case: io_read tries to read a file, starting before eof and ending before or after eof
 * Preconditions: io_open and io_close must work (TestOpen)
 * Test steps: 
 *  - Initialize data structures
 *  - Call io_open to open/create a file
 *  - Call io_read with position + bytesToRead >= size
 *  - Close the file with io_close
 *  - Delete the file
 *  - loop
 * Expected result:
 *  - io_read must not return NULL
 *  - io_read must read the expected number of bytes
 *  - io_read must correctly read the end of the file
 */
TEST(TestRead, ReadTooSmallFile2)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	int fd; // File descriptor
	
	char data_file[FILE_SIZE];
	ssize_t bytes;
	UINT bytesrw;
	void * buffer;
	UINT file_size;
	UINT positionStart;
	UINT btr; // Bytes to read

	for (file_size = 1; file_size <= FILE_SIZE; file_size++)
	{
		for (positionStart = 0; positionStart < file_size; positionStart++)
		{
			for (btr = 1; btr <= file_size + 2; btr++)
			{
				// Create and fill the file
				fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
				CHECK(fd != -1);
				randomString(file_size, data_file);
				bytes = write(fd, data_file, file_size);
				CHECK(bytes == file_size);
				close(fd);
						
				// Open the file
				io_file = io_open(filename, FA_WRITE | FA_READ);
				CHECK(io_file != NULL);
				CHECK(f_size(io_file->file) == file_size);
				
				// Read
				buffer = io_read(io_file, positionStart, btr, &bytesrw);
				CHECK(buffer != NULL);
				if(btr + positionStart >= file_size)
				{
					CHECK(bytesrw == file_size - positionStart);
				}
				else
				{
					CHECK(bytesrw == btr);
				}
				
				// Check buffer contents
				MEMCMP_EQUAL(data_file + positionStart, (const char *)buffer, bytesrw);
				
				// Close the file
				io_close(io_file);
				CHECK(remove(filename) == 0);
			}
		}
	}
}

/**
 * Test: ChangeFileSize TruncateExpandFile
 * Test case: io_truncate expands a file's size
 * Preconditions:
 * Test steps: 
 *  - Initialize data structures
 *  - Create a random file with random data in it
 *  - Call io_truncate to expand file size
 *  - Close the file with io_close
 *  - Delete the file
 *  - loop
 * Expected result:
 *  - the file must have the correct size (after closing it)
 *  - io_truncate must return FR_OK
 */
TEST(ChangeFileSize, TruncateExpandFile)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	int fd; // File descriptor
	FRESULT res;
	
	ssize_t bytes;
	char data_file[FILE_SIZE];
	UINT data_length;
	UINT expansion;
	
	for (data_length = 0; data_length <= FILE_SIZE; data_length++)
	{
		for (expansion = 0; expansion < FILE_SIZE - data_length; expansion++)
		{
			// Create and fill the file
			fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
			CHECK(fd != -1);
			randomString(data_length, data_file);
			bytes = write(fd, data_file, data_length);
			CHECK(bytes == data_length);
			close(fd);
			
			// Expand the file
			io_file = io_open(filename, FA_WRITE | FA_READ);
			res = io_truncate(io_file, data_length + expansion);
			CHECK(res == FR_OK);
			io_close(io_file);
			
			// Check file size
			CHECK_EQUAL(getFileSize((uint8_t*)filename), data_length + expansion);
			
			// Close and delete the file
			close(fd);
			CHECK(remove(filename) == 0);
		}
	}
}

/**
 * Test: ChangeFileSize TruncateReduceFile
 * Test case: io_truncate reduces a file's size
 * Preconditions:
 * Test steps: 
 *  - Initialize data structures
 *  - Create a random file with random data in it
 *  - Call io_truncate to reduce file size
 *  - Close the file with io_close
 *  - Delete the file
 *  - loop
 * Expected result:
 *  - the file must have the correct size (after closing it)
 *  - io_truncate must return FR_OK
 */
TEST(ChangeFileSize, TruncateReduceFile)
{
	IO_FileDescriptor* io_file;
	const char filename[] = "testTmpFile";
	int fd; // File descriptor
	FRESULT res;
	
	ssize_t bytes;
	char data_file[FILE_SIZE];
	UINT data_length;
	UINT reduce;
	
	for (data_length = 0; data_length <= FILE_SIZE; data_length++)
	{
		for (reduce = 0; reduce <= data_length; reduce++)
		{
			// Create and fill the file
			fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
			CHECK(fd != -1);
			randomString(data_length, data_file);
			bytes = write(fd, data_file, data_length);
			CHECK(bytes == data_length);
			close(fd);
			
			// Reduce the file size
			io_file = io_open(filename, FA_WRITE | FA_READ);
			res = io_truncate(io_file, data_length - reduce);
			CHECK(res == FR_OK);
			io_close(io_file);
			
			// Check file size
			CHECK_EQUAL(getFileSize((uint8_t*)filename), data_length - reduce);
			
			// Close and delete the file
			close(fd);
			CHECK(remove(filename) == 0);
		}
	}
}

/**
  * @brief Generates a random string
  * @param length[in] The length of the array
  * @param str[out] a pointer to the array
  * @retval None
  */
static void randomString(size_t length, char * str)
{
	uint8_t symbols[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	size_t index = 0;

	if ((length != 0) && (str != NULL))
	{
		for(index = 0; index < length - 1; index++)
		{
			str[index] = symbols[rand() % (int)strlen((const char *)symbols)];
		}

		str[length-1] = '\0';
	}
}

/**
  * @brief Deletes a file
  * @param filename[in] The file to delete
  * @param size[in] Size of the string filename
  * @retval None
  */
static void deleteTempFile(uint8_t * filename, uint8_t size)
{
	struct stat statbuf;
	// Check the length of the string to prevent buffer overflow
	if (strnlen((const char *)filename, size) == size)
	{
		filename[size-1] = '\0';
	}
	
	// Delete the file
	if (stat((const TCHAR*)filename, &statbuf) == 0)
	{
		remove((const TCHAR*)filename);
	}
}

/**
  * @brief Get the size of a non opened file
  * @param filename[in] The file
  * @retval The size of the file
  */
static off_t getFileSize(uint8_t * filename)
{
	int fd;
	off_t size;
	fd = open((const char*)filename, O_RDONLY);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	return size;
}
