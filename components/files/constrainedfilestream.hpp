#ifndef OPENMW_CONSTRAINEDFILESTREAM_H
#define OPENMW_CONSTRAINEDFILESTREAM_H

#include <istream>

namespace Files
{

/// A file stream constrained to a specific region in the file, specified by the 'start' and 'length' parameters.
class ConstrainedFileStream : public std::istream
{
public:
    ConstrainedFileStream(const char *filename,
                          size_t start=0, size_t length=0xFFFFFFFF);
    virtual ~ConstrainedFileStream();
};

}

#endif
