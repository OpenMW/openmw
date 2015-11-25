#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <vector>

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


#define MAKE_PTRID(id) ((void*)(uintptr_t)id)
#define GET_PTRID(ptr) ((ALuint)(uintptr_t)ptr)

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

static double getBufferLength(ALuint buffer)
{
    ALint bufferSize, frequency, channels, bitsPerSample;
    alGetBufferi(buffer, AL_SIZE, &bufferSize);
    alGetBufferi(buffer, AL_FREQUENCY, &frequency);
    alGetBufferi(buffer, AL_CHANNELS, &channels);
    alGetBufferi(buffer, AL_BITS, &bitsPerSample);
    throwALerror();

    return (8.0*bufferSize)/(frequency*channels*bitsPerSample);
}


//
// A streaming OpenAL sound.
//
class OpenAL_SoundStream : public Sound
{
    static const ALuint sNumBuffers = 6;
    static const ALfloat sBufferLength;

protected:
    OpenAL_Output &mOutput;

    ALuint mSource;

private:
    ALuint mBuffers[sNumBuffers];
    ALint mCurrentBufIdx;

    ALenum mFormat;
    ALsizei mSampleRate;
    ALuint mBufferSize;
    ALuint mFrameSize;
    ALint mSilence;

    DecoderPtr mDecoder;

    volatile bool mIsFinished;

    void updateAll(bool local);

    OpenAL_SoundStream(const OpenAL_SoundStream &rhs);
    OpenAL_SoundStream& operator=(const OpenAL_SoundStream &rhs);

    friend class OpenAL_Output;

public:
    OpenAL_SoundStream(OpenAL_Output &output, ALuint src, DecoderPtr decoder, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags);
    virtual ~OpenAL_SoundStream();

    virtual void stop();
    virtual bool isPlaying();
    virtual double getTimeOffset();
    virtual void update();

    void play();
    bool process();
    ALint refillQueue();
};
const ALfloat OpenAL_SoundStream::sBufferLength = 0.125f;

class OpenAL_SoundStream3D : public OpenAL_SoundStream
{
    OpenAL_SoundStream3D(const OpenAL_SoundStream3D &rhs);
    OpenAL_SoundStream3D& operator=(const OpenAL_SoundStream3D &rhs);

public:
    OpenAL_SoundStream3D(OpenAL_Output &output, ALuint src, DecoderPtr decoder, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
      : OpenAL_SoundStream(output, src, decoder, pos, vol, basevol, pitch, mindist, maxdist, flags)
    { }

    virtual void update();
};


//
// A background streaming thread (keeps active streams processed)
//
struct OpenAL_Output::StreamThread {
    typedef std::vector<OpenAL_SoundStream*> StreamVec;
    StreamVec mStreams;
    boost::recursive_mutex mMutex;
    boost::thread mThread;

    StreamThread()
      : mThread(boost::ref(*this))
    {
    }
    ~StreamThread()
    {
        mThread.interrupt();
    }

    // boost::thread entry point
    void operator()()
    {
        while(1)
        {
            boost::unique_lock<boost::recursive_mutex> lock(mMutex);
            StreamVec::iterator iter = mStreams.begin();
            while(iter != mStreams.end())
            {
                if((*iter)->process() == false)
                    iter = mStreams.erase(iter);
                else
                    ++iter;
            }
            lock.unlock();
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        }
    }

    void add(OpenAL_SoundStream *stream)
    {
        boost::lock_guard<boost::recursive_mutex> lock(mMutex);
        if(std::find(mStreams.begin(), mStreams.end(), stream) == mStreams.end())
            mStreams.push_back(stream);
    }

    void remove(OpenAL_SoundStream *stream)
    {
        boost::lock_guard<boost::recursive_mutex> lock(mMutex);
        StreamVec::iterator iter = std::find(mStreams.begin(), mStreams.end(), stream);
        if(iter != mStreams.end()) mStreams.erase(iter);
    }

