//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_STATS_HPP
#define OPENMW_STATS_HPP

#define STATSFUNCTIONS \
    {"getName",                 StatsFunctions::getName},\
    {"setName",                 StatsFunctions::setName},\
    \
    {"getRace",                 StatsFunctions::getRace},\
    {"setRace",                 StatsFunctions::setRace},\
    \
    {"getHead",                 StatsFunctions::getHead},\
    {"setHead",                 StatsFunctions::setHead},\
    \
    {"GetHair",                 StatsFunctions::getHairstyle},\
    {"SetHair",                 StatsFunctions::setHairstyle},\
    \
    {"getIsMale",               StatsFunctions::getIsMale},\
    {"setIsMale",               StatsFunctions::setIsMale},\
    \
    {"getLevel",                StatsFunctions::getLevel},\
    {"setLevel",                StatsFunctions::setLevel},\
    {"getLevelProgress",        StatsFunctions::getLevelProgress},\
    {"setLevelProgress",        StatsFunctions::setLevelProgress},\
    \
    {"getBirthsign",            StatsFunctions::getBirthsign},\
    {"setBirthsign",            StatsFunctions::setBirthsign},\
    \
    {"getAttributeCount",       StatsFunctions::getAttributeCount},\
    {"getSkillCount",           StatsFunctions::getSkillCount},\
    {"getAttributeId",          StatsFunctions::getAttributeId},\
    {"getSkillId",              StatsFunctions::getSkillId},\
    {"getAttributeName",        StatsFunctions::getAttributeName},\
    {"getSkillName",            StatsFunctions::getSkillName},\
    \
    {"getAttributeBase",        StatsFunctions::getAttributeBase},\
    {"setAttributeBase",        StatsFunctions::setAttributeBase},\
    {"getAttributeCurrent",     StatsFunctions::getAttributeCurrent},\
    {"setAttributeCurrent",     StatsFunctions::setAttributeCurrent},\
    \
    {"getSkillBase",            StatsFunctions::getSkillBase},\
    {"setSkillBase",            StatsFunctions::setSkillBase},\
    {"getSkillCurrent",         StatsFunctions::getSkillCurrent},\
    {"setSkillCurrent",         StatsFunctions::setSkillCurrent},\
    {"getSkillProgress",        StatsFunctions::getSkillProgress},\
    {"setSkillProgress",        StatsFunctions::setSkillProgress},\
    \
    {"getHealthBase",           StatsFunctions::getHealthBase},\
    {"setHealthBase",           StatsFunctions::setHealthBase},\
    {"getHealthCurrent",        StatsFunctions::getHealthCurrent},\
    {"setHealthCurrent",        StatsFunctions::setHealthCurrent},\
    \
    {"getMagickaBase",          StatsFunctions::getMagickaBase},\
    {"setMagickaBase",          StatsFunctions::setMagickaBase},\
    {"getMagickaCurrent",       StatsFunctions::getMagickaCurrent},\
    {"setMagickaCurrent",       StatsFunctions::setMagickaCurrent},\
    \
    {"setFatigueBase",          StatsFunctions::setFatigueBase},\
    {"getFatigueBase",          StatsFunctions::getFatigueBase},\
    {"setFatigueCurrent",       StatsFunctions::setFatigueCurrent},\
    {"getFatigueCurrent",       StatsFunctions::getFatigueCurrent},\
    \
    {"getSkillIncrease",        StatsFunctions::getSkillIncrease},\
    {"setSkillIncrease",        StatsFunctions::setSkillIncrease},\
    \
    {"setCharGenStage",         StatsFunctions::setCharGenStage},\
    {"resurrect",               StatsFunctions::resurrect},\
    {"sendBaseInfo",            StatsFunctions::sendBaseInfo},\
    \
    {"sendDynamicStats",        StatsFunctions::sendDynamicStats},\
    {"sendAttributes",          StatsFunctions::sendAttributes},\
    {"sendSkills",              StatsFunctions::sendSkills},\
    {"sendLevel",               StatsFunctions::sendLevel}

class StatsFunctions
{
public:
    static void setName(unsigned short pid, const char *name) noexcept;
    static const char *getName(unsigned short pid) noexcept;

    static void setBirthsign(unsigned short pid, const char *name) noexcept;
    static const char *getBirthsign(unsigned short pid) noexcept;

    static void setRace(unsigned short pid, const char *race) noexcept;
    static const char *getRace(unsigned short pid) noexcept;

    static void setHead(unsigned short pid, const char *head) noexcept;
    static const char *getHead(unsigned short pid) noexcept;

    static void setHairstyle(unsigned short pid, const char *style) noexcept;
    static const char *getHairstyle(unsigned short pid) noexcept;

    static void setIsMale(unsigned short pid, int male) noexcept;
    static int getIsMale(unsigned short pid) noexcept;

    static int getLevel(unsigned short pid) noexcept;
    static void setLevel(unsigned short pid, int value) noexcept;
    static int getLevelProgress(unsigned short pid) noexcept;
    static void setLevelProgress(unsigned short pid, int value) noexcept;

    static double getHealthBase(unsigned short pid) noexcept;
    static void setHealthBase(unsigned short pid, double value) noexcept;
    static double getHealthCurrent(unsigned short pid) noexcept;
    static void setHealthCurrent(unsigned short pid, double value) noexcept;

    static double getMagickaBase(unsigned short pid) noexcept;
    static void setMagickaBase(unsigned short pid, double value) noexcept;
    static double getMagickaCurrent(unsigned short pid) noexcept;
    static void setMagickaCurrent(unsigned short pid, double value) noexcept;

    static double getFatigueBase(unsigned short pid) noexcept;
    static void setFatigueBase(unsigned short pid, double value) noexcept;
    static double getFatigueCurrent(unsigned short pid) noexcept;
    static void setFatigueCurrent(unsigned short pid, double value) noexcept;

    static int getAttributeCount() noexcept;
    static int getSkillCount() noexcept;
    static int getAttributeId(const char *name) noexcept;
    static int getSkillId(const char *name) noexcept;
    static const char *getAttributeName(unsigned short attribute) noexcept;
    static const char *getSkillName(unsigned short skill) noexcept;

    static int getAttributeBase(unsigned short pid, unsigned short attribute) noexcept;
    static void setAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept;
    static int getAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept;
    static void setAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept;

    static int getSkillBase(unsigned short pid, unsigned short skill) noexcept;
    static void setSkillBase(unsigned short pid, unsigned short skill, int value) noexcept;
    static int getSkillCurrent(unsigned short pid, unsigned short skill) noexcept;
    static void setSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept;
    static double getSkillProgress(unsigned short pid, unsigned short skill) noexcept;
    static void setSkillProgress(unsigned short pid, unsigned short skill, double value) noexcept;

    static int getSkillIncrease(unsigned short pid, unsigned int pos) noexcept;
    static void setSkillIncrease(unsigned short pid, unsigned int pos, int value) noexcept;

    static void resurrect(unsigned short pid);
    static void setCharGenStage(unsigned short pid, int start, int end) noexcept;
    static void sendBaseInfo(unsigned short pid) noexcept;

    static void sendDynamicStats(unsigned short pid) noexcept;
    static void sendAttributes(unsigned short pid) noexcept;
    static void sendSkills(unsigned short pid) noexcept;
    static void sendLevel(unsigned short pid) noexcept;
};

#endif //OPENMW_STATS_HPP
