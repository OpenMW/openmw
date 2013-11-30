
#include "queststate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::QuestState::load (ESMReader &esm)
{
    mTopic = esm.getHNString ("YETO");
    esm.getHNOT (mState, "QSTAT");
    esm.getHNOT (mFinished, "QFIN");
}

void ESM::QuestState::save (ESMWriter &esm) const
{
    esm.writeHNString ("YETO", mTopic);
    esm.writeHNT ("QSTAT", mState);
    esm.writeHNT ("QFIN", mFinished);
}