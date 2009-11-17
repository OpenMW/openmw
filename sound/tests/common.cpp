// This file is included directly into the test programs

#include <iostream>
#include <exception>

using namespace std;

void play(const char* name, bool music=false)
{
  cout << "Playing " << name << "\n";

  Sound *snd = NULL;
  Instance *s = NULL;

  try
    {
      snd = mg.load(name, music);
      s = snd->getInstance(false, false);
      s->play();

      while(s->isPlaying())
        {
          usleep(10000);
          if(mg.needsUpdate) mg.update();
        }
    }
  catch(exception &e)
    {
      cout << "  ERROR: " << e.what() << "\n";
    }

  if(s) s->drop();
  if(snd) snd->drop();
}

int main()
{
  play("cow.wav");
  play("owl.ogg", true);
  return 0;
}
