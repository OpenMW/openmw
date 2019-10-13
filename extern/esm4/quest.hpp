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
#ifndef ESM4_QUST_H
#define ESM4_QUST_H

#include <vector>
#include <string>

#include "common.hpp"
#include "ctda.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Quest
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mQuestName, mScriptResult;
        struct LogEntry{
            LogEntry():mLogEntryFlag(0),unk(0){}
            ~LogEntry(){}
            char mLogEntryFlag;
            std::vector<Condition> conditions;
            std::string mLogString;
            ESM4::FormId unk;
        };
        struct Target{
            ESM4::FormId mTargetAlias;
            std::uint32_t mFlags;
            std::vector<Condition> mConditions;
        };
        struct QuestStage
        {
            void reset(){ mLogEntries.clear(); }
            std::uint16_t mIndex;
            std::vector<LogEntry> mLogEntries;
        };
        struct QuestData{ char mPriority,mQuestFlags; };

        QuestData mQuestData;
        ESM4::FormId mScript;
        std::vector<ESM4::FormId> mGlobalVars;
        std::string mIconFileName;
        //TES>4
        std::uint32_t  mEvent, mQuestDialogCondition;

        LogEntry *mCurrentLogEntry;
        QuestStage *mCurrentStage;
        Target *mCurrentTarget;
        std::vector<QuestStage> mStages;
        std::vector<Condition> mQuestConditions;
        std::vector<Target> mTargets;

        Quest();
        virtual ~Quest();

        virtual void load(ESM4::Reader& reader);
        virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };


}

#endif // ESM4_QUST_H
