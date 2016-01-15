#include <linux/string.h>
#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <asm/segment.h>

#include "lkm-hook.h"
/*************************file operation**********************************/
/**
 * get the filename from the path
 **/
char* get_filename_from_path(const char* path)
{
	return (char*)(strrchr(path, '/') ? strrchr(path, '/') + 1 : path); 
}

int open_shm_file(const char *shm_path) 
{
	int shm_fd = 0;
	mm_segment_t oldfs;
	//open file
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	shm_fd = orig_open(shm_path, O_RDWR|O_CREAT, 0644);
	set_fs(oldfs);
	//printk(KERN_EMERG "[LKM]open_shm:fd=%d %s\n", shm_fd, shm_path);
	return shm_fd;
}

void close_shm_file(int shm_fd)
{
	orig_close(shm_fd);
}

int open_elf(const char *file_path)
{
	int fd = 0;
	mm_segment_t oldfs;
	//open file
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	fd = orig_open(file_path, O_RDONLY|O_CLOEXEC, 0644);
	set_fs(oldfs);
	//printk(KERN_EMERG "[LKM]open_shm:fd=%d %s\n", shm_fd, shm_path);
	return fd;
}

void close_elf(int fd)
{
	orig_close(fd);
}
