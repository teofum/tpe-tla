#include <assert.h>
#include <libgen.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "fs.h"

// Source - https://stackoverflow.com/a/9210960
// Posted by Yaroslav Stavnichiy, modified by community. See post 'Timeline' for change history
// Retrieved 2026-10-01, License - CC BY-SA 4.0

int fs_mkpath(char* file_path, mode_t mode) {
  assert(file_path && *file_path);
  for (char* p = strchr(file_path, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mkdir(file_path, mode) == -1) {
      if (errno != EEXIST) {
        *p = '/';
        return -1;
      }
    }
    *p = '/';
  }
  return 0;
}
