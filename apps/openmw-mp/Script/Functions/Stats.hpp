//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_STATS_HPP
#define OPENMW_STATS_HPP

#define STATSFUNCTIONS \
    {"GetName",                 StatsFunctions::GetName},\
    {"SetName",                 StatsFunctions::SetName},\
    \
    {"GetRace",                 StatsFunctions::GetRace},\
    {"SetRace",                 StatsFunctions::SetRace},\
    \
    {"GetHead",                 StatsFunctions::GetHead},\
    {"SetHead",                 StatsFunctions::SetHead},\
    \
    {"GetHair",                 StatsFunctions::GetHairstyle},\
    {"SetHair",                 StatsFunctions::SetHairstyle},\
    \
    {"GetIsMale",               StatsFunctions::GetIsMale},\
    {"SetIsMale",               StatsFunctions::SetIsMale},\
    \
    {"GetLevel",                StatsFunctions::GetLevel},\
    {"SetLevel",                StatsFunctions::SetLevel},\
    {"GetLevelProgress",        StatsFunctions::GetLevelProgress},\
    {"SetLevelProgress",        StatsFunctions::SetLevelProgress},\
    \
    {"GetBirthsign",            StatsFunctions::GetBirthsign},\
    {"SetBirthsign",            StatsFunctions::SetBirthsign},\
    \
    {"GetAttributeCount",       StatsFunctions::GetAttributeCount},\
    {"GetSkillCount",           StatsFunctions::GetSkillCount},\
    {"GetAttributeId",          StatsFunctions::GetAttributeId},\
    {"GetSkillId",              StatsFunctions::GetSkillId},\
    {"GetAttributeName",        StatsFunctions::GetAttributeName},\
    {"GetSkillName",            StatsFunctions::GetSkillName},\
    \
    {"GetAttributeBase",        StatsFunctions::GetAttributeBase},\
    {"SetAttributeBase",        StatsFunctions::SetAttributeBase},\
    {"GetAttributeCurrent",     StatsFunctions::GetAttributeCurrent},\
    {"SetAttributeCurrent",     StatsFunctions::SetAttributeCurrent},\
    \
    {"GetSkillBase",            StatsFunctions::GetSkillBase},\
    {"SetSkillBase",            StatsFunctions::SetSkillBase},\
    {"GetSkillCurrent",         StatsFunctions::GetSkillCurrent},\
    {"SetSkillCurrent",         StatsFunctions::SetSkillCurrent},\
    {"GetSkillProgress",        StatsFunctions::GetSkillProgress},\
    {"SetSkillProgress",        StatsFunctions::SetSkillProgress},\
    \
    {"GetHealthBase",           StatsFunctions::GetHealthBase},\
    {"SetHealthBase",           StatsFunctions::SetHealthBase},\
    {"GetHealthCurrent",        StatsFunctions::GetHealthCurrent},\
    {"SetHealthCurrent",        StatsFunctions::SetHealthCurrent},\
    \
    {"GetMagickaBase",          StatsFunctions::GetMagickaBase},\
    {"SetMagickaBase",          StatsFunctions::SetMagickaBase},\
    {"GetMagickaCurrent",       StatsFunctions::GetMagickaCurrent},\
    {"SetMagickaCurrent",       StatsFunctions::SetMagickaCurrent},\
    \
    {"SetFatigueBase",          StatsFunctions::SetFatigueBase},\
    {"GetFatigueBase",          StatsFunctions::GetFatigueBase},\
    {"SetFatigueCurrent",       StatsFunctions::SetFatigueCurrent},\
    {"GetFatigueCurrent",       StatsFunctions::GetFatigueCurrent},\
    \
    {"GetSkillIncrease",        StatsFunctions::GetSkillIncrease},\
    {"SetSkillIncrease",        StatsFunctions::SetSkillIncrease},\
    \
    {"SetCharGenStage",         StatsFunctions::SetCharGenStage},\
    {"Resurrect",               StatsFunctions::Resurrect},\
    {"SendBaseInfo",            StatsFunctions::SendBaseInfo},\
    \
    {"SendDynamicStats",        StatsFunctions::SendDynamicStats},\
    {"SendAttributes",          StatsFunctions::SendAttributes},\
    {"SendSkills",              StatsFunctions::SendSkills},\
    {"SendLevel",               StatsFunctions::SendLevel}

