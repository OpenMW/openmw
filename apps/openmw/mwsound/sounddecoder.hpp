#ifndef GAME_SOUND_SOUNDDECODER_H
#define GAME_SOUND_SOUNDDECODER_H

#include <components/vfs/pathutil.hpp>

#include <string>
#include <vector>

namespace VFS
{
    class Manager;
}

namespace MWSound
{
    enum SampleType
    {
        SampleType_UInt8,
        SampleType_Int16,
        SampleType_Float32
    };
    const char* getSampleTypeName(SampleType type);

    enum ChannelConfig
    {
        ChannelConfig_Mono,
        ChannelConfig_Stereo,
        ChannelConfig_Quad,
        ChannelConfig_5point1,
        ChannelConfig_7point1
    };
    const char* getChannelConfigName(ChannelConfig config);

    size_t framesToBytes(size_t frames, ChannelConfig config, SampleType type);
    size_t bytesToFrames(size_t bytes, ChannelConfig config, SampleType type);

    struct SoundDecoder
    {
        const VFS::Manager* mResourceMgr;

        virtual void open(VFS::Path::NormalizedView fname) = 0;
        virtual void close() = 0;

        virtual std::string getName() = 0;
        virtual void getInfo(int* samplerate, ChannelConfig* chans, SampleType* type) = 0;

        virtual size_t read(char* buffer, size_t bytes) = 0;
        virtual void readAll(std::vector<char>& output);
        virtual size_t getSampleOffset() = 0;

        SoundDecoder(const VFS::Manager* resourceMgr)
            : mResourceMgr(resourceMgr)
        {
        }
        virtual ~SoundDecoder() {}

    private:
        SoundDecoder(const SoundDecoder& rhs);
        SoundDecoder& operator=(const SoundDecoder& rhs);
    };
}

#endif
