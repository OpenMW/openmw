#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <memory>

#include <stdint.h>

#include <components/vfs/manager.hpp>

#include <boost/thread.hpp>

#include "openal_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"
#include "soundmanagerimp.hpp"
#include "loudness.hpp"

#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
#endif

#ifndef ALC_SOFT_HRTF
#define ALC_SOFT_HRTF 1
#define ALC_HRTF_SOFT                            0x1992
#define ALC_DONT_CARE_SOFT                       0x0002
#define ALC_HRTF_STATUS_SOFT                     0x1993
#define ALC_HRTF_DISABLED_SOFT                   0x0000
#define ALC_HRTF_ENABLED_SOFT                    0x0001
#define ALC_HRTF_DENIED_SOFT                     0x0002
#define ALC_HRTF_REQUIRED_SOFT                   0x0003
#define ALC_HRTF_HEADPHONES_DETECTED_SOFT        0x0004
#define ALC_HRTF_UNSUPPORTED_FORMAT_SOFT         0x0005
#define ALC_NUM_HRTF_SPECIFIERS_SOFT             0x1994
#define ALC_HRTF_SPECIFIER_SOFT                  0x1995
#define ALC_HRTF_ID_SOFT                         0x1996
typedef const ALCchar* (ALC_APIENTRY*LPALCGETSTRINGISOFT)(ALCdevice *device, ALCenum paramName, ALCsizei index);
typedef ALCboolean (ALC_APIENTRY*LPALCRESETDEVICESOFT)(ALCdevice *device, const ALCint *attribs);
#ifdef AL_ALEXT_PROTOTYPES
ALC_API const ALCchar* ALC_APIENTRY alcGetStringiSOFT(ALCdevice *device, ALCenum paramName, ALCsizei index);
ALC_API ALCboolean ALC_APIENTRY alcResetDeviceSOFT(ALCdevice *device, const ALCint *attribs);
#endif
#endif


#define MAKE_PTRID(id) ((void*)(uintptr_t)id)
#define GET_PTRID(ptr) ((ALuint)(uintptr_t)ptr)

namespace
{

const int sLoudnessFPS = 20; // loudness values per second of audio

// Helper to get an OpenAL extension function
template<typename T, typename R>
void convertPointer(T& dest, R src)
{
    memcpy(&dest, &src, sizeof(src));
}

template<typename T>
void getFunc(T& func, ALCdevice *device, const char *name)
{
    void* funcPtr = alcGetProcAddress(device, name);
    convertPointer(func, funcPtr);
}

}

namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("OpenAL exception: " + msg); }

static void throwALCerror(ALCdevice *device)
{
    ALCenum err = alcGetError(device);
    if(err != ALC_NO_ERROR)
    {
        const ALCchar *errstring = alcGetString(device, err);
        fail(errstring ? errstring : "");
    }
}

static void throwALerror()
{
    ALenum err = alGetError();
    if(err != AL_NO_ERROR)
    {
        const ALchar *errstring = alGetString(err);
        fail(errstring ? errstring : "");
    }
}


