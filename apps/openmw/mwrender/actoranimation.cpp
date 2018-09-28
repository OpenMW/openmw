#include "actoranimation.hpp"

#include <utility>

#include <osg/Node>
#include <osg/Group>
#include <osg/Vec4f>

#include <components/esm/loadligh.hpp>
#include <components/esm/loadcell.hpp>

#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>

#include <components/fallback/fallback.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "vismask.hpp"

namespace MWRender
{

ActorAnimation::ActorAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
    : Animation(ptr, parentNode, resourceSystem)
{
    MWWorld::ContainerStore& store = mPtr.getClass().getContainerStore(mPtr);

    for (MWWorld::ConstContainerStoreIterator iter = store.cbegin(MWWorld::ContainerStore::Type_Light);
         iter != store.cend(); ++iter)
    {
        const ESM::Light* light = iter->get<ESM::Light>()->mBase;
        if (!(light->mData.mFlags & ESM::Light::Carry))
        {
            addHiddenItemLight(*iter, light);
        }
    }

    // Make sure we cleaned object from effects, just in cast if we re-use node
    removeEffects();
}

ActorAnimation::~ActorAnimation()
{
    for (ItemLightMap::iterator iter = mItemLights.begin(); iter != mItemLights.end(); ++iter)
    {
        mInsert->removeChild(iter->second);
    }
}

void ActorAnimation::itemAdded(const MWWorld::ConstPtr& item, int /*count*/)
{
    if (item.getTypeName() == typeid(ESM::Light).name())
    {
        const ESM::Light* light = item.get<ESM::Light>()->mBase;
        if (!(light->mData.mFlags & ESM::Light::Carry))
        {
            addHiddenItemLight(item, light);
        }
    }
}

void ActorAnimation::itemRemoved(const MWWorld::ConstPtr& item, int /*count*/)
{
    if (item.getTypeName() == typeid(ESM::Light).name())
    {
        ItemLightMap::iterator iter = mItemLights.find(item);
        if (iter != mItemLights.end())
        {
            if (!item.getRefData().getCount())
            {
                removeHiddenItemLight(item);
            }
        }
    }
}

void ActorAnimation::addHiddenItemLight(const MWWorld::ConstPtr& item, const ESM::Light* esmLight)
{
    if (mItemLights.find(item) != mItemLights.end())
        return;

    const Fallback::Map* fallback = MWBase::Environment::get().getWorld()->getFallback();
    static bool outQuadInLin = fallback->getFallbackBool("LightAttenuation_OutQuadInLin");
    static bool useQuadratic = fallback->getFallbackBool("LightAttenuation_UseQuadratic");
    static float quadraticValue = fallback->getFallbackFloat("LightAttenuation_QuadraticValue");
    static float quadraticRadiusMult = fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
    static bool useLinear = fallback->getFallbackBool("LightAttenuation_UseLinear");
    static float linearRadiusMult = fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
    static float linearValue = fallback->getFallbackFloat("LightAttenuation_LinearValue");
    bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

    osg::Vec4f ambient(1,1,1,1);
    osg::ref_ptr<SceneUtil::LightSource> lightSource = SceneUtil::createLightSource(esmLight, Mask_Lighting, exterior, outQuadInLin,
                                 useQuadratic, quadraticValue, quadraticRadiusMult, useLinear, linearRadiusMult, linearValue, ambient);

    mInsert->addChild(lightSource);

    if (mLightListCallback && mPtr == MWMechanics::getPlayer())
        mLightListCallback->getIgnoredLightSources().insert(lightSource.get());

    mItemLights.insert(std::make_pair(item, lightSource));
}

void ActorAnimation::removeHiddenItemLight(const MWWorld::ConstPtr& item)
{
    ItemLightMap::iterator iter = mItemLights.find(item);
    if (iter == mItemLights.end())
        return;

    if (mLightListCallback && mPtr == MWMechanics::getPlayer())
    {
        std::set<SceneUtil::LightSource*>::iterator ignoredIter = mLightListCallback->getIgnoredLightSources().find(iter->second.get());
        if (ignoredIter != mLightListCallback->getIgnoredLightSources().end())
            mLightListCallback->getIgnoredLightSources().erase(ignoredIter);
    }

    mInsert->removeChild(iter->second);
    mItemLights.erase(iter);
}

}
