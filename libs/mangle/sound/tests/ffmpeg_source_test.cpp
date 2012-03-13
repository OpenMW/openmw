#include <iostream>

#include "../../stream/servers/file_stream.hpp"
#include "../sources/ffmpeg_source.hpp"

#include <assert.h>
#include <string.h>

using namespace std;
using namespace Mangle::Stream;
using namespace Mangle::Sound;

// Contents and size of cow.raw
void *orig;
size_t orig_size;

void run(SampleSourcePtr &src)
{
  int rate, channels, bits;
  src->getInfo(&rate, &channels, &bits);
  cout << "rate=" << rate << "\nchannels=" << channels
       << "\nbits=" << bits << endl;

  cout << "Reading entire buffer into memory\n";
  void *buf = malloc(orig_size);
  size_t ss = src->read(buf, orig_size);
  cout << "Actually read: " << ss << endl;
  assert(ss == orig_size);

  cout << "Comparing...\n";
  if(memcmp(buf, orig, ss) != 0)
    {
      cout << "Oops!\n";
      assert(0);
    }

  cout << "Done\n";
}

int main()
{
  {
    cout << "Reading cow.raw first\n";
    FileStream tmp("cow.raw");
    orig_size = tmp.size();
    cout << "Size: " << orig_size << endl;
    orig = malloc(orig_size);
    tmp.read(orig, orig_size);
    cout << "Done\n";
  }

  // Initializes the library, not used for anything else.
  FFMpegLoader fm;

  {
    cout << "\nLoading cow.wav by filename:\n";
    SampleSourcePtr cow_file( new FFMpegSource("cow.wav") );
    run(cow_file);
  }

  return 0;
}
