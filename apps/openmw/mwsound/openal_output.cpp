#include <stdexcept>
#include <iostream>
#include <vector>

#include "openal_output.hpp"
#include "sound_decoder.hpp"
#include "sound.hpp"


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


ALuint LoadBuffer(std::auto_ptr<Sound_Decoder> decoder)
{
    int srate;
    Sound_Decoder::ChannelConfig chans;
    Sound_Decoder::SampleType type;
    ALenum format;

    decoder->GetInfo(&srate, &chans, &type);
    format = getALFormat(chans, type);

    std::vector<char> data(32768);
    size_t got, total = 0;
    while((got=decoder->Read(&data[total], data.size()-total)) > 0)
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
    static const ALuint NumBuffers = 150;
    static const ALuint BufferSize = 32768;

    ALuint Source;
    ALuint Buffers[NumBuffers];

    ALenum Format;
    ALsizei SampleRate;

    std::auto_ptr<Sound_Decoder> Decoder;

public:
    OpenAL_SoundStream(std::auto_ptr<Sound_Decoder> decoder);
    virtual ~OpenAL_SoundStream();

    void Play();
    virtual void Stop();
    virtual bool isPlaying();
};

class OpenAL_Sound : public Sound
{
public:
    ALuint Source;
    ALuint Buffer;

    OpenAL_Sound(ALuint src, ALuint buf);
    virtual ~OpenAL_Sound();

    virtual void Stop();
    virtual bool isPlaying();
};


OpenAL_SoundStream::OpenAL_SoundStream(std::auto_ptr<Sound_Decoder> decoder)
  : Decoder(decoder)
{
    throwALerror();

    alGenSources(1, &Source);
    throwALerror();
    try
    {
        alGenBuffers(NumBuffers, Buffers);
        throwALerror();
    }
    catch(std::exception &e)
    {
        alDeleteSources(1, &Source);
        alGetError();
        throw;
    }

    try
    {
        int srate;
        Sound_Decoder::ChannelConfig chans;
        Sound_Decoder::SampleType type;

        Decoder->GetInfo(&srate, &chans, &type);
        Format = getALFormat(chans, type);
        SampleRate = srate;
    }
    catch(std::exception &e)
    {
        alDeleteSources(1, &Source);
        alDeleteBuffers(NumBuffers, Buffers);
        alGetError();
        throw;
    }
}
OpenAL_SoundStream::~OpenAL_SoundStream()
{
    alDeleteSources(1, &Source);
    alDeleteBuffers(NumBuffers, Buffers);
    alGetError();
    Decoder->Close();
}

void OpenAL_SoundStream::Play()
{
    std::vector<char> data(BufferSize);

    alSourceStop(Source);
    alSourcei(Source, AL_BUFFER, 0);
    throwALerror();

    for(ALuint i = 0;i < NumBuffers;i++)
    {
        size_t got;
        got = Decoder->Read(&data[0], data.size());
        alBufferData(Buffers[i], Format, &data[0], got, SampleRate);
    }
    throwALerror();

    alSourceQueueBuffers(Source, NumBuffers, Buffers);
    alSourcePlay(Source);
    throwALerror();
}

void OpenAL_SoundStream::Stop()
{
    alSourceStop(Source);
    alSourcei(Source, AL_BUFFER, 0);
    throwALerror();
    // FIXME: Rewind decoder
}

bool OpenAL_SoundStream::isPlaying()
{
    ALint processed, state;

    alGetSourcei(Source, AL_SOURCE_STATE, &state);
    alGetSourcei(Source, AL_BUFFERS_PROCESSED, &processed);
    throwALerror();

    if(processed > 0)
    {
        std::vector<char> data(BufferSize);
        do {
            ALuint bufid;
            size_t got;

            alSourceUnqueueBuffers(Source, 1, &bufid);
            processed--;

            got = Decoder->Read(&data[0], data.size());
            if(got > 0)
            {
                alBufferData(bufid, Format, &data[0], got, SampleRate);
                alSourceQueueBuffers(Source, 1, &bufid);
            }
        } while(processed > 0);
        throwALerror();
    }

    if(state != AL_PLAYING && state != AL_PAUSED)
    {
        ALint queued;

        alGetSourcei(Source, AL_BUFFERS_QUEUED, &queued);
        throwALerror();
        if(queued == 0)
            return false;

        alSourcePlay(Source);
        throwALerror();
    }

    return true;
}


