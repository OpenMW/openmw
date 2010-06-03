#include "fileops.hpp"

// Windows-specific implementation (NOT TESTED)
#ifdef _WIN32

#include <windows.h>

bool isFile(const char *name)
{
  unsigned int stat = GetFileAttributes(name);
  return (stat != 0xFFFFFFFF &&
          (stat & FILE_ATTRIBUTE_DIRECTORY) == 0);
}
#endif // _WIN32

// Linux implementations
#ifdef __linux__

#include <sys/stat.h>
#include <unistd.h>

bool isFile(const char *name)
{
  // Does the file exist?
  if(access(name,0) != 0)
    return false;

  struct stat st;
  if(stat(name, &st)) return false;
  return S_ISREG(st.st_mode);
}
#endif // __linux__
