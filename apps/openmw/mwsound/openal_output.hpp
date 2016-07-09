#ifndef GAME_SOUND_OPENAL_OUTPUT_H
#define GAME_SOUND_OPENAL_OUTPUT_H

#include <string>
#include <vector>
#include <map>
#include <deque>

#include "alc.h"
#include "al.h"

#include "sound_output.hpp"

namespace MWSound
{
    class SoundManager;
    class Sound;

    class OpenAL_Output : public Sound_Output
    {
        ALCdevice *mDevice;
        ALCcontext *mContext;

        typedef std::deque<ALuint> IDDq;
        IDDq mFreeSources;

        typedef std::vector<MWBase::SoundPtr> SoundVec;
        SoundVec mActiveSounds;
        typedef std::vector<MWBase::SoundStreamPtr> StreamVec;
        StreamVec mActiveStreams;

        osg::Vec3f mListenerPos;
        Environment mListenerEnv;

        struct StreamThread;
        std::auto_ptr<StreamThread> mStreamThread;

        void initCommon2D(ALuint source, const osg::Vec3f &pos, ALfloat gain, ALfloat pitch, bool loop, bool useenv);
        void initCommon3D(ALuint source, const osg::Vec3f &pos, ALfloat mindist, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool loop, bool useenv);

        void updateCommon(ALuint source, const osg::Vec3f &pos, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool useenv, bool is3d);

        OpenAL_Output& operator=(const OpenAL_Output &rhs);
        OpenAL_Output(const OpenAL_Output &rhs);

    public:
        virtual std::vector<std::string> enumerate();
        virtual void init(const std::string &devname=std::string());
        virtual void deinit();

        virtual std::vector<std::string> enumerateHrtf();
        virtual void enableHrtf(const std::string &hrtfname, bool auto_enable);
        virtual void disableHrtf();

        virtual Sound_Handle loadSound(const std::string &fname);
        virtual void unloadSound(Sound_Handle data);
        virtual size_t getSoundDataSize(Sound_Handle data) const;

        virtual void playSound(MWBase::SoundPtr sound, Sound_Handle data, float offset);
        virtual void playSound3D(MWBase::SoundPtr sound, Sound_Handle data, float offset);
        virtual void finishSound(MWBase::SoundPtr sound);
        virtual bool isSoundPlaying(MWBase::SoundPtr sound);
        virtual void updateSound(MWBase::SoundPtr sound);

        virtual void streamSound(DecoderPtr decoder, MWBase::SoundStreamPtr sound);
        virtual void streamSound3D(DecoderPtr decoder, MWBase::SoundStreamPtr sound, bool getLoudnessData);
        virtual void finishStream(MWBase::SoundStreamPtr sound);
        virtual double getStreamDelay(MWBase::SoundStreamPtr sound);
        virtual double getStreamOffset(MWBase::SoundStreamPtr sound);
        virtual float getStreamLoudness(MWBase::SoundStreamPtr sound);
        virtual bool isStreamPlaying(MWBase::SoundStreamPtr sound);
        virtual void updateStream(MWBase::SoundStreamPtr sound);

        virtual void startUpdate();
        virtual void finishUpdate();

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env);

        virtual void pauseSounds(int types);
        virtual void resumeSounds(int types);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT(x) ::MWSound::OpenAL_Output((x))
#endif
}

#endif
