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
        std::vector<std::string> enumerate() override;
        bool init(const std::string &devname, const std::string &hrtfname, HrtfMode hrtfmode) override;
        void deinit() override;

        std::vector<std::string> enumerateHrtf() override;
        void setHrtf(const std::string &hrtfname, HrtfMode hrtfmode) override;

        std::pair<Sound_Handle,size_t> loadSound(const std::string &fname) override;
        size_t unloadSound(Sound_Handle data) override;

        bool playSound(Sound *sound, Sound_Handle data, float offset) override;
        bool playSound3D(Sound *sound, Sound_Handle data, float offset) override;
        void finishSound(Sound *sound) override;
        bool isSoundPlaying(Sound *sound) override;
        void updateSound(Sound *sound) override;

        bool streamSound(DecoderPtr decoder, Stream *sound, bool getLoudnessData=false) override;
        bool streamSound3D(DecoderPtr decoder, Stream *sound, bool getLoudnessData) override;
        void finishStream(Stream *sound) override;
        double getStreamDelay(Stream *sound) override;
        double getStreamOffset(Stream *sound) override;
        float getStreamLoudness(Stream *sound) override;
        bool isStreamPlaying(Stream *sound) override;
        void updateStream(Stream *sound) override;

        void startUpdate() override;
        void finishUpdate() override;

        void updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env) override;

        void pauseSounds(int types) override;
        void resumeSounds(int types) override;

        void pauseActiveDevice() override;
        void resumeActiveDevice() override;

        OpenAL_Output(SoundManager &mgr);
        virtual ~OpenAL_Output();
    };
}

#endif
