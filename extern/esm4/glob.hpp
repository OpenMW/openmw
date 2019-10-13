/*
  Copyright (C) 2019 mp3butcher

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  mp3butcher@hotmail.com

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.
Copyright(c)  Julien Valentin 2019
*/
#ifndef ESM4_GLOB_H
#define ESM4_GLOB_H

#include <string>
#include <vector>

#include "common.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    ////refering to  https://en.uesp.net/wiki/Tes4Mod:Mod_File_Format/GLOB
    ////             https://en.uesp.net/wiki/Tes5Mod:Mod_File_Format/GLOB
    struct Global
    {

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        enum Type{
            Short = 0x73,
            Long = 0x6C,
            Char = 0x66
        };

        Type mType;
        float mValue;

        Global();
        virtual ~Global();

        virtual void load(ESM4::Reader& reader);
        virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_GLOB_H
