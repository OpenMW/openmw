#ifndef OPENMW_BASESTRUCTS_HPP
#define OPENMW_BASESTRUCTS_HPP

#include <components/esm/statstate.hpp>

#include <RakNetTypes.h>

namespace mwmp
{
    struct Target
    {
        std::string refId;
        int refNumIndex;
        int mpNum;

        RakNet::RakNetGUID guid;
    };

    class Attack
    {
    public:

        Target target;

        char type; // 0 - melee, 1 - magic, 2 - throwable
        enum TYPE
        {
            MELEE = 0,
            MAGIC,
            THROWABLE
        };

        std::string spellId; // id of spell (e.g. "fireball")

        float damage;

        bool success;
        bool block;
        
        bool pressed;
        bool knockdown;

        bool shouldSend;
    };

    struct Animation
    {
        std::string groupname;
        int mode;
        int count;
        bool persist;
    };

    struct SimpleCreatureStats
    {
        ESM::StatState<float> mDynamic[3];
    };
}

#endif //OPENMW_BASESTRUCTS_HPP
