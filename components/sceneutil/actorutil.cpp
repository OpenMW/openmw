#include "actorutil.hpp"

namespace SceneUtil
{
    std::string getActorSkeleton(bool firstPerson, bool isFemale, bool isBeast, bool isWerewolf)
    {
        if (!firstPerson)
        {
            if (isWerewolf)
                return "meshes\\wolf\\skin.nif";
            else if (isBeast)
                return "meshes\\base_animkna.nif";
            else if (isFemale)
                return "meshes\\base_anim_female.nif";
            else
                return "meshes\\base_anim.nif";
        }
        else
        {
            if (isWerewolf)
                return "meshes\\wolf\\skin.1st.nif";
            else if (isBeast)
                return "meshes\\base_animkna.1st.nif";
            else if (isFemale)
                return "meshes\\base_anim_female.1st.nif";
            else
                return "meshes\\base_anim.1st.nif";
        }
    }
}
