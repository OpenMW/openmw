#include <cstring>
#include <vector>
#include <memory>

#include <apps/openmw/mwsound/alext.h>

#include "openalutil.hpp"

#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
#endif

std::vector<const char *> Launcher::enumerateOpenALDevices()
{
    std::vector<const char *> devlist;
    const ALCchar *devnames;

    if(alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
    {
        devnames = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    }
    else
    {
        devnames = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
    }
    
    while(devnames && *devnames)
    {
        devlist.emplace_back(devnames);
        devnames += strlen(devnames)+1;
    }
    return devlist;
}

std::vector<const char *> Launcher::enumerateOpenALDevicesHrtf()
{
    std::vector<const char *> ret;

    ALCdevice *device = alcOpenDevice(nullptr);
    if(device)
    {
        if(alcIsExtensionPresent(device, "ALC_SOFT_HRTF"))
        {
            LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
            void* funcPtr = alcGetProcAddress(device, "alcGetStringiSOFT");
            memcpy(&alcGetStringiSOFT, &funcPtr, sizeof(funcPtr));
            ALCint num_hrtf;
            alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
            ret.reserve(num_hrtf);
            for(ALCint i = 0;i < num_hrtf;++i)
            {
                const ALCchar *entry = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
                if(strcmp(entry, "") == 0)
                    break;
                ret.emplace_back(entry);
            }
        }
        alcCloseDevice(device);
    }
    return ret;
}