OpenAL_Sound::OpenAL_Sound(ALuint src, ALuint buf)
  : Source(src), Buffer(buf)
{
}
OpenAL_Sound::~OpenAL_Sound()
{
    alDeleteSources(1, &Source);
    alDeleteBuffers(1, &Buffer);
    alGetError();
}

void OpenAL_Sound::Stop()
{
    alSourceStop(Source);
    throwALerror();
}

bool OpenAL_Sound::isPlaying()
{
    ALint state;

    alGetSourcei(Source, AL_SOURCE_STATE, &state);
    throwALerror();

    return state==AL_PLAYING;
}


bool OpenAL_Output::Initialize(const std::string &devname)
{
    if(Context)
        fail("Device already initialized");

    Device = alcOpenDevice(devname.c_str());
    if(!Device)
    {
        std::cout << "Failed to open \""<<devname<<"\"" << std::endl;
        return false;
    }
    std::cout << "Opened \""<<alcGetString(Device, ALC_DEVICE_SPECIFIER)<<"\"" << std::endl;

    Context = alcCreateContext(Device, NULL);
    if(!Context || alcMakeContextCurrent(Context) == ALC_FALSE)
    {
        std::cout << "Failed to setup device context" << std::endl;
        if(Context)
            alcDestroyContext(Context);
        Context = 0;
        alcCloseDevice(Device);
        Device = 0;
        return false;
    }

    return true;
}

void OpenAL_Output::Deinitialize()
{
    alcMakeContextCurrent(0);
    if(Context)
        alcDestroyContext(Context);
    Context = 0;
    if(Device)
        alcCloseDevice(Device);
    Device = 0;
}


Sound* OpenAL_Output::PlaySound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder,
                                float volume, float pitch, bool loop)
{
    throwALerror();

    decoder->Open(fname);

    ALuint src=0, buf=0;
    try
    {
        buf = LoadBuffer(decoder);
        alGenSources(1, &src);
        throwALerror();
    }
    catch(std::exception &e)
    {
        if(alIsSource(buf))
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

Sound* OpenAL_Output::PlaySound3D(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder,
                                  MWWorld::Ptr ptr, float volume, float pitch,
                                  float min, float max, bool loop)
{
    throwALerror();

    decoder->Open(fname);

    ALuint src=0, buf=0;
    try
    {
        buf = LoadBuffer(decoder);
        alGenSources(1, &src);
        throwALerror();
    }
    catch(std::exception &e)
    {
        if(alIsSource(buf))
            alDeleteSources(1, &src);
        if(alIsBuffer(buf))
            alDeleteBuffers(1, &buf);
        alGetError();
        throw;
    }

    std::auto_ptr<OpenAL_Sound> sound(new OpenAL_Sound(src, buf));
    const float *pos = ptr.getCellRef().pos.pos;
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


Sound* OpenAL_Output::StreamSound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder)
{
    std::auto_ptr<OpenAL_SoundStream> sound;

    decoder->Open(fname);

    sound.reset(new OpenAL_SoundStream(decoder));
    sound->Play();

    return sound.release();
}


void OpenAL_Output::UpdateListener(float pos[3], float atdir[3], float updir[3])
{
    float orient[6] = { atdir[0], atdir[1], atdir[2], updir[0], updir[1], updir[2] };
    alListenerfv(AL_POSITION, pos);
    alListenerfv(AL_ORIENTATION, orient);
    throwALerror();
}


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), Device(0), Context(0)
{
}

OpenAL_Output::~OpenAL_Output()
{
    Deinitialize();
}

}
