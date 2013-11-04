#ifndef CSM_DOC_STAGE_H
#define CSM_DOC_STAGE_H

#include <vector>
#include <string>

namespace CSMDoc
{
    class Stage
    {
        public:

            virtual ~Stage();

            virtual int setup() = 0;
            ///< \return number of steps

            virtual void perform (int stage, std::vector<std::string>& messages) = 0;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif

