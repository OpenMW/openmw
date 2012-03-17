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

    virtual bool Play();
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

bool OpenAL_SoundStream::Play()
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

    return true;
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


Sound* OpenAL_Output::StreamSound(const std::string &fname, std::auto_ptr<Sound_Decoder> decoder)
{
    std::auto_ptr<OpenAL_SoundStream> sound;

    if(!decoder->Open(fname))
        return NULL;

    sound.reset(new OpenAL_SoundStream(decoder));
    sound->Play();

    return sound.release();
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
