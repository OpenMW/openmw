#include <stdexcept>
#include <iostream>
#include <vector>

#include "openal_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"
#include "soundmanager.hpp"


namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("OpenAL exception: " + msg); }

static void throwALerror()
{
    ALenum err = alGetError();
    if(err != AL_NO_ERROR)
        fail(alGetString(err));
}


static ALenum getALFormat(Sound_Decoder::ChannelConfig chans, Sound_Decoder::SampleType type)
{
    if(chans == Sound_Decoder::MonoChannels)
    {
        if(type == Sound_Decoder::Int16Sample)
            return AL_FORMAT_MONO16;
        else if(type == Sound_Decoder::UInt8Sample)
            return AL_FORMAT_MONO8;
        else
            fail("Unsupported sample type");
    }
    else if(chans == Sound_Decoder::StereoChannels)
    {
        if(type == Sound_Decoder::Int16Sample)
            return AL_FORMAT_STEREO16;
        else if(type == Sound_Decoder::UInt8Sample)
            return AL_FORMAT_STEREO8;
        else
            fail("Unsupported sample type");
    }
    else
        fail("Unsupported channel config");
    return AL_NONE;
}


ALuint LoadBuffer(DecoderPtr decoder)
{
    int srate;
    Sound_Decoder::ChannelConfig chans;
    Sound_Decoder::SampleType type;
    ALenum format;

    decoder->getInfo(&srate, &chans, &type);
    format = getALFormat(chans, type);

    std::vector<char> data(32768);
    size_t got, total = 0;
    while((got=decoder->read(&data[total], data.size()-total)) > 0)
    {
        total += got;
        data.resize(total*2);
    }
    data.resize(total);

    ALuint buf;
    alGenBuffers(1, &buf);
    alBufferData(buf, format, &data[0], total, srate);
    return buf;
}


class OpenAL_SoundStream : public Sound
{
    // This should be something sane, like 4, but currently cell loads tend to
    // cause the stream to underrun
    static const ALuint sNumBuffers = 150;
    static const ALuint sBufferSize = 32768;

    ALuint mSource;
    ALuint mBuffers[sNumBuffers];

    ALenum mFormat;
    ALsizei mSampleRate;

    DecoderPtr mDecoder;

public:
    OpenAL_SoundStream(DecoderPtr decoder);
    virtual ~OpenAL_SoundStream();

    void Play(float volume, float pitch);
    virtual void Stop();
    virtual bool isPlaying();
    virtual void Update(const float *pos);
};

class OpenAL_Sound : public Sound
{
public:
    ALuint mSource;
    ALuint mBuffer;

    OpenAL_Sound(ALuint src, ALuint buf);
    virtual ~OpenAL_Sound();

    virtual void Stop();
    virtual bool isPlaying();
    virtual void Update(const float *pos);
};


OpenAL_SoundStream::OpenAL_SoundStream(DecoderPtr decoder)
  : mDecoder(decoder)
{
    throwALerror();

    alGenSources(1, &mSource);
    throwALerror();
    try
    {
        alGenBuffers(sNumBuffers, mBuffers);
        throwALerror();
    }
    catch(std::exception &e)
    {
        alDeleteSources(1, &mSource);
        alGetError();
        throw;
    }

    try
    {
        int srate;
        Sound_Decoder::ChannelConfig chans;
        Sound_Decoder::SampleType type;

        mDecoder->getInfo(&srate, &chans, &type);
        mFormat = getALFormat(chans, type);
        mSampleRate = srate;
    }
    catch(std::exception &e)
    {
        alDeleteSources(1, &mSource);
        alDeleteBuffers(sNumBuffers, mBuffers);
        alGetError();
        throw;
    }
}
OpenAL_SoundStream::~OpenAL_SoundStream()
{
    alDeleteSources(1, &mSource);
    alDeleteBuffers(sNumBuffers, mBuffers);
    alGetError();
    mDecoder->close();
}

void OpenAL_SoundStream::Play(float volume, float pitch)
{
    std::vector<char> data(sBufferSize);

    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    alSourcef(mSource, AL_GAIN, volume);
    alSourcef(mSource, AL_PITCH, pitch);
    throwALerror();

    for(ALuint i = 0;i < sNumBuffers;i++)
    {
        size_t got;
        got = mDecoder->read(&data[0], data.size());
        alBufferData(mBuffers[i], mFormat, &data[0], got, mSampleRate);
    }
    throwALerror();

    alSourceQueueBuffers(mSource, sNumBuffers, mBuffers);
    alSourcePlay(mSource);
    throwALerror();
}

void OpenAL_SoundStream::Stop()
{
    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    throwALerror();
    // FIXME: Rewind decoder
}

bool OpenAL_SoundStream::isPlaying()
{
    ALint processed, state;

    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
    throwALerror();

    if(processed > 0)
    {
        std::vector<char> data(sBufferSize);
        do {
            ALuint bufid;
            size_t got;

            alSourceUnqueueBuffers(mSource, 1, &bufid);
            processed--;

            got = mDecoder->read(&data[0], data.size());
            if(got > 0)
            {
                alBufferData(bufid, mFormat, &data[0], got, mSampleRate);
                alSourceQueueBuffers(mSource, 1, &bufid);
            }
        } while(processed > 0);
        throwALerror();
    }

    if(state != AL_PLAYING && state != AL_PAUSED)
    {
        ALint queued;

        alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
        throwALerror();
        if(queued == 0)
            return false;

        alSourcePlay(mSource);
        throwALerror();
    }

    return true;
}

