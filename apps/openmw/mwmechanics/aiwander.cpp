#include "aiwander.hpp"

#include <OgreVector3.h>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"

namespace MWMechanics
{
    // NOTE: determined empirically but probably need further tweaking
    static const int COUNT_BEFORE_RESET = 200;
    static const float DIST_SAME_SPOT = 1.8f;
    static const float DURATION_SAME_SPOT = 1.0f;
    static const float DURATION_TO_EVADE = 0.4f;

    AiWander::AiWander(int distance, int duration, int timeOfDay, const std::vector<int>& idle, bool repeat):
        mDistance(distance), mDuration(duration), mTimeOfDay(timeOfDay), mIdle(idle), mRepeat(repeat)
      , mCellX(std::numeric_limits<int>::max())
      , mCellY(std::numeric_limits<int>::max())
      , mXCell(0)
      , mYCell(0)
      , mX(0)
      , mY(0)
      , mZ(0)
      , mPrevX(0)
      , mPrevY(0)
      , mWalkState(State_Norm)
      , mStuckCount(0)
      , mEvadeDuration(0)
      , mStuckDuration(0)
      , mSaidGreeting(false)
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

    // TODO: duration is passed in but never used, check if it is needed
    bool AiWander::execute (const MWWorld::Ptr& actor,float duration)
    {
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);
        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, false);
        MWBase::World *world = MWBase::Environment::get().getWorld();
        if(mDuration)
        {
            // End package if duration is complete or mid-night hits:
            MWWorld::TimeStamp currentTime = world->getTimeStamp();
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

        // Once off initialization to discover & store allowed node points for this actor.
        if(!mStoredAvailableNodes)
        {
            mPathgrid = world->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->getCell());

            mCellX = actor.getCell()->getCell()->mData.mX;
            mCellY = actor.getCell()->getCell()->mData.mY;

            // TODO: If there is no path does this actor get stuck forever?
            if(!mPathgrid)
                mDistance = 0;
            else if(mPathgrid->mPoints.empty())
                mDistance = 0;

            if(mDistance) // A distance value is initially passed into the constructor.
            {
                mXCell = 0;
                mYCell = 0;
                if(actor.getCell()->getCell()->isExterior())
                {
                    mXCell = mCellX * ESM::Land::REAL_SIZE;
                    mYCell = mCellY * ESM::Land::REAL_SIZE;
                }

                // convert npcPos to local (i.e. cell) co-ordinates
                Ogre::Vector3 npcPos(actor.getRefData().getPosition().pos);
                npcPos[0] = npcPos[0] - mXCell;
                npcPos[1] = npcPos[1] - mYCell;

                // populate mAllowedNodes for this actor with pathgrid point indexes based on mDistance
                // NOTE: mPoints and mAllowedNodes contain points in local co-ordinates
                for(unsigned int counter = 0; counter < mPathgrid->mPoints.size(); counter++)
                {
                    Ogre::Vector3 nodePos(mPathgrid->mPoints[counter].mX,
                                          mPathgrid->mPoints[counter].mY,
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
                        Ogre::Vector3 nodePos(mAllowedNodes[counterThree].mX,
                                              mAllowedNodes[counterThree].mY,
                                              mAllowedNodes[counterThree].mZ);
                        float tempDist = npcPos.squaredDistance(nodePos);
                        if(tempDist < closestNode)
                            index = counterThree;
                    }
                    mCurrentNode = mAllowedNodes[index];
                    mAllowedNodes.erase(mAllowedNodes.begin() + index);

                    mStoredAvailableNodes = true; // set only if successful in finding allowed nodes
                }
            }
        }

        // TODO: Does this actor stay in one spot forever while in AiWander?
        if(mAllowedNodes.empty())
            mDistance = 0;

        // Don't try to move if you are in a new cell (ie: positioncell command called) but still play idles.
        if(mDistance && (mCellX != actor.getCell()->getCell()->mData.mX || mCellY != actor.getCell()->getCell()->mData.mY))
            mDistance = 0;

        if(mChooseAction) // Initially set true by the constructor.
        {
            mPlayedIdle = 0;
            unsigned short idleRoll = 0;

            for(unsigned int counter = 0; counter < mIdle.size(); counter++)
            {
                unsigned short idleChance = mIdleChanceMultiplier * mIdle[counter];
                unsigned short randSelect = (int)(rand() / ((double)RAND_MAX + 1) * int(100 / mIdleChanceMultiplier));
                if(randSelect < idleChance && randSelect > idleRoll)
                {
                    mPlayedIdle = counter+2;
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
                MWWorld::TimeStamp currentTime = world->getTimeStamp();
                mStartTime = currentTime;
                playIdle(actor, mPlayedIdle);
                mChooseAction = false;
                mIdleNow = true;

                // Play idle voiced dialogue entries randomly
                int hello = actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Hello).getModified();
                if (hello > 0)
                {
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    float chance = store.get<ESM::GameSetting>().find("fVoiceIdleOdds")->getFloat();
                    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

                    // Don't bother if the player is out of hearing range
                    if (roll < chance && Ogre::Vector3(player.getRefData().getPosition().pos).distance(Ogre::Vector3(actor.getRefData().getPosition().pos)) < 1500)
                        MWBase::Environment::get().getDialogueManager()->say(actor, "idle");
                }
            }
        }

        // Allow interrupting a walking actor to trigger a greeting
        if(mIdleNow || (mWalking && (mWalkState != State_Norm)))
        {
            // Play a random voice greeting if the player gets too close
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

            int hello = actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Hello).getModified();
            float helloDistance = hello;
            int iGreetDistanceMultiplier = store.get<ESM::GameSetting>().find("iGreetDistanceMultiplier")->getInt();
            helloDistance *= iGreetDistanceMultiplier;

            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            float playerDist = Ogre::Vector3(player.getRefData().getPosition().pos).distance(
                        Ogre::Vector3(actor.getRefData().getPosition().pos));

            if(mWalking && playerDist <= helloDistance)
            {
                stopWalking(actor);
                mMoveNow = false;
                mWalking = false;
                mWalkState = State_Norm;
            }

            if (!mSaidGreeting)
            {
                // TODO: check if actor is aware / has line of sight
                if (playerDist <= helloDistance
                        // Only play a greeting if the player is not moving
                        && Ogre::Vector3(player.getClass().getMovementSettings(player).mPosition).squaredLength() == 0)
                {
                    mSaidGreeting = true;
                    MWBase::Environment::get().getDialogueManager()->say(actor, "hello");
                    // TODO: turn to face player and interrupt the idle animation?
                }
            }
            else
            {
                float fGreetDistanceReset = store.get<ESM::GameSetting>().find("fGreetDistanceReset")->getFloat();
                if (playerDist >= fGreetDistanceReset * iGreetDistanceMultiplier)
                    mSaidGreeting = false;
            }

            // Check if idle animation finished
            if(!checkIdle(actor, mPlayedIdle))
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
                assert(mAllowedNodes.size());
                unsigned int randNode = (int)(rand() / ((double)RAND_MAX + 1) * mAllowedNodes.size());
                Ogre::Vector3 destNodePos(mAllowedNodes[randNode].mX, mAllowedNodes[randNode].mY, mAllowedNodes[randNode].mZ);

                ESM::Pathgrid::Point dest;
                dest.mX = destNodePos[0] + mXCell;
                dest.mY = destNodePos[1] + mYCell;
                dest.mZ = destNodePos[2];

                // actor position is already in world co-ordinates
                ESM::Pathgrid::Point start;
                start.mX = pos.pos[0];
                start.mY = pos.pos[1];
                start.mZ = pos.pos[2];

                // don't take shortcuts for wandering
                mPathFinder.buildPath(start, dest, actor.getCell(), false);

                if(mPathFinder.isPathConstructed())
                {
                    // buildPath inserts dest in case it is not a pathgraph point index
                    // which is a duplicate for AiWander
                    //if(mPathFinder.getPathSize() > 1)
                        //mPathFinder.getPath().pop_back();

                    // Remove this node as an option and add back the previously used node
                    // (stops NPC from picking the same node):
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
            if(mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], pos.pos[2]))
            {
                stopWalking(actor);
                mMoveNow = false;
                mWalking = false;
                mChooseAction = true;
            }
            else
            {
                /*               f                    t
                 *  State_Norm <---> State_CheckStuck --> State_Evade
                 *   ^  ^   |          ^   |               ^   |  |
                 *   |  |   |          |   |               |   |  |
                 *   |  +---+          +---+               +---+  | u
                 *   |   any            < t                 < u   |
                 *   +--------------------------------------------+
                 *
                 * f = one frame
                 * t = how long before considered stuck
                 * u = how long to move sideways
                 */
                bool samePosition = (abs(pos.pos[0] - mPrevX) < DIST_SAME_SPOT) &&
                                    (abs(pos.pos[1] - mPrevY) < DIST_SAME_SPOT);

                switch(mWalkState)
                {
                    case State_Norm:
                    {
                        if(!samePosition)
                            break;
                        else
                            mWalkState = State_CheckStuck;
                    }
                        /* FALL THROUGH */
                    case State_CheckStuck:
                    {
                        if(!samePosition)
                        {
                            mWalkState = State_Norm;
                            mStuckDuration = 0;
                            break;
                        }
                        else
                        {
                            mStuckDuration += duration;
                            // consider stuck only if position unchanges for a period
                            if(mStuckDuration > DURATION_SAME_SPOT)
                            {
                                mWalkState = State_Evade;
                                mStuckDuration = 0;
                                mStuckCount++;
                            }
                            else
                                break; // still in the same state, but duration added to timer
                        }
                    }
                        /* FALL THROUGH */
                    case State_Evade:
                    {
                        mEvadeDuration += duration;
                        if(mEvadeDuration < DURATION_TO_EVADE)
                            break;
                        else
                        {
                            mWalkState = State_Norm; // tried to evade, assume all is ok and start again
                            mEvadeDuration = 0;
                        }
                    }
                    /* NO DEFAULT CASE */
                }

                if(mWalkState == State_Evade)
                {
                    //std::cout << "Stuck \""<<actor.getClass().getName(actor)<<"\"" << std::endl;

                    // diagonal should have same animation as walk forward
                    actor.getClass().getMovementSettings(actor).mPosition[0] = 1;
                    actor.getClass().getMovementSettings(actor).mPosition[1] = 0.01f;
                    // change the angle a bit, too
                    zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0] + 1, pos.pos[1])));
                }
                else
                {
                    // normal walk forward
                    actor.getClass().getMovementSettings(actor).mPosition[1] = 1;
                    // turn towards the next point in mPath
                    // TODO: possibly no need to check every frame, maybe every 30 should be ok?
                    zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1])));
                }

                if(mStuckCount >= COUNT_BEFORE_RESET) // something has gone wrong, reset
                {
                    //std::cout << "Reset \""<<actor.getClass().getName(actor)<<"\"" << std::endl;
                    mWalkState = State_Norm;
                    mStuckCount = 0;

                    stopWalking(actor);
                    mMoveNow = false;
                    mWalking = false;
                    mChooseAction = true;
                }

                // update position
                ESM::Position updatedPos = actor.getRefData().getPosition();
                mPrevX = updatedPos.pos[0];
                mPrevY = updatedPos.pos[1];
            }
        }

        return false;
    }

    int AiWander::getTypeId() const
    {
        return TypeIdWander;
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

