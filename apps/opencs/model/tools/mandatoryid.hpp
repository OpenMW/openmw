#ifndef CSM_TOOLS_MANDATORYID_H
#define CSM_TOOLS_MANDATORYID_H

#include <string>
#include <vector>

#include "../world/universalid.hpp"

#include "stage.hpp"

namespace CSMWorld
{
    class IdCollectionBase;
}

namespace CSMTools
{
    /// \brief Verify stage: make sure that records with specific IDs exist.
    class MandatoryIdStage : public Stage
    {
            const CSMWorld::IdCollectionBase& mIdCollection;
            CSMWorld::UniversalId mCollectionId;
            std::vector<std::string> mIds;

        public:

            MandatoryIdStage (const CSMWorld::IdCollectionBase& idCollection, const CSMWorld::UniversalId& collectionId,
                const std::vector<std::string>& ids);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages);
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
