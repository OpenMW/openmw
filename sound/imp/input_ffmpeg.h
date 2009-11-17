#ifndef GOOI_SOUND_FFMPEG_H
#define GOOI_SOUND_FFMPEG_H

#include "../input.h"
#include <exception>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace GOOI {
namespace Sound {

/// FFmpeg exception
class FFM_Exception : public std::exception
{
  std::string msg;

 public:

  FFM_Exception(const std::string &m);
  ~FFM_Exception() throw();
  virtual const char* what() const throw();
};

/// FFMpeg implementation of InputManager
class FFM_InputManager : public InputManager
{
  static bool init;

 public:
  FFM_InputManager();
  virtual InputSource *load(const std::string &file);
};

/// FFMpeg implementation of InputSource
class FFM_InputSource : public InputSource
{
  std::string name;

 public:
  FFM_InputSource(const std::string &file);

  virtual InputStream *getStream();
  virtual void drop();
};

/// FFMpeg implementation of InputStream
class FFM_InputStream : public InputStream
{
  AVFormatContext *FmtCtx;
  AVCodecContext *CodecCtx;
  int StreamNum;
  bool empty;

  std::vector<uint8_t> storage;

 public:
  FFM_InputStream(const std::string &file);
  ~FFM_InputStream();

  virtual void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
  virtual uint32_t getData(void *data, uint32_t length);
  virtual void drop();
};

}} // namespaces
#endif