    void removeAll()
    {
        boost::lock_guard<boost::recursive_mutex> lock(mMutex);
        mStreams.clear();
    }

private:
    StreamThread(const StreamThread &rhs);
    StreamThread& operator=(const StreamThread &rhs);
};


OpenAL_SoundStream::OpenAL_SoundStream(OpenAL_Output &output, ALuint src, DecoderPtr decoder, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
  : Sound(pos, vol, basevol, pitch, mindist, maxdist, flags)
  , mOutput(output), mSource(src), mCurrentBufIdx(0), mFrameSize(0), mSilence(0)
  , mDecoder(decoder), mIsFinished(true)
{
    throwALerror();

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
            case SampleType_UInt8: mSilence = 0x80;
            case SampleType_Int16: mSilence = 0x00;
            case SampleType_Float32: mSilence = 0x00;
        }

        mFrameSize = framesToBytes(1, chans, type);
        mBufferSize = static_cast<ALuint>(sBufferLength*srate);
        mBufferSize *= mFrameSize;

        mOutput.mActiveStreams.push_back(this);
    }
    catch(std::exception&)
    {
        alDeleteBuffers(sNumBuffers, mBuffers);
        alGetError();
        throw;
    }
}
OpenAL_SoundStream::~OpenAL_SoundStream()
{
    mOutput.mStreamThread->remove(this);

    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);

    mOutput.mFreeSources.push_back(mSource);
    alDeleteBuffers(sNumBuffers, mBuffers);
    alGetError();

    mDecoder->close();

    mOutput.mActiveStreams.erase(std::find(mOutput.mActiveStreams.begin(),
                                           mOutput.mActiveStreams.end(), this));
}

void OpenAL_SoundStream::play()
{
    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    throwALerror();

    mIsFinished = false;
    mOutput.mStreamThread->add(this);
}

void OpenAL_SoundStream::stop()
{
    mOutput.mStreamThread->remove(this);
    mIsFinished = true;

    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    throwALerror();

    mDecoder->rewind();
}

bool OpenAL_SoundStream::isPlaying()
{
    ALint state;

    boost::lock_guard<boost::recursive_mutex> lock(mOutput.mStreamThread->mMutex);
    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    throwALerror();

    if(state == AL_PLAYING || state == AL_PAUSED)
        return true;
    return !mIsFinished;
}

