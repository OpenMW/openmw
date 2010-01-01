#include <iostream>
#include <exception>

#include "../../stream/servers/file_stream.h"
#include "../sources/stream_source.h"
#include "../outputs/openal_out.h"

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

  // This initializes OpenAL for us, and serves no other purpose.
  OpenAL_Factory mg;

  OpenAL_Sound snd(source);
  try
    {
      snd.play();

      while(snd.isPlaying())
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
