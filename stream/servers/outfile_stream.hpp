#ifndef MANGLE_OSTREAM_FILESERVER_H
#define MANGLE_OSTREAM_FILESERVER_H

#include "std_ostream.hpp"
#include <fstream>

namespace Mangle {
namespace Stream {

/** File stream based on std::ofstream, only supports writing.
 */
class OutFileStream : public StdOStream
{
  std::ofstream file;

 public:
  /**
     By default we overwrite the file. If append=true, then we will
     open an existing file and seek to the end instead.
   */
  OutFileStream(const std::string &name, bool append=false)
    : StdOStream(&file)
    {
      std::ios::openmode mode = std::ios::binary;
      if(append)
        mode |= std::ios::app;
      else
        mode |= std::ios::trunc;

      file.open(name.c_str(), mode);

      if(file.fail())
        throw str_exception("OutFileStream: failed to open file " + name);
    }
  ~OutFileStream() { file.close(); }
};

typedef boost::shared_ptr<OutFileStream> OutFileStreamPtr;

}} // namespaces
#endif
