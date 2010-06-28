#ifndef __FILEOPS_H_
#define __FILEOPS_H_

namespace OMW { namespace Platform {

    /// Check if a given path is an existing file (not a directory)
    bool isFile(const char *name);
}}

#endif
