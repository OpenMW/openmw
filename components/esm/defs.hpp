#ifndef OPENMW_ESM_DEFS_H
#define OPENMW_ESM_DEFS_H

#include <stdexcept>
#include <stdint.h>

#include <components/esm/esmcommon.hpp>
#include <components/esm/fourcc.hpp>
#include <components/esm4/common.hpp>

namespace ESM
{

    struct EpochTimeStamp
    {
        float mGameHour;
        int32_t mDay;
        int32_t mMonth;
        int32_t mYear;
    };

    // Pixel color value. Standard four-byte rr,gg,bb,aa format.
    typedef uint32_t Color;

    enum RangeType
    {
        RT_Self = 0,
        RT_Touch = 1,
        RT_Target = 2
    };

    constexpr unsigned int sEsm4RecnameFlag = 0x00800000;

    constexpr unsigned int esm3Recname(const char (&name)[5])
    {
        if ((fourCC(name) & sEsm4RecnameFlag) == 0)
            return fourCC(name);
        else
            throw std::logic_error(
                "there must be no collision between esm3 records and esm4 records"); // The throw errors ensures at
                                                                                     // compile time that no collision
                                                                                     // between ESM4 and ESM3 is
                                                                                     // possible
    }

    constexpr unsigned int esm4Recname(const ESM4::RecordTypes recType)
    {
        if ((recType & sEsm4RecnameFlag) == 0)
            return (recType | sEsm4RecnameFlag);
        else
            throw std::logic_error(
                "there must be no collision between esm3 records and esm4 records"); // The throw errors ensures at
                                                                                     // compile time that no collision
                                                                                     // between ESM4 and ESM3 is
                                                                                     // possible
    }

    enum RecNameInts : unsigned int
    {
        // Special values. Can not be used in any ESM.
        // Added to this enum to guarantee that the values don't collide with any records.
        REC_INTERNAL_PLAYER = 0,
        REC_INTERNAL_MARKER = 1,

        // format 0 / legacy
        REC_ACTI = esm3Recname("ACTI"),
        REC_ALCH = esm3Recname("ALCH"),
        REC_APPA = esm3Recname("APPA"),
        REC_ARMO = esm3Recname("ARMO"),
        REC_BODY = esm3Recname("BODY"),
        REC_BOOK = esm3Recname("BOOK"),
        REC_BSGN = esm3Recname("BSGN"),
        REC_CELL = esm3Recname("CELL"),
        REC_CLAS = esm3Recname("CLAS"),
        REC_CLOT = esm3Recname("CLOT"),
        REC_CNTC = esm3Recname("CNTC"),
        REC_CONT = esm3Recname("CONT"),
        REC_CREA = esm3Recname("CREA"),
        REC_CREC = esm3Recname("CREC"),
        REC_DIAL = esm3Recname("DIAL"),
        REC_DOOR = esm3Recname("DOOR"),
        REC_ENCH = esm3Recname("ENCH"),
        REC_FACT = esm3Recname("FACT"),
        REC_GLOB = esm3Recname("GLOB"),
        REC_GMST = esm3Recname("GMST"),
        REC_INFO = esm3Recname("INFO"),
        REC_INGR = esm3Recname("INGR"),
        REC_LAND = esm3Recname("LAND"),
        REC_LEVC = esm3Recname("LEVC"),
        REC_LEVI = esm3Recname("LEVI"),
        REC_LIGH = esm3Recname("LIGH"),
        REC_LOCK = esm3Recname("LOCK"),
        REC_LTEX = esm3Recname("LTEX"),
        REC_MGEF = esm3Recname("MGEF"),
        REC_MISC = esm3Recname("MISC"),
        REC_NPC_ = esm3Recname("NPC_"),
        REC_NPCC = esm3Recname("NPCC"),
        REC_PGRD = esm3Recname("PGRD"),
        REC_PROB = esm3Recname("PROB"),
        REC_RACE = esm3Recname("RACE"),
        REC_REGN = esm3Recname("REGN"),
        REC_REPA = esm3Recname("REPA"),
        REC_SCPT = esm3Recname("SCPT"),
        REC_SKIL = esm3Recname("SKIL"),
        REC_SNDG = esm3Recname("SNDG"),
        REC_SOUN = esm3Recname("SOUN"),
        REC_SPEL = esm3Recname("SPEL"),
        REC_SSCR = esm3Recname("SSCR"),
        REC_STAT = esm3Recname("STAT"),
        REC_WEAP = esm3Recname("WEAP"),

