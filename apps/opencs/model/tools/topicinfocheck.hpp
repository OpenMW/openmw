#ifndef CSM_TOOLS_TOPICINFOCHECK_HPP

#include <set>
#include <utility>

#include <components/esm/loadclas.hpp>
#include <components/esm/loaddial.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/loadglob.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadregn.hpp>

#include "../world/cell.hpp"
#include "../world/idcollection.hpp"
#include "../world/infocollection.hpp"
#include "../world/refiddata.hpp"
#include "../world/resources.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: check topics
    class TopicInfoCheckStage : public CSMDoc::Stage
    {
    public:
        
        TopicInfoCheckStage(const CSMWorld::InfoCollection& topicInfos,
                            const CSMWorld::InfoCollection& journalInfos,
                            const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
                            const CSMWorld::IdCollection<ESM::Class>& classes,
                            const CSMWorld::IdCollection<ESM::Faction>& factions,
                            const CSMWorld::IdCollection<ESM::GameSetting>& gmsts,
                            const CSMWorld::IdCollection<ESM::Global>& globals,
                            const CSMWorld::IdCollection<ESM::Dialogue>& journals,
                            const CSMWorld::IdCollection<ESM::Race>& races,
                            const CSMWorld::IdCollection<ESM::Region>& regions,
                            const CSMWorld::IdCollection<ESM::Dialogue>& topics,
                            const CSMWorld::RefIdData& referencables,
                            const CSMWorld::Resources& soundFiles);
        
        virtual int setup();
        ///< \return number of steps
        
        virtual void perform(int step, CSMDoc::Messages& messages);
        ///< Messages resulting from this stage will be appended to \a messages
        
    protected:
        
        const CSMWorld::InfoCollection& mTopicInfos;
        const CSMWorld::InfoCollection& mJournalInfos;
        
        const CSMWorld::IdCollection<CSMWorld::Cell>& mCells;
        const CSMWorld::IdCollection<ESM::Class>& mClasses;
        const CSMWorld::IdCollection<ESM::Faction>& mFactions;
        const CSMWorld::IdCollection<ESM::GameSetting>& mGameSettings;
        const CSMWorld::IdCollection<ESM::Global>& mGlobals;
        const CSMWorld::IdCollection<ESM::Dialogue>& mJournals;
        const CSMWorld::IdCollection<ESM::Race>& mRaces;
        const CSMWorld::IdCollection<ESM::Region>& mRegions;
        const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
        
        const CSMWorld::RefIdData& mReferencables;
        const CSMWorld::Resources& mSoundFiles;
        
        std::set<std::string> mCellNames;
        
        //  Checks
        
        bool checkActor(const std::string& actorName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkCell(const std::string& cellName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkClass(const std::string& className, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkFaction(const std::string& factionName, int rank,
                    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
        bool checkGlobal(const std::string& globalName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkItem(const std::string& itemName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkJournal(const std::string& journalName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool checkRace(const std::string& raceName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkResponse(const std::string& response, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkSelectStruct(const ESM::DialInfo::SelectStruct& selectStruct, 
                    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
        void checkSound(const std::string& soundFile, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkFunctionRange(const std::string& functionIndex, char op, const ESM::Variant& variant,
                    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
        void checkJournalRange(const std::string& journalName, char op, const ESM::Variant& variant,
                    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
        // Misc functions
        
        std::string getFunctionName(int code);
        
        std::pair<int,int> getRange(char op, int value, bool lower, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        bool rangeIntersects(std::pair<int,int> r1, std::pair<int,int> r2);
        
        void testIntRange(const std::pair<int,int>& range, char op, const ESM::Variant& variant,
                    const std::string& name, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
    };
}

#endif
