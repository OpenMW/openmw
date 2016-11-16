//
// Created by koncord on 29.08.16.
//

#ifndef OPENMW_CHARCLASS_HPP
#define OPENMW_CHARCLASS_HPP

#include "../Types.hpp"

#define CHARCLASSFUNCTIONS \
{"setDefaultClass",        CharClassFunctions::setDefaultClass},\
{"setClassName",           CharClassFunctions::setClassName},\
{"setClassDesc",           CharClassFunctions::setClassDesc},\
{"setClassMajorAttribute", CharClassFunctions::setClassMajorAttribute},\
{"setClassSpecialization", CharClassFunctions::setClassSpecialization},\
{"setClassMajorSkill",     CharClassFunctions::setClassMajorSkill},\
{"setClassMinorSkill",     CharClassFunctions::setClassMinorSkill},\
{"getDefaultClass",        CharClassFunctions::getDefaultClass},\
{"getClassName",           CharClassFunctions::getClassName},\
{"getClassDesc",           CharClassFunctions::getClassDesc},\
{"getClassMajorAttribute", CharClassFunctions::getClassMajorAttribute},\
{"getClassSpecialization", CharClassFunctions::getClassSpecialization},\
{"getClassMajorSkill",     CharClassFunctions::getClassMajorSkill},\
{"getClassMinorSkill",     CharClassFunctions::getClassMinorSkill},\
{"isClassDefault",         CharClassFunctions::isClassDefault},\
{"sendClass",              CharClassFunctions::sendClass}


class CharClassFunctions
{
public:
    CharClassFunctions() {}
    static void setDefaultClass(unsigned short pid, const char *id) noexcept;
    static void setClassName(unsigned short pid, const char *name) noexcept;
    static void setClassDesc(unsigned short pid, const char *desc) noexcept;
    /**
     * \param pid
     * \param slot 0 = first, 1 = second
     * \param attrId
     */
    static void setClassMajorAttribute(unsigned short pid, unsigned char slot, int attrId) noexcept;
    /**
     * \param pid
     * \param spec 0 = Combat, 1 = Magic, 2 = Stealth
     */
    static void setClassSpecialization(unsigned short pid, int spec) noexcept;
    /**
     * \param pid
     * \param slot 0 to 4
     * \param skillId
     */
    static void setClassMajorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept;
    static void setClassMinorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept;

    static const char *getDefaultClass(unsigned short pid) noexcept;
    static const char *getClassName(unsigned short pid) noexcept;
    static const char *getClassDesc(unsigned short pid) noexcept;
    /**
     * \param pid
     * \param slot 0 = first, 1 = second
     * \return attrId
     */
    static int getClassMajorAttribute(unsigned short pid, unsigned char slot) noexcept;
    /**
     * \param pid
     * \return spec 0 = Combat, 1 = Magic, 2 = Stealth
     */
    static int getClassSpecialization(unsigned short pid) noexcept;
    /**
     * \param pid
     * \param slot 0 to 4
     * \return skillId
     */
    static int getClassMajorSkill(unsigned short pid, unsigned char slot) noexcept;
    static int getClassMinorSkill(unsigned short pid, unsigned char slot) noexcept;

    static int isClassDefault(unsigned short pid) noexcept;

    static void sendClass(unsigned short pid) noexcept;
};

#endif //OPENMW_CHARCLASS_HPP
