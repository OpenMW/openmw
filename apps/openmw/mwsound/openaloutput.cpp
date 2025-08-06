#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include <cstdint>

#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/thread.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>

#include "efxpresets.h"
#include "loudness.hpp"
#include "openaloutput.hpp"
#include "sound.hpp"
#include "sounddecoder.hpp"
#include "soundmanagerimp.hpp"

#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
#endif

#define MAKE_PTRID(id) ((void*)(uintptr_t)id)
#define GET_PTRID(ptr) ((ALuint)(uintptr_t)ptr)

namespace
{

    const float sLoudnessFPS = 20.0f; // loudness values per second of audio

    ALCenum checkALCError(ALCdevice* device, const char* func, int line)
    {
        ALCenum err = alcGetError(device);
        if (err != ALC_NO_ERROR)
            Log(Debug::Error) << "ALC error " << alcGetString(device, err) << " (" << err << ") @ " << func << ":"
                              << line;
        return err;
    }
#define getALCError(d) checkALCError((d), __FUNCTION__, __LINE__)

    ALenum checkALError(const char* func, int line)
    {
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
            Log(Debug::Error) << "AL error " << alGetString(err) << " (" << err << ") @ " << func << ":" << line;
        return err;
    }
#define getALError() checkALError(__FUNCTION__, __LINE__)

    // Helper to get an OpenAL extension function
    template <typename T, typename R>
    void convertPointer(T& dest, R src)
    {
        memcpy(&dest, &src, sizeof(src));
    }

    template <typename T>
    void getALCFunc(T& func, ALCdevice* device, const char* name)
    {
        void* funcPtr = alcGetProcAddress(device, name);
        convertPointer(func, funcPtr);
    }

    template <typename T>
    void getALFunc(T& func, const char* name)
    {
        void* funcPtr = alGetProcAddress(name);
        convertPointer(func, funcPtr);
    }

    // Effect objects
    LPALGENEFFECTS alGenEffects;
    LPALDELETEEFFECTS alDeleteEffects;
    LPALISEFFECT alIsEffect;
    LPALEFFECTI alEffecti;
    LPALEFFECTIV alEffectiv;
    LPALEFFECTF alEffectf;
    LPALEFFECTFV alEffectfv;
    LPALGETEFFECTI alGetEffecti;
    LPALGETEFFECTIV alGetEffectiv;
    LPALGETEFFECTF alGetEffectf;
    LPALGETEFFECTFV alGetEffectfv;
    // Filter objects
    LPALGENFILTERS alGenFilters;
    LPALDELETEFILTERS alDeleteFilters;
    LPALISFILTER alIsFilter;
    LPALFILTERI alFilteri;
    LPALFILTERIV alFilteriv;
    LPALFILTERF alFilterf;
    LPALFILTERFV alFilterfv;
    LPALGETFILTERI alGetFilteri;
    LPALGETFILTERIV alGetFilteriv;
    LPALGETFILTERF alGetFilterf;
    LPALGETFILTERFV alGetFilterfv;
    // Auxiliary slot objects
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
    LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
    LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
    LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
    LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
    LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
    LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
    LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

    LPALEVENTCONTROLSOFT alEventControlSOFT;
    LPALEVENTCALLBACKSOFT alEventCallbackSOFT;
    LPALCREOPENDEVICESOFT alcReopenDeviceSOFT;

    void LoadEffect(ALuint effect, const EFXEAXREVERBPROPERTIES& props)
    {
        ALint type = AL_NONE;
        alGetEffecti(effect, AL_EFFECT_TYPE, &type);
        if (type == AL_EFFECT_EAXREVERB)
        {
            alEffectf(effect, AL_EAXREVERB_DIFFUSION, props.flDiffusion);
            alEffectf(effect, AL_EAXREVERB_DENSITY, props.flDensity);
            alEffectf(effect, AL_EAXREVERB_GAIN, props.flGain);
            alEffectf(effect, AL_EAXREVERB_GAINHF, props.flGainHF);
            alEffectf(effect, AL_EAXREVERB_GAINLF, props.flGainLF);
            alEffectf(effect, AL_EAXREVERB_DECAY_TIME, props.flDecayTime);
            alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, props.flDecayHFRatio);
            alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, props.flDecayLFRatio);
            alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, props.flReflectionsGain);
            alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, props.flReflectionsDelay);
            alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, props.flReflectionsPan);
            alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, props.flLateReverbGain);
            alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, props.flLateReverbDelay);
            alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, props.flLateReverbPan);
            alEffectf(effect, AL_EAXREVERB_ECHO_TIME, props.flEchoTime);
            alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, props.flEchoDepth);
            alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, props.flModulationTime);
            alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, props.flModulationDepth);
            alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, props.flAirAbsorptionGainHF);
            alEffectf(effect, AL_EAXREVERB_HFREFERENCE, props.flHFReference);
            alEffectf(effect, AL_EAXREVERB_LFREFERENCE, props.flLFReference);
            alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, props.flRoomRolloffFactor);
            alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, props.iDecayHFLimit ? AL_TRUE : AL_FALSE);
        }
        else if (type == AL_EFFECT_REVERB)
        {
            alEffectf(effect, AL_REVERB_DIFFUSION, props.flDiffusion);
            alEffectf(effect, AL_REVERB_DENSITY, props.flDensity);
            alEffectf(effect, AL_REVERB_GAIN, props.flGain);
            alEffectf(effect, AL_REVERB_GAINHF, props.flGainHF);
            alEffectf(effect, AL_REVERB_DECAY_TIME, props.flDecayTime);
            alEffectf(effect, AL_REVERB_DECAY_HFRATIO, props.flDecayHFRatio);
            alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, props.flReflectionsGain);
            alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, props.flReflectionsDelay);
            alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, props.flLateReverbGain);
            alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, props.flLateReverbDelay);
            alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, props.flAirAbsorptionGainHF);
            alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, props.flRoomRolloffFactor);
            alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, props.iDecayHFLimit ? AL_TRUE : AL_FALSE);
        }
        getALError();
    }

    std::basic_string_view<ALCchar> getDeviceName(ALCdevice* device)
    {
        const ALCchar* name = nullptr;
        if (alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
            name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
        if (alcGetError(device) != AL_NO_ERROR || !name)
            name = alcGetString(device, ALC_DEVICE_SPECIFIER);
        if (name == nullptr) // Prevent assigning nullptr to std::string
            return {};
        return name;
    }
}

namespace MWSound
{

