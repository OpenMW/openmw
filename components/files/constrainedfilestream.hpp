#ifndef OPENMW_CONSTRAINEDFILESTREAM_H
#define OPENMW_CONSTRAINEDFILESTREAM_H

#include <istream>
#include <memory>
#include <limits>
#include <string>

namespace Files
{

/// A file stream constrained to a specific region in the file, specified by the 'start' and 'length' parameters.
class ConstrainedFileStream : public std::istream
{
public:
    ConstrainedFileStream(std::unique_ptr<std::streambuf> buf);
    virtual ~ConstrainedFileStream() {};

private:
    std::unique_ptr<std::streambuf> mBuf;
};

typedef std::shared_ptr<std::istream> IStreamPtr;

IStreamPtr openConstrainedFileStream(const std::string& filename, std::size_t start = 0,
    std::size_t length = std::numeric_limits<std::size_t>::max());

}

#endif
