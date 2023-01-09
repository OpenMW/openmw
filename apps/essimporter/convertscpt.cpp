#include "convertscpt.hpp"

#include "convertscri.hpp"

#include <components/misc/strings/lower.hpp>

namespace ESSImport
{

    void convertSCPT(const SCPT& scpt, ESM::GlobalScript& out)
    {
        out.mId = ESM::RefId::stringRefId(scpt.mSCHD.mName.toString());
        out.mRunning = scpt.mRunning;
        out.mTargetRef = ESM::RefNum{}; // TODO: convert target reference of global script
        convertSCRI(scpt.mSCRI, out.mLocals);
    }

}
