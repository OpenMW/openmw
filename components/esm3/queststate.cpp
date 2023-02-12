#include "queststate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void QuestState::load(ESMReader& esm)
    {
        mTopic = esm.getHNRefId("YETO");
        esm.getHNOT(mState, "QSTA");
        esm.getHNOT(mFinished, "QFIN");
    }

    void QuestState::save(ESMWriter& esm) const
    {
        esm.writeHNRefId("YETO", mTopic);
        esm.writeHNT("QSTA", mState);
        esm.writeHNT("QFIN", mFinished);
    }

}