        // format 0 - saved games
        REC_SAVE = esm3Recname("SAVE"),
        REC_JOUR = esm3Recname("JOUR"),
        REC_QUES = esm3Recname("QUES"),
        REC_GSCR = esm3Recname("GSCR"),
        REC_PLAY = esm3Recname("PLAY"),
        REC_CSTA = esm3Recname("CSTA"),
        REC_GMAP = esm3Recname("GMAP"),
        REC_DIAS = esm3Recname("DIAS"),
        REC_WTHR = esm3Recname("WTHR"),
        REC_KEYS = esm3Recname("KEYS"),
        REC_DYNA = esm3Recname("DYNA"),
        REC_ASPL = esm3Recname("ASPL"),
        REC_ACTC = esm3Recname("ACTC"),
        REC_MPRJ = esm3Recname("MPRJ"),
        REC_PROJ = esm3Recname("PROJ"),
        REC_DCOU = esm3Recname("DCOU"),
        REC_MARK = esm3Recname("MARK"),
        REC_ENAB = esm3Recname("ENAB"),
        REC_CAM_ = esm3Recname("CAM_"),
        REC_STLN = esm3Recname("STLN"),
        REC_INPU = esm3Recname("INPU"),

        // format 1
        REC_FILT = esm3Recname("FILT"),
        REC_DBGP = esm3Recname("DBGP"), ///< only used in project files
        REC_SELG = esm3Recname("SELG"),

        REC_LUAL = esm3Recname("LUAL"), // LuaScriptsCfg (only in omwgame or omwaddon)

        // format 16 - Lua scripts in saved games
        REC_LUAM = esm3Recname("LUAM"), // LuaManager data

        // format 21 - Random state in saved games.
        REC_RAND = esm3Recname("RAND"), // Random state.

        REC_ATTR = esm3Recname("ATTR"), // Attribute

