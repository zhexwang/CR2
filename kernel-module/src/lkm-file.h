#ifndef _LKM_FILE_H_
#define _LKM_FILE_H_

extern char* get_filename_from_path(const char* path);
extern int open_shm_file(const char *shm_path);
extern int open_elf(const char *file_path);
extern void close_shm_file(int shm_fd);
extern void close_elf(int fd);


#endif
