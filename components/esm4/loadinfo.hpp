/*
  Copyright (C) 2020 cc9cii

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

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_INFO_H
#define ESM4_INFO_H

#include <cstdint>
#include <string>

#include "formid.hpp"
#include "script.hpp" // TargetCondition
#include "dialogue.hpp" // DialType

namespace ESM4
{
    class Reader;
    class Writer;

    enum InfoFlag
    {
        INFO_Goodbye         = 0x0001,
        INFO_Random          = 0x0002,
        INFO_SayOnce         = 0x0004,
        INFO_RunImmediately  = 0x0008,
        INFO_InfoRefusal     = 0x0010,
        INFO_RandomEnd       = 0x0020,
        INFO_RunForRumors    = 0x0040,
        INFO_SpeechChallenge = 0x0080,
        INFO_SayOnceADay     = 0x0100,
        INFO_AlwaysDarken    = 0x0200
    };

    struct DialogInfo
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId; // FIXME: no such record for INFO, but keep here to avoid extra work for now

        FormId mQuest;
        FormId mSound; // unused?

        TargetResponseData mResponseData;
        std::string mResponse;
        std::string mNotes;
        std::string mEdits;

        std::uint8_t  mDialType;  // DialType
        std::uint8_t  mNextSpeaker;
        std::uint16_t mInfoFlags; // see above enum

        TargetCondition mTargetCondition;
        FormId mParam3; // TES5 only

        ScriptDefinition mScript; // FIXME: ignoring the second one after the NEXT sub-record

        DialogInfo();
        virtual ~DialogInfo();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_INFO_H
