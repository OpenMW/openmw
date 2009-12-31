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
      file.open(name, ios::binary);
    }
  ~FileStream() { file.close(); }
};

typedef boost::shared_ptr<FileStream> FileStreamPtr;

}} // namespaces
#endif
