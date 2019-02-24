#include "movieaudiofactory.hpp"

#include <extern/osg-ffmpeg-videoplayer/audiodecoder.hpp>
#include <extern/osg-ffmpeg-videoplayer/videostate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "sound_decoder.hpp"
#include "sound.hpp"

namespace MWSound
{

    class MovieAudioDecoder;
    class MWSoundDecoderBridge final : public Sound_Decoder
    {
    public:
        MWSoundDecoderBridge(MWSound::MovieAudioDecoder* decoder)
            : Sound_Decoder(nullptr)
            , mDecoder(decoder)
        {
        }

    private:
        MWSound::MovieAudioDecoder* mDecoder;

        void open(const std::string &fname) override;
        void close() override;
        std::string getName() override;
        void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type) override;
        size_t read(char *buffer, size_t bytes) override;
        size_t getSampleOffset() override;
    };

    class MovieAudioDecoder : public Video::MovieAudioDecoder
    {
    public:
        MovieAudioDecoder(Video::VideoState *videoState)
            : Video::MovieAudioDecoder(videoState), mAudioTrack(nullptr)
        {
            mDecoderBridge.reset(new MWSoundDecoderBridge(this));
        }

        size_t getSampleOffset()
        {
            ssize_t clock_delay = (mFrameSize-mFramePos) / av_get_channel_layout_nb_channels(mOutputChannelLayout) /
                                  av_get_bytes_per_sample(mOutputSampleFormat);
            return (size_t)(mAudioClock*mAudioContext->sample_rate) - clock_delay;
        }

        std::string getStreamName()
        {
            return std::string();
        }

    private:
        // MovieAudioDecoder overrides

        virtual double getAudioClock()
        {
            return (double)getSampleOffset()/(double)mAudioContext->sample_rate -
                   MWBase::Environment::get().getSoundManager()->getTrackTimeDelay(mAudioTrack);
        }

        virtual void adjustAudioSettings(AVSampleFormat& sampleFormat, uint64_t& channelLayout, int& sampleRate)
        {
            if (sampleFormat == AV_SAMPLE_FMT_U8P || sampleFormat == AV_SAMPLE_FMT_U8)
                sampleFormat = AV_SAMPLE_FMT_U8;
            else if (sampleFormat == AV_SAMPLE_FMT_S16P || sampleFormat == AV_SAMPLE_FMT_S16)
                sampleFormat = AV_SAMPLE_FMT_S16;
            else if (sampleFormat == AV_SAMPLE_FMT_FLTP || sampleFormat == AV_SAMPLE_FMT_FLT)
                sampleFormat = AV_SAMPLE_FMT_S16; // FIXME: check for AL_EXT_FLOAT32 support
            else
                sampleFormat = AV_SAMPLE_FMT_S16;

            if (channelLayout == AV_CH_LAYOUT_5POINT1 || channelLayout == AV_CH_LAYOUT_7POINT1
                    || channelLayout == AV_CH_LAYOUT_QUAD) // FIXME: check for AL_EXT_MCFORMATS support
                channelLayout = AV_CH_LAYOUT_STEREO;
            else if (channelLayout != AV_CH_LAYOUT_MONO
                     && channelLayout != AV_CH_LAYOUT_STEREO)
                channelLayout = AV_CH_LAYOUT_STEREO;
        }

    public:
        ~MovieAudioDecoder()
        {
            if(mAudioTrack)
                MWBase::Environment::get().getSoundManager()->stopTrack(mAudioTrack);
            mAudioTrack = nullptr;
            mDecoderBridge.reset();
        }

        MWBase::SoundStream *mAudioTrack;
        std::shared_ptr<MWSoundDecoderBridge> mDecoderBridge;
    };


    void MWSoundDecoderBridge::open(const std::string &fname)
    {
        throw std::runtime_error("Method not implemented");
    }
    void MWSoundDecoderBridge::close() {}

    std::string MWSoundDecoderBridge::getName()
    {
        return mDecoder->getStreamName();
    }

    void MWSoundDecoderBridge::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
    {
        *samplerate = mDecoder->getOutputSampleRate();

        uint64_t outputChannelLayout = mDecoder->getOutputChannelLayout();
        if (outputChannelLayout == AV_CH_LAYOUT_MONO)
            *chans = ChannelConfig_Mono;
        else if (outputChannelLayout == AV_CH_LAYOUT_5POINT1)
            *chans = ChannelConfig_5point1;
        else if (outputChannelLayout == AV_CH_LAYOUT_7POINT1)
            *chans = ChannelConfig_7point1;
        else if (outputChannelLayout == AV_CH_LAYOUT_STEREO)
            *chans = ChannelConfig_Stereo;
        else if (outputChannelLayout == AV_CH_LAYOUT_QUAD)
            *chans = ChannelConfig_Quad;
        else
            throw std::runtime_error("Unsupported channel layout: "+
                                     std::to_string(outputChannelLayout));

        AVSampleFormat outputSampleFormat = mDecoder->getOutputSampleFormat();
        if (outputSampleFormat == AV_SAMPLE_FMT_U8)
            *type = SampleType_UInt8;
        else if (outputSampleFormat == AV_SAMPLE_FMT_FLT)
            *type = SampleType_Float32;
        else if (outputSampleFormat == AV_SAMPLE_FMT_S16)
            *type = SampleType_Int16;
        else
        {
            char str[1024];
            av_get_sample_fmt_string(str, sizeof(str), outputSampleFormat);
            throw std::runtime_error(std::string("Unsupported sample format: ")+str);
        }
    }

    size_t MWSoundDecoderBridge::read(char *buffer, size_t bytes)
    {
        return mDecoder->read(buffer, bytes);
    }

    size_t MWSoundDecoderBridge::getSampleOffset()
    {
        return mDecoder->getSampleOffset();
    }



    std::shared_ptr<Video::MovieAudioDecoder> MovieAudioFactory::createDecoder(Video::VideoState* videoState)
    {
        std::shared_ptr<MWSound::MovieAudioDecoder> decoder(new MWSound::MovieAudioDecoder(videoState));
        decoder->setupFormat();

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        MWBase::SoundStream *sound = sndMgr->playTrack(decoder->mDecoderBridge, MWSound::Type::Movie);
        if (!sound)
        {
            decoder.reset();
            return decoder;
        }

        decoder->mAudioTrack = sound;
        return decoder;
    }

}
