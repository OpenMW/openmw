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

*/
#ifndef ESM4_INFO_H
#define ESM4_INFO_H

#include <vector>
#include <string>

#include "common.hpp"
#include "ctda.hpp"

namespace ESM4
{
    class Reader;
    class Writer;
    class Dialog;

    struct Info
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::uint32_t mINFOogSubtype;
        struct Data
        {
            bool mUnknow0;
            std::uint8_t mINFOogTab, mSubTypeID, mUnused;
        };
        struct Emotion{
             std::uint32_t mType,mValue;
             char unk0[4];
             char mResponseNumber;
             char unk1[3];
        };
    struct Response{
        Emotion mEmotion; std::string mResponseText,mActorNotes;
    };
    std::vector<Response> mResponse;
    Response* mCurrentResponse;
    struct DialogChoice
    {
        Condition mCond;
        ESM4::FormId mChoiceID;
    };



    std::vector<ESM4::FormId> mAddTopic;
     std::vector<Condition> mConditions;
    DialogChoice* mCurrentChoice;
    std::vector<DialogChoice> mChoices;
    struct InfoData{ char mDialogType; std::uint16_t mDialogFlags; };
    InfoData mDialogData;
    // Result script (uncompiled) to run whenever this dialog item is
    // selected
    std::string mResultScript;



        FormId mQuestId, mPreviousInfoId;
        float mPriority;
        std::string mPlayerINFOog;
        Data mINFOogData;
        std::uint32_t mInfoCount; //count of topic INFO subrecords;

        Info();
        virtual ~Info();

        virtual void load(ESM4::Reader& reader);
        virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_INFO_H
