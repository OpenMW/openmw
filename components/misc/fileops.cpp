#include "fileops.hpp"
#include <boost/filesystem.hpp>
#include <string>

bool isFile(const char *name)
{
  boost::filesystem::path cfg_file_path(name);
  return boost::filesystem::exists(cfg_file_path);
}

#ifdef __MACOSX__
#include <CoreFoundation/CoreFoundation.h>

std::string macBundlePath()
{
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);
	
    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);

    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    return std::string(path);
}
#endif
