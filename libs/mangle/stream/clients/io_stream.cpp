#include "io_stream.hpp"

// This seems to work
#ifndef EOF
#define EOF -1
#endif

using namespace Mangle::Stream;

#define BSIZE 1024

// Streambuf for normal stream reading
class _istreambuf : public std::streambuf
{
  StreamPtr client;
  char buf[BSIZE];

public:
  _istreambuf(StreamPtr strm) : client(strm)
  {
    // Make sure we picked the right class
    assert(client->isReadable);
    assert(!client->hasPtr);

    // Tell streambuf to delegate reading operations to underflow()
    setg(NULL,NULL,NULL);

    // Disallow writing
    setp(NULL,NULL);
  }

  /* Underflow is called when there is no more info to read in the
     input buffer. We need to refill buf with new data (if any), and
     set up the internal pointers with setg() to reflect the new
     state.
  */
  int underflow()
  {
    // Read some more data
    size_t read = client->read(buf, BSIZE);
    assert(read <= BSIZE);

    // If we're out of data, then EOF
    if(read == 0)
      return EOF;

    // Otherwise, set up input buffer
    setg(buf, buf, buf+read);

    // Return the first char
    return *((unsigned char*)buf);
  }

  // Seek stream, if the source supports it. Ignores the second
  // parameter as Mangle doesn't separate input and output pointers.
  std::streampos seekpos(std::streampos pos, std::ios_base::openmode = std::ios_base::in)
  {
    // Does this stream know how to seek?
    if(!client->isSeekable || !client->hasPosition)
      // If not, signal an error.
      return -1;

    // Set stream position and reset the buffer.
    client->seek(pos);
    setg(NULL,NULL,NULL);

    return client->tell();
  }
};

// Streambuf optimized for pointer-based input streams
class _ptrstreambuf : public std::streambuf
{
  StreamPtr client;

public:
  _ptrstreambuf(StreamPtr strm) : client(strm)
  {
    // Make sure we picked the right class
    assert(client->isReadable);
    assert(client->hasPtr);

    // seekpos() does all the work
    seekpos(0);
  }

  // Underflow is only called when we're at the end of the file
  int underflow() { return EOF; }

  // Seek to a new position within the memory stream. This bypasses
  // client->seek() entirely so isSeekable doesn't have to be set.
  std::streampos seekpos(std::streampos pos, std::ios_base::openmode = std::ios_base::in)
  {
    // All pointer streams need a size
    assert(client->hasSize);

    // Figure out how much will be left of the stream after seeking
    size_t size = client->size() - pos;

    // Get a pointer
    char* ptr = (char*)client->getPtr(pos,size);

    // And use it
    setg(ptr,ptr,ptr+size);

    return pos;
  }
};

// Streambuf for stream writing
class _ostreambuf : public std::streambuf
{
  StreamPtr client;
  char buf[BSIZE];

public:
  _ostreambuf(StreamPtr strm) : client(strm)
  {
    // Make sure we picked the right class
    assert(client->isWritable);

    // Inform streambuf about our nice buffer
    setp(buf, buf+BSIZE);

    // Disallow reading
    setg(NULL,NULL,NULL);
  }

  /* Sync means to flush (write) all current data to the output
     stream. It will also set up the entire output buffer to be usable
     again.
  */
  int sync()
  {
    // Get the number of bytes that streambuf wants us to write
    int num = pptr() - pbase();
    assert(num >= 0);

    // Is there any work to do?
    if(num == 0) return 0;

    if((int)client->write(pbase(), num) != num)
      // Inform caller that writing failed
      return -1;

    // Reset output buffer pointers
    setp(buf, buf+BSIZE);

    // No error
    return 0;
  }

  /* Called whenever the output buffer is full.
   */
  int overflow(int c)
  {
    // First, write all existing data
    if(sync()) return EOF;

    // Put the requested character in the next round of output
    if(c != EOF)
      {
        *pptr() = c;
        pbump(1);
      }

    // No error
    return 0;
  }

  // Seek stream, if the source supports it.
  std::streampos seekpos(std::streampos pos, std::ios_base::openmode = std::ios_base::out)
  {
    if(!client->isSeekable || !client->hasPosition)
      return -1;

    // Flush data and reset buffers
    sync();

    // Set stream position
    client->seek(pos);

    return client->tell();
  }
};

MangleIStream::MangleIStream(StreamPtr inp)
  : std::istream(NULL)
{
  assert(inp->isReadable);

  // Pick the right streambuf implementation based on whether the
  // input supports pointers or not.
  if(inp->hasPtr)
    buf = new _ptrstreambuf(inp);
  else
    buf = new _istreambuf(inp);

  rdbuf(buf);
}

MangleIStream::~MangleIStream()
{
  delete buf;
}

MangleOStream::MangleOStream(StreamPtr out)
  : std::ostream(NULL)
{
  assert(out->isWritable);
  buf = new _ostreambuf(out);

  rdbuf(buf);
}

MangleOStream::~MangleOStream()
{
  // Make sure we don't have lingering data on exit
  flush();
  delete buf;
}
