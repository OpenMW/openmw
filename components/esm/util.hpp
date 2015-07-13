#ifndef OPENMW_ESM_UTIL_H
#define OPENMW_ESM_UTIL_H

#include <string>

#include <osg/Vec3f>
#include <osg/Quat>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "loadsscr.hpp"
#include "loadglob.hpp"
#include "loadrace.hpp"
#include "loadgmst.hpp"
#include "loadskil.hpp"
#include "loadmgef.hpp"
#include "loadland.hpp"
#include "loadpgrd.hpp"
#include "debugprofile.hpp"
#include "filter.hpp"

namespace ESM
{

// format 0, savegames only

struct Quaternion
{
    float mValues[4];

    Quaternion() {}

    Quaternion(const osg::Quat& q)
    {
        mValues[0] = q.w();
        mValues[1] = q.x();
        mValues[2] = q.y();
        mValues[3] = q.z();
    }

    operator osg::Quat () const
    {
        return osg::Quat(mValues[1], mValues[2], mValues[3], mValues[0]);
    }
};

struct Vector3
{
    float mValues[3];

    Vector3() {}

    Vector3(const osg::Vec3f& v)
    {
        mValues[0] = v.x();
        mValues[1] = v.y();
        mValues[2] = v.z();
    }

    operator osg::Vec3f () const
    {
        return osg::Vec3f(mValues[0], mValues[1], mValues[2]);
    }
};

bool readDeleSubRecord(ESMReader &esm);
void writeDeleSubRecord(ESMWriter &esm);

template <class RecordT>
bool isRecordDeleted(const RecordT &record)
{
    return record.mIsDeleted;
}

// The following records can't be deleted (for now)
template <>
bool isRecordDeleted<StartScript>(const StartScript &script);

template <>
bool isRecordDeleted<Race>(const Race &race);

template <>
bool isRecordDeleted<GameSetting>(const GameSetting &gmst);

template <>
bool isRecordDeleted<Skill>(const Skill &skill);

template <>
bool isRecordDeleted<MagicEffect>(const MagicEffect &mgef);

template <>
bool isRecordDeleted<Pathgrid>(const Pathgrid &pgrd);

template <>
bool isRecordDeleted<Land>(const Land &land);

template <>
bool isRecordDeleted<DebugProfile>(const DebugProfile &profile);

template <>
bool isRecordDeleted<Filter>(const Filter &filter);

}

#endif
