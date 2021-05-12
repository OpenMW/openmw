#ifndef OPENMW_ESM_DEFS_H
#define OPENMW_ESM_DEFS_H

#include <stdint.h>

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

#pragma pack(push)
#pragma pack(1)

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
};
#pragma pack(pop)

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

template <int a, int b, int c, int d>
struct FourCC
{
    static constexpr unsigned int value = (((((d << 8) | c) << 8) | b) << 8) | a;
};

enum RecNameInts
{
    // format 0 / legacy
    REC_ACTI = FourCC<'A','C','T','I'>::value,
    REC_ALCH = FourCC<'A','L','C','H'>::value,
    REC_APPA = FourCC<'A','P','P','A'>::value,
    REC_ARMO = FourCC<'A','R','M','O'>::value,
    REC_BODY = FourCC<'B','O','D','Y'>::value,
    REC_BOOK = FourCC<'B','O','O','K'>::value,
    REC_BSGN = FourCC<'B','S','G','N'>::value,
    REC_CELL = FourCC<'C','E','L','L'>::value,
    REC_CLAS = FourCC<'C','L','A','S'>::value,
    REC_CLOT = FourCC<'C','L','O','T'>::value,
    REC_CNTC = FourCC<'C','N','T','C'>::value,
    REC_CONT = FourCC<'C','O','N','T'>::value,
    REC_CREA = FourCC<'C','R','E','A'>::value,
    REC_CREC = FourCC<'C','R','E','C'>::value,
    REC_DIAL = FourCC<'D','I','A','L'>::value,
    REC_DOOR = FourCC<'D','O','O','R'>::value,
    REC_ENCH = FourCC<'E','N','C','H'>::value,
    REC_FACT = FourCC<'F','A','C','T'>::value,
    REC_GLOB = FourCC<'G','L','O','B'>::value,
    REC_GMST = FourCC<'G','M','S','T'>::value,
    REC_INFO = FourCC<'I','N','F','O'>::value,
    REC_INGR = FourCC<'I','N','G','R'>::value,
    REC_LAND = FourCC<'L','A','N','D'>::value,
    REC_LEVC = FourCC<'L','E','V','C'>::value,
    REC_LEVI = FourCC<'L','E','V','I'>::value,
    REC_LIGH = FourCC<'L','I','G','H'>::value,
    REC_LOCK = FourCC<'L','O','C','K'>::value,
    REC_LTEX = FourCC<'L','T','E','X'>::value,
    REC_MGEF = FourCC<'M','G','E','F'>::value,
    REC_MISC = FourCC<'M','I','S','C'>::value,
    REC_NPC_ = FourCC<'N','P','C','_'>::value,
    REC_NPCC = FourCC<'N','P','C','C'>::value,
    REC_PGRD = FourCC<'P','G','R','D'>::value,
    REC_PROB = FourCC<'P','R','O','B'>::value,
    REC_RACE = FourCC<'R','A','C','E'>::value,
    REC_REGN = FourCC<'R','E','G','N'>::value,
    REC_REPA = FourCC<'R','E','P','A'>::value,
    REC_SCPT = FourCC<'S','C','P','T'>::value,
    REC_SKIL = FourCC<'S','K','I','L'>::value,
    REC_SNDG = FourCC<'S','N','D','G'>::value,
    REC_SOUN = FourCC<'S','O','U','N'>::value,
    REC_SPEL = FourCC<'S','P','E','L'>::value,
    REC_SSCR = FourCC<'S','S','C','R'>::value,
    REC_STAT = FourCC<'S','T','A','T'>::value,
    REC_WEAP = FourCC<'W','E','A','P'>::value,

    // format 0 - saved games
    REC_SAVE = FourCC<'S','A','V','E'>::value,
    REC_JOUR_LEGACY = FourCC<0xa4,'U','O','R'>::value, // "\xa4UOR", rather than "JOUR", little oversight when magic numbers were
                                                       // calculated by hand, needs to be supported for older files now
    REC_JOUR = FourCC<'J','O','U','R'>::value,
    REC_QUES = FourCC<'Q','U','E','S'>::value,
    REC_GSCR = FourCC<'G','S','C','R'>::value,
    REC_PLAY = FourCC<'P','L','A','Y'>::value,
    REC_CSTA = FourCC<'C','S','T','A'>::value,
    REC_GMAP = FourCC<'G','M','A','P'>::value,
    REC_DIAS = FourCC<'D','I','A','S'>::value,
    REC_WTHR = FourCC<'W','T','H','R'>::value,
    REC_KEYS = FourCC<'K','E','Y','S'>::value,
    REC_DYNA = FourCC<'D','Y','N','A'>::value,
    REC_ASPL = FourCC<'A','S','P','L'>::value,
    REC_ACTC = FourCC<'A','C','T','C'>::value,
    REC_MPRJ = FourCC<'M','P','R','J'>::value,
    REC_PROJ = FourCC<'P','R','O','J'>::value,
    REC_DCOU = FourCC<'D','C','O','U'>::value,
    REC_MARK = FourCC<'M','A','R','K'>::value,
    REC_ENAB = FourCC<'E','N','A','B'>::value,
    REC_CAM_ = FourCC<'C','A','M','_'>::value,
    REC_STLN = FourCC<'S','T','L','N'>::value,
    REC_INPU = FourCC<'I','N','P','U'>::value,

    // format 1
    REC_FILT = FourCC<'F','I','L','T'>::value,
    REC_DBGP = FourCC<'D','B','G','P'>::value ///< only used in project files
};

/// Common subrecords
enum SubRecNameInts
{
    SREC_DELE = ESM::FourCC<'D','E','L','E'>::value,
    SREC_NAME = ESM::FourCC<'N','A','M','E'>::value
};

}
#endif