static ALenum getALFormat(ChannelConfig chans, SampleType type)
{
    static const struct {
        ALenum format;
        ChannelConfig chans;
        SampleType type;
    } fmtlist[] = {
        { AL_FORMAT_MONO16,   ChannelConfig_Mono,   SampleType_Int16 },
        { AL_FORMAT_MONO8,    ChannelConfig_Mono,   SampleType_UInt8 },
        { AL_FORMAT_STEREO16, ChannelConfig_Stereo, SampleType_Int16 },
        { AL_FORMAT_STEREO8,  ChannelConfig_Stereo, SampleType_UInt8 },
    };
    static const size_t fmtlistsize = sizeof(fmtlist)/sizeof(fmtlist[0]);

    for(size_t i = 0;i < fmtlistsize;i++)
    {
        if(fmtlist[i].chans == chans && fmtlist[i].type == type)
            return fmtlist[i].format;
    }

    if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
    {
        static const struct {
            char name[32];
            ChannelConfig chans;
            SampleType type;
        } mcfmtlist[] = {
            { "AL_FORMAT_QUAD16",   ChannelConfig_Quad,    SampleType_Int16 },
            { "AL_FORMAT_QUAD8",    ChannelConfig_Quad,    SampleType_UInt8 },
            { "AL_FORMAT_51CHN16",  ChannelConfig_5point1, SampleType_Int16 },
            { "AL_FORMAT_51CHN8",   ChannelConfig_5point1, SampleType_UInt8 },
            { "AL_FORMAT_71CHN16",  ChannelConfig_7point1, SampleType_Int16 },
            { "AL_FORMAT_71CHN8",   ChannelConfig_7point1, SampleType_UInt8 },
        };
        static const size_t mcfmtlistsize = sizeof(mcfmtlist)/sizeof(mcfmtlist[0]);

        for(size_t i = 0;i < mcfmtlistsize;i++)
        {
            if(mcfmtlist[i].chans == chans && mcfmtlist[i].type == type)
            {
                ALenum format = alGetEnumValue(mcfmtlist[i].name);
                if(format != 0 && format != -1)
                    return format;
            }
        }
    }
    if(alIsExtensionPresent("AL_EXT_FLOAT32"))
    {
        static const struct {
            char name[32];
            ChannelConfig chans;
            SampleType type;
        } fltfmtlist[] = {
            { "AL_FORMAT_MONO_FLOAT32",   ChannelConfig_Mono,   SampleType_Float32 },
            { "AL_FORMAT_STEREO_FLOAT32", ChannelConfig_Stereo, SampleType_Float32 },
        };
        static const size_t fltfmtlistsize = sizeof(fltfmtlist)/sizeof(fltfmtlist[0]);

        for(size_t i = 0;i < fltfmtlistsize;i++)
        {
            if(fltfmtlist[i].chans == chans && fltfmtlist[i].type == type)
            {
                ALenum format = alGetEnumValue(fltfmtlist[i].name);
                if(format != 0 && format != -1)
                    return format;
            }
        }
        if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            static const struct {
                char name[32];
                ChannelConfig chans;
                SampleType type;
            } fltmcfmtlist[] = {
                { "AL_FORMAT_QUAD32",  ChannelConfig_Quad,    SampleType_Float32 },
                { "AL_FORMAT_51CHN32", ChannelConfig_5point1, SampleType_Float32 },
                { "AL_FORMAT_71CHN32", ChannelConfig_7point1, SampleType_Float32 },
            };
            static const size_t fltmcfmtlistsize = sizeof(fltmcfmtlist)/sizeof(fltmcfmtlist[0]);

            for(size_t i = 0;i < fltmcfmtlistsize;i++)
            {
                if(fltmcfmtlist[i].chans == chans && fltmcfmtlist[i].type == type)
                {
                    ALenum format = alGetEnumValue(fltmcfmtlist[i].name);
                    if(format != 0 && format != -1)
                        return format;
                }
            }
        }
    }

    fail(std::string("Unsupported sound format (")+getChannelConfigName(chans)+", "+getSampleTypeName(type)+")");
    return AL_NONE;
}


//
// A streaming OpenAL sound.
//
class OpenAL_SoundStream
{
    static const ALuint sNumBuffers = 6;
    static const ALfloat sBufferLength;

private:
    ALuint mSource;

    ALuint mBuffers[sNumBuffers];
    ALint mCurrentBufIdx;

    ALenum mFormat;
    ALsizei mSampleRate;
    ALuint mBufferSize;
    ALuint mFrameSize;
    ALint mSilence;

    DecoderPtr mDecoder;

    std::auto_ptr<Sound_Loudness> mLoudnessAnalyzer;

    volatile bool mIsFinished;

    void updateAll(bool local);

    OpenAL_SoundStream(const OpenAL_SoundStream &rhs);
    OpenAL_SoundStream& operator=(const OpenAL_SoundStream &rhs);

    friend class OpenAL_Output;

public:
    OpenAL_SoundStream(ALuint src, DecoderPtr decoder, bool getLoudnessData=false);
    ~OpenAL_SoundStream();

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
struct OpenAL_Output::StreamThread {
    typedef std::vector<OpenAL_SoundStream*> StreamVec;
    StreamVec mStreams;

    volatile bool mQuitNow;
    boost::mutex mMutex;
    boost::condition_variable mCondVar;
    boost::thread mThread;

    StreamThread()
      : mQuitNow(false), mThread(boost::ref(*this))
    {
    }
    ~StreamThread()
    {
        mQuitNow = true;
        mMutex.lock(); mMutex.unlock();
        mCondVar.notify_all();
        mThread.join();
    }

    // boost::thread entry point
    void operator()()
    {
        boost::unique_lock<boost::mutex> lock(mMutex);
        while(!mQuitNow)
        {
            StreamVec::iterator iter = mStreams.begin();
            while(iter != mStreams.end())
            {
                if((*iter)->process() == false)
                    iter = mStreams.erase(iter);
                else
                    ++iter;
            }

            mCondVar.timed_wait(lock, boost::posix_time::milliseconds(50));
        }
    }

    void add(OpenAL_SoundStream *stream)
    {
        boost::unique_lock<boost::mutex> lock(mMutex);
        if(std::find(mStreams.begin(), mStreams.end(), stream) == mStreams.end())
        {
            mStreams.push_back(stream);
            lock.unlock();
            mCondVar.notify_all();
        }
    }

    void remove(OpenAL_SoundStream *stream)
    {
        boost::lock_guard<boost::mutex> lock(mMutex);
        StreamVec::iterator iter = std::find(mStreams.begin(), mStreams.end(), stream);
        if(iter != mStreams.end()) mStreams.erase(iter);
    }

    void removeAll()
    {
        boost::lock_guard<boost::mutex> lock(mMutex);
        mStreams.clear();
    }

private:
    StreamThread(const StreamThread &rhs);
    StreamThread& operator=(const StreamThread &rhs);
};


OpenAL_SoundStream::OpenAL_SoundStream(ALuint src, DecoderPtr decoder, bool getLoudnessData)
  : mSource(src), mCurrentBufIdx(0), mFrameSize(0), mSilence(0), mDecoder(decoder), mIsFinished(false)
{
    alGenBuffers(sNumBuffers, mBuffers);
    throwALerror();
    try
    {
        int srate;
        ChannelConfig chans;
        SampleType type;

        mDecoder->getInfo(&srate, &chans, &type);
        mFormat = getALFormat(chans, type);
        mSampleRate = srate;

        switch(type)
        {
            case SampleType_UInt8: mSilence = 0x80; break;
            case SampleType_Int16: mSilence = 0x00; break;
            case SampleType_Float32: mSilence = 0x00; break;
        }

        mFrameSize = framesToBytes(1, chans, type);
        mBufferSize = static_cast<ALuint>(sBufferLength*srate);
        mBufferSize *= mFrameSize;

        if (getLoudnessData)
            mLoudnessAnalyzer.reset(new Sound_Loudness(sLoudnessFPS, mSampleRate, chans, type));
    }
    catch(std::exception&)
    {
        alDeleteBuffers(sNumBuffers, mBuffers);
        alGetError();
        throw;
    }
    mIsFinished = false;
}
OpenAL_SoundStream::~OpenAL_SoundStream()
{
    alDeleteBuffers(sNumBuffers, mBuffers);
    alGetError();

    mDecoder->close();
}

bool OpenAL_SoundStream::isPlaying()
{
    ALint state;

    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    throwALerror();

    if(state == AL_PLAYING || state == AL_PAUSED)
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
    if(state == AL_PLAYING || state == AL_PAUSED)
    {
        ALint queued;
        alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
        ALint inqueue = mBufferSize/mFrameSize*queued - offset;
        d = (double)inqueue / (double)mSampleRate;
    }

    throwALerror();
    return d;
}

double OpenAL_SoundStream::getStreamOffset() const
{
    ALint state = AL_STOPPED;
    ALint offset;
    double t;

    alGetSourcei(mSource, AL_SAMPLE_OFFSET, &offset);
    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    if(state == AL_PLAYING || state == AL_PAUSED)
    {
        ALint queued;
        alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
        ALint inqueue = mBufferSize/mFrameSize*queued - offset;
        t = (double)(mDecoder->getSampleOffset() - inqueue) / (double)mSampleRate;
    }
    else
    {
        /* Underrun, or not started yet. The decoder offset is where we'll play
         * next. */
        t = (double)mDecoder->getSampleOffset() / (double)mSampleRate;
    }

    throwALerror();
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
    try {
        if(refillQueue() > 0)
        {
            ALint state;
            alGetSourcei(mSource, AL_SOURCE_STATE, &state);
            if(state != AL_PLAYING && state != AL_PAUSED)
            {
                refillQueue();
                alSourcePlay(mSource);
            }
        }
    }
    catch(std::exception&) {
        std::cout<< "Error updating stream \""<<mDecoder->getName()<<"\"" <<std::endl;
        mIsFinished = true;
    }
    return !mIsFinished;
}

ALint OpenAL_SoundStream::refillQueue()
{
    ALint processed;
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
    while(processed > 0)
    {
        ALuint buf;
        alSourceUnqueueBuffers(mSource, 1, &buf);
        --processed;
    }

    ALint queued;
    alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
    if(!mIsFinished && (ALuint)queued < sNumBuffers)
    {
        std::vector<char> data(mBufferSize);
        for(;!mIsFinished && (ALuint)queued < sNumBuffers;++queued)
        {
            size_t got = mDecoder->read(&data[0], data.size());
            if(got < data.size())
            {
                mIsFinished = true;
                memset(&data[got], mSilence, data.size()-got);
            }
            if(got > 0)
            {
                if (mLoudnessAnalyzer.get())
                    mLoudnessAnalyzer->analyzeLoudness(data);

                ALuint bufid = mBuffers[mCurrentBufIdx];
                alBufferData(bufid, mFormat, &data[0], data.size(), mSampleRate);
                alSourceQueueBuffers(mSource, 1, &bufid);
                mCurrentBufIdx = (mCurrentBufIdx+1) % sNumBuffers;
            }
        }
    }

    return queued;
}


//
// An OpenAL output device
//
std::vector<std::string> OpenAL_Output::enumerate()
{
    std::vector<std::string> devlist;
    const ALCchar *devnames;

    if(alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
        devnames = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    else
        devnames = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    while(devnames && *devnames)
    {
        devlist.push_back(devnames);
        devnames += strlen(devnames)+1;
    }
    return devlist;
}

void OpenAL_Output::init(const std::string &devname)
{
    deinit();

    mDevice = alcOpenDevice(devname.c_str());
    if(!mDevice)
    {
        if(devname.empty())
            fail("Failed to open default device");
        else
            fail("Failed to open \""+devname+"\"");
    }
    else
    {
        const ALCchar *name = NULL;
        if(alcIsExtensionPresent(mDevice, "ALC_ENUMERATE_ALL_EXT"))
            name = alcGetString(mDevice, ALC_ALL_DEVICES_SPECIFIER);
        if(alcGetError(mDevice) != AL_NO_ERROR || !name)
            name = alcGetString(mDevice, ALC_DEVICE_SPECIFIER);
        std::cout << "Opened \""<<name<<"\"" << std::endl;
    }

    mContext = alcCreateContext(mDevice, NULL);
    if(!mContext || alcMakeContextCurrent(mContext) == ALC_FALSE)
    {
        if(mContext)
            alcDestroyContext(mContext);
        mContext = 0;
        fail(std::string("Failed to setup context: ")+alcGetString(mDevice, alcGetError(mDevice)));
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    throwALerror();

    ALCint maxmono=0, maxstereo=0;
    alcGetIntegerv(mDevice, ALC_MONO_SOURCES, 1, &maxmono);
    alcGetIntegerv(mDevice, ALC_STEREO_SOURCES, 1, &maxstereo);
    throwALCerror(mDevice);

    try
    {
        ALCuint maxtotal = std::min<ALCuint>(maxmono+maxstereo, 256);
        if (maxtotal == 0) // workaround for broken implementations
            maxtotal = 256;
        for(size_t i = 0;i < maxtotal;i++)
        {
            ALuint src = 0;
            alGenSources(1, &src);
            throwALerror();
            mFreeSources.push_back(src);
        }
    }
    catch(std::exception &e)
    {
        std::cout <<"Error: "<<e.what()<<", trying to continue"<< std::endl;
    }
    if(mFreeSources.empty())
        fail("Could not allocate any sources");

    mInitialized = true;
}

void OpenAL_Output::deinit()
{
    mStreamThread->removeAll();

    for(size_t i = 0;i < mFreeSources.size();i++)
        alDeleteSources(1, &mFreeSources[i]);
    mFreeSources.clear();

    alcMakeContextCurrent(0);
    if(mContext)
        alcDestroyContext(mContext);
    mContext = 0;
    if(mDevice)
        alcCloseDevice(mDevice);
    mDevice = 0;

    mInitialized = false;
}


std::vector<std::string> OpenAL_Output::enumerateHrtf()
{
    if(!mDevice)
        fail("Device not initialized");

    std::vector<std::string> ret;
    if(!alcIsExtensionPresent(mDevice, "ALC_SOFT_HRTF"))
        return ret;

    LPALCGETSTRINGISOFT alcGetStringiSOFT = 0;
    getFunc(alcGetStringiSOFT, mDevice, "alcGetStringiSOFT");

    ALCint num_hrtf;
    alcGetIntegerv(mDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
    ret.reserve(num_hrtf);
    for(ALCint i = 0;i < num_hrtf;++i)
    {
        const ALCchar *entry = alcGetStringiSOFT(mDevice, ALC_HRTF_SPECIFIER_SOFT, i);
        ret.push_back(entry);
    }

    return ret;
}

void OpenAL_Output::enableHrtf(const std::string &hrtfname, bool auto_enable)
{
    if(!alcIsExtensionPresent(mDevice, "ALC_SOFT_HRTF"))
    {
        std::cerr<< "HRTF extension not present" <<std::endl;
        return;
    }


    LPALCGETSTRINGISOFT alcGetStringiSOFT = 0;
    getFunc(alcGetStringiSOFT, mDevice, "alcGetStringiSOFT");

    LPALCRESETDEVICESOFT alcResetDeviceSOFT = 0;
    getFunc(alcResetDeviceSOFT, mDevice, "alcResetDeviceSOFT");

    std::vector<ALCint> attrs;
    attrs.push_back(ALC_HRTF_SOFT);
    attrs.push_back(auto_enable ? ALC_DONT_CARE_SOFT : ALC_TRUE);
    if(!hrtfname.empty())
    {
        ALCint index = -1;
        ALCint num_hrtf;
        alcGetIntegerv(mDevice, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
        for(ALCint i = 0;i < num_hrtf;++i)
        {
            const ALCchar *entry = alcGetStringiSOFT(mDevice, ALC_HRTF_SPECIFIER_SOFT, i);
            if(hrtfname == entry)
            {
                index = i;
                break;
            }
        }

        if(index < 0)
            std::cerr<< "Failed to find HRTF name \""<<hrtfname<<"\", using default" <<std::endl;
        else
        {
            attrs.push_back(ALC_HRTF_ID_SOFT);
            attrs.push_back(index);
        }
    }
    attrs.push_back(0);
    alcResetDeviceSOFT(mDevice, &attrs[0]);

    ALCint hrtf_state;
    alcGetIntegerv(mDevice, ALC_HRTF_SOFT, 1, &hrtf_state);
    if(!hrtf_state)
        std::cerr<< "Failed to enable HRTF" <<std::endl;
    else
    {
        const ALCchar *hrtf = alcGetString(mDevice, ALC_HRTF_SPECIFIER_SOFT);
        std::cout<< "Enabled HRTF "<<hrtf <<std::endl;
    }
}

void OpenAL_Output::disableHrtf()
{
    if(!alcIsExtensionPresent(mDevice, "ALC_SOFT_HRTF"))
    {
        std::cerr<< "HRTF extension not present" <<std::endl;
        return;
    }

    LPALCRESETDEVICESOFT alcResetDeviceSOFT = 0;
    getFunc(alcResetDeviceSOFT, mDevice, "alcResetDeviceSOFT");

    std::vector<ALCint> attrs;
    attrs.push_back(ALC_HRTF_SOFT);
    attrs.push_back(ALC_FALSE);
    attrs.push_back(0);
    alcResetDeviceSOFT(mDevice, &attrs[0]);

    ALCint hrtf_state;
    alcGetIntegerv(mDevice, ALC_HRTF_SOFT, 1, &hrtf_state);
    if(hrtf_state)
        std::cerr<< "Failed to disable HRTF" <<std::endl;
    else
        std::cout<< "Disabled HRTF" <<std::endl;
}


Sound_Handle OpenAL_Output::loadSound(const std::string &fname)
{
    throwALerror();

    DecoderPtr decoder = mManager.getDecoder();
    // Workaround: Bethesda at some point converted some of the files to mp3, but the references were kept as .wav.
    if(decoder->mResourceMgr->exists(fname))
        decoder->open(fname);
    else
    {
        std::string file = fname;
        std::string::size_type pos = file.rfind('.');
        if(pos != std::string::npos)
            file = file.substr(0, pos)+".mp3";
        decoder->open(file);
    }

    std::vector<char> data;
    ChannelConfig chans;
    SampleType type;
    ALenum format;
    int srate;

    decoder->getInfo(&srate, &chans, &type);
    format = getALFormat(chans, type);

    decoder->readAll(data);
    decoder->close();

    ALuint buf = 0;
    try {
        alGenBuffers(1, &buf);
        alBufferData(buf, format, &data[0], data.size(), srate);
        throwALerror();
    }
    catch(...) {
        if(buf && alIsBuffer(buf))
            alDeleteBuffers(1, &buf);
        throw;
    }
    return MAKE_PTRID(buf);
}

void OpenAL_Output::unloadSound(Sound_Handle data)
{
    ALuint buffer = GET_PTRID(data);
    // Make sure no sources are playing this buffer before unloading it.
    SoundVec::const_iterator iter = mActiveSounds.begin();
    for(;iter != mActiveSounds.end();++iter)
    {
        if(!(*iter)->mHandle)
            continue;

        ALuint source = GET_PTRID((*iter)->mHandle);
        ALint srcbuf;
        alGetSourcei(source, AL_BUFFER, &srcbuf);
        if((ALuint)srcbuf == buffer)
        {
            alSourceStop(source);
            alSourcei(source, AL_BUFFER, 0);
        }
    }
    alDeleteBuffers(1, &buffer);
}

size_t OpenAL_Output::getSoundDataSize(Sound_Handle data) const
{
    ALuint buffer = GET_PTRID(data);
    ALint size = 0;

    alGetBufferi(buffer, AL_SIZE, &size);
    throwALerror();

    return (ALuint)size;
}


void OpenAL_Output::initCommon2D(ALuint source, const osg::Vec3f &pos, ALfloat gain, ALfloat pitch, bool loop, bool useenv)
{
    alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
    alSourcef(source, AL_MAX_DISTANCE, 1000.0f);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

    if(useenv && mListenerEnv == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(source, AL_GAIN, gain);
    alSourcef(source, AL_PITCH, pitch);
    alSourcefv(source, AL_POSITION, pos.ptr());
    alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
}

void OpenAL_Output::initCommon3D(ALuint source, const osg::Vec3f &pos, ALfloat mindist, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool loop, bool useenv)
{
    alSourcef(source, AL_REFERENCE_DISTANCE, mindist);
    alSourcef(source, AL_MAX_DISTANCE, maxdist);
    alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

    if((pos - mListenerPos).length2() > maxdist*maxdist)
        gain = 0.0f;
    if(useenv && mListenerEnv == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(source, AL_GAIN, gain);
    alSourcef(source, AL_PITCH, pitch);
    alSourcefv(source, AL_POSITION, pos.ptr());
    alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
}

void OpenAL_Output::updateCommon(ALuint source, const osg::Vec3f& pos, ALfloat maxdist, ALfloat gain, ALfloat pitch, bool useenv, bool is3d)
{
    if(is3d)
    {
        if((pos - mListenerPos).length2() > maxdist*maxdist)
            gain = 0.0f;
    }
    if(useenv && mListenerEnv == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(source, AL_GAIN, gain);
    alSourcef(source, AL_PITCH, pitch);
    alSourcefv(source, AL_POSITION, pos.ptr());
    alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
}


void OpenAL_Output::playSound(MWBase::SoundPtr sound, Sound_Handle data, float offset)
{
    ALuint source;

    if(mFreeSources.empty())
        fail("No free sources");
    source = mFreeSources.front();
    mFreeSources.pop_front();

    try {
        initCommon2D(source, sound->getPosition(), sound->getRealVolume(), sound->getPitch(),
                     sound->getIsLooping(), sound->getUseEnv());

        alSourcef(source, AL_SEC_OFFSET, offset);
        throwALerror();

        alSourcei(source, AL_BUFFER, GET_PTRID(data));
        alSourcePlay(source);
        throwALerror();

        mActiveSounds.push_back(sound);
    }
    catch(std::exception&) {
        mFreeSources.push_back(source);
        throw;
    }

    sound->mHandle = MAKE_PTRID(source);
}

void OpenAL_Output::playSound3D(MWBase::SoundPtr sound, Sound_Handle data, float offset)
{
    ALuint source;

    if(mFreeSources.empty())
        fail("No free sources");
    source = mFreeSources.front();
    mFreeSources.pop_front();

    try {
        initCommon3D(source, sound->getPosition(), sound->getMinDistance(), sound->getMaxDistance(),
                     sound->getRealVolume(), sound->getPitch(), sound->getIsLooping(),
                     sound->getUseEnv());

        alSourcef(source, AL_SEC_OFFSET, offset);
        throwALerror();

        alSourcei(source, AL_BUFFER, GET_PTRID(data));
        alSourcePlay(source);
        throwALerror();

        mActiveSounds.push_back(sound);
    }
    catch(std::exception&) {
        mFreeSources.push_back(source);
        throw;
    }

    sound->mHandle = MAKE_PTRID(source);
}

void OpenAL_Output::finishSound(MWBase::SoundPtr sound)
{
    if(!sound->mHandle) return;
    ALuint source = GET_PTRID(sound->mHandle);
    sound->mHandle = 0;

    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);

    mFreeSources.push_back(source);
    mActiveSounds.erase(std::find(mActiveSounds.begin(), mActiveSounds.end(), sound));
}

bool OpenAL_Output::isSoundPlaying(MWBase::SoundPtr sound)
{
    if(!sound->mHandle) return false;
    ALuint source = GET_PTRID(sound->mHandle);
    ALint state;

    alGetSourcei(source, AL_SOURCE_STATE, &state);
    throwALerror();

    return state == AL_PLAYING || state == AL_PAUSED;
}

void OpenAL_Output::updateSound(MWBase::SoundPtr sound)
{
    if(!sound->mHandle) return;
    ALuint source = GET_PTRID(sound->mHandle);

    updateCommon(source, sound->getPosition(), sound->getMaxDistance(), sound->getRealVolume(),
                 sound->getPitch(), sound->getUseEnv(), sound->getIs3D());
}


void OpenAL_Output::streamSound(DecoderPtr decoder, MWBase::SoundStreamPtr sound)
{
    OpenAL_SoundStream *stream = 0;
    ALuint source;

    if(mFreeSources.empty())
        fail("No free sources");
    source = mFreeSources.front();
    mFreeSources.pop_front();

    if(sound->getIsLooping())
        std::cout <<"Warning: cannot loop stream \""<<decoder->getName()<<"\""<< std::endl;
    try {
        initCommon2D(source, sound->getPosition(), sound->getRealVolume(), sound->getPitch(),
                     false, sound->getUseEnv());
        throwALerror();

        stream = new OpenAL_SoundStream(source, decoder);
        mStreamThread->add(stream);
        mActiveStreams.push_back(sound);
    }
    catch(std::exception&) {
        mStreamThread->remove(stream);
        delete stream;
        mFreeSources.push_back(source);
        throw;
    }

    sound->mHandle = stream;
}

void OpenAL_Output::streamSound3D(DecoderPtr decoder, MWBase::SoundStreamPtr sound, bool getLoudnessData)
{
    OpenAL_SoundStream *stream = 0;
    ALuint source;

    if(mFreeSources.empty())
        fail("No free sources");
    source = mFreeSources.front();
    mFreeSources.pop_front();

    if(sound->getIsLooping())
        std::cout <<"Warning: cannot loop stream \""<<decoder->getName()<<"\""<< std::endl;
    try {
        initCommon3D(source, sound->getPosition(), sound->getMinDistance(), sound->getMaxDistance(),
                     sound->getRealVolume(), sound->getPitch(), false, sound->getUseEnv());
        throwALerror();

        stream = new OpenAL_SoundStream(source, decoder, getLoudnessData);
        mStreamThread->add(stream);
        mActiveStreams.push_back(sound);
    }
    catch(std::exception&) {
        mStreamThread->remove(stream);
        delete stream;
        mFreeSources.push_back(source);
        throw;
    }

    sound->mHandle = stream;
}

void OpenAL_Output::finishStream(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    ALuint source = stream->mSource;

    sound->mHandle = 0;
    mStreamThread->remove(stream);

    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);

    mFreeSources.push_back(source);
    mActiveStreams.erase(std::find(mActiveStreams.begin(), mActiveStreams.end(), sound));

    delete stream;
}

double OpenAL_Output::getStreamDelay(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return 0.0;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    return stream->getStreamDelay();
}

double OpenAL_Output::getStreamOffset(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return 0.0;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    boost::lock_guard<boost::mutex> lock(mStreamThread->mMutex);
    return stream->getStreamOffset();
}

float OpenAL_Output::getStreamLoudness(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return 0.0;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    boost::lock_guard<boost::mutex> lock(mStreamThread->mMutex);
    return stream->getCurrentLoudness();
}

bool OpenAL_Output::isStreamPlaying(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return false;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    boost::lock_guard<boost::mutex> lock(mStreamThread->mMutex);
    return stream->isPlaying();
}

void OpenAL_Output::updateStream(MWBase::SoundStreamPtr sound)
{
    if(!sound->mHandle) return;
    OpenAL_SoundStream *stream = reinterpret_cast<OpenAL_SoundStream*>(sound->mHandle);
    ALuint source = stream->mSource;

    updateCommon(source, sound->getPosition(), sound->getMaxDistance(), sound->getRealVolume(),
                 sound->getPitch(), sound->getUseEnv(), sound->getIs3D());
}


void OpenAL_Output::startUpdate()
{
    alcSuspendContext(alcGetCurrentContext());
}

void OpenAL_Output::finishUpdate()
{
    alcProcessContext(alcGetCurrentContext());
}


void OpenAL_Output::updateListener(const osg::Vec3f &pos, const osg::Vec3f &atdir, const osg::Vec3f &updir, Environment env)
{
    if(mContext)
    {
        ALfloat orient[6] = {
            atdir.x(), atdir.y(), atdir.z(),
            updir.x(), updir.y(), updir.z()
        };
        alListenerfv(AL_POSITION, pos.ptr());
        alListenerfv(AL_ORIENTATION, orient);
        throwALerror();
    }

    mListenerPos = pos;
    mListenerEnv = env;
}


void OpenAL_Output::pauseSounds(int types)
{
    std::vector<ALuint> sources;
    SoundVec::const_iterator sound = mActiveSounds.begin();
    for(;sound != mActiveSounds.end();++sound)
    {
        if(*sound && (*sound)->mHandle && ((*sound)->getPlayType()&types))
            sources.push_back(GET_PTRID((*sound)->mHandle));
    }
    StreamVec::const_iterator stream = mActiveStreams.begin();
    for(;stream != mActiveStreams.end();++stream)
    {
        if(*stream && (*stream)->mHandle && ((*stream)->getPlayType()&types))
        {
            OpenAL_SoundStream *strm = reinterpret_cast<OpenAL_SoundStream*>((*stream)->mHandle);
            sources.push_back(strm->mSource);
        }
    }
    if(!sources.empty())
    {
        alSourcePausev(sources.size(), &sources[0]);
        throwALerror();
    }
}

void OpenAL_Output::resumeSounds(int types)
{
    std::vector<ALuint> sources;
    SoundVec::const_iterator sound = mActiveSounds.begin();
    for(;sound != mActiveSounds.end();++sound)
    {
        if(*sound && (*sound)->mHandle && ((*sound)->getPlayType()&types))
            sources.push_back(GET_PTRID((*sound)->mHandle));
    }
    StreamVec::const_iterator stream = mActiveStreams.begin();
    for(;stream != mActiveStreams.end();++stream)
    {
        if(*stream && (*stream)->mHandle && ((*stream)->getPlayType()&types))
        {
            OpenAL_SoundStream *strm = reinterpret_cast<OpenAL_SoundStream*>((*stream)->mHandle);
            sources.push_back(strm->mSource);
        }
    }
    if(!sources.empty())
    {
        alSourcePlayv(sources.size(), &sources[0]);
        throwALerror();
    }
}


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), mDevice(0), mContext(0)
  , mListenerPos(0.0f, 0.0f, 0.0f), mListenerEnv(Env_Normal)
  , mStreamThread(new StreamThread)
{
}

OpenAL_Output::~OpenAL_Output()
{
    deinit();
}

}