double OpenAL_SoundStream::getTimeOffset()
{
    ALint state = AL_STOPPED;
    ALint offset;
    double t;

    boost::lock_guard<boost::recursive_mutex> lock(mOutput.mStreamThread->mMutex);
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

void OpenAL_SoundStream::updateAll(bool local)
{
    alSourcef(mSource, AL_REFERENCE_DISTANCE, mMinDistance);
    alSourcef(mSource, AL_MAX_DISTANCE, mMaxDistance);
    if(local)
    {
        alSourcef(mSource, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(mSource, AL_SOURCE_RELATIVE, AL_TRUE);
    }
    else
    {
        alSourcef(mSource, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcei(mSource, AL_SOURCE_RELATIVE, AL_FALSE);
    }
    alSourcei(mSource, AL_LOOPING, AL_FALSE);

    update();
}

void OpenAL_SoundStream::update()
{
    ALfloat gain = mVolume*mBaseVolume;
    ALfloat pitch = mPitch;
    if(!(mFlags&MWBase::SoundManager::Play_NoEnv) && mOutput.mLastEnvironment == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(mSource, AL_GAIN, gain);
    alSourcef(mSource, AL_PITCH, pitch);
    alSource3f(mSource, AL_POSITION, mPos[0], mPos[1], mPos[2]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
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
                ALuint bufid = mBuffers[mCurrentBufIdx];
                alBufferData(bufid, mFormat, &data[0], data.size(), mSampleRate);
                alSourceQueueBuffers(mSource, 1, &bufid);
                mCurrentBufIdx = (mCurrentBufIdx+1) % sNumBuffers;
            }
        }
    }

    return queued;
}

void OpenAL_SoundStream3D::update()
{
    ALfloat gain = mVolume*mBaseVolume;
    ALfloat pitch = mPitch;
    if((mPos - mOutput.mPos).length2() > mMaxDistance*mMaxDistance)
        gain = 0.0f;
    else if(!(mFlags&MWBase::SoundManager::Play_NoEnv) && mOutput.mLastEnvironment == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(mSource, AL_GAIN, gain);
    alSourcef(mSource, AL_PITCH, pitch);
    alSource3f(mSource, AL_POSITION, mPos[0], mPos[1], mPos[2]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
}


//
// A regular 2D OpenAL sound
//
class OpenAL_Sound : public Sound
{
protected:
    OpenAL_Output &mOutput;

    ALuint mSource;

    friend class OpenAL_Output;

    void updateAll(bool local);

private:
    OpenAL_Sound(const OpenAL_Sound &rhs);
    OpenAL_Sound& operator=(const OpenAL_Sound &rhs);

public:
    OpenAL_Sound(OpenAL_Output &output, ALuint src, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags);
    virtual ~OpenAL_Sound();

    virtual void stop();
    virtual bool isPlaying();
    virtual double getTimeOffset();
    virtual void update();
};

//
// A regular 3D OpenAL sound
//
class OpenAL_Sound3D : public OpenAL_Sound
{
    OpenAL_Sound3D(const OpenAL_Sound &rhs);
    OpenAL_Sound3D& operator=(const OpenAL_Sound &rhs);

public:
    OpenAL_Sound3D(OpenAL_Output &output, ALuint src, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
      : OpenAL_Sound(output, src, pos, vol, basevol, pitch, mindist, maxdist, flags)
    { }

    virtual void update();
};

OpenAL_Sound::OpenAL_Sound(OpenAL_Output &output, ALuint src, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
  : Sound(pos, vol, basevol, pitch, mindist, maxdist, flags)
  , mOutput(output), mSource(src)
{
    mOutput.mActiveSounds.push_back(this);
}
OpenAL_Sound::~OpenAL_Sound()
{
    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);

    mOutput.mFreeSources.push_back(mSource);

    mOutput.mActiveSounds.erase(std::find(mOutput.mActiveSounds.begin(),
                                          mOutput.mActiveSounds.end(), this));
}

void OpenAL_Sound::stop()
{
    alSourceStop(mSource);
    throwALerror();
}

bool OpenAL_Sound::isPlaying()
{
    ALint state;

    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    throwALerror();

    return state==AL_PLAYING || state==AL_PAUSED;
}

double OpenAL_Sound::getTimeOffset()
{
    ALfloat t;

    alGetSourcef(mSource, AL_SEC_OFFSET, &t);
    throwALerror();

    return t;
}

void OpenAL_Sound::updateAll(bool local)
{
    alSourcef(mSource, AL_REFERENCE_DISTANCE, mMinDistance);
    alSourcef(mSource, AL_MAX_DISTANCE, mMaxDistance);
    if(local)
    {
        alSourcef(mSource, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(mSource, AL_SOURCE_RELATIVE, AL_TRUE);
    }
    else
    {
        alSourcef(mSource, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcei(mSource, AL_SOURCE_RELATIVE, AL_FALSE);
    }
    alSourcei(mSource, AL_LOOPING, (mFlags&MWBase::SoundManager::Play_Loop) ? AL_TRUE : AL_FALSE);

    update();
}

void OpenAL_Sound::update()
{
    ALfloat gain = mVolume*mBaseVolume;
    ALfloat pitch = mPitch;

    if(!(mFlags&MWBase::SoundManager::Play_NoEnv) && mOutput.mLastEnvironment == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(mSource, AL_GAIN, gain);
    alSourcef(mSource, AL_PITCH, pitch);
    alSource3f(mSource, AL_POSITION, mPos[0], mPos[1], mPos[2]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
}

void OpenAL_Sound3D::update()
{
    ALfloat gain = mVolume*mBaseVolume;
    ALfloat pitch = mPitch;
    if((mPos - mOutput.mPos).length2() > mMaxDistance*mMaxDistance)
        gain = 0.0f;
    else if(!(mFlags&MWBase::SoundManager::Play_NoEnv) && mOutput.mLastEnvironment == Env_Underwater)
    {
        gain *= 0.9f;
        pitch *= 0.7f;
    }

    alSourcef(mSource, AL_GAIN, gain);
    alSourcef(mSource, AL_PITCH, pitch);
    alSource3f(mSource, AL_POSITION, mPos[0], mPos[1], mPos[2]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
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


Sound_Handle OpenAL_Output::loadSound(const std::string &fname)
{
    throwALerror();

    DecoderPtr decoder = mManager.getDecoder();
    // Workaround: Bethesda at some point converted some of the files to mp3, but the references were kept as .wav.
    std::string file = fname;
    if (!decoder->mResourceMgr->exists(file))
    {
        std::string::size_type pos = file.rfind('.');
        if(pos != std::string::npos)
            file = file.substr(0, pos)+".mp3";
    }
    decoder->open(file);

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
        if(!(*iter)->mSource)
            continue;

        ALint srcbuf;
        alGetSourcei((*iter)->mSource, AL_BUFFER, &srcbuf);
        if((ALuint)srcbuf == buffer)
        {
            alSourceStop((*iter)->mSource);
            alSourcei((*iter)->mSource, AL_BUFFER, 0);
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


MWBase::SoundPtr OpenAL_Output::playSound(Sound_Handle data, float vol, float basevol, float pitch, int flags,float offset)
{
    boost::shared_ptr<OpenAL_Sound> sound;
    ALuint src;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    try {
        sound.reset(new OpenAL_Sound(*this, src, osg::Vec3f(0.f, 0.f, 0.f), vol, basevol, pitch, 1.0f, 1000.0f, flags));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        throw;
    }

    sound->updateAll(true);
    if(offset < 0.0f)
        offset = 0.0f;
    if(offset > 1.0f)
        offset = 1.0f;

    ALuint buffer = GET_PTRID(data);
    alSourcei(src, AL_BUFFER, buffer);
    alSourcef(src, AL_SEC_OFFSET, static_cast<ALfloat>(getBufferLength(buffer)*offset / pitch));
    alSourcePlay(src);
    throwALerror();

    return sound;
}

MWBase::SoundPtr OpenAL_Output::playSound3D(Sound_Handle data, const osg::Vec3f &pos, float vol, float basevol, float pitch,
                                            float min, float max, int flags, float offset)
{
    boost::shared_ptr<OpenAL_Sound> sound;
    ALuint src;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    try {
        sound.reset(new OpenAL_Sound3D(*this, src, pos, vol, basevol, pitch, min, max, flags));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        throw;
    }

    sound->updateAll(false);
    if(offset < 0.0f)
        offset = 0.0f;
    if(offset > 1.0f)
        offset = 1.0f;

    ALuint buffer = GET_PTRID(data);
    alSourcei(src, AL_BUFFER, buffer);
    alSourcef(src, AL_SEC_OFFSET, static_cast<ALfloat>(getBufferLength(buffer)*offset / pitch));

    alSourcePlay(src);
    throwALerror();

    return sound;
}


MWBase::SoundPtr OpenAL_Output::streamSound(DecoderPtr decoder, float basevol, float pitch, int flags)
{
    boost::shared_ptr<OpenAL_SoundStream> sound;
    ALuint src;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    if((flags&MWBase::SoundManager::Play_Loop))
        std::cout <<"Warning: cannot loop stream \""<<decoder->getName()<<"\""<< std::endl;
    try
    {
        sound.reset(new OpenAL_SoundStream(*this, src, decoder, osg::Vec3f(0.0f, 0.0f, 0.0f), 1.0f, basevol, pitch, 1.0f, 1000.0f, flags));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        throw;
    }

    sound->updateAll(true);

    sound->play();
    return sound;
}


MWBase::SoundPtr OpenAL_Output::streamSound3D(DecoderPtr decoder, const osg::Vec3f &pos, float volume, float basevol, float pitch, float min, float max, int flags)
{
    boost::shared_ptr<OpenAL_SoundStream> sound;
    ALuint src;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    if((flags&MWBase::SoundManager::Play_Loop))
        std::cout <<"Warning: cannot loop stream \""<<decoder->getName()<<"\""<< std::endl;
    try
    {
        sound.reset(new OpenAL_SoundStream3D(*this, src, decoder, pos, volume, basevol, pitch, min, max, flags));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        throw;
    }

    sound->updateAll(false);

    sound->play();
    return sound;
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
    mPos = pos;
    mLastEnvironment = env;

    if(mContext)
    {
        ALfloat orient[6] = {
            atdir.x(), atdir.y(), atdir.z(),
            updir.x(), updir.y(), updir.z()
        };
        alListener3f(AL_POSITION, mPos.x(), mPos.y(), mPos.z());
        alListenerfv(AL_ORIENTATION, orient);
        throwALerror();
    }
}


void OpenAL_Output::pauseSounds(int types)
{
    std::vector<ALuint> sources;
    SoundVec::const_iterator sound = mActiveSounds.begin();
    for(;sound != mActiveSounds.end();++sound)
    {
        if(*sound && (*sound)->mSource && ((*sound)->getPlayType()&types))
            sources.push_back((*sound)->mSource);
    }
    StreamVec::const_iterator stream = mActiveStreams.begin();
    for(;stream != mActiveStreams.end();++stream)
    {
        if(*stream && (*stream)->mSource && ((*stream)->getPlayType()&types))
            sources.push_back((*stream)->mSource);
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
        if(*sound && (*sound)->mSource && ((*sound)->getPlayType()&types))
            sources.push_back((*sound)->mSource);
    }
    StreamVec::const_iterator stream = mActiveStreams.begin();
    for(;stream != mActiveStreams.end();++stream)
    {
        if(*stream && (*stream)->mSource && ((*stream)->getPlayType()&types))
            sources.push_back((*stream)->mSource);
    }
    if(!sources.empty())
    {
        alSourcePlayv(sources.size(), &sources[0]);
        throwALerror();
    }
}


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), mDevice(0), mContext(0), mLastEnvironment(Env_Normal),
    mStreamThread(new StreamThread)
{
}

OpenAL_Output::~OpenAL_Output()
{
    deinit();
}

}
