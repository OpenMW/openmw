#ifndef CSM_TOOLS_BODYPARTCHECK_H
#define CSM_TOOLS_BODYPARTCHECK_H

#include <components/esm/loadbody.hpp>
#include <components/esm/loadrace.hpp>

#include "../world/resources.hpp"
#include "../world/idcollection.hpp"

#include "../doc/stage.hpp"

namespace CSMTools
{
    /// \brief VerifyStage: make sure that body part records are internally consistent
    class BodyPartCheckStage : public CSMDoc::Stage
    {
        const CSMWorld::IdCollection<ESM::BodyPart> &mBodyParts;
        const CSMWorld::Resources                   &mMeshes;
        const CSMWorld::IdCollection<ESM::Race>     &mRaces;
        bool                                        mIgnoreBaseRecords;

    public:
        BodyPartCheckStage(
            const CSMWorld::IdCollection<ESM::BodyPart> &bodyParts,
            const CSMWorld::Resources                   &meshes,
            const CSMWorld::IdCollection<ESM::Race>     &races );

        int setup() override;
        ///< \return number of steps

        void perform(int stage, CSMDoc::Messages &messages) override;
        ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif
