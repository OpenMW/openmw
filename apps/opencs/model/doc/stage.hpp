#ifndef CSM_DOC_STAGE_H
#define CSM_DOC_STAGE_H

namespace CSMDoc
{
    class Messages;
    class Stage
    {
    public:
        virtual ~Stage() = default;

        virtual int setup() = 0;
        ///< \return number of steps

        virtual void perform(int stage, Messages& messages) = 0;
        ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
