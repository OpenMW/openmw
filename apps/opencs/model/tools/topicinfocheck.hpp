#ifndef CSM_TOOLS_TOPICINFOCHECK_HPP

#include <set>

#include <components/esm/loadclas.hpp>
#include <components/esm/loaddial.hpp>
#include <components/esm/loadfact.hpp>
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
                            const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
                            const CSMWorld::IdCollection<ESM::Class>& classes,
                            const CSMWorld::IdCollection<ESM::Faction>& factions,
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
        
        const CSMWorld::IdCollection<CSMWorld::Cell>& mCells;
        const CSMWorld::IdCollection<ESM::Class>& mClasses;
        const CSMWorld::IdCollection<ESM::Faction>& mFactions;
        const CSMWorld::IdCollection<ESM::Race>& mRaces;
        const CSMWorld::IdCollection<ESM::Region>& mRegions;
        const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
        
        const CSMWorld::RefIdData& mReferencables;
        const CSMWorld::Resources& mSoundFiles;
        
        std::set<std::string> mCellNames;
        
        
        void checkActor(const std::string& actorName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkCell(const std::string& cellName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkClass(const std::string& className, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkFaction(const std::string& factionName, int rank,
                    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages);
        
        void checkRace(const std::string& raceName, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkResponse(const std::string& response, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
        
        void checkSound(const std::string& soundFile, const CSMWorld::UniversalId& id,
                    CSMDoc::Messages& messages);
    };
}

#endif