void OpenAL_SoundStream::Update(const float *pos)
{
    alSource3f(mSource, AL_POSITION, pos[0], pos[2], -pos[1]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
}


OpenAL_Sound::OpenAL_Sound(ALuint src, ALuint buf)
  : mSource(src), mBuffer(buf)
{
}
OpenAL_Sound::~OpenAL_Sound()
{
    alDeleteSources(1, &mSource);
    alDeleteBuffers(1, &mBuffer);
    alGetError();
}

void OpenAL_Sound::Stop()
{
    alSourceStop(mSource);
    throwALerror();
}

bool OpenAL_Sound::isPlaying()
{
    ALint state;

    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    throwALerror();

    return state==AL_PLAYING;
}

void OpenAL_Sound::Update(const float *pos)
{
    alSource3f(mSource, AL_POSITION, pos[0], pos[2], -pos[1]);
    alSource3f(mSource, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(mSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    throwALerror();
}


bool OpenAL_Output::init(const std::string &devname)
{
    if(mContext)
        fail("Device already initialized");

    mDevice = alcOpenDevice(devname.c_str());
    if(!mDevice)
    {
        std::cout << "Failed to open \""<<devname<<"\"" << std::endl;
        return false;
    }
    std::cout << "Opened \""<<alcGetString(mDevice, ALC_DEVICE_SPECIFIER)<<"\"" << std::endl;

    mContext = alcCreateContext(mDevice, NULL);
    if(!mContext || alcMakeContextCurrent(mContext) == ALC_FALSE)
    {
        std::cout << "Failed to setup device context" << std::endl;
        if(mContext)
            alcDestroyContext(mContext);
        mContext = 0;
        alcCloseDevice(mDevice);
        mDevice = 0;
        return false;
    }
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    throwALerror();

    return true;
}

void OpenAL_Output::deinit()
{
    alcMakeContextCurrent(0);
    if(mContext)
        alcDestroyContext(mContext);
    mContext = 0;
    if(mDevice)
        alcCloseDevice(mDevice);
    mDevice = 0;
}


Sound* OpenAL_Output::playSound(const std::string &fname, float volume, float pitch, bool loop)
{
    throwALerror();

    DecoderPtr decoder = mManager.getDecoder();
    decoder->open(fname);

    ALuint src=0, buf=0;
    try
    {
        buf = LoadBuffer(decoder);
        decoder->close();
        alGenSources(1, &src);
        throwALerror();
    }
    catch(std::exception &e)
    {
        if(alIsSource(src))
            alDeleteSources(1, &src);
        if(alIsBuffer(buf))
            alDeleteBuffers(1, &buf);
        alGetError();
        throw;
    }

    std::auto_ptr<OpenAL_Sound> sound(new OpenAL_Sound(src, buf));
    alSource3f(src, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(src, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(src, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    alSourcef(src, AL_REFERENCE_DISTANCE, 1.0f);
    alSourcef(src, AL_MAX_DISTANCE, 1000.0f);
    alSourcef(src, AL_ROLLOFF_FACTOR, 0.0f);

    alSourcef(src, AL_GAIN, volume);
    alSourcef(src, AL_PITCH, pitch);

    alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(src, AL_LOOPING, (loop?AL_TRUE:AL_FALSE));
    throwALerror();

    alSourcei(src, AL_BUFFER, buf);
    alSourcePlay(src);
    throwALerror();

    return sound.release();
}

Sound* OpenAL_Output::playSound3D(const std::string &fname, const float *pos, float volume, float pitch,
                                  float min, float max, bool loop)
{
    throwALerror();

    DecoderPtr decoder = mManager.getDecoder();
    decoder->open(fname);

    ALuint src=0, buf=0;
    try
    {
        buf = LoadBuffer(decoder);
        decoder->close();
        alGenSources(1, &src);
        throwALerror();
    }
    catch(std::exception &e)
    {
        if(alIsSource(src))
            alDeleteSources(1, &src);
        if(alIsBuffer(buf))
            alDeleteBuffers(1, &buf);
        alGetError();
        throw;
    }

    std::auto_ptr<OpenAL_Sound> sound(new OpenAL_Sound(src, buf));
    alSource3f(src, AL_POSITION, pos[0], pos[2], -pos[1]);
    alSource3f(src, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(src, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    alSourcef(src, AL_REFERENCE_DISTANCE, min);
    alSourcef(src, AL_MAX_DISTANCE, max);
    alSourcef(src, AL_ROLLOFF_FACTOR, 1.0f);

    alSourcef(src, AL_GAIN, volume);
    alSourcef(src, AL_PITCH, pitch);

    alSourcei(src, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcei(src, AL_LOOPING, (loop?AL_TRUE:AL_FALSE));
    throwALerror();

    alSourcei(src, AL_BUFFER, buf);
    alSourcePlay(src);
    throwALerror();

    return sound.release();
}


Sound* OpenAL_Output::streamSound(const std::string &fname, float volume, float pitch)
{
    std::auto_ptr<OpenAL_SoundStream> sound;

    DecoderPtr decoder = mManager.getDecoder();
    decoder->open(fname);

    sound.reset(new OpenAL_SoundStream(decoder));
    sound->Play(volume, pitch);

    return sound.release();
}


void OpenAL_Output::updateListener(const float *pos, const float *atdir, const float *updir)
{
    float orient[6] = {
        atdir[0], atdir[2], -atdir[1],
        updir[0], updir[2], -updir[1]
    };

    alListener3f(AL_POSITION, pos[0], pos[2], -pos[1]);
    alListenerfv(AL_ORIENTATION, orient);
    throwALerror();
}


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), mDevice(0), mContext(0)
{
}

OpenAL_Output::~OpenAL_Output()
{
    deinit();
}

}
