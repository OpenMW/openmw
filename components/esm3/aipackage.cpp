#include "aipackage.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    template <Misc::SameAsWithoutCvref<AIWander> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mDistance, v.mDuration, v.mTimeOfDay, v.mIdle, v.mShouldRepeat);
    }

    template <Misc::SameAsWithoutCvref<AITravel> T>
    void decompose(T&& v, const auto& f)
    {
        char padding[3] = { 0, 0, 0 };
        f(v.mX, v.mY, v.mZ, v.mShouldRepeat, padding);
    }

    template <Misc::SameAsWithoutCvref<AITarget> T>
    void decompose(T&& v, const auto& f)
    {
        char padding = 0;
        f(v.mX, v.mY, v.mZ, v.mDuration, v.mId.mData, v.mShouldRepeat, padding);
    }

    template <Misc::SameAsWithoutCvref<AIActivate> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mName.mData, v.mShouldRepeat);
    }

    void AIData::blank()
    {
        mHello = mFight = mFlee = mAlarm = 0;
        mServices = 0;
    }

    void AIPackageList::add(ESMReader& esm)
    {
        AIPackage pack;
        if (esm.retSubName() == AI_CNDT)
        {
            if (mList.empty())
            {
                esm.fail("AIPackge with an AI_CNDT applying to no cell.");
            }
            else
            {
                mList.back().mCellName = esm.getHString();
            }
        }
        else if (esm.retSubName() == AI_Wander)
        {
            pack.mType = AI_Wander;
            esm.getSubComposite(pack.mWander);
            mList.push_back(pack);
        }
        else if (esm.retSubName() == AI_Travel)
        {
            pack.mType = AI_Travel;
            esm.getSubComposite(pack.mTravel);
            mList.push_back(pack);
        }
        else if (esm.retSubName() == AI_Escort || esm.retSubName() == AI_Follow)
        {
            pack.mType = (esm.retSubName() == AI_Escort) ? AI_Escort : AI_Follow;
            esm.getSubComposite(pack.mTarget);
            mList.push_back(pack);
        }
        else if (esm.retSubName() == AI_Activate)
        {
            pack.mType = AI_Activate;
            esm.getSubComposite(pack.mActivate);
            mList.push_back(pack);
        }
    }

    void AIPackageList::save(ESMWriter& esm) const
    {
        for (const AIPackage& package : mList)
        {
            switch (package.mType)
            {
                case AI_Wander:
                    esm.writeNamedComposite("AI_W", package.mWander);
                    break;

                case AI_Travel:
                    esm.writeNamedComposite("AI_T", package.mTravel);
                    break;

                case AI_Activate:
                    esm.writeNamedComposite("AI_A", package.mActivate);
                    break;

                case AI_Escort:
                case AI_Follow:
                {
                    const NAME name = (package.mType == AI_Escort) ? NAME("AI_E") : NAME("AI_F");
                    esm.writeNamedComposite(name, package.mTarget);
                    esm.writeHNOCString("CNDT", package.mCellName);
                    break;
                }

                default:
                    break;
            }
        }
    }
}
