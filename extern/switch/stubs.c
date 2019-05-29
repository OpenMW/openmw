// various stubs for the missing stdlib stuff

#include <stdlib.h>
#include <errno.h>

#include <dlfcn.h>

void *dlopen(const char *filename, int flag) { return NULL; }
char *dlerror(void) { return "unsupported"; }
void *dlsym(void *handle, const char *symbol) { return NULL; }
int dlclose(void *handle) { return 0; }

#include <pthread.h>

void pthread_yield(void) { }

#include <unistd.h>
#include <limits.h>
#include <string.h>

ssize_t readlink(const char *path, char *buf, size_t bufsiz) { errno = EINVAL; return -1; }
long pathconf(const char *path, int name) { errno = 0; return -1; }
int symlink(const char *target, const char *linkpath) { errno = ENOSYS; return -1; }
char *realpath(const char *path, char *resolved_path)
{
    if (!path) return NULL;

    if (resolved_path)
    {
        strncpy(resolved_path, path, PATH_MAX-1);
        return resolved_path;
    }

    char *rp = malloc(PATH_MAX+1);
    if (!rp) return NULL;

    rp[0] = 0;
    strncpy(rp, path, PATH_MAX-1);
    return rp;
}

#include <fcntl.h>
#include <sys/stat.h>

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) { return 0; }
