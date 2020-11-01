#ifndef CSM_TOOLS_MANDATORYID_H
#define CSM_TOOLS_MANDATORYID_H

#include <string>
#include <vector>

#include "../world/universalid.hpp"

#include "../doc/stage.hpp"

namespace CSMWorld
{
    class CollectionBase;
}

namespace CSMTools
{
    /// \brief Verify stage: make sure that records with specific IDs exist.
    class MandatoryIdStage : public CSMDoc::Stage
    {
            const CSMWorld::CollectionBase& mIdCollection;
            CSMWorld::UniversalId mCollectionId;
            std::vector<std::string> mIds;

        public:

            MandatoryIdStage (const CSMWorld::CollectionBase& idCollection, const CSMWorld::UniversalId& collectionId,
                const std::vector<std::string>& ids);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, CSMDoc::Messages& messages) override;
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
