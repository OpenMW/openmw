#ifndef OPENMW_ACTORAPI_HPP
#define OPENMW_ACTORAPI_HPP

#define ACTORAPI \
    {"InitScriptActorList",        ActorFunctions::InitScriptActorList},\
    \
    {"GetActorListSize",           ActorFunctions::GetActorListSize},\
    {"GetLastActorListAction",     ActorFunctions::GetLastActorListAction},\
    \
    {"GetActorCell",               ActorFunctions::GetActorCell},\
    {"GetActorRefId",              ActorFunctions::GetActorRefId},\
    {"GetActorRefNumIndex",        ActorFunctions::GetActorRefNumIndex},\
    {"GetActorMpNum",              ActorFunctions::GetActorMpNum},\
    \
    {"GetActorPosX",               ActorFunctions::GetActorPosX},\
    {"GetActorPosY",               ActorFunctions::GetActorPosY},\
    {"GetActorPosZ",               ActorFunctions::GetActorPosZ},\
    {"GetActorRotX",               ActorFunctions::GetActorRotX},\
    {"GetActorRotY",               ActorFunctions::GetActorRotY},\
    {"GetActorRotZ",               ActorFunctions::GetActorRotZ},\
    \
    {"GetActorHealthBase",         ActorFunctions::GetActorHealthBase},\
    {"GetActorHealthCurrent",      ActorFunctions::GetActorHealthCurrent},\
    {"GetActorMagickaBase",        ActorFunctions::GetActorMagickaBase},\
    {"GetActorMagickaCurrent",     ActorFunctions::GetActorMagickaCurrent},\
    {"GetActorFatigueBase",        ActorFunctions::GetActorFatigueBase},\
    {"GetActorFatigueCurrent",     ActorFunctions::GetActorFatigueCurrent},\
    \
    {"SetScriptActorListCell",     ActorFunctions::SetScriptActorListCell},\
    {"SetScriptActorListAction",   ActorFunctions::SetScriptActorListAction},\
    \
    {"SetActorCell",               ActorFunctions::SetActorCell},\
    {"SetActorRefId",              ActorFunctions::SetActorRefId},\
    {"SetActorRefNumIndex",        ActorFunctions::SetActorRefNumIndex},\
    {"SetActorMpNum",              ActorFunctions::SetActorMpNum},\
    {"SetActorPosition",           ActorFunctions::SetActorPosition},\
    {"SetActorRotation",           ActorFunctions::SetActorRotation},\
    \
    {"AddActor",                   ActorFunctions::AddActor},\
    \
    {"SendActorList",              ActorFunctions::SendActorList},\
    {"SendActorAuthority",         ActorFunctions::SendActorAuthority},\
    {"SendActorCellChange",        ActorFunctions::SendActorCellChange}

class ActorFunctions
{
public:

    static void InitScriptActorList(unsigned short pid) noexcept;

    static unsigned int GetActorListSize() noexcept;
    static unsigned char GetLastActorListAction() noexcept;

    static const char *GetActorCell(unsigned int i) noexcept;
    static const char *GetActorRefId(unsigned int i) noexcept;
    static int GetActorRefNumIndex(unsigned int i) noexcept;
    static int GetActorMpNum(unsigned int i) noexcept;

    static double GetActorPosX(unsigned int i) noexcept;
    static double GetActorPosY(unsigned int i) noexcept;
    static double GetActorPosZ(unsigned int i) noexcept;
    static double GetActorRotX(unsigned int i) noexcept;
    static double GetActorRotY(unsigned int i) noexcept;
    static double GetActorRotZ(unsigned int i) noexcept;

    static double GetActorHealthBase(unsigned int i) noexcept;
    static double GetActorHealthCurrent(unsigned int i) noexcept;
    static double GetActorMagickaBase(unsigned int i) noexcept;
    static double GetActorMagickaCurrent(unsigned int i) noexcept;
    static double GetActorFatigueBase(unsigned int i) noexcept;
    static double GetActorFatigueCurrent(unsigned int i) noexcept;

    static void SetScriptActorListCell(const char* cellDescription) noexcept;
    static void SetScriptActorListAction(unsigned char action) noexcept;

    static void SetActorCell(const char* cellDescription) noexcept;
    static void SetActorRefId(const char* refId) noexcept;
    static void SetActorRefNumIndex(int refNumIndex) noexcept;
    static void SetActorMpNum(int mpNum) noexcept;
    static void SetActorPosition(double x, double y, double z) noexcept;
    static void SetActorRotation(double x, double y, double z) noexcept;

    static void AddActor() noexcept;

    static void SendActorList() noexcept;
    static void SendActorAuthority() noexcept;
    static void SendActorCellChange() noexcept;
};


#endif //OPENMW_ACTORAPI_HPP