class StatsFunctions
{
public:
    static void SetName(unsigned short pid, const char *name) noexcept;
    static const char *GetName(unsigned short pid) noexcept;

    static void SetBirthsign(unsigned short pid, const char *name) noexcept;
    static const char *GetBirthsign(unsigned short pid) noexcept;

    static void SetRace(unsigned short pid, const char *race) noexcept;
    static const char *GetRace(unsigned short pid) noexcept;

    static void SetHead(unsigned short pid, const char *head) noexcept;
    static const char *GetHead(unsigned short pid) noexcept;

    static void SetHairstyle(unsigned short pid, const char *style) noexcept;
    static const char *GetHairstyle(unsigned short pid) noexcept;

    static void SetIsMale(unsigned short pid, int male) noexcept;
    static int GetIsMale(unsigned short pid) noexcept;

    static int GetLevel(unsigned short pid) noexcept;
    static void SetLevel(unsigned short pid, int value) noexcept;
    static int GetLevelProgress(unsigned short pid) noexcept;
    static void SetLevelProgress(unsigned short pid, int value) noexcept;

    static double GetHealthBase(unsigned short pid) noexcept;
    static void SetHealthBase(unsigned short pid, double value) noexcept;
    static double GetHealthCurrent(unsigned short pid) noexcept;
    static void SetHealthCurrent(unsigned short pid, double value) noexcept;

    static double GetMagickaBase(unsigned short pid) noexcept;
    static void SetMagickaBase(unsigned short pid, double value) noexcept;
    static double GetMagickaCurrent(unsigned short pid) noexcept;
    static void SetMagickaCurrent(unsigned short pid, double value) noexcept;

    static double GetFatigueBase(unsigned short pid) noexcept;
    static void SetFatigueBase(unsigned short pid, double value) noexcept;
    static double GetFatigueCurrent(unsigned short pid) noexcept;
    static void SetFatigueCurrent(unsigned short pid, double value) noexcept;

    static int GetAttributeCount() noexcept;
    static int GetSkillCount() noexcept;
    static int GetAttributeId(const char *name) noexcept;
    static int GetSkillId(const char *name) noexcept;
    static const char *GetAttributeName(unsigned short attribute) noexcept;
    static const char *GetSkillName(unsigned short skill) noexcept;

    static int GetAttributeBase(unsigned short pid, unsigned short attribute) noexcept;
    static void SetAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept;
    static int GetAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept;
    static void SetAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept;

    static int GetSkillBase(unsigned short pid, unsigned short skill) noexcept;
    static void SetSkillBase(unsigned short pid, unsigned short skill, int value) noexcept;
    static int GetSkillCurrent(unsigned short pid, unsigned short skill) noexcept;
    static void SetSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept;
    static double GetSkillProgress(unsigned short pid, unsigned short skill) noexcept;
    static void SetSkillProgress(unsigned short pid, unsigned short skill, double value) noexcept;

    static int GetSkillIncrease(unsigned short pid, unsigned int pos) noexcept;
    static void SetSkillIncrease(unsigned short pid, unsigned int pos, int value) noexcept;

    static void Resurrect(unsigned short pid);
    static void SetCharGenStage(unsigned short pid, int start, int end) noexcept;
    static void SendBaseInfo(unsigned short pid) noexcept;

    static void SendDynamicStats(unsigned short pid) noexcept;
    static void SendAttributes(unsigned short pid) noexcept;
    static void SendSkills(unsigned short pid) noexcept;
    static void SendLevel(unsigned short pid) noexcept;
};

#endif //OPENMW_STATS_HPP
