#ifndef _ESM_DEFS_AI_H
#define _ESM_DEFS_AI_H

#include "esm_reader.hpp"

#include <vector>

namespace ESM
{
    #pragma pack(push)
    #pragma pack(1)

    struct AIDTstruct
    {
        // These are probabilities
        char hello, u1, fight, flee, alarm, u2, u3, u4;
        // The last u's might be the skills that this NPC can train you
        // in?
        int services; // See the Services enum
    }; // 12 bytes

    struct AIWander
    {
        short   distance;
        short   duration;
        char    timeOfDay;
        char    idle[8];
        char    unk;
    };

    struct AITravel
    {
        float   x, y, z;
        long    unk;
    };

    struct AITarget
    {
        float   x, y, z;
        short   duration;
        NAME32  id;
        short   unk;
    };

    struct AIActivate
    {
        NAME32  name;
        char    unk;
    };

    #pragma pack(pop)

    enum
    {
        AI_Wander = 0x575f4941,
        AI_Travel = 0x545f4941,
        AI_Follow = 0x465f4941,
        AI_Escort = 0x455f4941,
        AI_Activate = 0x415f4941,
    };

    /// \note Used for storaging packages in a single container
    /// w/o manual memory allocation accordingly to policy standards
    struct AIPackage
    {
        int type;

        // Anonymous union
        union
        {
            AIWander wander;
            AITravel travel;
            AITarget target;
            AIActivate activate;
        };

        /// \note for AITarget only, placed here to stick with union,
        /// overhead should be not so awful
        std::string cellName;
    };

    struct AIPackageList
    {
        std::vector<AIPackage> list;

        /// \note This breaks consistency of subrecords reading:
        /// after calling it subrecord name is already read, so
        /// it needs to use retSubName() if needed. But, hey, there
        /// is only one field left (XSCL) and only two records uses AI
        void load(ESMReader &esm) {
            while (esm.hasMoreSubs()) {
                // initialize every iteration
                AIPackage pack;

                esm.getSubName();
                std::cout << esm.retSubName().toString() << std::endl;
                if (esm.retSubName() == 0x54444e43) { // CNDT
                    list.back().cellName = esm.getHString();
                } else if (esm.retSubName() == AI_Wander) {
                    pack.type = AI_Wander;
                    esm.getHExact(&pack.wander, 14);
                    list.push_back(pack);
                } else if (esm.retSubName() == AI_Travel) {
                    pack.type = AI_Travel;
                    esm.getHExact(&pack.travel, 16);
                    list.push_back(pack);
                } else if (esm.retSubName() == AI_Escort ||
                           esm.retSubName() == AI_Follow)
                {
                    pack.type =
                        (esm.retSubName() == AI_Escort) ? AI_Escort : AI_Follow;

                    esm.getHExact(&pack.target, 48);
                    list.push_back(pack);
                } else if (esm.retSubName() == AI_Activate) {
                    pack.type = AI_Activate;
                    esm.getHExact(&pack.activate, 33);
                    list.push_back(pack);
                } else { // not AI package related data, so leave
                     return;
                }
            }
        }
    };
}

#endif