        REC_AACT4 = esm4Recname(ESM4::REC_AACT), // Action
        REC_ACHR4 = esm4Recname(ESM4::REC_ACHR), // Actor Reference
        REC_ACTI4 = esm4Recname(ESM4::REC_ACTI), // Activator
        REC_ADDN4 = esm4Recname(ESM4::REC_ADDN), // Addon Node
        REC_ALCH4 = esm4Recname(ESM4::REC_ALCH), // Potion
        REC_AMMO4 = esm4Recname(ESM4::REC_AMMO), // Ammo
        REC_ANIO4 = esm4Recname(ESM4::REC_ANIO), // Animated Object
        REC_APPA4 = esm4Recname(ESM4::REC_APPA), // Apparatus (probably unused)
        REC_ARMA4 = esm4Recname(ESM4::REC_ARMA), // Armature (Model)
        REC_ARMO4 = esm4Recname(ESM4::REC_ARMO), // Armor
        REC_ARTO4 = esm4Recname(ESM4::REC_ARTO), // Art Object
        REC_ASPC4 = esm4Recname(ESM4::REC_ASPC), // Acoustic Space
        REC_ASTP4 = esm4Recname(ESM4::REC_ASTP), // Association Type
        REC_AVIF4 = esm4Recname(ESM4::REC_AVIF), // Actor Values/Perk Tree Graphics
        REC_BOOK4 = esm4Recname(ESM4::REC_BOOK), // Book
        REC_BPTD4 = esm4Recname(ESM4::REC_BPTD), // Body Part Data
        REC_CAMS4 = esm4Recname(ESM4::REC_CAMS), // Camera Shot
        REC_CELL4 = esm4Recname(ESM4::REC_CELL), // Cell
        REC_CLAS4 = esm4Recname(ESM4::REC_CLAS), // Class
        REC_CLFM4 = esm4Recname(ESM4::REC_CLFM), // Color
        REC_CLMT4 = esm4Recname(ESM4::REC_CLMT), // Climate
        REC_CLOT4 = esm4Recname(ESM4::REC_CLOT), // Clothing
        REC_COBJ4 = esm4Recname(ESM4::REC_COBJ), // Constructible Object (recipes)
        REC_COLL4 = esm4Recname(ESM4::REC_COLL), // Collision Layer
        REC_CONT4 = esm4Recname(ESM4::REC_CONT), // Container
        REC_CPTH4 = esm4Recname(ESM4::REC_CPTH), // Camera Path
        REC_CREA4 = esm4Recname(ESM4::REC_CREA), // Creature
        REC_CSTY4 = esm4Recname(ESM4::REC_CSTY), // Combat Style
        REC_DEBR4 = esm4Recname(ESM4::REC_DEBR), // Debris
        REC_DIAL4 = esm4Recname(ESM4::REC_DIAL), // Dialog Topic
        REC_DLBR4 = esm4Recname(ESM4::REC_DLBR), // Dialog Branch
        REC_DLVW4 = esm4Recname(ESM4::REC_DLVW), // Dialog View
        REC_DOBJ4 = esm4Recname(ESM4::REC_DOBJ), // Default Object Manager
        REC_DOOR4 = esm4Recname(ESM4::REC_DOOR), // Door
        REC_DUAL4 = esm4Recname(ESM4::REC_DUAL), // Dual Cast Data (possibly unused)
        REC_ECZN4 = esm4Recname(ESM4::REC_ECZN), // Encounter Zone
        REC_EFSH4 = esm4Recname(ESM4::REC_EFSH), // Effect Shader
        REC_ENCH4 = esm4Recname(ESM4::REC_ENCH), // Enchantment
        REC_EQUP4 = esm4Recname(ESM4::REC_EQUP), // Equip Slot (flag-type values)
        REC_EXPL4 = esm4Recname(ESM4::REC_EXPL), // Explosion
        REC_EYES4 = esm4Recname(ESM4::REC_EYES), // Eyes
        REC_FACT4 = esm4Recname(ESM4::REC_FACT), // Faction
        REC_FLOR4 = esm4Recname(ESM4::REC_FLOR), // Flora
        REC_FLST4 = esm4Recname(ESM4::REC_FLST), // Form List (non-levelled list)
        REC_FSTP4 = esm4Recname(ESM4::REC_FSTP), // Footstep
        REC_FSTS4 = esm4Recname(ESM4::REC_FSTS), // Footstep Set
        REC_FURN4 = esm4Recname(ESM4::REC_FURN), // Furniture
        REC_GLOB4 = esm4Recname(ESM4::REC_GLOB), // Global Variable
        REC_GMST4 = esm4Recname(ESM4::REC_GMST), // Game Setting
        REC_GRAS4 = esm4Recname(ESM4::REC_GRAS), // Grass
        REC_GRUP4 = esm4Recname(ESM4::REC_GRUP), // Form Group
        REC_HAIR4 = esm4Recname(ESM4::REC_HAIR), // Hair
        REC_HAZD4 = esm4Recname(ESM4::REC_HAZD), // Hazard
        REC_HDPT4 = esm4Recname(ESM4::REC_HDPT), // Head Part
        REC_IDLE4 = esm4Recname(ESM4::REC_IDLE), // Idle Animation
        REC_IDLM4 = esm4Recname(ESM4::REC_IDLM), // Idle Marker
        REC_IMAD4 = esm4Recname(ESM4::REC_IMAD), // Image Space Modifier
        REC_IMGS4 = esm4Recname(ESM4::REC_IMGS), // Image Space
        REC_INFO4 = esm4Recname(ESM4::REC_INFO), // Dialog Topic Info
        REC_INGR4 = esm4Recname(ESM4::REC_INGR), // Ingredient
        REC_IPCT4 = esm4Recname(ESM4::REC_IPCT), // Impact Data
        REC_IPDS4 = esm4Recname(ESM4::REC_IPDS), // Impact Data Set
        REC_KEYM4 = esm4Recname(ESM4::REC_KEYM), // Key
        REC_KYWD4 = esm4Recname(ESM4::REC_KYWD), // Keyword
        REC_LAND4 = esm4Recname(ESM4::REC_LAND), // Land
        REC_LCRT4 = esm4Recname(ESM4::REC_LCRT), // Location Reference Type
        REC_LCTN4 = esm4Recname(ESM4::REC_LCTN), // Location
        REC_LGTM4 = esm4Recname(ESM4::REC_LGTM), // Lighting Template
        REC_LIGH4 = esm4Recname(ESM4::REC_LIGH), // Light
        REC_LSCR4 = esm4Recname(ESM4::REC_LSCR), // Load Screen
        REC_LTEX4 = esm4Recname(ESM4::REC_LTEX), // Land Texture
        REC_LVLC4 = esm4Recname(ESM4::REC_LVLC), // Leveled Creature
        REC_LVLI4 = esm4Recname(ESM4::REC_LVLI), // Leveled Item
        REC_LVLN4 = esm4Recname(ESM4::REC_LVLN), // Leveled Actor
        REC_LVSP4 = esm4Recname(ESM4::REC_LVSP), // Leveled Spell
        REC_MATO4 = esm4Recname(ESM4::REC_MATO), // Material Object
        REC_MATT4 = esm4Recname(ESM4::REC_MATT), // Material Type
        REC_MESG4 = esm4Recname(ESM4::REC_MESG), // Message
        REC_MGEF4 = esm4Recname(ESM4::REC_MGEF), // Magic Effect
        REC_MISC4 = esm4Recname(ESM4::REC_MISC), // Misc. Object
        REC_MOVT4 = esm4Recname(ESM4::REC_MOVT), // Movement Type
        REC_MSTT4 = esm4Recname(ESM4::REC_MSTT), // Movable Static
        REC_MUSC4 = esm4Recname(ESM4::REC_MUSC), // Music Type
        REC_MUST4 = esm4Recname(ESM4::REC_MUST), // Music Track
        REC_NAVI4 = esm4Recname(ESM4::REC_NAVI), // Navigation (master data)
        REC_NAVM4 = esm4Recname(ESM4::REC_NAVM), // Nav Mesh
        REC_NOTE4 = esm4Recname(ESM4::REC_NOTE), // Note
        REC_NPC_4 = esm4Recname(ESM4::REC_NPC_), // Actor (NPC, Creature)
        REC_OTFT4 = esm4Recname(ESM4::REC_OTFT), // Outfit
        REC_PACK4 = esm4Recname(ESM4::REC_PACK), // AI Package
        REC_PERK4 = esm4Recname(ESM4::REC_PERK), // Perk
        REC_PGRE4 = esm4Recname(ESM4::REC_PGRE), // Placed grenade
        REC_PHZD4 = esm4Recname(ESM4::REC_PHZD), // Placed hazard
        REC_PROJ4 = esm4Recname(ESM4::REC_PROJ), // Projectile
        REC_QUST4 = esm4Recname(ESM4::REC_QUST), // Quest
        REC_RACE4 = esm4Recname(ESM4::REC_RACE), // Race / Creature type
        REC_REFR4 = esm4Recname(ESM4::REC_REFR), // Object Reference
        REC_REGN4 = esm4Recname(ESM4::REC_REGN), // Region (Audio/Weather)
        REC_RELA4 = esm4Recname(ESM4::REC_RELA), // Relationship
        REC_REVB4 = esm4Recname(ESM4::REC_REVB), // Reverb Parameters
        REC_RFCT4 = esm4Recname(ESM4::REC_RFCT), // Visual Effect
        REC_SBSP4 = esm4Recname(ESM4::REC_SBSP), // Subspace (TES4 only?)
        REC_SCEN4 = esm4Recname(ESM4::REC_SCEN), // Scene
        REC_SCPT4 = esm4Recname(ESM4::REC_SCPT), // Script
        REC_SCRL4 = esm4Recname(ESM4::REC_SCRL), // Scroll
        REC_SGST4 = esm4Recname(ESM4::REC_SGST), // Sigil Stone
        REC_SHOU4 = esm4Recname(ESM4::REC_SHOU), // Shout
        REC_SLGM4 = esm4Recname(ESM4::REC_SLGM), // Soul Gem
        REC_SMBN4 = esm4Recname(ESM4::REC_SMBN), // Story Manager Branch Node
        REC_SMEN4 = esm4Recname(ESM4::REC_SMEN), // Story Manager Event Node
        REC_SMQN4 = esm4Recname(ESM4::REC_SMQN), // Story Manager Quest Node
        REC_SNCT4 = esm4Recname(ESM4::REC_SNCT), // Sound Category
        REC_SNDR4 = esm4Recname(ESM4::REC_SNDR), // Sound Reference
        REC_SOPM4 = esm4Recname(ESM4::REC_SOPM), // Sound Output Model
        REC_SOUN4 = esm4Recname(ESM4::REC_SOUN), // Sound
        REC_SPEL4 = esm4Recname(ESM4::REC_SPEL), // Spell
        REC_SPGD4 = esm4Recname(ESM4::REC_SPGD), // Shader Particle Geometry
        REC_STAT4 = esm4Recname(ESM4::REC_STAT), // Static
        REC_TACT4 = esm4Recname(ESM4::REC_TACT), // Talking Activator
        REC_TERM4 = esm4Recname(ESM4::REC_TERM), // Terminal
        REC_TES44 = esm4Recname(ESM4::REC_TES4), // Plugin info
        REC_TREE4 = esm4Recname(ESM4::REC_TREE), // Tree
        REC_TXST4 = esm4Recname(ESM4::REC_TXST), // Texture Set
        REC_VTYP4 = esm4Recname(ESM4::REC_VTYP), // Voice Type
        REC_WATR4 = esm4Recname(ESM4::REC_WATR), // Water Type
        REC_WEAP4 = esm4Recname(ESM4::REC_WEAP), // Weapon
        REC_WOOP4 = esm4Recname(ESM4::REC_WOOP), // Word Of Power
        REC_WRLD4 = esm4Recname(ESM4::REC_WRLD), // World Space
        REC_WTHR4 = esm4Recname(ESM4::REC_WTHR), // Weather
        REC_ACRE4 = esm4Recname(ESM4::REC_ACRE), // Placed Creature (TES4 only?)
        REC_PGRD4 = esm4Recname(ESM4::REC_PGRD), // Pathgrid (TES4 only?)
        REC_ROAD4 = esm4Recname(ESM4::REC_ROAD), // Road (TES4 only?)
        REC_IMOD4 = esm4Recname(ESM4::REC_IMOD), // Item Mod
        REC_PWAT4 = esm4Recname(ESM4::REC_PWAT), // Placeable Water
        REC_SCOL4 = esm4Recname(ESM4::REC_SCOL), // Static Collection
        REC_CCRD4 = esm4Recname(ESM4::REC_CCRD), // Caravan Card
        REC_CMNY4 = esm4Recname(ESM4::REC_CMNY), // Caravan Money
        REC_ALOC4 = esm4Recname(ESM4::REC_ALOC), // Audio Location Controller
        REC_MSET4 = esm4Recname(ESM4::REC_MSET) // Media Set
    };

    constexpr bool isESM4Rec(RecNameInts RecName)
    {
        return RecName & sEsm4RecnameFlag;
    }

    constexpr inline FixedString<6> getRecNameString(ESM::RecNameInts recName)
    {
        ESM::FixedString<6> name;
        name.mData[5] = '\0';

        ESM::NAME fourCCName(recName & ~ESM::sEsm4RecnameFlag);
        for (int i = 0; i < 4; i++)
            name.mData[i] = fourCCName.mData[i];

        name.mData[4] = ESM::isESM4Rec(recName) ? '4' : '\0';
        return name;
    }

    /// Common subrecords
    enum SubRecNameInts
    {
        SREC_DELE = ESM::fourCC("DELE"),
        SREC_NAME = ESM::fourCC("NAME")
    };

}
#endif
