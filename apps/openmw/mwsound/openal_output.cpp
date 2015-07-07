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

namespace
{
    const int loudnessFPS = 20; // loudness values per second of audio
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

static ALint getBufferSampleCount(ALuint buf)
{
    ALint size, bits, channels;

    alGetBufferi(buf, AL_SIZE, &size);
    alGetBufferi(buf, AL_BITS, &bits);
    alGetBufferi(buf, AL_CHANNELS, &channels);
    throwALerror();

    return size / channels * 8 / bits;
}

//
// A streaming OpenAL sound.
//
class OpenAL_SoundStream : public Sound
{
    static const ALuint sNumBuffers = 6;
    static const ALfloat sBufferLength;

    OpenAL_Output &mOutput;

    ALuint mSource;
    ALuint mBuffers[sNumBuffers];

    ALenum mFormat;
    ALsizei mSampleRate;
    ALuint mBufferSize;

    ALuint mSamplesQueued;

    DecoderPtr mDecoder;

    volatile bool mIsFinished;
    volatile bool mIsInitialBatchEnqueued;

    void updateAll(bool local);

    OpenAL_SoundStream(const OpenAL_SoundStream &rhs);
    OpenAL_SoundStream& operator=(const OpenAL_SoundStream &rhs);

    friend class OpenAL_Output;

public:
    OpenAL_SoundStream(OpenAL_Output &output, ALuint src, DecoderPtr decoder, float basevol, float pitch, int flags);
    virtual ~OpenAL_SoundStream();

    virtual void stop();
    virtual bool isPlaying();
    virtual double getTimeOffset();
    virtual void update();

    void play();
    bool process();
};

const ALfloat OpenAL_SoundStream::sBufferLength = 0.125f;

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
            mMutex.lock();
            StreamVec::iterator iter = mStreams.begin();
            while(iter != mStreams.end())
            {
                if((*iter)->process() == false)
                    iter = mStreams.erase(iter);
                else
                    ++iter;
            }
            mMutex.unlock();
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        }
    }

    void add(OpenAL_SoundStream *stream)
    {
        mMutex.lock();
        if(std::find(mStreams.begin(), mStreams.end(), stream) == mStreams.end())
            mStreams.push_back(stream);
        mMutex.unlock();
    }

    void remove(OpenAL_SoundStream *stream)
    {
        mMutex.lock();
        StreamVec::iterator iter = std::find(mStreams.begin(), mStreams.end(), stream);
        if(iter != mStreams.end())
            mStreams.erase(iter);
        mMutex.unlock();
    }

    void removeAll()
    {
        mMutex.lock();
        mStreams.clear();
        mMutex.unlock();
    }

private:
    StreamThread(const StreamThread &rhs);
    StreamThread& operator=(const StreamThread &rhs);
};


OpenAL_SoundStream::OpenAL_SoundStream(OpenAL_Output &output, ALuint src, DecoderPtr decoder, float basevol, float pitch, int flags)
  : Sound(osg::Vec3f(0.f, 0.f, 0.f), 1.0f, basevol, pitch, 1.0f, 1000.0f, flags)
  , mOutput(output), mSource(src), mSamplesQueued(0), mDecoder(decoder), mIsFinished(true), mIsInitialBatchEnqueued(false)
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

        mBufferSize = static_cast<ALuint>(sBufferLength*srate);
        mBufferSize = framesToBytes(mBufferSize, chans, type);

        mOutput.mActiveSounds.push_back(this);
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

    mOutput.mActiveSounds.erase(std::find(mOutput.mActiveSounds.begin(),
                                          mOutput.mActiveSounds.end(), this));
}

void OpenAL_SoundStream::play()
{
    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    throwALerror();
    mSamplesQueued = 0;
    mIsFinished = false;
    mIsInitialBatchEnqueued = false;
    mOutput.mStreamThread->add(this);
}

