//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_STATAPI_HPP
#define OPENMW_STATAPI_HPP

#define STATAPI \
    {"GetAttributeCount",       StatsFunctions::GetAttributeCount},\
    {"GetSkillCount",           StatsFunctions::GetSkillCount},\
    {"GetAttributeId",          StatsFunctions::GetAttributeId},\
    {"GetSkillId",              StatsFunctions::GetSkillId},\
    {"GetAttributeName",        StatsFunctions::GetAttributeName},\
    {"GetSkillName",            StatsFunctions::GetSkillName},\
    \
    {"GetName",                 StatsFunctions::GetName},\
    {"GetRace",                 StatsFunctions::GetRace},\
    {"GetHead",                 StatsFunctions::GetHead},\
    {"GetHair",                 StatsFunctions::GetHairstyle},\
    {"GetIsMale",               StatsFunctions::GetIsMale},\
    {"GetBirthsign",            StatsFunctions::GetBirthsign},\
    {"GetCreatureModel",        StatsFunctions::GetCreatureModel},\
    {"IsCreatureName",          StatsFunctions::IsCreatureName},\
    {"GetDeathReason",          StatsFunctions::GetDeathReason},\
    \
    {"GetLevel",                StatsFunctions::GetLevel},\
    {"GetLevelProgress",        StatsFunctions::GetLevelProgress},\
    \
    {"GetHealthBase",           StatsFunctions::GetHealthBase},\
    {"GetHealthCurrent",        StatsFunctions::GetHealthCurrent},\
    \
    {"GetMagickaBase",          StatsFunctions::GetMagickaBase},\
    {"GetMagickaCurrent",       StatsFunctions::GetMagickaCurrent},\
    \
    {"GetFatigueBase",          StatsFunctions::GetFatigueBase},\
    {"GetFatigueCurrent",       StatsFunctions::GetFatigueCurrent},\
    \
    {"GetAttributeBase",        StatsFunctions::GetAttributeBase},\
    {"GetAttributeCurrent",     StatsFunctions::GetAttributeCurrent},\
    \
    {"GetSkillBase",            StatsFunctions::GetSkillBase},\
    {"GetSkillCurrent",         StatsFunctions::GetSkillCurrent},\
    {"GetSkillProgress",        StatsFunctions::GetSkillProgress},\
    {"GetSkillIncrease",        StatsFunctions::GetSkillIncrease},\
    \
    {"GetBounty",               StatsFunctions::GetBounty},\
    \
    {"SetName",                 StatsFunctions::SetName},\
    {"SetRace",                 StatsFunctions::SetRace},\
    {"SetHead",                 StatsFunctions::SetHead},\
    {"SetHair",                 StatsFunctions::SetHairstyle},\
    {"SetIsMale",               StatsFunctions::SetIsMale},\
    {"SetBirthsign",            StatsFunctions::SetBirthsign},\
    {"SetCreatureModel",        StatsFunctions::SetCreatureModel},\
    \
    {"SetLevel",                StatsFunctions::SetLevel},\
    {"SetLevelProgress",        StatsFunctions::SetLevelProgress},\
    \
    {"SetHealthBase",           StatsFunctions::SetHealthBase},\
    {"SetHealthCurrent",        StatsFunctions::SetHealthCurrent},\
    {"SetMagickaBase",          StatsFunctions::SetMagickaBase},\
    {"SetMagickaCurrent",       StatsFunctions::SetMagickaCurrent},\
    {"SetFatigueBase",          StatsFunctions::SetFatigueBase},\
    {"SetFatigueCurrent",       StatsFunctions::SetFatigueCurrent},\
    \
    {"SetAttributeBase",        StatsFunctions::SetAttributeBase},\
    {"SetAttributeCurrent",     StatsFunctions::SetAttributeCurrent},\
    \
    {"SetSkillBase",            StatsFunctions::SetSkillBase},\
    {"SetSkillCurrent",         StatsFunctions::SetSkillCurrent},\
    {"SetSkillProgress",        StatsFunctions::SetSkillProgress},\
    {"SetSkillIncrease",        StatsFunctions::SetSkillIncrease},\
    \
    {"SetBounty",               StatsFunctions::SetBounty},\
    {"SetCharGenStage",         StatsFunctions::SetCharGenStage},\
    \
    {"Resurrect",               StatsFunctions::Resurrect},\
    {"SendBaseInfo",            StatsFunctions::SendBaseInfo},\
    \
    {"SendStatsDynamic",        StatsFunctions::SendStatsDynamic},\
    {"SendAttributes",          StatsFunctions::SendAttributes},\
    {"SendSkills",              StatsFunctions::SendSkills},\
    {"SendLevel",               StatsFunctions::SendLevel},\
    {"SendBounty",              StatsFunctions::SendBounty}

