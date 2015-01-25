#include "convertscpt.hpp"

#include <components/misc/stringops.hpp>

#include "convertscri.hpp"

namespace ESSImport
{

    void convertSCPT(const SCPT &scpt, ESM::GlobalScript &out)
    {
        out.mId = Misc::StringUtils::lowerCase(scpt.mSCHD.mName.toString());
        out.mRunning = scpt.mRunning;
        convertSCRI(scpt.mSCRI, out.mLocals);
    }

}
