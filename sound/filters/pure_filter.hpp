#ifndef MANGLE_SOUND_OUTPUT_PUREFILTER_H
#define MANGLE_SOUND_OUTPUT_PUREFILTER_H

#include "../output.hpp"

namespace Mangle
{
  namespace Sound
  {
    // For use in writing other filters
    class SoundFilter : public Sound
    {
    protected:
      SoundPtr client;

    public:
      SoundFilter(SoundPtr c) : client(c) {}
      void play() { client->play(); }
      void stop() { client->stop(); }
      void pause() { client->pause(); }
      bool isPlaying() const { return client->isPlaying(); }
      void setVolume(float f) { client->setVolume(f); }
      void setPan(float f) { client->setPan(f); }
      void setPos(float x, float y, float z)
      { client->setPos(x,y,z); }
      void setPitch(float p) { client->setPitch(p); }
      void setRepeat(bool b) { client->setRepeat(b); }
      void setStreaming(bool b) { client->setStreaming(b); }

      // The clone() function is not implemented here, as you will
      // almost certainly want to override it yourself
    };

    class FactoryFilter : public SoundFactory
    {
    protected:
      SoundFactoryPtr client;

    public:
      FactoryFilter(SoundFactoryPtr c) : client(c)
      {
        needsUpdate = client->needsUpdate;
        has3D = client->has3D;
        canLoadFile = client->canLoadFile;
        canLoadStream = client->canLoadStream;
        canLoadSource = client->canLoadSource;
      }

      SoundPtr loadRaw(SampleSourcePtr input)
      { return client->loadRaw(input); }

      SoundPtr load(Stream::StreamPtr input)
      { return client->load(input); }

      SoundPtr load(const std::string &file)
      { return client->load(file); }

      void update()
      { client->update(); }

      void setListenerPos(float x, float y, float z,
                          float fx, float fy, float fz,
                          float ux, float uy, float uz)
      {
        client->setListenerPos(x,y,z,fx,fy,fz,ux,uy,uz);
      }
    };
  }
}
#endif
