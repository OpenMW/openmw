#ifndef OPENMW_ESM_DEFS_H
#define OPENMW_ESM_DEFS_H

#include <stdint.h>

#include <tuple>

#include <osg/Vec3f>

namespace ESM
{

struct TimeStamp
{
    float mHour;
    int mDay;
};

struct EpochTimeStamp
{
    float mGameHour;
    int mDay;
    int mMonth;
    int mYear;
};

// Pixel color value. Standard four-byte rr,gg,bb,aa format.
typedef uint32_t Color;

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

// Position and rotation
struct Position
{
    float pos[3];

    // In radians
    float rot[3];

    osg::Vec3f asVec3() const
    {
        return osg::Vec3f(pos[0], pos[1], pos[2]);
    }

    osg::Vec3f asRotationVec3() const
    {
        return osg::Vec3f(rot[0], rot[1], rot[2]);
    }

    friend inline bool operator<(const Position& l, const Position& r)
    {
        const auto tuple = [](const Position& v) { return std::tuple(v.asVec3(), v.asRotationVec3()); };
        return tuple(l) < tuple(r);
    }
};

bool inline operator== (const Position& left, const Position& right) noexcept
{
    return left.pos[0] == right.pos[0] &&
           left.pos[1] == right.pos[1] &&
           left.pos[2] == right.pos[2] &&
           left.rot[0] == right.rot[0] &&
           left.rot[1] == right.rot[1] &&
           left.rot[2] == right.rot[2];
}

bool inline operator!= (const Position& left, const Position& right) noexcept
{
    return left.pos[0] != right.pos[0] ||
           left.pos[1] != right.pos[1] ||
           left.pos[2] != right.pos[2] ||
           left.rot[0] != right.rot[0] ||
           left.rot[1] != right.rot[1] ||
           left.rot[2] != right.rot[2];
}

template<std::size_t len>
constexpr unsigned int fourCC(const char(&name)[len]) {
    static_assert(len == 5, "Constant must be 4 characters long. (Plus null terminator)");
    return static_cast<unsigned char>(name[0]) | (static_cast<unsigned char>(name[1]) << 8) | (static_cast<unsigned char>(name[2]) << 16) | (static_cast<unsigned char>(name[3]) << 24);
}

enum RecNameInts : unsigned int
{
    // Special values. Can not be used in any ESM.
    // Added to this enum to guarantee that the values don't collide with any records.
    REC_INTERNAL_PLAYER = 0,
    REC_INTERNAL_MARKER = 1,

    // format 0 / legacy
    REC_ACTI = fourCC("ACTI"),
    REC_ALCH = fourCC("ALCH"),
    REC_APPA = fourCC("APPA"),
    REC_ARMO = fourCC("ARMO"),
    REC_BODY = fourCC("BODY"),
    REC_BOOK = fourCC("BOOK"),
    REC_BSGN = fourCC("BSGN"),
    REC_CELL = fourCC("CELL"),
    REC_CLAS = fourCC("CLAS"),
    REC_CLOT = fourCC("CLOT"),
    REC_CNTC = fourCC("CNTC"),
    REC_CONT = fourCC("CONT"),
    REC_CREA = fourCC("CREA"),
    REC_CREC = fourCC("CREC"),
    REC_DIAL = fourCC("DIAL"),
    REC_DOOR = fourCC("DOOR"),
    REC_ENCH = fourCC("ENCH"),
    REC_FACT = fourCC("FACT"),
    REC_GLOB = fourCC("GLOB"),
    REC_GMST = fourCC("GMST"),
    REC_INFO = fourCC("INFO"),
    REC_INGR = fourCC("INGR"),
    REC_LAND = fourCC("LAND"),
    REC_LEVC = fourCC("LEVC"),
    REC_LEVI = fourCC("LEVI"),
    REC_LIGH = fourCC("LIGH"),
    REC_LOCK = fourCC("LOCK"),
    REC_LTEX = fourCC("LTEX"),
    REC_MGEF = fourCC("MGEF"),
    REC_MISC = fourCC("MISC"),
    REC_NPC_ = fourCC("NPC_"),
    REC_NPCC = fourCC("NPCC"),
    REC_PGRD = fourCC("PGRD"),
    REC_PROB = fourCC("PROB"),
    REC_RACE = fourCC("RACE"),
    REC_REGN = fourCC("REGN"),
    REC_REPA = fourCC("REPA"),
    REC_SCPT = fourCC("SCPT"),
    REC_SKIL = fourCC("SKIL"),
    REC_SNDG = fourCC("SNDG"),
    REC_SOUN = fourCC("SOUN"),
    REC_SPEL = fourCC("SPEL"),
    REC_SSCR = fourCC("SSCR"),
    REC_STAT = fourCC("STAT"),
    REC_WEAP = fourCC("WEAP"),

    // format 0 - saved games
    REC_SAVE = fourCC("SAVE"),
    REC_JOUR_LEGACY = fourCC("\xa4UOR"), // "\xa4UOR", rather than "JOUR", little oversight when magic numbers were
                                         // calculated by hand, needs to be supported for older files now
    REC_JOUR = fourCC("JOUR"),
    REC_QUES = fourCC("QUES"),
    REC_GSCR = fourCC("GSCR"),
    REC_PLAY = fourCC("PLAY"),
    REC_CSTA = fourCC("CSTA"),
    REC_GMAP = fourCC("GMAP"),
    REC_DIAS = fourCC("DIAS"),
    REC_WTHR = fourCC("WTHR"),
    REC_KEYS = fourCC("KEYS"),
    REC_DYNA = fourCC("DYNA"),
    REC_ASPL = fourCC("ASPL"),
    REC_ACTC = fourCC("ACTC"),
    REC_MPRJ = fourCC("MPRJ"),
    REC_PROJ = fourCC("PROJ"),
    REC_DCOU = fourCC("DCOU"),
    REC_MARK = fourCC("MARK"),
    REC_ENAB = fourCC("ENAB"),
    REC_CAM_ = fourCC("CAM_"),
    REC_STLN = fourCC("STLN"),
    REC_INPU = fourCC("INPU"),

    // format 1
    REC_FILT = fourCC("FILT"),
    REC_DBGP = fourCC("DBGP"), ///< only used in project files
    REC_LUAL = fourCC("LUAL"),  // LuaScriptsCfg

    // format 16 - Lua scripts in saved games
    REC_LUAM = fourCC("LUAM"),  // LuaManager data

    // format 21 - Random state in saved games.
    REC_RAND = fourCC("RAND"),  // Random state.
};

/// Common subrecords
enum SubRecNameInts
{
    SREC_DELE = ESM::fourCC("DELE"),
    SREC_NAME = ESM::fourCC("NAME")
};

}
#endif
