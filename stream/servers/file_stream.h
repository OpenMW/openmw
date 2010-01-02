#ifndef MANGLE_STREAM_FILESERVER_H
#define MANGLE_STREAM_FILESERVER_H

#include "std_stream.h"
#include <fstream>

namespace Mangle {
namespace Stream {

/** Very simple file input stream, based on std::ifstream
 */
class FileStream : public StdStream
{
  std::ifstream file;

 public:
  FileStream(const std::string &name)
    : StdStream(&file)
    {
      file.open(name.c_str(), std::ios::binary);

      if(file.fail())
        throw str_exception("FileStream: failed to open file " + name);
    }
  ~FileStream() { file.close(); }
};

typedef boost::shared_ptr<FileStream> FileStreamPtr;

}} // namespaces
#endif
