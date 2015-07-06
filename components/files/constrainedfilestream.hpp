#ifndef OPENMW_CONSTRAINEDFILESTREAM_H
#define OPENMW_CONSTRAINEDFILESTREAM_H

#include <istream>

#include <boost/shared_ptr.hpp>

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

typedef boost::shared_ptr<std::istream> IStreamPtr;

IStreamPtr openConstrainedFileStream(const char *filename, size_t start=0, size_t length=0xFFFFFFFF);

}

#endif
