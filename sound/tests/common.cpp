// This file is included directly into the test programs

#include <iostream>
#include <fstream>
#include <exception>

using namespace std;

class TestStream : public Mangle::Stream::Stream
{
  ifstream io;

public:

  TestStream(const char* name)
  {
    io.open(name, ios::binary);
    isSeekable = true;
    hasPosition = true;
    hasSize = false;
  }

  size_t read(void* buf, size_t len)
  {
    io.read((char*)buf, len);
    return io.gcount();
  }

  void seek(size_t pos)
  {
    io.seekg(pos);
  }

  size_t tell() const
  { return ((TestStream*)this)->io.tellg(); }

  size_t size() const
  { return 0; }

  bool eof() const
  { return io.eof(); }
};

void play(const char* name, bool music=false, bool stream=false)
{
  // Only load streams if the backend supports it
  if(stream && !mg.canLoadStream)
    return;

  cout << "Playing " << name;
  if(stream) cout << " (from stream)";
  cout << "\n";

  Sound *snd = NULL;
  Instance *s = NULL;

  try
    {
      if(stream)
        snd = mg.load(new TestStream(name), music);
      else
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
  play("cow.wav", false, true);
  return 0;
}
