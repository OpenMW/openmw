#ifndef OPENMW_ESM_SCPT_H
#define OPENMW_ESM_SCPT_H

#include <string>
#include <vector>

#include "esmcommon.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

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

        NAME32 mName;

        // These describe the sizes we need to allocate for the script
        // data.
        int mNumShorts, mNumLongs, mNumFloats, mScriptDataSize, mStringTableSize;
    }; // 52 bytes

    SCHDstruct mData;

    std::vector<std::string> mVarNames; // Variable names
    std::vector<char> mScriptData; // Compiled bytecode
    std::string mScriptText; // Uncompiled script

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