void OpenAL_SoundStream::stop()
{
    mOutput.mStreamThread->remove(this);
    mIsFinished = true;
    mIsInitialBatchEnqueued = false;

    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    throwALerror();
    mSamplesQueued = 0;

    mDecoder->rewind();
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

double OpenAL_SoundStream::getTimeOffset()
{
    ALint state = AL_STOPPED;
    ALfloat offset = 0.0f;
    double t;

    mOutput.mStreamThread->mMutex.lock();
    alGetSourcef(mSource, AL_SEC_OFFSET, &offset);
    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    if(state == AL_PLAYING || state == AL_PAUSED)
        t = (double)(mDecoder->getSampleOffset() - mSamplesQueued)/(double)mSampleRate + offset;
    else
        t = (double)mDecoder->getSampleOffset() / (double)mSampleRate;
    mOutput.mStreamThread->mMutex.unlock();

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
        bool finished = mIsFinished;
        ALint processed, state;

        alGetSourcei(mSource, AL_SOURCE_STATE, &state);
        alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
        throwALerror();

        if(processed > 0)
        {
            std::vector<char> data(mBufferSize);
            do {
                ALuint bufid = 0;
                size_t got;

                alSourceUnqueueBuffers(mSource, 1, &bufid);
                mSamplesQueued -= getBufferSampleCount(bufid);
                processed--;

                if(finished)
                    continue;

                got = mDecoder->read(&data[0], data.size());
                finished = (got < data.size());
                if(got > 0)
                {
                    alBufferData(bufid, mFormat, &data[0], got, mSampleRate);
                    alSourceQueueBuffers(mSource, 1, &bufid);
                    mSamplesQueued += getBufferSampleCount(bufid);
                }
            } while(processed > 0);
            throwALerror();
        }
        else if (!mIsInitialBatchEnqueued) { // nothing enqueued yet
            std::vector<char> data(mBufferSize);

            for(ALuint i = 0;i < sNumBuffers && !finished;i++)
            {
                size_t got = mDecoder->read(&data[0], data.size());
                finished = (got < data.size());
                if(got > 0)
                {
                    ALuint bufid = mBuffers[i];
                    alBufferData(bufid, mFormat, &data[0], got, mSampleRate);
                    alSourceQueueBuffers(mSource, 1, &bufid);
                    throwALerror();
                    mSamplesQueued += getBufferSampleCount(bufid);
                }
            }
            mIsInitialBatchEnqueued = true;
        }

        if(state != AL_PLAYING && state != AL_PAUSED)
        {
            ALint queued = 0;

            alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
            if(queued > 0)
                alSourcePlay(mSource);
            throwALerror();
        }

        mIsFinished = finished;
    }
    catch(std::exception&) {
        std::cout<< "Error updating stream \""<<mDecoder->getName()<<"\"" <<std::endl;
        mSamplesQueued = 0;
        mIsFinished = true;
        mIsInitialBatchEnqueued = false;
    }
    return !mIsFinished;
}

//
// A regular 2D OpenAL sound
//
class OpenAL_Sound : public Sound
{
protected:
    OpenAL_Output &mOutput;

    ALuint mSource;
    ALuint mBuffer;

    friend class OpenAL_Output;

    void updateAll(bool local);

private:
    OpenAL_Sound(const OpenAL_Sound &rhs);
    OpenAL_Sound& operator=(const OpenAL_Sound &rhs);

public:
    OpenAL_Sound(OpenAL_Output &output, ALuint src, ALuint buf, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags);
    virtual ~OpenAL_Sound();

    virtual void stop();
    virtual bool isPlaying();
    virtual double getTimeOffset();
    virtual double getLength();
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
    OpenAL_Sound3D(OpenAL_Output &output, ALuint src, ALuint buf, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
      : OpenAL_Sound(output, src, buf, pos, vol, basevol, pitch, mindist, maxdist, flags)
    { }

    virtual void update();
};

OpenAL_Sound::OpenAL_Sound(OpenAL_Output &output, ALuint src, ALuint buf, const osg::Vec3f& pos, float vol, float basevol, float pitch, float mindist, float maxdist, int flags)
  : Sound(pos, vol, basevol, pitch, mindist, maxdist, flags)
  , mOutput(output), mSource(src), mBuffer(buf)
{
    mOutput.mActiveSounds.push_back(this);
}
OpenAL_Sound::~OpenAL_Sound()
{
    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);

