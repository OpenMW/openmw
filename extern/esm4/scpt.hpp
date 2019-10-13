/*
  Copyright (C) 2019 cc9cii

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
Copyright(c)  Julien Valentin 2019
*/
#ifndef ESM4_SCPT_H
#define ESM4_SCPT_H

#include <string>
#include <vector>

#include "common.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    ////refering to  https://en.uesp.net/wiki/Tes4Mod:Mod_File_Format/SCPT
    struct Script
    {

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mText;
        struct SCHRstruct //CompiledScriptMetaData
        {
            int mUnknow0, mRefCount, mCompiledSize, mVariableCount, mScriptType;
        };
        enum ScriptType
        {
            OBJECT= 0x00000000,
            QUEST= 0x00000001,
            MAGICEFFECT= 0x00000100
        };
        SCHRstruct mCompiledScriptMetaData;

        struct SLSDstruct //LocalVarMetaData
        {
            int mIndex, mUnknow1, mUnknow2, mUnknow3,
            mVariableType, mUnknow5;
        };
        enum VariableType
        {
            FLOATORREF = 0x00000000, // Float of ref?
            LONG_OR_SHORT = 0x00000001, // Long or short (shorts are stored as longs)
            DEFAULT_VALUE = 0xCDCDCD00 // Default value (uninitialized data?) MY Null Ref?
        };
        std::vector<SLSDstruct> mLocalVarMetaDatas;
        std::vector<std::string> mLocalVarName;
        unsigned char *mSCDADataBuf; //Compiled Script Data (assuming no more than 4096 TODO copy constructor)
        int mSCDADataBufsize;
        std::vector<ESM4::FormId> mRefVariables;//seems to hold the ref variable index as stored in the variable's SLSD subrecord.
        std::vector<ESM4::FormId> mGlobalVars;// formid of the object ref used in script

        Script();
        Script(const Script &p2);
        virtual ~Script();

        virtual void load(ESM4::Reader& reader);
        virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_SCPT_H
