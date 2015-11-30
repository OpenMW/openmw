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

        OpenAL_Output& operator=(const OpenAL_Output &rhs);
        OpenAL_Output(const OpenAL_Output &rhs);

    public:
        virtual std::vector<std::string> enumerate();
        virtual void init(const std::string &devname="");
        virtual void deinit();

        virtual Sound_Handle loadSound(const std::string &fname);
        virtual void unloadSound(Sound_Handle data);
        virtual size_t getSoundDataSize(Sound_Handle data) const;

        virtual MWBase::SoundPtr playSound(Sound_Handle data, float vol, float basevol, float pitch, int flags, float offset);
        virtual MWBase::SoundPtr playSound3D(Sound_Handle data, const osg::Vec3f &pos,
                                             float vol, float basevol, float pitch, float min, float max, int flags, float offset);
        virtual void stopSound(MWBase::SoundPtr sound);
        virtual bool isSoundPlaying(MWBase::SoundPtr sound);
        virtual void updateSound(MWBase::SoundPtr sound);

        virtual MWBase::SoundStreamPtr streamSound(DecoderPtr decoder, float basevol, float pitch, int flags);
        virtual MWBase::SoundStreamPtr streamSound3D(DecoderPtr decoder, const osg::Vec3f &pos,
                                                     float vol, float basevol, float pitch, float min, float max, int flags);
        virtual void stopStream(MWBase::SoundStreamPtr sound);
        virtual double getStreamDelay(MWBase::SoundStreamPtr sound);
        virtual double getStreamOffset(MWBase::SoundStreamPtr sound);
        virtual bool isStreamPlaying(MWBase::SoundStreamPtr sound);
        virtual void updateStream(MWBase::SoundStreamPtr sound);

        virtual void startUpdate();
        virtual void finishUpdate();

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env);

        virtual void pauseSounds(int types);
        virtual void resumeSounds(int types);

        virtual void loadLoudnessAsync(DecoderPtr decoder, Sound_Loudness *loudness);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT(x) ::MWSound::OpenAL_Output((x))
#endif
}

#endif
