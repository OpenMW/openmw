#include <iostream>

#include "../sources/wav_source.hpp"
#include "../../stream/servers/file_stream.hpp"

#include <assert.h>
#include <string.h>

using namespace std;
using namespace Mangle::Sound;
using namespace Mangle::Stream;

int main()
{
  WavSource wav("cow.wav");

  cout << "Source size: " << wav.size() << endl;
  int rate, channels, bits;
  wav.getInfo(&rate, &channels, &bits);
  cout << "rate=" << rate << "\nchannels=" << channels
       << "\nbits=" << bits << endl;

  cout << "Reading entire buffer into memory\n";
  void *buf = malloc(wav.size());
  wav.read(buf, wav.size());

  cout << "\nReading cow.raw\n";
  FileStream tmp("cow.raw");
  cout << "Size: " << tmp.size() << endl;
  void *buf2 = malloc(tmp.size());
  tmp.read(buf2, tmp.size());

  cout << "\nComparing...\n";
  if(tmp.size() != wav.size())
    {
      cout << "SIZE MISMATCH!\n";
      assert(0);
    }

  if(memcmp(buf, buf2, wav.size()) != 0)
    {
      cout << "CONTENT MISMATCH!\n";
      assert(0);
    }

  cout << "\nDone\n";
  return 0;
}
