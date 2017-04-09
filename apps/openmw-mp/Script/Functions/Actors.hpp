#ifndef OPENMW_ACTORAPI_HPP
#define OPENMW_ACTORAPI_HPP

#define ACTORAPI \
    {"InitActorList",          ActorFunctions::InitActorList},\
    \
    {"GetActorListSize",       ActorFunctions::GetActorListSize},\
    \
    {"SendActorList",          ActorFunctions::SendActorList},\
    {"SendActorAuthority",     ActorFunctions::SendActorAuthority}\

class ActorFunctions
{
public:

    static void InitActorList(unsigned short pid) noexcept;

    static unsigned int GetActorListSize() noexcept;

    static void SendActorList() noexcept;
    static void SendActorAuthority() noexcept;
};


#endif //OPENMW_ACTORAPI_HPP
