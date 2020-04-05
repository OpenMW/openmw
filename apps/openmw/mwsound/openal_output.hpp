#ifndef GAME_SOUND_OPENAL_OUTPUT_H
#define GAME_SOUND_OPENAL_OUTPUT_H

#include <string>
#include <vector>
#include <map>
#include <deque>

#include "alc.h"
#include "al.h"
#include "alext.h"

#include "sound_output.hpp"

namespace MWSound
{
    class SoundManager;
    class Sound;
    class Stream;

    class OpenAL_Output : public Sound_Output
    {
        ALCdevice *mDevice;
        ALCcontext *mContext;

        struct {
            bool EXT_EFX : 1;
            bool SOFT_HRTF : 1;
        } ALC = {false, false};
        struct {
            bool SOFT_source_spatialize : 1;
        } AL = {false};

        typedef std::deque<ALuint> IDDq;
        IDDq mFreeSources;

        typedef std::vector<Sound*> SoundVec;
        SoundVec mActiveSounds;
        typedef std::vector<Stream*> StreamVec;
        StreamVec mActiveStreams;

        osg::Vec3f mListenerPos;
        Environment mListenerEnv;

        ALuint mWaterFilter;
        ALuint mWaterEffect;
        ALuint mDefaultEffect;
        ALuint mEffectSlot;

        struct StreamThread;
        std::unique_ptr<StreamThread> mStreamThread;

        void initCommon2D(ALuint source, const osg::Vec3f &pos, ALfloat gain, ALfloat pitch, bool loop, bool useenv);
        void initCommon3D(ALuint source, const osg::Vec3f &pos, ALfloat mindist, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool loop, bool useenv);

        void updateCommon(ALuint source, const osg::Vec3f &pos, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool useenv, bool is3d);

        OpenAL_Output& operator=(const OpenAL_Output &rhs);
        OpenAL_Output(const OpenAL_Output &rhs);

    public:
        virtual std::vector<std::string> enumerate();
        virtual bool init(const std::string &devname, const std::string &hrtfname, HrtfMode hrtfmode);
        virtual void deinit();

        virtual std::vector<std::string> enumerateHrtf();
        virtual void setHrtf(const std::string &hrtfname, HrtfMode hrtfmode);

        virtual std::pair<Sound_Handle,size_t> loadSound(const std::string &fname);
        virtual size_t unloadSound(Sound_Handle data);

        virtual bool playSound(Sound *sound, Sound_Handle data, float offset);
        virtual bool playSound3D(Sound *sound, Sound_Handle data, float offset);
        virtual void finishSound(Sound *sound);
        virtual bool isSoundPlaying(Sound *sound);
        virtual void updateSound(Sound *sound);

        virtual bool streamSound(DecoderPtr decoder, Stream *sound, bool getLoudnessData=false);
        virtual bool streamSound3D(DecoderPtr decoder, Stream *sound, bool getLoudnessData);
        virtual void finishStream(Stream *sound);
        virtual double getStreamDelay(Stream *sound);
        virtual double getStreamOffset(Stream *sound);
        virtual float getStreamLoudness(Stream *sound);
        virtual bool isStreamPlaying(Stream *sound);
        virtual void updateStream(Stream *sound);

        virtual void startUpdate();
        virtual void finishUpdate();

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env);

        virtual void pauseSounds(int types);
        virtual void resumeSounds(int types);

        virtual void pauseActiveDevice();
        virtual void resumeActiveDevice();

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT(x) ::MWSound::OpenAL_Output((x))
#endif
}

#endif
