#ifndef CSM_TOOLS_STAGE_H
#define CSM_TOOLS_STAGE_H

namespace CSMTools
{
    class Stage
    {
            public:

                virtual ~Stage();

                virtual int setup() = 0;
                ///< \return number of steps

                virtual void perform (int stage) = 0;
    };
}

#endif

