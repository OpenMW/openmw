//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_STATS_HPP
#define OPENMW_STATS_HPP

#define STATSFUNCTIONS \
    {"GetName",             StatsFunctions::GetName},\
    {"SetName",             StatsFunctions::SetName},\
    \
    {"GetRace",             StatsFunctions::GetRace},\
    {"SetRace",             StatsFunctions::SetRace},\
    \
    {"SetClass",            StatsFunctions::SetClass},\
    {"GetClass",            StatsFunctions::GetClass},\
    \
    {"GetHead",             StatsFunctions::GetHead},\
    {"SetHead",             StatsFunctions::SetHead},\
    \
    {"GetHair",             StatsFunctions::GetHairstyle},\
    {"SetHair",             StatsFunctions::SetHairstyle},\
    \
    {"GetIsMale",           StatsFunctions::GetIsMale},\
    {"SetIsMale",           StatsFunctions::SetIsMale},\
    \
    {"GetBirthsign",        StatsFunctions::GetBirthsign},\
    {"SetBirthsign",        StatsFunctions::SetBirthsign},\
    \
    {"GetAttributeId",      StatsFunctions::GetAttributeId},\
    {"GetSkillId",          StatsFunctions::GetSkillId},\
    {"GetAttributeName",    StatsFunctions::GetAttributeName},\
    {"GetSkillName",        StatsFunctions::GetSkillName},\
    \
    {"GetAttribute",        StatsFunctions::GetAttribute},\
    {"SetAttribute",        StatsFunctions::SetAttribute},\
    {"GetCurrentAttribute", StatsFunctions::GetCurrentAttribute},\
    {"SetCurrentAttribute", StatsFunctions::SetCurrentAttribute},\
    {"GetSkill",            StatsFunctions::GetSkill},\
    {"SetSkill",            StatsFunctions::SetSkill},\
    {"GetCurrentSkill",     StatsFunctions::GetCurrentSkill},\
    {"SetCurrentSkill",     StatsFunctions::SetCurrentSkill},\
    \
    {"GetHealth",           StatsFunctions::GetHealth},\
    {"SetHealth",           StatsFunctions::SetHealth},\
    {"GetCurrentHealth",    StatsFunctions::GetCurrentHealth},\
    {"SetCurrentHealth",    StatsFunctions::SetCurrentHealth},\
    \
    {"GetMagicka",          StatsFunctions::GetMagicka},\
    {"SetMagicka",          StatsFunctions::SetMagicka},\
    {"GetCurrentMagicka",   StatsFunctions::GetCurrentMagicka},\
    {"SetCurrentMagicka",   StatsFunctions::SetCurrentMagicka},\
    \
    {"SetFatigue",          StatsFunctions::SetFatigue},\
    {"GetFatigue",          StatsFunctions::GetFatigue},\
    {"SetCurrentFatigue",   StatsFunctions::SetCurrentFatigue},\
    {"GetCurrentFatigue",   StatsFunctions::GetCurrentFatigue},\
    \
    {"GetIncreaseSkill",    StatsFunctions::GetIncreaseSkill},\
    {"SetIncreaseSkill",    StatsFunctions::SetIncreaseSkill},\
    {"SetCharGenStage",     StatsFunctions::SetCharGenStage},\
    {"Resurrect",           StatsFunctions::Resurrect},\
    {"SendBaseInfo",        StatsFunctions::SendBaseInfo},\
    {"SendAttributes",      StatsFunctions::SendAttributes},\
    {"SendBaseStats",       StatsFunctions::SendBaseStats},\
    {"SendSkills",          StatsFunctions::SendSkills}

class StatsFunctions
{
public:
    static void SetName(unsigned short pid, const char *name) noexcept;
    static const char *GetName(unsigned short pid) noexcept;

    static void SetBirthsign(unsigned short pid, const char *name) noexcept;
    static const char *GetBirthsign(unsigned short pid) noexcept;

    static void SetRace(unsigned short pid, const char *race) noexcept;
    static const char *GetRace(unsigned short pid) noexcept;

    static void SetClass(unsigned short pid, const char *name) noexcept;
    static const char *GetClass(unsigned short pid) noexcept;

    static void SetHead(unsigned short pid, const char *head) noexcept;
    static const char *GetHead(unsigned short pid) noexcept;

    static void SetHairstyle(unsigned short pid, const char *style) noexcept;
    static const char *GetHairstyle(unsigned short pid) noexcept;

    static void SetIsMale(unsigned short pid, int male) noexcept;
    static int GetIsMale(unsigned short pid) noexcept;

    static float GetHealth(unsigned short pid) noexcept;
    static void SetHealth(unsigned short pid, float health) noexcept;
    static float GetCurrentHealth(unsigned short pid) noexcept;
    static void SetCurrentHealth(unsigned short pid, float health) noexcept;

    static float GetMagicka(unsigned short pid) noexcept;
    static void SetMagicka(unsigned short pid, float magicka) noexcept;
    static float GetCurrentMagicka(unsigned short pid) noexcept;
    static void SetCurrentMagicka(unsigned short pid, float magicka) noexcept;

    static float GetFatigue(unsigned short pid) noexcept;
    static void SetFatigue(unsigned short pid, float fatigue) noexcept;
    static float GetCurrentFatigue(unsigned short pid) noexcept;
    static void SetCurrentFatigue(unsigned short pid, float fatigue) noexcept;

    static int GetAttributeId(const char *name) noexcept;
    static int GetSkillId(const char *name) noexcept;
    static const char *GetAttributeName(unsigned short attribute) noexcept;
    static const char *GetSkillName(unsigned short skill) noexcept;

    static int GetAttribute(unsigned short pid, unsigned short attribute) noexcept;
    static void SetAttribute(unsigned short pid, unsigned short attribute, int value) noexcept;
    static int GetCurrentAttribute(unsigned short pid, unsigned short attribute) noexcept;
    static void SetCurrentAttribute(unsigned short pid, unsigned short attribute, int value) noexcept;

    static int GetSkill(unsigned short pid, unsigned short skill) noexcept;
    static void SetSkill(unsigned short pid, unsigned short skill, int value) noexcept;
    static int GetCurrentSkill(unsigned short pid, unsigned short skill) noexcept;
    static void SetCurrentSkill(unsigned short pid, unsigned short skill, int value) noexcept;

    static int GetIncreaseSkill(unsigned short pid, unsigned int pos) noexcept;
    static void SetIncreaseSkill(unsigned short pid, unsigned int pos, int value) noexcept;

    static void Resurrect(unsigned short pid);
    static void SetCharGenStage(unsigned short pid, int start, int end) noexcept;

    static void SendBaseInfo(unsigned short pid) noexcept;
    static void SendAttributes(unsigned short pid) noexcept;
    static void SendBaseStats(unsigned short pid) noexcept;
    static void SendSkills(unsigned short pid) noexcept;
};

#endif //OPENMW_STATS_HPP
