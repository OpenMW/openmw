#ifndef OPENMW_MWMECHANICS_AISTATEFWD_H
#define OPENMW_MWMECHANICS_AISTATEFWD_H

namespace MWMechanics
{
    template <class Base>
    class DerivedClassStorage;

    struct AiTemporaryBase;

    /// \brief Container for AI package status.
    using AiState = DerivedClassStorage<AiTemporaryBase>;
}

#endif
