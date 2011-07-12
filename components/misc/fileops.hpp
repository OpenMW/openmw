#ifndef MISC_FILEOPS_H
#define MISC_FILEOPS_H

#include <string>

namespace Misc
{

/// Check if a given path is an existing file (not a directory)
bool isFile(const char *name);

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
std::string macBundlePath();
#endif

}

#endif
