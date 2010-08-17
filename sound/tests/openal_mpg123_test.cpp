#include <iostream>
#include <exception>

#include "../../stream/servers/file_stream.hpp"
#include "../filters/openal_mpg123.hpp"

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;

OpenAL_Mpg123_Factory mg;

void play(const char* name, bool stream=false)
{
  // Only load streams if the backend supports it
  if(stream && !mg.canLoadStream)
    return;

  cout << "Playing " << name;
  if(stream) cout << " (from stream)";
  cout << "\n";

  SoundPtr snd;

  try
    {
      if(stream)
        snd = mg.load(StreamPtr(new FileStream(name)));
      else
        snd = mg.load(name);

      snd->setStreaming(true);
      snd->play();

      while(snd->isPlaying())
        {
          usleep(10000);
          if(mg.needsUpdate) mg.update();
        }
    }
  catch(exception &e)
    {
      cout << "  ERROR: " << e.what() << "\n";
    }
}

int main(int argc, char**argv)
{
  if(argc != 2)
    cout << "Please specify an MP3 file\n";
  else
    play(argv[1]);
  return 0;
}
