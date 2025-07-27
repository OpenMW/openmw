#include "convertscpt.hpp"

#include "convertscri.hpp"

#include <components/misc/strings/lower.hpp>

namespace ESSImport
{

    void convertSCPT(const SCPT& scpt, ESM::GlobalScript& out)
    {
        out.mId = ESM::RefId::stringRefId(scpt.mSCHD.mName.toString());
        out.mRunning = scpt.mRunning;
        out.mTargetRef = scpt.mRefNum;
        convertSCRI(scpt.mSCRI, out.mLocals);
    }

}
