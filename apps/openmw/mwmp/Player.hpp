//
// Created by koncord on 02.01.16.
//

#ifndef OPENMW_PLAYER_HPP
#define OPENMW_PLAYER_HPP

#include <components/esm/loadnpc.hpp>
#include <apps/openmw/mwworld/manualref.hpp>
#include <map>
#include <apps/openmw/mwmechanics/aisequence.hpp>

namespace mwmp
{
    class Player
    {
    public:
        MWWorld::Ptr getPtr();
        MWWorld::Ptr getLiveCellPtr();
        MWWorld::ManualRef* getRef();
        void Move(ESM::Position pos, MWWorld::CellStore* cell);

        static void CreatePlayer(int id, const std::string& name, const std::string &race, const std::string &head, const std::string &hair);
        static void DestroyPlayer(int id);
        static void CleanUp();
        static Player *GetPlayer(int id);

    private:
        Player();
        ~Player();
        int id;
        bool active;
        MWWorld::ManualRef* reference;
        MWWorld::Ptr ptr;

    private:
        static std::map <int, Player*> players;
    };
}
#endif //OPENMW_PLAYER_HPP