    static ALenum getALFormat(ChannelConfig chans, SampleType type)
    {
        struct FormatEntry
        {
            ALenum format;
            ChannelConfig chans;
            SampleType type;
        };
        struct FormatEntryExt
        {
            const char name[32];
            ChannelConfig chans;
            SampleType type;
        };
        static const std::array<FormatEntry, 4> fmtlist{ {
            { AL_FORMAT_MONO16, ChannelConfig_Mono, SampleType_Int16 },
            { AL_FORMAT_MONO8, ChannelConfig_Mono, SampleType_UInt8 },
            { AL_FORMAT_STEREO16, ChannelConfig_Stereo, SampleType_Int16 },
            { AL_FORMAT_STEREO8, ChannelConfig_Stereo, SampleType_UInt8 },
        } };

        for (auto& fmt : fmtlist)
        {
            if (fmt.chans == chans && fmt.type == type)
                return fmt.format;
        }

        if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            static const std::array<FormatEntryExt, 6> mcfmtlist{ {
                { "AL_FORMAT_QUAD16", ChannelConfig_Quad, SampleType_Int16 },
                { "AL_FORMAT_QUAD8", ChannelConfig_Quad, SampleType_UInt8 },
                { "AL_FORMAT_51CHN16", ChannelConfig_5point1, SampleType_Int16 },
                { "AL_FORMAT_51CHN8", ChannelConfig_5point1, SampleType_UInt8 },
                { "AL_FORMAT_71CHN16", ChannelConfig_7point1, SampleType_Int16 },
                { "AL_FORMAT_71CHN8", ChannelConfig_7point1, SampleType_UInt8 },
            } };

            for (auto& fmt : mcfmtlist)
            {
                if (fmt.chans == chans && fmt.type == type)
                {
                    ALenum format = alGetEnumValue(fmt.name);
                    if (format != 0 && format != -1)
                        return format;
                }
            }
        }
        if (alIsExtensionPresent("AL_EXT_FLOAT32"))
        {
            static const std::array<FormatEntryExt, 2> fltfmtlist{ {
                { "AL_FORMAT_MONO_FLOAT32", ChannelConfig_Mono, SampleType_Float32 },
                { "AL_FORMAT_STEREO_FLOAT32", ChannelConfig_Stereo, SampleType_Float32 },
            } };

            for (auto& fmt : fltfmtlist)
            {
                if (fmt.chans == chans && fmt.type == type)
                {
                    ALenum format = alGetEnumValue(fmt.name);
                    if (format != 0 && format != -1)
                        return format;
                }
            }

            if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
            {
                static const std::array<FormatEntryExt, 3> fltmcfmtlist{ {
                    { "AL_FORMAT_QUAD32", ChannelConfig_Quad, SampleType_Float32 },
                    { "AL_FORMAT_51CHN32", ChannelConfig_5point1, SampleType_Float32 },
                    { "AL_FORMAT_71CHN32", ChannelConfig_7point1, SampleType_Float32 },
                } };

                for (auto& fmt : fltmcfmtlist)
                {
                    if (fmt.chans == chans && fmt.type == type)
                    {
                        ALenum format = alGetEnumValue(fmt.name);
                        if (format != 0 && format != -1)
                            return format;
                    }
                }
            }
        }

