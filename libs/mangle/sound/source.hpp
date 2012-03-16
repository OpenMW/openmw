#ifndef MANGLE_SOUND_SOURCE_H
#define MANGLE_SOUND_SOURCE_H

#include <string>
#include <boost/cstdint.hpp>
#include <assert.h>

#include "../stream/stream.hpp"

namespace Mangle {
namespace Sound {

typedef boost::int32_t int32_t;

/// A stream containing raw sound data and information about the format
class SampleSource : public Stream::Stream
{
 protected:
  bool isEof;

 public:
  SampleSource() : isEof(false) {}

  /// Get the sample rate, number of channels, and bits per
  /// sample. NULL parameters are ignored.
  virtual void getInfo(int32_t *rate, int32_t *channels, int32_t *bits) = 0;

  bool eof() const { return isEof; }

  // Disabled functions by default. You can still override them in
  // subclasses.
  void seek(size_t pos) { assert(0); }
  size_t tell() const { assert(0); return 0; }
  size_t size() const { assert(0); return 0; }
};

typedef boost::shared_ptr<SampleSource> SampleSourcePtr;

/// A factory interface for loading SampleSources from file or stream
class SampleSourceLoader
{
 public:
  /// If true, the stream version of load() works
  bool canLoadStream;

  /// If true, the file version of load() works
  bool canLoadFile;

  /// Load a sound input source from file (if canLoadFile is true)
  virtual SampleSourcePtr load(const std::string &file) = 0;

  /// Load a sound input source from stream (if canLoadStream is true)
  virtual SampleSourcePtr load(Stream::StreamPtr input) = 0;

  /// Virtual destructor
  virtual ~SampleSourceLoader() {}
};

typedef boost::shared_ptr<SampleSourceLoader> SampleSourceLoaderPtr;

}} // namespaces
#endif
