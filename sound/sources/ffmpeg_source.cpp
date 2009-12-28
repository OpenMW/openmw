// NOT UPDATED - WIP

#include "input_ffmpeg.h"
#include <assert.h>

using namespace Mangle::Sound;

// Static output buffer. Not thread safe, but supports multiple
// streams operated from the same thread.
static uint8_t outBuf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
bool FFM_InputManager::init = false;

/// FFmpeg exception.
class FFM_Exception : public std::exception
{
  std::string msg;

 public:

  FFM_Exception(const std::string &m);
  ~FFM_Exception() throw();
  virtual const char* what() const throw();
};

FFM_Exception::FFM_Exception(const std::string &m)
  : msg(m) {}

const char* FFM_Exception::what() const throw()
{ return msg.c_str(); }

FFM_Exception::~FFM_Exception() throw() {}

static void fail(const std::string &msg)
{
  throw FFM_Exception("FFMpeg exception: " + msg);
}


// --- Manager ---

FFM_InputManager::FFM_InputManager()
{
  if(!init)
    {
      av_register_all();
      av_log_set_level(AV_LOG_ERROR);
      init = true;
    }

  canLoadStream = false;
}

InputSource *FFM_InputManager::load(const std::string &file)
{ return new FFM_InputSource(file); }


// --- Source ---

FFM_InputSource::FFM_InputSource(const std::string &file)
{
  // FFmpeg doesn't handle several instances from one source. So we
  // just store the filename.
  name = file;
}

InputStream *FFM_InputSource::getStream()
{ return new FFM_InputStream(name); }

void FFM_InputSource::drop()
{ delete this; }


// --- Stream ---

FFM_InputStream::FFM_InputStream(const std::string &file)
{
  std::string msg;
  AVCodec *codec;

  empty = false;

  if(av_open_input_file(&FmtCtx, file.c_str(), NULL, 0, NULL) != 0)
    fail("Error loading audio file " + file);
  
  if(av_find_stream_info(FmtCtx) < 0)
    {
      msg = "Error in file stream " + file;
      goto err;
    }

  // Pick the first audio stream, if any
  for(StreamNum = 0; StreamNum < FmtCtx->nb_streams; StreamNum++)
    {
      // Pick the first audio stream
      if(FmtCtx->streams[StreamNum]->codec->codec_type == CODEC_TYPE_AUDIO)
        break;
    }

  if(StreamNum == FmtCtx->nb_streams)
    fail("File " + file + " didn't contain any audio streams");

  // Open the decoder
  CodecCtx = FmtCtx->streams[StreamNum]->codec;
  codec = avcodec_find_decoder(CodecCtx->codec_id);

  if(!codec || avcodec_open(CodecCtx, codec) < 0)
    {
      msg = "Error loading " + file + ": ";
      if(codec)
        msg += "coded error";
      else
        msg += "no codec found";
      goto err;
    }

  // No errors, we're done
  return;

  // Handle errors
 err:
  av_close_input_file(FmtCtx);
  fail(msg);
}

FFM_InputStream::~FFM_InputStream()
{
  avcodec_close(CodecCtx);
  av_close_input_file(FmtCtx);
}

void FFM_InputStream::getInfo(int32_t *rate, int32_t *channels, int32_t *bits)
{
  if(rate) *rate = CodecCtx->sample_rate;
  if(channels) *channels = CodecCtx->channels;
  if(bits) *bits = 16;
}

uint32_t FFM_InputStream::getData(void *data, uint32_t length)
{
  if(empty) return 0;

  uint32_t left = length;
  uint8_t *outPtr = (uint8_t*)data;

  // First, copy over any stored data we might be sitting on
  {
    int s = storage.size();
    int copy = s;
    if(s)
      {
        // Make sure there's room
        if(copy > left)
          copy = left;

        // Copy
        memcpy(outPtr, &storage[0], copy);
        outPtr += copy;
        left -= copy;

        // Is there anything left in the storage?
        s -= copy;
        if(s)
          {
            assert(left == 0);

            // Move it to the start and resize
            memmove(&storage[0], &storage[copy], s);
            storage.resize(s);
          }
      }
  }

  // Next, get more input data from stream, and decode it
  while(left)
    {
      AVPacket packet;

      // Get the next packet, if any
      if(av_read_frame(FmtCtx, &packet) < 0)
        break;

      // We only allow one stream per file at the moment
      assert(StreamNum == packet.stream_index);

      // Decode the packet
      int len = AVCODEC_MAX_AUDIO_FRAME_SIZE;
      int tmp = avcodec_decode_audio2(CodecCtx, (int16_t*)outBuf,
                                      &len, packet.data, packet.size);
      assert(tmp < 0 || tmp == packet.size);

      // We don't need the input packet any longer
      av_free_packet(&packet);

      if(tmp < 0)
        fail("Error decoding audio stream");

      // Copy whatever data we got, and advance the pointer
      if(len > 0)
        {
          // copy = how many bytes do we copy now
          int copy = len;
          if(copy > left)
            copy = left;

          // len = how many bytes are left uncopied
          len -= copy;

          // copy data
          memcpy(outPtr, outBuf, copy);

          // left = how much space is left in the caller output
          // buffer
          left -= copy;
          outPtr += copy;
          assert(left >= 0);

          if(len > 0)
            {
              // There were uncopied bytes. Store them for later.
              assert(left == 0);
              storage.resize(len);
              memcpy(&storage[0], outBuf, len);
            }
        }
    }

  // End of loop. Return the number of bytes copied.
  assert(left <= length);

  // If we're returning less than asked for, then we're done
  if(left > 0)
    empty = true;

  return length - left;
}

void FFM_InputStream::drop()
{ delete this; }