    mOutput.mFreeSources.push_back(mSource);
    mOutput.bufferFinished(mBuffer);

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

double OpenAL_Sound::getLength()
{
    ALint bufferSize, frequency, channels, bitsPerSample;
    alGetBufferi(mBuffer, AL_SIZE, &bufferSize);
    alGetBufferi(mBuffer, AL_FREQUENCY, &frequency);
    alGetBufferi(mBuffer, AL_CHANNELS, &channels);
    alGetBufferi(mBuffer, AL_BITS, &bitsPerSample);

    return (8.0*bufferSize)/(frequency*channels*bitsPerSample);
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

    mBufferRefs.clear();
    mUnusedBuffers.clear();
    while(!mBufferCache.empty())
    {
        alDeleteBuffers(1, &mBufferCache.begin()->second.mALBuffer);
        mBufferCache.erase(mBufferCache.begin());
    }

    alcMakeContextCurrent(0);
    if(mContext)
        alcDestroyContext(mContext);
    mContext = 0;
    if(mDevice)
        alcCloseDevice(mDevice);
    mDevice = 0;

    mInitialized = false;
}


const CachedSound& OpenAL_Output::getBuffer(const std::string &fname)
{
    ALuint buf = 0;

    NameMap::iterator iditer = mBufferCache.find(fname);
    if(iditer != mBufferCache.end())
    {
        buf = iditer->second.mALBuffer;
        if(mBufferRefs[buf]++ == 0)
        {
            IDDq::iterator iter = std::find(mUnusedBuffers.begin(),
                                            mUnusedBuffers.end(), buf);
            if(iter != mUnusedBuffers.end())
                mUnusedBuffers.erase(iter);
        }

        return iditer->second;
    }
    throwALerror();

    std::vector<char> data;
    ChannelConfig chans;
    SampleType type;
    ALenum format;
    int srate;

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

    decoder->getInfo(&srate, &chans, &type);
    format = getALFormat(chans, type);

    decoder->readAll(data);
    decoder->close();

    CachedSound cached;
    analyzeLoudness(data, srate, chans, type, cached.mLoudnessVector, static_cast<float>(loudnessFPS));

    alGenBuffers(1, &buf);
    throwALerror();

    alBufferData(buf, format, &data[0], data.size(), srate);
    mBufferRefs[buf] = 1;
    cached.mALBuffer = buf;
    mBufferCache[fname] = cached;

    ALint bufsize = 0;
    alGetBufferi(buf, AL_SIZE, &bufsize);
    mBufferCacheMemSize += bufsize;

    // NOTE: Max buffer cache: 15MB
    while(mBufferCacheMemSize > 15*1024*1024)
    {
        if(mUnusedBuffers.empty())
        {
            std::cout <<"No more unused buffers to clear!"<< std::endl;
            break;
        }

        ALuint oldbuf = mUnusedBuffers.front();
        mUnusedBuffers.pop_front();

        NameMap::iterator nameiter = mBufferCache.begin();
        while(nameiter != mBufferCache.end())
        {
            if(nameiter->second.mALBuffer == oldbuf)
                mBufferCache.erase(nameiter++);
            else
                ++nameiter;
        }

        bufsize = 0;
        alGetBufferi(oldbuf, AL_SIZE, &bufsize);
        alDeleteBuffers(1, &oldbuf);
        mBufferCacheMemSize -= bufsize;
    }

    return mBufferCache[fname];
}

void OpenAL_Output::bufferFinished(ALuint buf)
{
    if(mBufferRefs.at(buf)-- == 1)
    {
        mBufferRefs.erase(mBufferRefs.find(buf));
        mUnusedBuffers.push_back(buf);
    }
}

MWBase::SoundPtr OpenAL_Output::playSound(const std::string &fname, float vol, float basevol, float pitch, int flags,float offset)
{
    boost::shared_ptr<OpenAL_Sound> sound;
    ALuint src=0, buf=0;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    try
    {
        buf = getBuffer(fname).mALBuffer;
        sound.reset(new OpenAL_Sound(*this, src, buf, osg::Vec3f(0.f, 0.f, 0.f), vol, basevol, pitch, 1.0f, 1000.0f, flags));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        if(buf && alIsBuffer(buf))
            bufferFinished(buf);
        alGetError();
        throw;
    }

    sound->updateAll(true);
    if(offset<0)
        offset=0;
    if(offset>1)
        offset=1;

    alSourcei(src, AL_BUFFER, buf);
    alSourcef(src, AL_SEC_OFFSET, static_cast<ALfloat>(sound->getLength()*offset / pitch));
    alSourcePlay(src);
    throwALerror();

    return sound;
}

MWBase::SoundPtr OpenAL_Output::playSound3D(const std::string &fname, const osg::Vec3f &pos, float vol, float basevol, float pitch,
                                            float min, float max, int flags, float offset, bool extractLoudness)
{
    boost::shared_ptr<OpenAL_Sound> sound;
    ALuint src=0, buf=0;

    if(mFreeSources.empty())
        fail("No free sources");
    src = mFreeSources.front();
    mFreeSources.pop_front();

    try
    {
        const CachedSound& cached = getBuffer(fname);
        buf = cached.mALBuffer;

        sound.reset(new OpenAL_Sound3D(*this, src, buf, pos, vol, basevol, pitch, min, max, flags));
        if (extractLoudness)
            sound->setLoudnessVector(cached.mLoudnessVector, static_cast<float>(loudnessFPS));
    }
    catch(std::exception&)
    {
        mFreeSources.push_back(src);
        if(buf && alIsBuffer(buf))
            bufferFinished(buf);
        alGetError();
        throw;
    }

    sound->updateAll(false);

    if(offset<0)
        offset=0;
    if(offset>1)
        offset=1;

    alSourcei(src, AL_BUFFER, buf);
    alSourcef(src, AL_SEC_OFFSET, static_cast<ALfloat>(sound->getLength()*offset / pitch));

    alSourcePlay(src);
    throwALerror();

    return sound;
}


MWBase::SoundPtr OpenAL_Output::streamSound(DecoderPtr decoder, float volume, float pitch, int flags)
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
        sound.reset(new OpenAL_SoundStream(*this, src, decoder, volume, pitch, flags));
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
    SoundVec::const_iterator iter = mActiveSounds.begin();
    while(iter != mActiveSounds.end())
    {
        const OpenAL_SoundStream *stream = dynamic_cast<OpenAL_SoundStream*>(*iter);
        if(stream)
        {
            if(stream->mSource && (stream->getPlayType()&types))
                sources.push_back(stream->mSource);
        }
        else
        {
            const OpenAL_Sound *sound = dynamic_cast<OpenAL_Sound*>(*iter);
            if(sound && sound->mSource && (sound->getPlayType()&types))
                sources.push_back(sound->mSource);
        }
        ++iter;
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
    SoundVec::const_iterator iter = mActiveSounds.begin();
    while(iter != mActiveSounds.end())
    {
        const OpenAL_SoundStream *stream = dynamic_cast<OpenAL_SoundStream*>(*iter);
        if(stream)
        {
            if(stream->mSource && (stream->getPlayType()&types))
                sources.push_back(stream->mSource);
        }
        else
        {
            const OpenAL_Sound *sound = dynamic_cast<OpenAL_Sound*>(*iter);
            if(sound && sound->mSource && (sound->getPlayType()&types))
                sources.push_back(sound->mSource);
        }
        ++iter;
    }
    if(!sources.empty())
    {
        alSourcePlayv(sources.size(), &sources[0]);
        throwALerror();
    }
}


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), mDevice(0), mContext(0), mBufferCacheMemSize(0),
    mLastEnvironment(Env_Normal), mStreamThread(new StreamThread)
{
}

OpenAL_Output::~OpenAL_Output()
{
    deinit();
}

}
