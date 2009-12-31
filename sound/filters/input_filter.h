#ifndef MANGLE_INPUT_FILTER_H
#define MANGLE_INPUT_FILTER_H

#include "../output.h"

#include <assert.h>

namespace Mangle {
namespace Sound {

/**
   @brief This filter class adds file loading capabilities to a
   Sound::SoundFactory class, by associating an SampleSourceLoader
   with it.

   The class takes an existing SoundFactory able to load streams, and
   associates an SampleSourceLoader with it. The combined class is
   able to load files directly.
*/
class InputFilter : public SoundFactory
{
 protected:
  SoundFactoryPtr snd;
  SampleSourceLoaderPtr inp;

 public:
  /// Empty constructor
  InputFilter() {}

  /// Assign an input manager and a sound manager to this object
  InputFilter(SoundFactoryPtr _snd, SampleSourceLoaderPtr _inp)
    { set(_snd, _inp); }

  /// Assign an input manager and a sound manager to this object
  void set(SoundFactoryPtr _snd, SampleSourceLoaderPtr _inp)
  {
    inp = _inp;
    snd = _snd;

    // Set capabilities
    needsUpdate = snd->needsUpdate;
    has3D = snd->has3D;
    canRepeatStream = snd->canRepeatStream;
    canLoadStream = inp->canLoadStream;

    // Both these should be true, or the use of this class is pretty
    // pointless
    canLoadSource = snd->canLoadSource;
    canLoadFile = canLoadSource;
    assert(canLoadSource && canLoadFile);
  }

  virtual SoundPtr load(const std::string &file)
  { return load(inp->load(file), stream); }

  virtual SoundPtr load(Stream::StreamPtr input)
  { return load(inp->load(input), stream); }

  virtual SoundPtr load(SampleSourcePtr input)
  { return snd->load(input, stream); }

  virtual void update() { snd->update(); }
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz)
    { snd->setListenerPos(x,y,z,fx,fy,fz,ux,uy,uz); }
};

}}
#endif
