#ifndef GOOI_SOUND_PAIR_H
#define GOOI_SOUND_PAIR_H

#include "sound.h"

#include <assert.h>

namespace GOOI {
namespace Sound {

/**
   @brief This filter class adds file loading capabilities to a
   Sound::Manager class, by associating an InputManager with it.

   The class takes an existing Manager able to load streams, and
   associates an InputManager with it. The combined class is able to
   load files directly.

   Example:
   \code

   // Combine FFmpeg input and OpenAL output. OpenAL cannot decode
   // sound files on its own.
   SoundPairManager mg(new FFM_InputManager, new OpenAL_Manager);

   // We can now load filenames directly.
   mg.load("file1.mp3");
   \endcode
*/
class PairManager : public Manager
{
 protected:
  Manager *snd;
  InputManager *inp;

 public:
  /// Empty constructor
  PairManager() {}

  /// Assign an input manager and a sound manager to this object
  PairManager(InputManager *_inp, Manager *_snd)
    { set(_inp, _snd); }

  /// Assign an input manager and a sound manager to this object
  void set(InputManager *_inp, Manager *_snd)
    {
      inp = _inp;
      snd = _snd;

      needsUpdate = snd->needsUpdate;
      has3D = snd->has3D;
      canRepeatStream = snd->canRepeatStream;

      // Both these should be true, or the use of this class is pretty
      // pointless
      canLoadSource = snd->canLoadSource;
      canLoadFile = canLoadSource;
      assert(canLoadSource && canLoadFile);
    }

  virtual Sound *load(const std::string &file, bool stream=false)
    { return load(inp->load(file), stream); }

  virtual Sound *load(InputSource *input, bool stream=false)
    { return snd->load(input, stream); }

  virtual void update() { snd->update(); }
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz)
    { snd->setListenerPos(x,y,z,fx,fy,fz,ux,uy,uz); }
};

}}
#endif
