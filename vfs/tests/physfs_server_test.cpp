#include "../servers/physfs_vfs.h"

#include "server_common.cpp"

#include <physfs.h>

int main()
{
  // Set up the library and paths
  PHYSFS_init("blah");
  PHYSFS_addToSearchPath("test.zip", 1);
  PHYSFS_addToSearchPath("./", 1);

  // Create our interface
  PhysVFS vfs;

  // Run the test
  testAll(vfs);

  return 0;
}
