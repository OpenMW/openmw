#include "creatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSubEntity.h>

#include "renderconst.hpp"

#include "../mwbase/world.hpp"

using namespace Ogre;
using namespace NifOgre;
namespace MWRender{

CreatureAnimation::~CreatureAnimation()
{
}                 

CreatureAnimation::CreatureAnimation(const MWWorld::Ptr& ptr, OEngine::Render::OgreRenderer& _rend): Animation(_rend)
{
    mInsert = ptr.getRefData().getBaseNode();
    MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

    assert (ref->base != NULL);
    if(!ref->base->model.empty())
    {
        const std::string &mesh = "meshes\\" + ref->base->model;
        std::string meshNumbered = mesh + getUniqueID(mesh) + ">|";
        NifOgre::NIFLoader::load(meshNumbered);
        mBase = mRend.getScene()->createEntity(meshNumbered);
        mBase->setVisibilityFlags(RV_Actors);

        bool transparent = false;
        for (unsigned int i=0; i < mBase->getNumSubEntities(); ++i)
        {
            Ogre::MaterialPtr mat = mBase->getSubEntity(i)->getMaterial();
            Ogre::Material::TechniqueIterator techIt = mat->getTechniqueIterator();
            while (techIt.hasMoreElements())
            {
                Ogre::Technique* tech = techIt.getNext();
                Ogre::Technique::PassIterator passIt = tech->getPassIterator();
                while (passIt.hasMoreElements())
                {
                    Ogre::Pass* pass = passIt.getNext();

                    if (pass->getDepthWriteEnabled() == false)
                        transparent = true;
                }
            }
        }
        mBase->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);

        std::string meshZero = mesh + "0000>|";

        if((mTransformations = (NIFLoader::getSingletonPtr())->getAnim(meshZero)))
        {
            for(std::size_t init = 0; init < mTransformations->size(); init++)
            {
                mRindexI.push_back(0);
                mTindexI.push_back(0);
            }
            mStopTime = mTransformations->begin()->getStopTime();
            mStartTime = mTransformations->begin()->getStartTime();
            mShapes = (NIFLoader::getSingletonPtr())->getShapes(meshZero);
        }
        mTextmappings = NIFLoader::getSingletonPtr()->getTextIndices(meshZero);
        mInsert->attachObject(mBase);
    }
}

void CreatureAnimation::runAnimation(float timepassed)
{
    mVecRotPos.clear();
    if(mAnimate > 0)
    {
        //Add the amount of time passed to time

        //Handle the animation transforms dependent on time

        //Handle the shapes dependent on animation transforms
        mTime += timepassed;
        if(mTime >= mStopTime)
        {
            mAnimate--;
            //std::cout << "Stopping the animation\n";
            if(mAnimate == 0)
                mTime = mStopTime;
            else
                mTime = mStartTime + (mTime - mStopTime);
        }

        handleAnimationTransforms();
        handleShapes(mShapes, mBase, mBase->getSkeleton());

    }
}

}
