#ifndef OPENMW_ACTORFUNCTIONS_HPP
#define OPENMW_ACTORFUNCTIONS_HPP

#define ACTORAPI \
    {"InitActorList",          ActorFunctions::InitActorList},\
    \
    {"GetActorListSize",       ActorFunctions::GetActorListSize}

class ActorFunctions
{
public:

    static void InitActorList(unsigned short pid) noexcept;

    static unsigned int GetActorListSize() noexcept;
};


#endif //OPENMW_ACTORFUNCTIONS_HPP
