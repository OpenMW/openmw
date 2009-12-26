#ifndef MANGLE_INPUT_FILTER_H
#define MANGLE_INPUT_FILTER_H

#include "../sound.h"

#include <assert.h>

namespace Mangle {
namespace Sound {

/**
   @brief This filter class adds file loading capabilities to a
   Sound::Manager class, by associating an InputManager with it.

   The class takes an existing Manager able to load streams, and
   associates an InputManager with it. The combined class is able to
   load files directly.

   Example:
   \code

   // Add FFmpeg input to an OpenAL soud output manager. OpenAL cannot
   // decode sound files on its own.
   InputFilter mg(new OpenAL_Manager, new FFM_InputManager);

   // We can now load filenames directly.
   mg.load("file1.mp3");
   \endcode
*/
class InputFilter : public Manager
{
 protected:
  Manager *snd;
  InputManager *inp;

 public:
  /// Empty constructor
  InputFilter() {}

  /// Assign an input manager and a sound manager to this object
  InputFilter(Manager *_snd, InputManager *_inp)
    { set(_snd, _inp); }

  /// Assign an input manager and a sound manager to this object
  void set(Manager *_snd, InputManager *_inp)
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

  virtual Sound *load(const std::string &file, bool stream=false)
  { return load(inp->load(file), stream); }

  virtual Sound *load(Stream::InputStream *input, bool stream=false)
  { return load(inp->load(input), stream); }

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
