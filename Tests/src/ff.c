#include "ff.h"	
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include "fatfs.h"
#include <sys/vfs.h>

FIL* file;
int fileDescriptor;
int foo = 0;
FATFS fsvar;
FATFS *fs = &fsvar;
_FDID obj;
const TCHAR* pathvar;

FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode)
{
	int flags;
	
	if ((mode & FA_WRITE) && !(mode & FA_READ))
	{
		flags = O_WRONLY;
	}	
	if (!(mode & FA_WRITE) && (mode & FA_READ))
	{
		flags = O_RDONLY;
	}
	if ((mode & FA_WRITE) && (mode & FA_READ))
	{
		flags = O_RDWR;
	}
	flags |= O_CREAT;
	
	file = fp;
	fileDescriptor = open((const char *)path, flags, S_IRUSR | S_IWUSR | S_IXUSR);
	
	// Initializing sector size :
	#if (_MAX_SS != _MIN_SS)
		fp->obj.fs->ssize = FAKE_SSIZE;
	#endif
	
	if (fileDescriptor != -1)
	{
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_close (FIL* fp)
{
	file = fp;
	fileDescriptor = close(fileDescriptor);
	
	if (fileDescriptor != -1)
	{
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br)
{
	long int res;
	file = fp;
	res = read(fileDescriptor, buff, btr);
		
	if (res != -1)
	{
		*br = (UINT)res;
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw)
{
	ssize_t res;
	file = fp;
	res = write(fileDescriptor, buff, btw);
	
	if (res != -1)
	{
		*bw = (UINT)res;
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_lseek (FIL* fp, FSIZE_t ofs)
{
	off_t res;
	file = fp;
	
	res = lseek(fileDescriptor, (off_t)ofs, SEEK_SET);
	
	if (ofs > f_size(file))
	{
		f_truncate(file);
	}
	
	if (res != -1)
	{
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_sync (FIL* fp)
{
	file = fp;
	sync();
	return FR_OK;
}

FRESULT f_unlink (const TCHAR* path)
{
	int res;
	res = unlink(path);
	
	if (res != -1)
	{
		return FR_OK;
	}
	return FR_INT_ERR;
}

FRESULT f_stat (const TCHAR* path, FILINFO* fno)
{
	struct stat statbuf;
	int res;
	(void)(fno);
	res = stat(path, &statbuf);
	if (res == 0)
	{
		return FR_OK;
	}
	return FR_NO_FILE;
}

UINT f_size(FIL* fp)
{
	off_t current;
	off_t new;
	off_t size;
	file = fp;
	current = lseek(fileDescriptor, 0, SEEK_CUR);
	if (current == -1)
	{
		return 0;
	}
	
	size = lseek(fileDescriptor, 0, SEEK_END);
	if (size == -1)
	{
		return 0;
	}
	
	new = lseek(fileDescriptor, current, SEEK_SET);
	if (new != current)
	{
		return 0;
	}
	
	return (UINT)size;
}

FRESULT f_expand (FIL* fp, FSIZE_t fsz, BYTE opt)
{
	file = fp;
	foo = opt;
	f_lseek(file, fsz);
	return f_truncate(file);
}

FRESULT f_truncate (FIL* fp)
{
	file = fp;
	off_t size = (off_t)(f_tell(file));
	
	if (ftruncate(fileDescriptor, size) == 0)
	{
		return FR_OK;
	}
	return FR_INT_ERR;
}

int f_error(FIL* fp)
{
	file = fp;
	return 0;
} 


FSIZE_t f_tell(FIL* fp)
{
	off_t res;
	file = fp;
	res = lseek(fileDescriptor, 0, SEEK_CUR);
	return (FSIZE_t)res;
}

FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs)
{
	pathvar = path;
	*fatfs = fs;
	obj.fs = fs;

	*nclst = 10;
	fs->csize = 30000;
	
#if (_MAX_SS != _MIN_SS)
	fs->ssize = 4000;
#endif
	fs->n_fatent = 100;
	
	return FR_OK;
	
}
