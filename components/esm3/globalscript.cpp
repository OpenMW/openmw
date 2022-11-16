#include "globalscript.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm3/cellref.hpp>
#include <components/esm3/locals.hpp>

namespace ESM
{

    void GlobalScript::load(ESMReader& esm)
    {
        mId = ESM::RefId::stringRefId(esm.getHNString("NAME"));

        mLocals.load(esm);

        mRunning = 0;
        esm.getHNOT(mRunning, "RUN_");

        mTargetRef.unset();
        mTargetId = ESM::RefId::stringRefId(esm.getHNOString("TARG"));
        if (esm.peekNextSub("FRMR"))
            mTargetRef.load(esm, true, "FRMR");
    }

    void GlobalScript::save(ESMWriter& esm) const
    {
        esm.writeHNString("NAME", mId.getRefIdString());

        mLocals.save(esm);

        if (mRunning)
            esm.writeHNT("RUN_", mRunning);

        if (!mTargetId.empty())
        {
            esm.writeHNOString("TARG", mTargetId.getRefIdString());
            if (mTargetRef.isSet())
                mTargetRef.save(esm, true, "FRMR");
        }
    }

}
