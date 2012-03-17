#ifdef OPENMW_USE_MPG123

#include "mpgsnd_decoder.hpp"


namespace MWSound
{

bool MpgSnd_Decoder::Open(const std::string &fname)
{
    return false;
}

void MpgSnd_Decoder::Close()
{
}


MpgSnd_Decoder::MpgSnd_Decoder()
{
    static bool initdone = false;
    if(!initdone)
        mpg123_init();
    initdone = true;
}

MpgSnd_Decoder::~MpgSnd_Decoder()
{
}

}

#endif
