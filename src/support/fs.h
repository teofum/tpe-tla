#ifndef FS_HEADER
#define FS_HEADER

#include <sys/stat.h>

int fs_mkpath(char* file_path, mode_t mode);

char *fs_stem(const char *path);

#endif
