#ifndef OPENMW_MWMECHANICS_AISTATE_H
#define OPENMW_MWMECHANICS_AISTATE_H

namespace MWMechanics
{
    /// \brief base class for the temporary storage of AiPackages.
    /**
     * Each AI package with temporary values needs a AiPackageStorage class
     * which is derived from AiTemporaryBase. The Actor holds a container
     * AiState where one of these storages can be stored at a time.
     * The execute(...) member function takes this container as an argument.
     * */
    struct AiTemporaryBase
    {
        virtual ~AiTemporaryBase() = default;
    };
}

#endif
