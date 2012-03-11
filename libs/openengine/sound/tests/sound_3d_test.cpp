#include <iostream>
#include <exception>
#include <assert.h>

#include <mangle/stream/servers/file_stream.hpp>
#include <mangle/sound/filters/openal_audiere.hpp>

#include <sound/sndmanager.hpp>

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;
using namespace OEngine::Sound;

const std::string sound = "../../mangle/sound/tests/cow.wav";

SoundManagerPtr m;

// Play and wait for finish
void play(float x, float y, float z)
{
  cout << "Playing at " << x << "," << y << "," << z << endl;

  SoundPtr snd = m->play3D(sound,x,y,z);

  while(snd->isPlaying())
    {
      usleep(10000);
      m->update();
    }
}

int main()
{
  SoundFactoryPtr oaf(new OpenAL_Audiere_Factory);
  SoundManagerPtr mg(new SoundManager(oaf));
  m = mg;

  mg->setListenerPos(0,0,0,0,1,0,0,0,1);

  play(0,0,0);
  play(1,1,0);
  play(-1,0,0);

  return 0;
}
