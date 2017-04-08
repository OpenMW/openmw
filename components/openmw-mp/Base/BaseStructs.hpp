#ifndef OPENMW_BASESTRUCTS_HPP
#define OPENMW_BASESTRUCTS_HPP

namespace mwmp
{
    struct Animation
    {
        std::string groupname;
        int mode;
        int count;
        bool persist;
    };

    struct AnimStates
    {
        int idlestate;
        int movestate;
        int jumpstate;
        bool forcestateupdate;
    };

    struct Movement
    {
        float x;
        float y;
        float z;
    };
}

#endif //OPENMW_BASESTRUCTS_HPP