        Log(Debug::Warning) << "Unsupported sound format (" << getChannelConfigName(chans) << ", "
                            << getSampleTypeName(type) << ")";
        return AL_NONE;
    }

    //
    // A streaming OpenAL sound.
    //
    class OpenAL_SoundStream
    {
        static const ALfloat sBufferLength;

    private:
        ALuint mSource;

        std::array<ALuint, 6> mBuffers;
        ALint mCurrentBufIdx;

        ALenum mFormat;
        ALsizei mSampleRate;
        ALuint mBufferSize;
        ALuint mFrameSize;
        ALint mSilence;

        DecoderPtr mDecoder;

        std::unique_ptr<Sound_Loudness> mLoudnessAnalyzer;

        std::atomic<bool> mIsFinished;

        OpenAL_SoundStream(const OpenAL_SoundStream& rhs);
        OpenAL_SoundStream& operator=(const OpenAL_SoundStream& rhs);

        friend class OpenALOutput;

    public:
        OpenAL_SoundStream(ALuint src, DecoderPtr decoder);
        ~OpenAL_SoundStream();

        bool init(bool getLoudnessData = false);

        bool isPlaying();
        double getStreamDelay() const;
        double getStreamOffset() const;

        float getCurrentLoudness() const;

        bool process();
        ALint refillQueue();
    };
    const ALfloat OpenAL_SoundStream::sBufferLength = 0.125f;

    //
    // A background streaming thread (keeps active streams processed)
    //
    struct OpenALOutput::StreamThread
    {
        std::vector<OpenAL_SoundStream*> mStreams;

        std::atomic<bool> mQuitNow;
        std::mutex mMutex;
        std::condition_variable mCondVar;
        std::thread mThread;

        StreamThread()
            : mQuitNow(false)
            , mThread([this] { run(); })
        {
        }
        ~StreamThread()
        {
            mQuitNow = true;
            mMutex.lock();
            mMutex.unlock();
            mCondVar.notify_all();
            mThread.join();
        }

        // thread entry point
        void run()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            while (!mQuitNow)
            {
                auto iter = mStreams.begin();
                while (iter != mStreams.end())
                {
                    if ((*iter)->process() == false)
                        iter = mStreams.erase(iter);
                    else
                        ++iter;
                }

                mCondVar.wait_for(lock, std::chrono::milliseconds(50));
            }
        }

        void add(OpenAL_SoundStream* stream)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (std::find(mStreams.begin(), mStreams.end(), stream) == mStreams.end())
            {
                mStreams.push_back(stream);
                mCondVar.notify_all();
            }
        }

        void remove(OpenAL_SoundStream* stream)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto iter = std::find(mStreams.begin(), mStreams.end(), stream);
            if (iter != mStreams.end())
                mStreams.erase(iter);
        }

        void removeAll()
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStreams.clear();
        }

        StreamThread(const StreamThread& rhs) = delete;
        StreamThread& operator=(const StreamThread& rhs) = delete;
    };

    class OpenALOutput::DefaultDeviceThread
    {
    public:
        std::basic_string<ALCchar> mCurrentName;

    private:
        OpenALOutput& mOutput;

        std::atomic<bool> mQuitNow;
        std::mutex mMutex;
        std::condition_variable mCondVar;
        std::thread mThread;

        DefaultDeviceThread(const DefaultDeviceThread&) = delete;
        DefaultDeviceThread& operator=(const DefaultDeviceThread&) = delete;

        void run()
        {
            Misc::setCurrentThreadIdlePriority();
            std::unique_lock<std::mutex> lock(mMutex);
            while (!mQuitNow)
            {
                {
                    const std::lock_guard<std::mutex> openLock(mOutput.mReopenMutex);
                    std::basic_string_view<ALCchar> defaultName = getDeviceName(nullptr);
                    if (mCurrentName != defaultName)
                    {
                        mCurrentName = defaultName;
                        Log(Debug::Info) << "Default audio device changed to \"" << mCurrentName << "\"";
                        ALCboolean reopened = alcReopenDeviceSOFT(
                            mOutput.mDevice, mCurrentName.data(), mOutput.mContextAttributes.data());
                        if (reopened == AL_FALSE)
                            Log(Debug::Warning) << "Failed to switch to new audio device";
                    }
                }
                mCondVar.wait_for(lock, std::chrono::seconds(2));
            }
        }

    public:
        DefaultDeviceThread(OpenALOutput& output, std::basic_string_view<ALCchar> name = {})
            : mCurrentName(name)
            , mOutput(output)
            , mQuitNow(false)
            , mThread([this] { run(); })
        {
        }

        ~DefaultDeviceThread()
        {
            mQuitNow = true;
            mMutex.lock();
            mMutex.unlock();
            mCondVar.notify_all();
            mThread.join();
        }
    };

    OpenAL_SoundStream::OpenAL_SoundStream(ALuint src, DecoderPtr decoder)
        : mSource(src)
        , mCurrentBufIdx(0)
        , mFormat(AL_NONE)
        , mSampleRate(0)
        , mBufferSize(0)
        , mFrameSize(0)
        , mSilence(0)
        , mDecoder(std::move(decoder))
        , mLoudnessAnalyzer(nullptr)
        , mIsFinished(true)
    {
        mBuffers.fill(0);
    }

    OpenAL_SoundStream::~OpenAL_SoundStream()
    {
        if (mBuffers[0] && alIsBuffer(mBuffers[0]))
            alDeleteBuffers(mBuffers.size(), mBuffers.data());
        alGetError();

        mDecoder->close();
    }

    bool OpenAL_SoundStream::init(bool getLoudnessData)
    {
        alGenBuffers(mBuffers.size(), mBuffers.data());
        ALenum err = getALError();
        if (err != AL_NO_ERROR)
            return false;

        ChannelConfig chans;
        SampleType type;

        try
        {
            mDecoder->getInfo(&mSampleRate, &chans, &type);
            mFormat = getALFormat(chans, type);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Failed to get stream info: " << e.what();
            return false;
        }

        switch (type)
        {
            case SampleType_UInt8:
                mSilence = 0x80;
                break;
            case SampleType_Int16:
                mSilence = 0x00;
                break;
            case SampleType_Float32:
                mSilence = 0x00;
                break;
        }

        mFrameSize = framesToBytes(1, chans, type);
        mBufferSize = static_cast<ALuint>(sBufferLength * mSampleRate);
        mBufferSize *= mFrameSize;

        if (getLoudnessData)
            mLoudnessAnalyzer = std::make_unique<Sound_Loudness>(sLoudnessFPS, mSampleRate, chans, type);

        mIsFinished = false;
        return true;
    }

    bool OpenAL_SoundStream::isPlaying()
    {
        ALint state;

        alGetSourcei(mSource, AL_SOURCE_STATE, &state);
        getALError();

        if (state == AL_PLAYING || state == AL_PAUSED)
            return true;
        return !mIsFinished;
    }

    double OpenAL_SoundStream::getStreamDelay() const
    {
        ALint state = AL_STOPPED;
        double d = 0.0;
        ALint offset;

        alGetSourcei(mSource, AL_SAMPLE_OFFSET, &offset);
        alGetSourcei(mSource, AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING || state == AL_PAUSED)
        {
            ALint queued;
            alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
            ALint inqueue = mBufferSize / mFrameSize * queued - offset;
            d = (double)inqueue / (double)mSampleRate;
        }

        getALError();
        return d;
    }

    double OpenAL_SoundStream::getStreamOffset() const
    {
        ALint state = AL_STOPPED;
        ALint offset;
        double t;

        alGetSourcei(mSource, AL_SAMPLE_OFFSET, &offset);
        alGetSourcei(mSource, AL_SOURCE_STATE, &state);
        if (state == AL_PLAYING || state == AL_PAUSED)
        {
            ALint queued;
            alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
            ALint inqueue = mBufferSize / mFrameSize * queued - offset;
            t = (double)(mDecoder->getSampleOffset() - inqueue) / (double)mSampleRate;
        }
        else
        {
            /* Underrun, or not started yet. The decoder offset is where we'll play
             * next. */
            t = (double)mDecoder->getSampleOffset() / (double)mSampleRate;
        }

        getALError();
        return t;
    }

    float OpenAL_SoundStream::getCurrentLoudness() const
    {
        if (!mLoudnessAnalyzer.get())
            return 0.f;

        float time = getStreamOffset();
        return mLoudnessAnalyzer->getLoudnessAtTime(time);
    }

    bool OpenAL_SoundStream::process()
    {
        try
        {
            if (refillQueue() > 0)
            {
                ALint state;
                alGetSourcei(mSource, AL_SOURCE_STATE, &state);
                if (state != AL_PLAYING && state != AL_PAUSED)
                {
                    // Ensure all processed buffers are removed so we don't replay them.
                    refillQueue();

                    alSourcePlay(mSource);
                }
            }
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Error updating stream \"" << mDecoder->getName() << "\": " << e.what();
            mIsFinished = true;
        }
        return !mIsFinished;
    }

    ALint OpenAL_SoundStream::refillQueue()
    {
        ALint processed;
        alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
        while (processed > 0)
        {
            ALuint buf;
            alSourceUnqueueBuffers(mSource, 1, &buf);
            --processed;
        }

        ALint queued;
        alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
        if (!mIsFinished && (ALuint)queued < mBuffers.size())
        {
            std::vector<char> data(mBufferSize);
            for (; !mIsFinished && (ALuint)queued < mBuffers.size(); ++queued)
            {
                size_t got = mDecoder->read(data.data(), data.size());
                if (got < data.size())
                {
                    mIsFinished = true;
                    std::fill(data.begin() + got, data.end(), mSilence);
                }
                if (got > 0)
                {
                    if (mLoudnessAnalyzer.get())
                        mLoudnessAnalyzer->analyzeLoudness(data);

                    ALuint bufid = mBuffers[mCurrentBufIdx];
                    alBufferData(bufid, mFormat, data.data(), data.size(), mSampleRate);
                    alSourceQueueBuffers(mSource, 1, &bufid);
                    mCurrentBufIdx = (mCurrentBufIdx + 1) % mBuffers.size();
                }
            }
        }

        return queued;
    }

    //
    // An OpenAL output device
    //
    std::vector<std::string> OpenALOutput::enumerate()
    {
        std::vector<std::string> devlist;
        const ALCchar* devnames;

        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
            devnames = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        else
            devnames = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        while (devnames && *devnames)
        {
            devlist.emplace_back(devnames);
            devnames += strlen(devnames) + 1;
        }
        return devlist;
    }

    void OpenALOutput::eventCallback(
        ALenum eventType, ALuint object, ALuint param, ALsizei length, const ALchar* message, void* userParam)
    {
        if (eventType == AL_EVENT_TYPE_DISCONNECTED_SOFT)
            static_cast<OpenALOutput*>(userParam)->onDisconnect();
    }

    void OpenALOutput::onDisconnect()
    {
        if (!mInitialized || !alcReopenDeviceSOFT)
            return;
        const std::lock_guard<std::mutex> lock(mReopenMutex);
        Log(Debug::Warning) << "Audio device disconnected, attempting to reopen...";
        ALCboolean reopened = alcReopenDeviceSOFT(mDevice, mDeviceName.c_str(), mContextAttributes.data());
        if (reopened == AL_FALSE && !mDeviceName.empty())
        {
            reopened = alcReopenDeviceSOFT(mDevice, nullptr, mContextAttributes.data());
            if (reopened == AL_TRUE && !mDefaultDeviceThread)
                mDefaultDeviceThread = std::make_unique<DefaultDeviceThread>(*this);
        }
        if (reopened == AL_FALSE)
            Log(Debug::Error) << "Failed to reopen audio device";
        else
        {
            Log(Debug::Info) << "Reopened audio device";
            if (mDefaultDeviceThread)
                mDefaultDeviceThread->mCurrentName = getDeviceName(mDevice);
        }
    }

    bool OpenALOutput::init(const std::string& devname, const std::string& hrtfname, HrtfMode hrtfmode)
    {
        deinit();
        std::lock_guard<std::mutex> lock(mReopenMutex);

        Log(Debug::Info) << "Initializing OpenAL...";

        mDeviceName = devname;
        mDevice = alcOpenDevice(devname.c_str());
        if (!mDevice && !devname.empty())
        {
            Log(Debug::Warning) << "Failed to open \"" << devname << "\", trying default";
            mDevice = alcOpenDevice(nullptr);
            mDeviceName.clear();
        }

        if (!mDevice)
        {
            Log(Debug::Error) << "Failed to open default audio device";
            return false;
        }

        auto name = getDeviceName(mDevice);
        Log(Debug::Info) << "Opened \"" << name << "\"";

        ALCint major = 0, minor = 0;
        alcGetIntegerv(mDevice, ALC_MAJOR_VERSION, 1, &major);
        alcGetIntegerv(mDevice, ALC_MINOR_VERSION, 1, &minor);
        Log(Debug::Info) << "  ALC Version: " << major << "." << minor << "\n"
                         << "  ALC Extensions: " << alcGetString(mDevice, ALC_EXTENSIONS);

        ALC.EXT_EFX = alcIsExtensionPresent(mDevice, "ALC_EXT_EFX");
        ALC.SOFT_HRTF = alcIsExtensionPresent(mDevice, "ALC_SOFT_HRTF");

        mContextAttributes.clear();
        mContextAttributes.reserve(15);
        if (ALC.SOFT_HRTF)
        {
            LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
            getALCFunc(alcGetStringiSOFT, mDevice, "alcGetStringiSOFT");

            mContextAttributes.push_back(ALC_HRTF_SOFT);
            mContextAttributes.push_back(hrtfmode == HrtfMode::Disable ? ALC_FALSE
                    : hrtfmode == HrtfMode::Enable                     ? ALC_TRUE
                                                                       :
                                                   /*hrtfmode == HrtfMode::Auto ?*/ ALC_DONT_CARE_SOFT);
            if (!hrtfname.empty())
            {
                ALCint index = -1;
                ALCint numHrtf;
                alcGetIntegerv(mDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &numHrtf);
                for (ALCint i = 0; i < numHrtf; ++i)
                {
                    const ALCchar* entry = alcGetStringiSOFT(mDevice, ALC_HRTF_SPECIFIER_SOFT, i);
                    if (hrtfname == entry)
                    {
                        index = i;
                        break;
                    }
                }

                if (index < 0)
                    Log(Debug::Warning) << "Failed to find HRTF \"" << hrtfname << "\", using default";
                else
                {
                    mContextAttributes.push_back(ALC_HRTF_ID_SOFT);
                    mContextAttributes.push_back(index);
                }
            }
        }
        mContextAttributes.push_back(0);

        mContext = alcCreateContext(mDevice, mContextAttributes.data());
        if (!mContext || alcMakeContextCurrent(mContext) == ALC_FALSE)
        {
            Log(Debug::Error) << "Failed to setup audio context: " << alcGetString(mDevice, alcGetError(mDevice));
            if (mContext)
                alcDestroyContext(mContext);
            mContext = nullptr;
            alcCloseDevice(mDevice);
            mDevice = nullptr;
            return false;
        }

        Log(Debug::Info) << "  Vendor: " << alGetString(AL_VENDOR) << "\n"
                         << "  Renderer: " << alGetString(AL_RENDERER) << "\n"
                         << "  Version: " << alGetString(AL_VERSION) << "\n"
                         << "  Extensions: " << alGetString(AL_EXTENSIONS);

        if (alIsExtensionPresent("AL_SOFT_events"))
        {
            getALFunc(alEventControlSOFT, "alEventControlSOFT");
            getALFunc(alEventCallbackSOFT, "alEventCallbackSOFT");
        }
        if (alcIsExtensionPresent(mDevice, "ALC_SOFT_reopen_device"))
            getALFunc(alcReopenDeviceSOFT, "alcReopenDeviceSOFT");
        if (alEventControlSOFT)
        {
            static const std::array<ALenum, 1> events{ { AL_EVENT_TYPE_DISCONNECTED_SOFT } };
            alEventControlSOFT(events.size(), events.data(), AL_TRUE);
            alEventCallbackSOFT(&OpenALOutput::eventCallback, this);
        }
        else
            Log(Debug::Warning) << "Cannot detect audio device changes";
        if (mDeviceName.empty() && !name.empty())
        {
            // If we opened the default device, switch devices if a new default is selected
            if (alcReopenDeviceSOFT)
                mDefaultDeviceThread = std::make_unique<DefaultDeviceThread>(*this, name);
            else
                Log(Debug::Warning) << "Cannot switch audio devices if the default changes";
        }

        if (!ALC.SOFT_HRTF)
            Log(Debug::Warning) << "HRTF status unavailable";
        else
        {
            ALCint hrtfState;
            alcGetIntegerv(mDevice, ALC_HRTF_SOFT, 1, &hrtfState);
            if (!hrtfState)
                Log(Debug::Info) << "HRTF disabled";
            else
            {
                const ALCchar* hrtf = alcGetString(mDevice, ALC_HRTF_SPECIFIER_SOFT);
                Log(Debug::Info) << "Enabled HRTF " << hrtf;
            }
        }

        AL.SOFT_source_spatialize = alIsExtensionPresent("AL_SOFT_source_spatialize");

        ALCuint maxtotal;
        ALCint maxmono = 0, maxstereo = 0;
        alcGetIntegerv(mDevice, ALC_MONO_SOURCES, 1, &maxmono);
        alcGetIntegerv(mDevice, ALC_STEREO_SOURCES, 1, &maxstereo);
        if (getALCError(mDevice) != ALC_NO_ERROR)
            maxtotal = 256;
        else
        {
            maxtotal = std::min<ALCuint>(maxmono + maxstereo, 256);
            if (maxtotal == 0) // workaround for broken implementations
                maxtotal = 256;
        }
        for (size_t i = 0; i < maxtotal; i++)
        {
            ALuint src = 0;
            alGenSources(1, &src);
            if (alGetError() != AL_NO_ERROR)
                break;
            mFreeSources.push_back(src);
        }
        if (mFreeSources.empty())
        {
            Log(Debug::Warning) << "Could not allocate any sound sourcess";
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(mContext);
            mContext = nullptr;
            alcCloseDevice(mDevice);
            mDevice = nullptr;
            return false;
        }
        Log(Debug::Info) << "Allocated " << mFreeSources.size() << " sound sources";

        if (ALC.EXT_EFX)
        {
#define LOAD_FUNC(x) getALFunc(x, #x)
            LOAD_FUNC(alGenEffects);
            LOAD_FUNC(alDeleteEffects);
            LOAD_FUNC(alIsEffect);
            LOAD_FUNC(alEffecti);
            LOAD_FUNC(alEffectiv);
            LOAD_FUNC(alEffectf);
            LOAD_FUNC(alEffectfv);
            LOAD_FUNC(alGetEffecti);
            LOAD_FUNC(alGetEffectiv);
            LOAD_FUNC(alGetEffectf);
            LOAD_FUNC(alGetEffectfv);
            LOAD_FUNC(alGenFilters);
            LOAD_FUNC(alDeleteFilters);
            LOAD_FUNC(alIsFilter);
            LOAD_FUNC(alFilteri);
            LOAD_FUNC(alFilteriv);
            LOAD_FUNC(alFilterf);
            LOAD_FUNC(alFilterfv);
            LOAD_FUNC(alGetFilteri);
            LOAD_FUNC(alGetFilteriv);
            LOAD_FUNC(alGetFilterf);
            LOAD_FUNC(alGetFilterfv);
            LOAD_FUNC(alGenAuxiliaryEffectSlots);
            LOAD_FUNC(alDeleteAuxiliaryEffectSlots);
            LOAD_FUNC(alIsAuxiliaryEffectSlot);
            LOAD_FUNC(alAuxiliaryEffectSloti);
            LOAD_FUNC(alAuxiliaryEffectSlotiv);
            LOAD_FUNC(alAuxiliaryEffectSlotf);
            LOAD_FUNC(alAuxiliaryEffectSlotfv);
            LOAD_FUNC(alGetAuxiliaryEffectSloti);
            LOAD_FUNC(alGetAuxiliaryEffectSlotiv);
            LOAD_FUNC(alGetAuxiliaryEffectSlotf);
            LOAD_FUNC(alGetAuxiliaryEffectSlotfv);
#undef LOAD_FUNC
            if (getALError() != AL_NO_ERROR)
            {
                ALC.EXT_EFX = false;
                goto skip_efx;
            }

            alGenFilters(1, &mWaterFilter);
            if (alGetError() == AL_NO_ERROR)
            {
                alFilteri(mWaterFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
                if (alGetError() == AL_NO_ERROR)
                {
                    Log(Debug::Info) << "Low-pass filter supported";
                    alFilterf(mWaterFilter, AL_LOWPASS_GAIN, 0.9f);
                    alFilterf(mWaterFilter, AL_LOWPASS_GAINHF, 0.125f);
                }
                else
                {
                    alDeleteFilters(1, &mWaterFilter);
                    mWaterFilter = 0;
                }
                alGetError();
            }

            alGenAuxiliaryEffectSlots(1, &mEffectSlot);
            alGetError();

            alGenEffects(1, &mDefaultEffect);
            if (alGetError() == AL_NO_ERROR)
            {
                alEffecti(mDefaultEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
                if (alGetError() == AL_NO_ERROR)
                    Log(Debug::Info) << "EAX Reverb supported";
                else
                {
                    alEffecti(mDefaultEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
                    if (alGetError() == AL_NO_ERROR)
                        Log(Debug::Info) << "Standard Reverb supported";
                }
                EFXEAXREVERBPROPERTIES props = EFX_REVERB_PRESET_LIVINGROOM;
                props.flGain = 0.0f;
                LoadEffect(mDefaultEffect, props);
            }

            alGenEffects(1, &mWaterEffect);
            if (alGetError() == AL_NO_ERROR)
            {
                alEffecti(mWaterEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
                if (alGetError() != AL_NO_ERROR)
                {
                    alEffecti(mWaterEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
                    alGetError();
                }
                LoadEffect(mWaterEffect, EFX_REVERB_PRESET_UNDERWATER);
            }

            alListenerf(AL_METERS_PER_UNIT, 1.0f / Constants::UnitsPerMeter);
        }
    skip_efx:
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
        // Speed of sound is in units per second. Take the sound speed in air (assumed
        // meters per second), multiply by the units per meter to get the speed in u/s.
        alSpeedOfSound(Constants::SoundSpeedInAir * Constants::UnitsPerMeter);
        alDopplerFactor(Settings::sound().mDopplerFactor);
        alGetError();

        mInitialized = true;
        return true;
    }

    void OpenALOutput::deinit()
    {
        mStreamThread->removeAll();
        mDefaultDeviceThread.reset();

        for (ALuint source : mFreeSources)
            alDeleteSources(1, &source);
        mFreeSources.clear();

        if (mEffectSlot)
            alDeleteAuxiliaryEffectSlots(1, &mEffectSlot);
        mEffectSlot = 0;
        if (mDefaultEffect)
            alDeleteEffects(1, &mDefaultEffect);
        mDefaultEffect = 0;
        if (mWaterEffect)
            alDeleteEffects(1, &mWaterEffect);
        mWaterEffect = 0;
        if (mWaterFilter)
            alDeleteFilters(1, &mWaterFilter);
        mWaterFilter = 0;

        if (alEventCallbackSOFT)
            alEventCallbackSOFT(nullptr, nullptr);

        alcMakeContextCurrent(nullptr);
        if (mContext)
            alcDestroyContext(mContext);
        mContext = nullptr;
        if (mDevice)
            alcCloseDevice(mDevice);
        mDevice = nullptr;

        mInitialized = false;
    }

    std::vector<std::string> OpenALOutput::enumerateHrtf()
    {
        std::vector<std::string> ret;

        if (!mDevice || !ALC.SOFT_HRTF)
            return ret;

        LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
        getALCFunc(alcGetStringiSOFT, mDevice, "alcGetStringiSOFT");

        ALCint numHrtf;
        alcGetIntegerv(mDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &numHrtf);
        ret.reserve(numHrtf);
        for (ALCint i = 0; i < numHrtf; ++i)
        {
            const ALCchar* entry = alcGetStringiSOFT(mDevice, ALC_HRTF_SPECIFIER_SOFT, i);
            ret.emplace_back(entry);
        }

        return ret;
    }

    std::pair<Sound_Handle, size_t> OpenALOutput::loadSound(VFS::Path::NormalizedView fname)
    {
        getALError();

        std::vector<char> data;
        ALenum format = AL_NONE;
        int srate = 0;

        try
        {
            DecoderPtr decoder = mManager.getDecoder();
            decoder->open(Misc::ResourceHelpers::correctSoundPath(fname, *decoder->mResourceMgr));

            ChannelConfig chans;
            SampleType type;
            decoder->getInfo(&srate, &chans, &type);
            format = getALFormat(chans, type);
            if (format)
                decoder->readAll(data);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Failed to load audio from " << fname << ": " << e.what();
        }

        if (data.empty())
        {
            // If we failed to get any usable audio, substitute with silence.
            format = AL_FORMAT_MONO8;
            srate = 8000;
            data.assign(8000, -128);
        }

        ALint size;
        ALuint buf = 0;
        alGenBuffers(1, &buf);
        alBufferData(buf, format, data.data(), data.size(), srate);
        alGetBufferi(buf, AL_SIZE, &size);
        if (getALError() != AL_NO_ERROR)
        {
            if (buf && alIsBuffer(buf))
                alDeleteBuffers(1, &buf);
            getALError();
            return std::make_pair(nullptr, 0);
        }
        return std::make_pair(MAKE_PTRID(buf), size);
    }

    size_t OpenALOutput::unloadSound(Sound_Handle data)
    {
        ALuint buffer = GET_PTRID(data);
        if (!buffer)
            return 0;

        // Make sure no sources are playing this buffer before unloading it.
        SoundVec::const_iterator iter = mActiveSounds.begin();
        for (; iter != mActiveSounds.end(); ++iter)
        {
            if (!(*iter)->mHandle)
                continue;

            ALuint source = GET_PTRID((*iter)->mHandle);
            ALint srcbuf;
            alGetSourcei(source, AL_BUFFER, &srcbuf);
            if ((ALuint)srcbuf == buffer)
            {
                alSourceStop(source);
                alSourcei(source, AL_BUFFER, 0);
            }
        }
        ALint size = 0;
        alGetBufferi(buffer, AL_SIZE, &size);
        alDeleteBuffers(1, &buffer);
        getALError();
        return size;
    }

    void OpenALOutput::initCommon2D(
        ALuint source, const osg::Vec3f& pos, ALfloat gain, ALfloat pitch, bool loop, bool useenv)
    {
        alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(source, AL_MAX_DISTANCE, 1000.0f);
        alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        if (AL.SOFT_source_spatialize)
            alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE);

        if (useenv)
        {
            if (mWaterFilter)
                alSourcei(source, AL_DIRECT_FILTER, (mListenerEnv == Env_Underwater) ? mWaterFilter : AL_FILTER_NULL);
            else if (mListenerEnv == Env_Underwater)
            {
                gain *= 0.9f;
                pitch *= 0.7f;
            }
            if (mEffectSlot)
                alSource3i(source, AL_AUXILIARY_SEND_FILTER, mEffectSlot, 0, AL_FILTER_NULL);
        }
        else
        {
            if (mWaterFilter)
                alSourcei(source, AL_DIRECT_FILTER, AL_FILTER_NULL);
            if (mEffectSlot)
                alSource3i(source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
        }

        alSourcef(source, AL_GAIN, gain);
        alSourcef(source, AL_PITCH, pitch);
        alSourcefv(source, AL_POSITION, pos.ptr());
        alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
        alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    }

    void OpenALOutput::initCommon3D(ALuint source, const osg::Vec3f& pos, const osg::Vec3f& vel, ALfloat mindist,
        ALfloat maxdist, ALfloat gain, ALfloat pitch, bool loop, bool useenv)
    {
        alSourcef(source, AL_REFERENCE_DISTANCE, mindist);
        alSourcef(source, AL_MAX_DISTANCE, maxdist);
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        if (AL.SOFT_source_spatialize)
            alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);

        if ((pos - mListenerPos).length2() > maxdist * maxdist)
            gain = 0.0f;
        if (useenv)
        {
            if (mWaterFilter)
                alSourcei(source, AL_DIRECT_FILTER, (mListenerEnv == Env_Underwater) ? mWaterFilter : AL_FILTER_NULL);
            else if (mListenerEnv == Env_Underwater)
            {
                gain *= 0.9f;
                pitch *= 0.7f;
            }
            if (mEffectSlot)
                alSource3i(source, AL_AUXILIARY_SEND_FILTER, mEffectSlot, 0, AL_FILTER_NULL);
        }
        else
        {
            if (mWaterFilter)
                alSourcei(source, AL_DIRECT_FILTER, AL_FILTER_NULL);
            if (mEffectSlot)
                alSource3i(source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
        }

        alSourcef(source, AL_GAIN, gain);
        alSourcef(source, AL_PITCH, pitch);
        alSourcefv(source, AL_POSITION, pos.ptr());
        alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
        alSourcefv(source, AL_VELOCITY, vel.ptr());
    }

    void OpenALOutput::updateCommon(ALuint source, const osg::Vec3f& pos, const osg::Vec3f& vel, ALfloat maxdist,
        ALfloat gain, ALfloat pitch, bool useenv)
    {
        if (useenv && mListenerEnv == Env_Underwater && !mWaterFilter)
        {
            gain *= 0.9f;
            pitch *= 0.7f;
        }

        alSourcef(source, AL_GAIN, gain);
        alSourcef(source, AL_PITCH, pitch);
        alSourcefv(source, AL_POSITION, pos.ptr());
        alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
        alSourcefv(source, AL_VELOCITY, vel.ptr());
    }

    bool OpenALOutput::playSound(Sound* sound, Sound_Handle data, float offset)
    {
        ALuint source;

        if (mFreeSources.empty())
        {
            Log(Debug::Warning) << "No free sources!";
            return false;
        }
        source = mFreeSources.front();

        initCommon2D(source, sound->getPosition(), sound->getRealVolume(), getTimeScaledPitch(sound),
            sound->getIsLooping(), sound->getUseEnv());
        alSourcei(source, AL_BUFFER, GET_PTRID(data));
        alSourcef(source, AL_SEC_OFFSET, offset);
        if (getALError() != AL_NO_ERROR)
        {
            alSourceRewind(source);
            alSourcei(source, AL_BUFFER, 0);
            alGetError();
            return false;
        }

        alSourcePlay(source);
        if (getALError() != AL_NO_ERROR)
        {
            alSourceRewind(source);
            alSourcei(source, AL_BUFFER, 0);
            alGetError();
            return false;
        }

        mFreeSources.pop_front();
        sound->mHandle = MAKE_PTRID(source);
        mActiveSounds.push_back(sound);

        return true;
    }

    bool OpenALOutput::playSound3D(Sound* sound, Sound_Handle data, float offset)
    {
        ALuint source;

        if (mFreeSources.empty())
        {
            Log(Debug::Warning) << "No free sources!";
            return false;
        }
        source = mFreeSources.front();

        initCommon3D(source, sound->getPosition(), sound->getVelocity(), sound->getMinDistance(),
            sound->getMaxDistance(), sound->getRealVolume(), getTimeScaledPitch(sound), sound->getIsLooping(),
            sound->getUseEnv());
        alSourcei(source, AL_BUFFER, GET_PTRID(data));
        alSourcef(source, AL_SEC_OFFSET, offset);
        if (getALError() != AL_NO_ERROR)
        {
            alSourceRewind(source);
            alSourcei(source, AL_BUFFER, 0);
            alGetError();
            return false;
        }

        alSourcePlay(source);
        if (getALError() != AL_NO_ERROR)
        {
            alSourceRewind(source);
            alSourcei(source, AL_BUFFER, 0);
            alGetError();
            return false;
        }

        mFreeSources.pop_front();
        sound->mHandle = MAKE_PTRID(source);
        mActiveSounds.push_back(sound);

        return true;
    }

    void OpenALOutput::finishSound(Sound* sound)
    {
        if (!sound->mHandle)
            return;
        ALuint source = GET_PTRID(sound->mHandle);
        sound->mHandle = nullptr;

        // Rewind the stream to put the source back into an AL_INITIAL state, for
        // the next time it's used.
        alSourceRewind(source);
        alSourcei(source, AL_BUFFER, 0);
        getALError();

        mFreeSources.push_back(source);
        mActiveSounds.erase(std::find(mActiveSounds.begin(), mActiveSounds.end(), sound));
    }

    bool OpenALOutput::isSoundPlaying(Sound* sound)
    {
        if (!sound->mHandle)
            return false;
        ALuint source = GET_PTRID(sound->mHandle);
        ALint state = AL_STOPPED;

        alGetSourcei(source, AL_SOURCE_STATE, &state);
        getALError();

        return state == AL_PLAYING || state == AL_PAUSED;
    }

    void OpenALOutput::updateSound(Sound* sound)
    {
        if (!sound->mHandle)
            return;
        ALuint source = GET_PTRID(sound->mHandle);

        updateCommon(source, sound->getPosition(), sound->getVelocity(), sound->getMaxDistance(),
            sound->getRealVolume(), getTimeScaledPitch(sound), sound->getUseEnv());
        getALError();
    }

    bool OpenALOutput::streamSound(DecoderPtr decoder, Stream* sound, bool getLoudnessData)
    {
        if (mFreeSources.empty())
        {
            Log(Debug::Warning) << "No free sources!";
            return false;
        }
        ALuint source = mFreeSources.front();

        if (sound->getIsLooping())
            Log(Debug::Warning) << "Warning: cannot loop stream \"" << decoder->getName() << "\"";

        initCommon2D(
            source, sound->getPosition(), sound->getRealVolume(), getTimeScaledPitch(sound), false, sound->getUseEnv());
        if (getALError() != AL_NO_ERROR)
            return false;

        OpenAL_SoundStream* stream = new OpenAL_SoundStream(source, std::move(decoder));
        if (!stream->init(getLoudnessData))
        {
            delete stream;
            return false;
        }
        mStreamThread->add(stream);

        mFreeSources.pop_front();
        sound->mHandle = stream;
        mActiveStreams.push_back(sound);
        return true;
    }

    bool OpenALOutput::streamSound3D(DecoderPtr decoder, Stream* sound, bool getLoudnessData)
    {
        if (mFreeSources.empty())
        {
            Log(Debug::Warning) << "No free sources!";
            return false;
        }
        ALuint source = mFreeSources.front();

        if (sound->getIsLooping())
            Log(Debug::Warning) << "Warning: cannot loop stream \"" << decoder->getName() << "\"";

        initCommon3D(source, sound->getPosition(), sound->getVelocity(), sound->getMinDistance(),
            sound->getMaxDistance(), sound->getRealVolume(), getTimeScaledPitch(sound), false, sound->getUseEnv());
        if (getALError() != AL_NO_ERROR)
            return false;

        OpenAL_SoundStream* stream = new OpenAL_SoundStream(source, std::move(decoder));
        if (!stream->init(getLoudnessData))
        {
            delete stream;
            return false;
        }
        mStreamThread->add(stream);

        mFreeSources.pop_front();
        sound->mHandle = stream;
        mActiveStreams.push_back(sound);
        return true;
    }

    void OpenALOutput::finishStream(Stream* sound)
    {
        if (!sound->mHandle)
            return;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        ALuint source = stream->mSource;

        sound->mHandle = nullptr;
        mStreamThread->remove(stream);

        // Rewind the stream to put the source back into an AL_INITIAL state, for
        // the next time it's used.
        alSourceRewind(source);
        alSourcei(source, AL_BUFFER, 0);
        getALError();

        mFreeSources.push_back(source);
        mActiveStreams.erase(std::find(mActiveStreams.begin(), mActiveStreams.end(), sound));

        delete stream;
    }

    double OpenALOutput::getStreamDelay(Stream* sound)
    {
        if (!sound->mHandle)
            return 0.0;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        return stream->getStreamDelay();
    }

    double OpenALOutput::getStreamOffset(Stream* sound)
    {
        if (!sound->mHandle)
            return 0.0;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        std::lock_guard<std::mutex> lock(mStreamThread->mMutex);
        return stream->getStreamOffset();
    }

    float OpenALOutput::getStreamLoudness(Stream* sound)
    {
        if (!sound->mHandle)
            return 0.0;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        std::lock_guard<std::mutex> lock(mStreamThread->mMutex);
        return stream->getCurrentLoudness();
    }

    bool OpenALOutput::isStreamPlaying(Stream* sound)
    {
        if (!sound->mHandle)
            return false;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        std::lock_guard<std::mutex> lock(mStreamThread->mMutex);
        return stream->isPlaying();
    }

    void OpenALOutput::updateStream(Stream* sound)
    {
        if (!sound->mHandle)
            return;
        OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
        ALuint source = stream->mSource;

        updateCommon(source, sound->getPosition(), sound->getVelocity(), sound->getMaxDistance(),
            sound->getRealVolume(), getTimeScaledPitch(sound), sound->getUseEnv());
        getALError();
    }

    void OpenALOutput::startUpdate()
    {
        alcSuspendContext(alcGetCurrentContext());
    }

    void OpenALOutput::finishUpdate()
    {
        alcProcessContext(alcGetCurrentContext());
    }

    void OpenALOutput::updateListener(
        const osg::Vec3f& pos, const osg::Vec3f& atdir, const osg::Vec3f& updir, const osg::Vec3f& vel, Environment env)
    {
        if (mContext)
        {
            ALfloat orient[6] = { atdir.x(), atdir.y(), atdir.z(), updir.x(), updir.y(), updir.z() };
            alListenerfv(AL_POSITION, pos.ptr());
            alListenerfv(AL_VELOCITY, vel.ptr());
            alListenerfv(AL_ORIENTATION, orient);

            if (env != mListenerEnv)
            {
                alSpeedOfSound(((env == Env_Underwater) ? Constants::SoundSpeedUnderwater : Constants::SoundSpeedInAir)
                    * Constants::UnitsPerMeter);

                // Update active sources with the environment's direct filter
                if (mWaterFilter)
                {
                    ALuint filter = (env == Env_Underwater) ? mWaterFilter : AL_FILTER_NULL;
                    for (Sound* sound : mActiveSounds)
                    {
                        if (sound->getUseEnv())
                            alSourcei(GET_PTRID(sound->mHandle), AL_DIRECT_FILTER, filter);
                    }
                    for (Stream* sound : mActiveStreams)
                    {
                        if (sound->getUseEnv())
                            alSourcei(reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle)->mSource, AL_DIRECT_FILTER,
                                filter);
                    }
                }
                // Update the environment effect
                if (mEffectSlot)
                    alAuxiliaryEffectSloti(
                        mEffectSlot, AL_EFFECTSLOT_EFFECT, (env == Env_Underwater) ? mWaterEffect : mDefaultEffect);
            }
            getALError();
        }

        mListenerPos = pos;
        mListenerVel = vel;
        mListenerEnv = env;
    }

    void OpenALOutput::pauseSounds(int types)
    {
        std::vector<ALuint> sources;
        for (Sound* sound : mActiveSounds)
        {
            if ((types & sound->getPlayType()))
                sources.push_back(GET_PTRID(sound->mHandle));
        }
        for (Stream* sound : mActiveStreams)
        {
            if ((types & sound->getPlayType()))
            {
                OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
                sources.push_back(stream->mSource);
            }
        }
        if (!sources.empty())
        {
            alSourcePausev(sources.size(), sources.data());
            getALError();
        }
    }

    void OpenALOutput::pauseActiveDevice()
    {
        if (mDevice == nullptr)
            return;

        if (alcIsExtensionPresent(mDevice, "ALC_SOFT_PAUSE_DEVICE"))
        {
            LPALCDEVICEPAUSESOFT alcDevicePauseSOFT = nullptr;
            getALCFunc(alcDevicePauseSOFT, mDevice, "alcDevicePauseSOFT");
            alcDevicePauseSOFT(mDevice);
            getALCError(mDevice);
        }

        alListenerf(AL_GAIN, 0.0f);
    }

    void OpenALOutput::resumeActiveDevice()
    {
        if (mDevice == nullptr)
            return;

        if (alcIsExtensionPresent(mDevice, "ALC_SOFT_PAUSE_DEVICE"))
        {
            LPALCDEVICERESUMESOFT alcDeviceResumeSOFT = nullptr;
            getALCFunc(alcDeviceResumeSOFT, mDevice, "alcDeviceResumeSOFT");
            alcDeviceResumeSOFT(mDevice);
            getALCError(mDevice);
        }

        alListenerf(AL_GAIN, 1.0f);
    }

    void OpenALOutput::resumeSounds(int types)
    {
        std::vector<ALuint> sources;
        for (Sound* sound : mActiveSounds)
        {
            if ((types & sound->getPlayType()))
                sources.push_back(GET_PTRID(sound->mHandle));
        }
        for (Stream* sound : mActiveStreams)
        {
            if ((types & sound->getPlayType()))
            {
                OpenAL_SoundStream* stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
                sources.push_back(stream->mSource);
            }
        }
        if (!sources.empty())
        {
            alSourcePlayv(sources.size(), sources.data());
            getALError();
        }
    }

    OpenALOutput::OpenALOutput(SoundManager& mgr)
        : SoundOutput(mgr)
        , mDevice(nullptr)
        , mContext(nullptr)
        , mListenerEnv(Env_Normal)
        , mWaterFilter(0)
        , mWaterEffect(0)
        , mDefaultEffect(0)
        , mEffectSlot(0)
        , mStreamThread(std::make_unique<StreamThread>())
    {
    }

    OpenALOutput::~OpenALOutput()
    {
        OpenALOutput::deinit();
    }

    float OpenALOutput::getTimeScaledPitch(SoundBase* sound)
    {
        const bool shouldScale = !(sound->mParams.mFlags & PlayMode::NoScaling);
        return shouldScale ? sound->getPitch() * mManager.getSimulationTimeScale() : sound->getPitch();
    }

}
