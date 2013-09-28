#ifndef _ESM_SCPT_H
#define _ESM_SCPT_H

#include "esm_reader.hpp"

namespace ESM
{

/*
 * Script definitions
 */

class Script
{
public:
    struct SCHDstruct
    {
        /* Script name.

         NOTE: You should handle the name "Main" (case insensitive) with
         care. With tribunal, modders got the ability to add 'start
         scripts' to their mods, which is a script that is run at
         startup and which runs throughout the game (I think.)

         However, before Tribunal, there was only one startup script,
         called "Main". If mods wanted to make their own start scripts,
         they had to overwrite Main. This is obviously problem if
         multiple mods to this at the same time.

         Although most mods have switched to using Trib-style startup
         scripts, some legacy mods might still overwrite Main, and this
         can cause problems if several mods do it. I think the best
         course of action is to NEVER overwrite main, but instead add
         each with a separate unique name and add them to the start
         script list. But there might be other problems with this
         approach though.
         */

        NAME32 name;

        // These describe the sizes we need to allocate for the script
        // data.
        int numShorts, numLongs, numFloats, scriptDataSize, stringTableSize;
    }; // 52 bytes

    SCHDstruct data;

    std::vector<std::string> varNames; // Variable names
    std::vector<char> scriptData; // Compiled bytecode
    std::string scriptText; // Uncompiled script

    void load(ESMReader &esm);
};
}
#endif
