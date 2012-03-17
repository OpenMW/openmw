#include "openal_output.hpp"

namespace MWSound
{

static void fail(const std::string &msg)
{ throw std::runtime_error("OpenAL exception: " + msg); }


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


OpenAL_Output::OpenAL_Output(SoundManager &mgr)
  : Sound_Output(mgr), Device(0), Context(0)
{
}

OpenAL_Output::~OpenAL_Output()
{
    Deinitialize();
}

}
