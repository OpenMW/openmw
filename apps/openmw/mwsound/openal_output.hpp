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

    struct CachedSound
    {
        ALuint mALBuffer;
        std::vector<float> mLoudnessVector;
    };

    class OpenAL_Output : public Sound_Output
    {
        ALCdevice *mDevice;
        ALCcontext *mContext;

        typedef std::deque<ALuint> IDDq;
        IDDq mFreeSources;
        IDDq mUnusedBuffers;

        typedef std::map<std::string,CachedSound> NameMap;
        NameMap mBufferCache;

        typedef std::map<ALuint,ALuint> IDRefMap;
        IDRefMap mBufferRefs;

        uint64_t mBufferCacheMemSize;

        typedef std::vector<Sound*> SoundVec;
        SoundVec mActiveSounds;

        const CachedSound& getBuffer(const std::string &fname);
        void bufferFinished(ALuint buffer);

        Environment mLastEnvironment;

        virtual std::vector<std::string> enumerate();
        virtual void init(const std::string &devname="");
        virtual void deinit();

        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound(const std::string &fname, float vol, float basevol, float pitch, int flags, float offset);
        /// @param offset Value from [0,1] meaning from which fraction the sound the playback starts.
        virtual MWBase::SoundPtr playSound3D(const std::string &fname, const osg::Vec3f &pos,
                                             float vol, float basevol, float pitch, float min, float max, int flags, float offset, bool extractLoudness=false);
        virtual MWBase::SoundPtr streamSound(DecoderPtr decoder, float volume, float pitch, int flags);

        virtual void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env);

        virtual void pauseSounds(int types);
        virtual void resumeSounds(int types);

        OpenAL_Output& operator=(const OpenAL_Output &rhs);
        OpenAL_Output(const OpenAL_Output &rhs);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();

        struct StreamThread;
        std::unique_ptr<StreamThread> mStreamThread;

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
