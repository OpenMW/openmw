//
// Created by koncord on 29.08.16.
//

#ifndef OPENMW_CHARCLASS_HPP
#define OPENMW_CHARCLASS_HPP

#include "../Types.hpp"

#define CHARCLASSFUNCTIONS \
{"SetDefaultClass",     CharClassFunctions::SetDefaultClass},\
{"SetClassName",        CharClassFunctions::SetClassName},\
{"SetClassDesc",        CharClassFunctions::SetClassDesc},\
{"SetClassMajorAttribute", CharClassFunctions::SetClassMajorAttribute},\
{"SetClassSpecialization", CharClassFunctions::SetClassSpecialization},\
{"SetClassMajorSkill", CharClassFunctions::SetClassMajorSkill},\
{"SetClassMinorSkill", CharClassFunctions::SetClassMinorSkill},\
{"GetDefaultClass",     CharClassFunctions::GetDefaultClass},\
{"GetClassName",        CharClassFunctions::GetClassName},\
{"GetClassDesc",        CharClassFunctions::GetClassDesc},\
{"GetClassMajorAttribute", CharClassFunctions::GetClassMajorAttribute},\
{"GetClassSpecialization", CharClassFunctions::GetClassSpecialization},\
{"GetClassMajorSkill", CharClassFunctions::GetClassMajorSkill},\
{"GetClassMinorSkill", CharClassFunctions::GetClassMinorSkill},\
{"IsClassDefault", CharClassFunctions::IsClassDefault},\
{"SendClass",           CharClassFunctions::SendClass}


class CharClassFunctions
{
public:
    CharClassFunctions() {}
    static void SetDefaultClass(unsigned short pid, const char *id) noexcept;
    static void SetClassName(unsigned short pid, const char *name) noexcept;
    static void SetClassDesc(unsigned short pid, const char *desc) noexcept;
    /**
     * \param pid
     * \param slot 0 = first, 1 = second
     * \param attrId
     */
    static void SetClassMajorAttribute(unsigned short pid, unsigned char slot, int attrId) noexcept;
    /**
     * \param pid
     * \param spec 0 = Combat, 1 = Magic, 2 = Stealth
     */
    static void SetClassSpecialization(unsigned short pid, int spec) noexcept;
    /**
     * \param pid
     * \param slot 0 to 4
     * \param skillId
     */
    static void SetClassMajorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept;
    static void SetClassMinorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept;

    static const char *GetDefaultClass(unsigned short pid) noexcept;
    static const char *GetClassName(unsigned short pid) noexcept;
    static const char *GetClassDesc(unsigned short pid) noexcept;
    /**
     * \param pid
     * \param slot 0 = first, 1 = second
     * \return attrId
     */
    static int GetClassMajorAttribute(unsigned short pid, unsigned char slot) noexcept;
    /**
     * \param pid
     * \return spec 0 = Combat, 1 = Magic, 2 = Stealth
     */
    static int GetClassSpecialization(unsigned short pid) noexcept;
    /**
     * \param pid
     * \param slot 0 to 4
     * \return skillId
     */
    static int GetClassMajorSkill(unsigned short pid, unsigned char slot) noexcept;
    static int GetClassMinorSkill(unsigned short pid, unsigned char slot) noexcept;

    static int IsClassDefault(unsigned short pid) noexcept;

    static void SendClass(unsigned short pid) noexcept;
};

#endif //OPENMW_CHARCLASS_HPP
