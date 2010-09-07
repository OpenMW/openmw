#ifndef MANGLE_STREAM_IOSTREAM_H
#define MANGLE_STREAM_IOSTREAM_H

#include <assert.h>
#include "../stream.hpp"
#include <iostream>
#include <vector>

// This seems to work (TODO: test)
#ifndef EOF
#define EOF -1
#endif

namespace Mangle {
namespace Stream {

  /** This file contains classes for wrapping an std::istream or
      std::ostream around a Mangle::Stream. Not to be confused with
      servers/std_(o)stream.hpp, which do the opposite (wrap a
      Mangle::Stream around an std::istream/ostream.)

      This allows you to use Mangle streams in places that require std
      streams. The std::iostream interface is horrible and NOT
      designed for easy subclassing. Defining your custom streams as
      Mangle streams and then wrapping them here will usually be much
      easier.
  */
  class IOStreamBuffer : public std::streambuf
  {
    StreamPtr client;
    std::vector<char> ibuf, obuf;

  public:
    IOStreamBuffer(StreamPtr strm) : client(strm)
    {
      // Set up input buffer
      setg(NULL,NULL,NULL);
      if(client->isReadable)
        {
          if(client->hasPtr)
            {
              assert(client->hasSize);

              // If the client supports direct pointer reading, then
              // this is really easy. No internal buffer is needed.
              char *ptr = (char*) client->getPtr();

              // Set up the entire file as the input buffer
              setg(ptr,ptr,ptr+client->size());
            }
          else
            {
              // We need to do some manual slave labor. Set up an
              // empty input buffer and let underflow() handle it.
              ibuf.resize(1024);
            }
        }

      // Only create output buffer if the stream is writable
      if(client->isWritable)
        {
          obuf.resize(1024);
          /* Set beginning and end pointers, tells streambuf to write
             to this area and call overflow() when it's full.

             Several examples use size-1, but the documentation for
             streambuf clearly states that the end pointers is just
             _past_ the last accessible position.
          */
          char *beg = &obuf[0];
          setp(beg, beg+obuf.size());
        }
      else
        // Writing not permitted
        setp(NULL, NULL);
    }

    /* Underflow is called when there is no more info to read in the
       input buffer. We need to refill ibuf with data (if any), and
       set up the internal pointers with setg() to reflect the new
       state.
    */
    int underflow()
    {
      assert(client->isReadable);

      // If we've exhausted a pointer stream, then there's no more to
      // be had.
      if(client->hasPtr)
        return EOF;

      // Read some more data
      assert(ibuf.size());
      char *iptr = &ibuf[0];
      size_t read = client->read(iptr, ibuf.size());

      // If we're out of data, then EOF
      if(read == 0)
        return EOF;

      // Otherwise, set up input buffer
      setg(iptr, iptr, iptr+read);

      // Return the first char
      return *((unsigned char*)iptr);
    }

    /* Sync means to flush (write) all current data to the output
       stream. It will also set up the entire output buffer to be
       usable again.
     */
    int sync()
    {
      assert(client->isWritable);
      assert(obuf.size() > 0);

      // Get the number of bytes that streambuf wants us to write
      int num = pptr() - pbase();
      assert(num >= 0);

      // Nothing to do
      if(num == 0) return 0;

      if((int)client->write(pbase(), num) != num)
        return -1;

      // Reset output buffer pointers
      char *beg = &obuf[0];
      setp(beg, beg+obuf.size());

      return 0;
    }

    int overflow(int c=EOF)
    {
      // First, write all existing data
      if(sync()) return EOF;

      // Put the requested character in the output
      if(c != EOF)
        {
          *pptr() = c;
          pbump(1);
        }

      return 0;
    }
  };

  class MangleIStream : public std::istream
  {
    IOStreamBuffer buf;
  public:
    MangleIStream(StreamPtr inp)
      : std::istream(&buf)
      , buf(inp)
    {
      assert(inp->isReadable);
    }
  };

  class MangleOStream : public std::ostream
  {
    IOStreamBuffer buf;
  public:
    MangleOStream(StreamPtr inp)
      : std::ostream(&buf)
      , buf(inp)
    {
      assert(inp->isWritable);
    }

    ~MangleOStream() { flush(); }
  };

}} // namespaces
#endif