class StatsFunctions
{
public:
    static int GetAttributeCount() noexcept;
    static int GetSkillCount() noexcept;
    static int GetAttributeId(const char *name) noexcept;
    static int GetSkillId(const char *name) noexcept;
    static const char *GetAttributeName(unsigned short attribute) noexcept;
    static const char *GetSkillName(unsigned short skill) noexcept;

    static const char *GetName(unsigned short pid) noexcept;
    static const char *GetRace(unsigned short pid) noexcept;
    static const char *GetHead(unsigned short pid) noexcept;
    static const char *GetHairstyle(unsigned short pid) noexcept;
    static int GetIsMale(unsigned short pid) noexcept;
    static const char *GetBirthsign(unsigned short pid) noexcept;
    static const char *GetCreatureModel(unsigned short pid) noexcept;
    static bool IsCreatureName(unsigned short pid) noexcept;
    static const char *GetDeathReason(unsigned short pid) noexcept;

    static int GetLevel(unsigned short pid) noexcept;
    static int GetLevelProgress(unsigned short pid) noexcept;

    static double GetHealthBase(unsigned short pid) noexcept;
    static double GetHealthCurrent(unsigned short pid) noexcept;
    static double GetMagickaBase(unsigned short pid) noexcept;
    static double GetMagickaCurrent(unsigned short pid) noexcept;
    static double GetFatigueBase(unsigned short pid) noexcept;
    static double GetFatigueCurrent(unsigned short pid) noexcept;

    static int GetAttributeBase(unsigned short pid, unsigned short attribute) noexcept;
    static int GetAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept;

    static int GetSkillBase(unsigned short pid, unsigned short skill) noexcept;
    static int GetSkillCurrent(unsigned short pid, unsigned short skill) noexcept;
    static double GetSkillProgress(unsigned short pid, unsigned short skill) noexcept;
    static int GetSkillIncrease(unsigned short pid, unsigned int pos) noexcept;

    static int GetBounty(unsigned short pid) noexcept;

    static void SetName(unsigned short pid, const char *name) noexcept;
    static void SetRace(unsigned short pid, const char *race) noexcept;
    static void SetHead(unsigned short pid, const char *head) noexcept;
    static void SetHairstyle(unsigned short pid, const char *style) noexcept;
    static void SetIsMale(unsigned short pid, int male) noexcept;
    static void SetBirthsign(unsigned short pid, const char *name) noexcept;
    static void SetCreatureModel(unsigned short pid, const char *name, bool useCreatureName) noexcept;
    
    static void SetLevel(unsigned short pid, int value) noexcept;
    static void SetLevelProgress(unsigned short pid, int value) noexcept;

    static void SetHealthBase(unsigned short pid, double value) noexcept;    
    static void SetHealthCurrent(unsigned short pid, double value) noexcept;
    static void SetMagickaBase(unsigned short pid, double value) noexcept;
    static void SetMagickaCurrent(unsigned short pid, double value) noexcept;
    static void SetFatigueBase(unsigned short pid, double value) noexcept;
    static void SetFatigueCurrent(unsigned short pid, double value) noexcept;

    static void SetAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept;
    static void SetAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept;

    static void SetSkillBase(unsigned short pid, unsigned short skill, int value) noexcept;    
    static void SetSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept;
    static void SetSkillProgress(unsigned short pid, unsigned short skill, double value) noexcept;
    static void SetSkillIncrease(unsigned short pid, unsigned int pos, int value) noexcept;

    static void SetBounty(unsigned short pid, int value) noexcept;
    static void SetCharGenStage(unsigned short pid, int start, int end) noexcept;

    static void Resurrect(unsigned short pid);
    static void SendBaseInfo(unsigned short pid) noexcept;

    static void SendStatsDynamic(unsigned short pid) noexcept;
    static void SendAttributes(unsigned short pid) noexcept;
    static void SendSkills(unsigned short pid) noexcept;
    static void SendLevel(unsigned short pid) noexcept;
    static void SendBounty(unsigned short pid) noexcept;
};

#endif //OPENMW_STATAPI_HPP
