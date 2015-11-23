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

        typedef std::vector<Sound*> SoundVec;
        SoundVec mActiveSounds;

        Environment mLastEnvironment;

        virtual std::vector<std::string> enumerate();
        virtual void init(const std::string &devname="");
        virtual void deinit();

        virtual Sound_Handle loadSound(const std::string &fname, Sound_Loudness *loudness);
        virtual void unloadSound(Sound_Handle data);
        virtual size_t getSoundDataSize(Sound_Handle data) const;

        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound(Sound_Handle data, float vol, float basevol, float pitch, int flags, float offset);
        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound3D(Sound_Handle data, const osg::Vec3f &pos,
                                             float vol, float basevol, float pitch, float min, float max, int flags, float offset);
        virtual MWBase::SoundPtr streamSound(DecoderPtr decoder, float volume, float pitch, int flags);

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env);

        virtual void pauseSounds(int types);
        virtual void resumeSounds(int types);

        OpenAL_Output& operator=(const OpenAL_Output &rhs);
        OpenAL_Output(const OpenAL_Output &rhs);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();

        struct StreamThread;
        std::auto_ptr<StreamThread> mStreamThread;

        friend class OpenAL_Sound;
        friend class OpenAL_Sound3D;
        friend class OpenAL_SoundStream;
        friend class SoundManager;
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT(x) ::MWSound::OpenAL_Output((x))
#endif
}

#endif
