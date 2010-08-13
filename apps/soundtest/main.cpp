#include <iostream>

#include <mangle/stream/servers/file_stream.hpp>
#include <mangle/sound/sources/stream_source.hpp>
#include <mangle/stream/filters/buffer_stream.hpp>

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;

#ifdef OPENMW_USE_AUDIERE
#include <mangle/sound/filters/openal_audiere.hpp>
AudiereLoader loader;
#endif

#ifdef OPENMW_USE_FFMPEG
#include <mangle/sound/filters/openal_ffmpeg.hpp>
FFMpegLoader loader;
#endif

#ifdef OPENMW_USE_MPG123
#include <mangle/sound/filters/openal_mpg123.hpp>
Mpg123Loader loader;
#endif

OpenAL_Factory openal;

void play(const char* name)
{
  try
    {
      cout << "Opening " << name << "\n";
      SampleSourcePtr samples = loader.load(name);
      cout << "Loading entire file into memory\n";
      StreamPtr buf(new BufferStream(samples));

      // Recreate the stream as a sample source (we're only doing it
      // this complicated to test each step individually)
      int a,b,c;
      samples->getInfo(&a,&b,&c);
      samples.reset(new Stream2Samples(buf, a,b,c));

      cout << "Creating OpenAL sound from data\n";
      SoundPtr snd = openal.loadRaw(samples);
      cout << "Playing (abort with Ctrl-C)\n";
      snd->play();

      while(snd->isPlaying())
        usleep(10000);
      cout << "Done playing\n";
    }
  catch(exception &e)
    {
      cout << "  ERROR: " << e.what() << "\n";
    }
}

int main(int argc, char** argv)
{
  if(argc==1)
    cout << "Specify sound file (wav, mp3, ogg) on command line.\n";
  for(int i=1; i<argc; i++)
    play(argv[i]);
  return 0;
}
