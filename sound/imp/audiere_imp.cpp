#include "audiere_imp.h"

// Exception handling
class Audiere_Exception : public std::exception
{
  std::string msg;

 public:

  Audiere_Exception(const std::string &m) : msg(m) {}
  ~Audiere_Exception() throw() {}
  virtual const char* what() const throw() { return msg.c_str(); }
};

static void fail(const std::string &msg)
{
  throw Audiere_Exception("Audiere exception: " + msg);
}

using namespace audiere;
using namespace Mangle::Sound;

AudiereManager::AudiereManager()
{
  needsUpdate = false;
  has3D = false;
  canRepeatStream = true;
  canLoadFile = true;
  canLoadSource = false;

  device = OpenDevice("");

  if(device == NULL)
    fail("Failed to open device");
}

// --- Manager ---

Sound *AudiereManager::load(const std::string &file, bool stream)
{ return new AudiereSound(file, device, stream); }


// --- Sound ---

AudiereSound::AudiereSound(const std::string &file,
                           AudioDevicePtr _device,
                           bool _stream)
  : device(_device), stream(_stream)
{
  sample = OpenSampleSource(file.c_str());
  if(!sample)
    fail("Couldn't load file " + file);

  buf = CreateSampleBuffer(sample);
}

Instance *AudiereSound::getInstance(bool is3d, bool repeat)
{
  // Ignore is3d. Audiere doesn't implement 3d sound. We could make a
  // hack software 3D implementation later, but it's not that
  // important.

  SampleSourcePtr sample = buf->openStream();
  if(!sample)
    fail("Failed to open sample stream");

  OutputStreamPtr sound = OpenSound(device, sample, stream);

  if(repeat)
    sound->setRepeat(true);

  return new AudiereInstance(sound);
}


// --- Instance ---

AudiereInstance::AudiereInstance(OutputStreamPtr _sound)
  : sound(_sound) {}
                                 
void AudiereInstance::play()
{ sound->play(); }

void AudiereInstance::stop()
{ sound->stop(); }

void AudiereInstance::pause()
{ stop(); }

bool AudiereInstance::isPlaying()
{ return sound->isPlaying(); }

void AudiereInstance::setVolume(float vol)
{ sound->setVolume(vol); }
