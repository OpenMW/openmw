#include "aiwander.hpp"

#include "movement.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include <OgreVector3.h>

namespace
{
    float sgn(float a)
    {
        if(a > 0)
            return 1.0;
        return -1.0;
    }
}

namespace MWMechanics
{
    AiWander::AiWander(int distance, int duration, int timeOfDay, const std::vector<int>& idle, bool repeat):
        mDistance(distance), mDuration(duration), mTimeOfDay(timeOfDay), mIdle(idle), mRepeat(repeat)
    {
        for(unsigned short counter = 0; counter < mIdle.size(); counter++)
        {
            if(mIdle[counter] >= 127 || mIdle[counter] < 0)
                mIdle[counter] = 0;
        }

        if(mDistance < 0)
            mDistance = 0;
        if(mDuration < 0)
            mDuration = 0;
        if(mDuration == 0)
            mTimeOfDay = 0;

        srand(time(NULL));
        mStartTime = MWBase::Environment::get().getWorld()->getTimeStamp();
        mPlayedIdle = 0;
        mPathgrid = NULL;
        mIdleChanceMultiplier =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fIdleChanceMultiplier")->getFloat();

        mStoredAvailableNodes = false;
        mChooseAction = true;
        mIdleNow = false;
        mMoveNow = false;
        mWalking = false;
    }

    AiPackage * MWMechanics::AiWander::clone() const
    {
        return new AiWander(*this);
    }

