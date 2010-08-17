#include <iostream>
#include <exception>

#include "../../stream/servers/file_stream.hpp"
#include "../sources/stream_source.hpp"
#include "../outputs/openal_out.hpp"

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;

int main()
{
  cout << "Loading cow.raw\n";

  int rate = 11025;
  int chan = 1;
  int bits = 16;

  cout << "  rate=" << rate << "\n  channels=" << chan
       << "\n  bits=" << bits << endl;

  StreamPtr file( new FileStream("cow.raw") );
  SampleSourcePtr source( new Stream2Samples( file, rate, chan, bits));

  cout << "Playing\n";

  OpenAL_Factory mg;

  SoundPtr snd = mg.loadRaw(source);

  try
    {
      // Try setting all kinds of stuff before playing. OpenAL_Sound
      // uses delayed buffer loading, but these should still work
      // without a buffer.
      snd->stop();
      snd->pause();
      snd->setVolume(0.8);
      snd->setPitch(0.9);

      snd->play();

      while(snd->isPlaying())
        {
          usleep(10000);
        }
    }
  catch(exception &e)
    {
      cout << "  ERROR: " << e.what() << "\n";
    }
  return 0;
}
