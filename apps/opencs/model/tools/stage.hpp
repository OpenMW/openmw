#ifndef CSM_TOOLS_STAGE_H
#define CSM_TOOLS_STAGE_H

#include <vector>
#include <string>

namespace CSMTools
{
    class Stage
    {
        public:

            virtual ~Stage();

            virtual int setup() = 0;
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages) = 0;
            ///< Messages resulting from this tage will be appended to \a messages.
    };
}

#endif

