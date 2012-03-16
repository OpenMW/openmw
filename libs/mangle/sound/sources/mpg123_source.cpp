#include "mpg123_source.hpp"

#include <stdexcept>

#include <mpg123.h>

using namespace Mangle::Stream;

/*
  TODOs:

  - mpg123 impressively enough supports custom stream reading. Which
    means we could (and SHOULD!) support reading from Mangle::Streams
    as well. But I'll save it til I need it.

    An alternative way to do this is through feeding (which they also
    support), but that's more messy.

  - the library also supports output, via various other sources,
    including ALSA, OSS, PortAudio, PulseAudio and SDL. Using this
    library as a pure output library (if that is possible) would be a
    nice shortcut over using those libraries - OTOH it's another
    dependency.

  - we could implement seek(), tell() and size(), but they aren't
    really necessary. Furthermore, since the returned size is only a
    guess, it is not safe to rely on it.
 */

static void fail(const std::string &msg)
{ throw std::runtime_error("Mangle::Mpg123 exception: " + msg); }

static void checkError(int err, void *mh = NULL)
{
  if(err != MPG123_OK)
    {
      std::string msg;
      if(mh) msg = mpg123_strerror((mpg123_handle*)mh);
      else msg = mpg123_plain_strerror(err);
      fail(msg);
    }
}

using namespace Mangle::Sound;

void Mpg123Source::getInfo(int32_t *pRate, int32_t *pChannels, int32_t *pBits)
{
  // Use the values we found in the constructor
  *pRate = rate;
  *pChannels = channels;
  *pBits = bits;
}

size_t Mpg123Source::read(void *data, size_t length)
{
  size_t done;
  // This is extraordinarily nice. I like this library.
  int err = mpg123_read((mpg123_handle*)mh, (unsigned char*)data, length, &done);
  assert(done <= length);
  if(err == MPG123_DONE)
    isEof = true;
  else
    checkError(err, mh);
  return done;
}

Mpg123Loader::Mpg123Loader(bool setup)
{
  // Do as we're told
  if(setup)
    {
      int err = mpg123_init();
      checkError(err);
    }
  didSetup = setup;
}

Mpg123Loader::~Mpg123Loader()
{
  // Deinitialize the library on exit
  if(didSetup)
    mpg123_exit();
}

Mpg123Source::Mpg123Source(const std::string &file)
{
  int err;

  // Create a new handle
  mh = mpg123_new(NULL, &err);
  if(mh == NULL)
    checkError(err, mh);

  mpg123_handle *mhh = (mpg123_handle*)mh;

  // Open the file (hack around constness)
  err = mpg123_open(mhh, (char*)file.c_str());
  checkError(err, mh);

  // Get the format
  int encoding;
  err = mpg123_getformat(mhh, &rate, &channels, &encoding);
  checkError(err, mh);
  if(encoding != MPG123_ENC_SIGNED_16)
    fail("Unsupported encoding in " + file);

  // This is the only bit size we support.
  bits = 16;
}

Mpg123Source::~Mpg123Source()
{
  mpg123_close((mpg123_handle*)mh);
  mpg123_delete((mpg123_handle*)mh);
}
