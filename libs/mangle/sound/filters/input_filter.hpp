#ifndef MANGLE_INPUT_FILTER_H
#define MANGLE_INPUT_FILTER_H

#include "../output.hpp"

#include <assert.h>

namespace Mangle {
namespace Sound {

/**
   @brief This filter class adds file loading capabilities to a
   Sound::SoundFactory class, by associating a SampleSourceLoader with
   it.

   The class takes an existing SoundFactory able to load streams, and
   associates a SampleSourceLoader with it. The combined class is able
   to load files directly.  */
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
    canLoadStream = inp->canLoadStream;

    // Both these should be true, or the use of this class is pretty
    // pointless
    canLoadSource = snd->canLoadSource;
    canLoadFile = inp->canLoadFile;
    assert(canLoadSource && canLoadFile);
  }

  virtual SoundPtr load(const std::string &file)
  { return loadRaw(inp->load(file)); }

  virtual SoundPtr load(Stream::StreamPtr input)
  { return loadRaw(inp->load(input)); }

  virtual SoundPtr loadRaw(SampleSourcePtr input)
  { return snd->loadRaw(input); }

  virtual void update() { snd->update(); }
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz)
    { snd->setListenerPos(x,y,z,fx,fy,fz,ux,uy,uz); }
};

}}
#endif
