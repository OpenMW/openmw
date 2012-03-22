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

        typedef std::vector<ALuint> IDVec;
        IDVec mFreeSources;

        typedef std::map<std::string,ALuint> NameMap;
        NameMap mBufferCache;

        typedef std::map<ALuint,ALuint> IDRefMap;
        IDRefMap mBufferRefs;

        typedef std::deque<ALuint> IDDq;
        IDDq mUnusedBuffers;

        uint64_t mBufferCacheMemSize;

        ALuint getBuffer(const std::string &fname);
        void bufferFinished(ALuint buffer);

        virtual std::vector<std::string> enumerate();
        virtual void init(const std::string &devname="");
        virtual void deinit();

        virtual Sound *playSound(const std::string &fname, float volume, float pitch, bool loop);
        virtual Sound *playSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                   float min, float max, bool loop);

        virtual Sound *streamSound(const std::string &fname, float volume, float pitch);
        virtual Sound *streamSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                     float min, float max);

        virtual void updateListener(const float *pos, const float *atdir, const float *updir);

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();

        class StreamThread;
        std::auto_ptr<StreamThread> mStreamThread;

        friend class OpenAL_Sound;
        friend class OpenAL_SoundStream;
        friend class SoundManager;
    };
#ifndef DEFAULT_OUTPUT
#define DEFAULT_OUTPUT OpenAL_Output
#endif
};

#endif
