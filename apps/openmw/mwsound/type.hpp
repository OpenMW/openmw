#ifndef GAME_SOUND_TYPE_H
#define GAME_SOUND_TYPE_H

namespace MWSound
{
    enum class Type
    {
        Sfx   = 1 << 5, /* Normal SFX sound */
        Voice = 1 << 6, /* Voice sound */
        Foot  = 1 << 7, /* Footstep sound */
        Music = 1 << 8, /* Music track */
        Movie = 1 << 9, /* Movie audio track */
        Mask  = Sfx | Voice | Foot | Music | Movie
    };
}

#endif
