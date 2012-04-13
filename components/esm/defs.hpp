#ifndef _ESM_DEFS_H
#define _ESM_DEFS_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

// Pixel color value. Standard four-byte rr,gg,bb,aa format.
typedef int32_t Color;

enum VarType
{
    VT_Unknown,
    VT_None,
    VT_Short,
    VT_Int,
    VT_Long,
    VT_Float,
    VT_String,
    VT_Ignored
};

enum Specialization
{
    SPC_Combat = 0,
    SPC_Magic = 1,
    SPC_Stealth = 2
};

enum RangeType
{
    RT_Self = 0,
    RT_Touch = 1,
    RT_Target = 2
};

/** A list of references to spells and spell effects. This is shared
 between the records BSGN, NPC and RACE.
 */
struct SpellList
{
    std::vector<std::string> list;

    void load(ESMReader &esm)
    {
        while (esm.isNextSub("NPCS"))
            list.push_back(esm.getHString());
    }
    void save(ESMWriter &esm)
    {
        for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it)
            esm.writeHNString("NPCS", *it, 32);
    }
};

/** Defines a spell effect. Shared between SPEL (Spells), ALCH
 (Potions) and ENCH (Item enchantments) records
 */
#pragma pack(push)
#pragma pack(1)

// Position and rotation
struct Position
{
    float pos[3];
    float rot[3];
};

struct ENAMstruct
{
    // Magical effect, hard-coded ID
    short effectID;

    // Which skills/attributes are affected (for restore/drain spells
    // etc.)
    signed char skill, attribute; // -1 if N/A

    // Other spell parameters
    int range; // 0 - self, 1 - touch, 2 - target (RangeType enum)
    int area, duration, magnMin, magnMax;

    // Struct size should be 24 bytes
};

struct AIDTstruct
{
    // These are probabilities
    char hello, u1, fight, flee, alarm, u2, u3, u4;
    // The last u's might be the skills that this NPC can train you
    // in?
    int services; // See the Services enum
};  // 12 bytes

struct DODTstruct
{
    float pos[3];
    float rot[3];
};

struct AI_Package
{
    void load(ESMReader& esm)
    {
        getData(esm);
        cndt = esm.getHNOString("CNDT");
    }

    void save(ESMWriter& esm)
    {
        esm.startSubRecord(getName());
        saveData(esm);
        esm.endRecord(getName());

        esm.writeHNOCString("CNDT", cndt);
    }

    std::string cndt;

    virtual void getData(ESMReader&) = 0;
    virtual void saveData(ESMWriter&) = 0;

    virtual std::string getName() const = 0;
    virtual int size() const = 0;
};

struct AI_Wstruct : AI_Package
{
    struct Data
    {
        short distance, duration;
        char timeofday;
        char idle[8];
        char unknown;
    };

    Data data;

    void getData(ESMReader& esm) { esm.getHExact(&data, sizeof(data)); }
    void saveData(ESMWriter& esm) { esm.writeT(data); }

    std::string getName() const { return "AI_W"; }
    int size() const { return sizeof(AI_Wstruct); }
};

struct AI_Tstruct : AI_Package
{
    struct Data
    {
        float pos[3];
        int unknown;
    };

    Data data;

    void getData(ESMReader& esm) { esm.getHExact(&data, sizeof(data)); }
    void saveData(ESMWriter& esm) { esm.writeT(data); }

    std::string getName() const { return "AI_T"; }
    int size() const { return sizeof(AI_Tstruct); }
};

struct AI_Fstruct : AI_Package
{
    struct Data
    {
        float pos[3];
        short duration;
        NAME32 id;
        short unknown;
    };

    Data data;
    
    void getData(ESMReader& esm) { esm.getHExact(&data, sizeof(data)); }
    void saveData(ESMWriter& esm) { esm.writeT(data); }

    std::string getName() const { return "AI_F"; }
    int size() const { return sizeof(AI_Fstruct); }
};

struct AI_Estruct : AI_Package
{
    struct Data
    {
        float pos[3];
        short duration;
        NAME32 id;
        short unknown;
    };

    Data data;

    void getData(ESMReader& esm) { esm.getHExact(&data, sizeof(data)); }
    void saveData(ESMWriter& esm) { esm.writeT(data); }

    std::string getName() const { return "AI_E"; }
    int size() const { return sizeof(AI_Estruct); }
};

struct AI_Astruct : AI_Package
{
    struct Data
    {
        NAME32 name;
        char unknown;
    };

    Data data;

    void getData(ESMReader& esm) { esm.getHExact(&data, sizeof(data)); }
    void saveData(ESMWriter& esm) { esm.writeT(data); }

    std::string getName() const { return "AI_A"; }
    int size() const { return sizeof(AI_Astruct); }
};

#pragma pack(pop)

struct EffectList
{
    std::vector<ENAMstruct> list;

    void load(ESMReader &esm)
    {
        ENAMstruct s;
        while (esm.isNextSub("ENAM"))
        {
            esm.getHT(s, 24);
            list.push_back(s);
        }
    }
    void save(ESMWriter &esm)
    {
        for (std::vector<ENAMstruct>::iterator it = list.begin(); it != list.end(); ++it)
        {
            esm.writeHNT<ENAMstruct>("ENAM", *it, 24);
        }
    }
};

struct AIData
{
    struct Travelstruct
    {
        DODTstruct dodt;
        std::string dnam;
    };

    AIDTstruct aidt;
    bool hasAI;
    std::vector<AI_Package*> packages;
    std::vector<Travelstruct> travel;
    
    void load(ESMReader &esm)
    {
        if (esm.isNextSub("AIDT"))
        {
            esm.getHExact(&aidt, sizeof(aidt));
            hasAI = true;
        }
        else
            hasAI = false;

#define LOAD_IF_FOUND(x)                        \
        if (esm.isNextSub(#x))                  \
        {                                       \
            found = true;                       \
            x##struct *t = new x##struct();     \
            t->load(esm);                       \
            packages.push_back(t);              \
        }
        
        bool found = true;
        while (esm.hasMoreSubs() && found)
        {
            found = false;

            if (esm.isNextSub("DODT"))
            {
                found = true;
                Travelstruct t;
                esm.getHExact(&t.dodt, sizeof(t.dodt));
                t.dnam = esm.getHNOString("DNAM");
                travel.push_back(t);
            }

            LOAD_IF_FOUND(AI_W);
            LOAD_IF_FOUND(AI_T);
            LOAD_IF_FOUND(AI_F);
            LOAD_IF_FOUND(AI_E);
            LOAD_IF_FOUND(AI_A);
        }
    }

    void save(ESMWriter &esm)
    {
        if (hasAI)
            esm.writeHNT("AIDT", aidt);

        for (std::vector<Travelstruct>::iterator it = travel.begin(); it != travel.end(); ++it)   
        {
            esm.writeHNT("DODT", it->dodt);
            esm.writeHNOCString("DNAM", it->dnam);
        }

        for (std::vector<AI_Package*>::iterator it = packages.begin(); it != packages.end(); ++it)
        {
            (*it)->save(esm);
        }
    }

    ~AIData()
    {
        for (std::vector<AI_Package*>::iterator it = packages.begin(); it != packages.end();)
        {
            delete *it;
            packages.erase(it++);
        }
    }
};

}
#endif
