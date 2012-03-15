#include <iostream>
#include <exception>

#include "../../stream/servers/file_stream.hpp"
#include "../filters/openal_various.hpp"

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;

OpenAL_Various_Factory mg;

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

int main()
{
  play("cow.wav");
  play("cow.wav", true);
  return 0;
}