    bool AiWander::execute (const MWWorld::Ptr& actor)
    {
        if(mDuration)
        {
            // End package if duration is complete or mid-night hits:
            MWWorld::TimeStamp currentTime = MWBase::Environment::get().getWorld()->getTimeStamp();
            if(currentTime.getHour() >= mStartTime.getHour() + mDuration)
            {
                if(!mRepeat)
                {
                    stopWalking(actor);
                    return true;
                }
                else
                    mStartTime = currentTime;
            }
            else if(int(currentTime.getHour()) == 0 && currentTime.getDay() != mStartTime.getDay())
            {
                if(!mRepeat)
                {
                    stopWalking(actor);
                    return true;
                }
                else
                    mStartTime = currentTime;
            }
        }

        ESM::Position pos = actor.getRefData().getPosition();

        if(!mStoredAvailableNodes)
        {
            mStoredAvailableNodes = true;
            mPathgrid =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->mCell);

            mCellX = actor.getCell()->mCell->mData.mX;
            mCellY = actor.getCell()->mCell->mData.mY;

            if(!mPathgrid)
                mDistance = 0;
            else if(mPathgrid->mPoints.empty())
                mDistance = 0;

            if(mDistance)
            {
                mXCell = 0;
                mYCell = 0;
                if(actor.getCell()->mCell->isExterior())
                {
                    mXCell = mCellX * ESM::Land::REAL_SIZE;
                    mYCell = mCellY * ESM::Land::REAL_SIZE;
                }

                Ogre::Vector3 npcPos(actor.getRefData().getPosition().pos);
                npcPos[0] = npcPos[0] - mXCell;
                npcPos[1] = npcPos[1] - mYCell;

                for(unsigned int counter = 0; counter < mPathgrid->mPoints.size(); counter++)
                {
                    Ogre::Vector3 nodePos(mPathgrid->mPoints[counter].mX, mPathgrid->mPoints[counter].mY,
                        mPathgrid->mPoints[counter].mZ);
                    if(npcPos.squaredDistance(nodePos) <= mDistance * mDistance)
                        mAllowedNodes.push_back(mPathgrid->mPoints[counter]);
                }
                if(!mAllowedNodes.empty())
                {
                    Ogre::Vector3 firstNodePos(mAllowedNodes[0].mX, mAllowedNodes[0].mY, mAllowedNodes[0].mZ);
                    float closestNode = npcPos.squaredDistance(firstNodePos);
                    unsigned int index = 0;
                    for(unsigned int counterThree = 1; counterThree < mAllowedNodes.size(); counterThree++)
                    {
                        Ogre::Vector3 nodePos(mAllowedNodes[counterThree].mX, mAllowedNodes[counterThree].mY,
                            mAllowedNodes[counterThree].mZ);
                        float tempDist = npcPos.squaredDistance(nodePos);
                        if(tempDist < closestNode)
                            index = counterThree;
                    }
                    mCurrentNode = mAllowedNodes[index];
                    mAllowedNodes.erase(mAllowedNodes.begin() + index);
                }

                if(mAllowedNodes.empty())
                    mDistance = 0;
            }
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        bool cellChange = actor.getCell()->mCell->mData.mX != mCellX || actor.getCell()->mCell->mData.mY != mCellY;

        if(actor.getCell()->mCell->mData.mX != player.getCell()->mCell->mData.mX)
        {
            int sideX = sgn(actor.getCell()->mCell->mData.mX - player.getCell()->mCell->mData.mX);
            // Check if actor is near the border of an inactive cell. If so, disable AiWander.
            // FIXME: This *should* pause the AiWander package instead of terminating it.
            if(sideX * (pos.pos[0] - actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE) > sideX * (ESM::Land::REAL_SIZE /
                2.0 - 200)) 
            {
                stopWalking(actor);
                return true;
            }
        }

        if(actor.getCell()->mCell->mData.mY != player.getCell()->mCell->mData.mY)
        {
            int sideY = sgn(actor.getCell()->mCell->mData.mY - player.getCell()->mCell->mData.mY);
            // Check if actor is near the border of an inactive cell. If so, disable AiWander.
            // FIXME: This *should* pause the AiWander package instead of terminating it.
            if(sideY * (pos.pos[1] - actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE) > sideY * (ESM::Land::REAL_SIZE /
                2.0 - 200)) 
            {
                stopWalking(actor);
                return true;
            }
        }

        // Don't try to move if you are in a new cell (ie: positioncell command called) but still play idles.
        if(mDistance && (cellChange || (mCellX != actor.getCell()->mCell->mData.mX || mCellY != actor.getCell()->mCell->mData.mY)))
            mDistance = 0;

        if(mChooseAction)
        {
            mPlayedIdle = 0;
            unsigned short idleRoll = 0;

            for(unsigned int counter = 1; counter < mIdle.size(); counter++)
            {
                unsigned short idleChance = mIdleChanceMultiplier * mIdle[counter];
                unsigned short randSelect = (int)(rand() / ((double)RAND_MAX + 1) * int(100 / mIdleChanceMultiplier));
                if(randSelect < idleChance && randSelect > idleRoll)
                {
                    mPlayedIdle = counter;
                    idleRoll = randSelect;
                }
            }

            if(!mPlayedIdle && mDistance)
            {
                mChooseAction = false;
                mMoveNow = true;
            }
            else
            {
                // Play idle animation and recreate vanilla (broken?) behavior of resetting start time of AIWander:
                MWWorld::TimeStamp currentTime = MWBase::Environment::get().getWorld()->getTimeStamp();
                mStartTime = currentTime;
                playIdle(actor, mPlayedIdle + 1);
                mChooseAction = false;
                mIdleNow = true;
            }
        }

        if(mIdleNow)
        {
            if(!checkIdle(actor, mPlayedIdle + 1))
            {
                mPlayedIdle = 0;
                mIdleNow = false;
                mChooseAction = true;
            }
        }

        if(mMoveNow && mDistance)
        {
            if(!mPathFinder.isPathConstructed())
            {
                unsigned int randNode = (int)(rand() / ((double)RAND_MAX + 1) * mAllowedNodes.size());
                Ogre::Vector3 destNodePos(mAllowedNodes[randNode].mX, mAllowedNodes[randNode].mY, mAllowedNodes[randNode].mZ);

                ESM::Pathgrid::Point dest;
                dest.mX = destNodePos[0] + mXCell;
                dest.mY = destNodePos[1] + mYCell;
                dest.mZ = destNodePos[2];

                ESM::Pathgrid::Point start;
                start.mX = pos.pos[0];
                start.mY = pos.pos[1];
                start.mZ = pos.pos[2];

                mPathFinder.buildPath(start, dest, mPathgrid, mXCell, mYCell, false);

                if(mPathFinder.isPathConstructed())
                {
                    // Remove this node as an option and add back the previously used node (stops NPC from picking the same node):
                    ESM::Pathgrid::Point temp = mAllowedNodes[randNode];
                    mAllowedNodes.erase(mAllowedNodes.begin() + randNode);
                    mAllowedNodes.push_back(mCurrentNode);
                    mCurrentNode = temp;

                    mMoveNow = false;
                    mWalking = true;
                }
                // Choose a different node and delete this one from possible nodes because it is uncreachable:
                else
                    mAllowedNodes.erase(mAllowedNodes.begin() + randNode);
            }
        }

        if(mWalking)
        {
            float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
            MWBase::Environment::get().getWorld()->rotateObject(actor, 0, 0, zAngle,false);
            MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;

            // Unclog path nodes by allowing the NPC to be a small distance away from the center. This way two NPCs can be
            // at the same path node at the same time and both will complete instead of endlessly walking into eachother:
            Ogre::Vector3 destNodePos(mCurrentNode.mX, mCurrentNode.mY, mCurrentNode.mZ);
            Ogre::Vector3 actorPos(actor.getRefData().getPosition().pos);
            actorPos[0] = actorPos[0] - mXCell;
            actorPos[1] = actorPos[1] - mYCell;
            float distance = actorPos.squaredDistance(destNodePos);

            if(distance < 1200 || mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], pos.pos[2]))
            {
                stopWalking(actor);
                mMoveNow = false;
                mWalking = false;
                mChooseAction = true;
            }
        }

        return false;
    }

    int AiWander::getTypeId() const
    {
        return 0;
    }

    void AiWander::stopWalking(const MWWorld::Ptr& actor)
    {
        mPathFinder.clearPath();
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
    }

    void AiWander::playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if(idleSelect == 2)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle2", 0, 1);
        else if(idleSelect == 3)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle3", 0, 1);
        else if(idleSelect == 4)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle4", 0, 1);
        else if(idleSelect == 5)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle5", 0, 1);
        else if(idleSelect == 6)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle6", 0, 1);
        else if(idleSelect == 7)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle7", 0, 1);
        else if(idleSelect == 8)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle8", 0, 1);
        else if(idleSelect == 9)
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle9", 0, 1);
    }

    bool AiWander::checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if(idleSelect == 2)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle2");
        else if(idleSelect == 3)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle3");
        else if(idleSelect == 4)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle4");
        else if(idleSelect == 5)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle5");
        else if(idleSelect == 6)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle6");
        else if(idleSelect == 7)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle7");
        else if(idleSelect == 8)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle8");
        else if(idleSelect == 9)
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, "idle9");
        else
            return false;
    }
}

