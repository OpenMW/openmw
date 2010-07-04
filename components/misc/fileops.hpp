#ifndef __FILEOPS_H_
#define __FILEOPS_H_

#include <string>

/// Check if a given path is an existing file (not a directory)
bool isFile(const char *name);

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
std::string macBundlePath();
#endif

#endif
