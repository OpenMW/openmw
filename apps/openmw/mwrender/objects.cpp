#include "objects.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSubEntity.h>
#include <OgreStaticGeometry.h>

#include <components/nifogre/ogrenifloader.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "renderconst.hpp"

using namespace MWRender;
float Objects::lightLinearValue()
{
    return mFallback->getFallbackFloat("LightAttenuation_LinearValue");
}
float Objects::lightLinearRadiusMult()
{
    return mFallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
}
float Objects::lightQuadraticValue()
{
    return mFallback->getFallbackFloat("LightAttenuation_QuadraticValue");
}
float Objects::lightQuadraticRadiusMult()
{
    return mFallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
}

bool Objects::lightOutQuadInLin()
{
    return mFallback->getFallbackBool("LightAttenuation_OutQuadInLin");
}
bool Objects::lightQuadratic()
{
    return mFallback->getFallbackBool("LightAttenuation_UseQuadratic");
}

int Objects::uniqueID = 0;

void Objects::clearSceneNode (Ogre::SceneNode *node)
{
    for (int i=node->numAttachedObjects()-1; i>=0; --i)
    {
        Ogre::MovableObject *object = node->getAttachedObject (i);

        // for entities, destroy any objects attached to bones
        if (object->getTypeFlags () == Ogre::SceneManager::ENTITY_TYPE_MASK)
        {
            Ogre::Entity* ent = static_cast<Ogre::Entity*>(object);
            Ogre::Entity::ChildObjectListIterator children = ent->getAttachedObjectIterator ();
            while (children.hasMoreElements())
            {
                mRenderer.getScene ()->destroyMovableObject (children.getNext ());
            }
        }

        node->detachObject (object);
        mRenderer.getScene()->destroyMovableObject (object);
    }

    Ogre::Node::ChildNodeIterator it = node->getChildIterator ();
    while (it.hasMoreElements ())
    {
        clearSceneNode(static_cast<Ogre::SceneNode*>(it.getNext ()));
    }
}

void Objects::setRootNode(Ogre::SceneNode* root)
{
    mRootNode = root;
}

void Objects::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_)
{
    Ogre::SceneNode* root = mRootNode;
    Ogre::SceneNode* cellnode;
    if(mCellSceneNodes.find(ptr.getCell()) == mCellSceneNodes.end())
    {
        //Create the scenenode and put it in the map
        cellnode = root->createChildSceneNode();
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
    {
        cellnode = mCellSceneNodes[ptr.getCell()];
    }

    Ogre::SceneNode* insert = cellnode->createChildSceneNode();
    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(f[0], f[1], f[2]);
    insert->setScale(ptr.getCellRef().mScale, ptr.getCellRef().mScale, ptr.getCellRef().mScale);


    // Convert MW rotation to a quaternion:
    f = ptr.getCellRef().mPos.rot;

    // Rotate around X axis
    Ogre::Quaternion xr(Ogre::Radian(-f[0]), Ogre::Vector3::UNIT_X);

    // Rotate around Y axis
    Ogre::Quaternion yr(Ogre::Radian(-f[1]), Ogre::Vector3::UNIT_Y);

    // Rotate around Z axis
    Ogre::Quaternion zr(Ogre::Radian(-f[2]), Ogre::Vector3::UNIT_Z);

    // Rotates first around z, then y, then x
    insert->setOrientation(xr*yr*zr);

    if (!enabled)
         insert->setVisible (false);
    ptr.getRefData().setBaseNode(insert);
    mIsStatic = static_;
}

void Objects::insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh, bool light)
{
    Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
    assert(insert);

    Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
    NifOgre::EntityList entities = NifOgre::Loader::createEntities(insert, mesh);
    for(size_t i = 0;i < entities.mEntities.size();i++)
        bounds.merge(entities.mEntities[i]->getWorldBoundingBox(true));

    Ogre::Vector3 extents = bounds.getSize();
    extents *= insert->getScale();
    float size = std::max(std::max(extents.x, extents.y), extents.z);

    bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) && Settings::Manager::getBool("limit small object distance", "Viewing distance");

    // do not fade out doors. that will cause holes and look stupid
    if (ptr.getTypeName().find("Door") != std::string::npos)
        small = false;

    if (mBounds.find(ptr.getCell()) == mBounds.end())
        mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;
    mBounds[ptr.getCell()].merge(bounds);

    bool anyTransparency = false;
    for(size_t i = 0;!anyTransparency && i < entities.mEntities.size();i++)
    {
        Ogre::Entity *ent = entities.mEntities[i];
        for(unsigned int i=0;!anyTransparency && i < ent->getNumSubEntities(); ++i)
        {
            anyTransparency = ent->getSubEntity(i)->getMaterial()->isTransparent();
        }
    }

    if(!mIsStatic || !Settings::Manager::getBool("use static geometry", "Objects") || anyTransparency)
    {
        for(size_t i = 0;i < entities.mEntities.size();i++)
        {
            Ogre::Entity *ent = entities.mEntities[i];
            for(unsigned int i=0; i < ent->getNumSubEntities(); ++i)
            {
                Ogre::SubEntity* subEnt = ent->getSubEntity(i);
                subEnt->setRenderQueueGroup(subEnt->getMaterial()->isTransparent() ? RQG_Alpha : RQG_Main);
            }
            ent->setRenderingDistance(small ? Settings::Manager::getInt("small object distance", "Viewing distance") : 0);
            ent->setVisibilityFlags(mIsStatic ? (small ? RV_StaticsSmall : RV_Statics) : RV_Misc);
        }
    }
    else
    {
        Ogre::StaticGeometry* sg = 0;

        if (small)
        {
            if( mStaticGeometrySmall.find(ptr.getCell()) == mStaticGeometrySmall.end())
            {
                uniqueID = uniqueID +1;
                sg = mRenderer.getScene()->createStaticGeometry( "sg" + Ogre::StringConverter::toString(uniqueID));
                mStaticGeometrySmall[ptr.getCell()] = sg;

                sg->setRenderingDistance(Settings::Manager::getInt("small object distance", "Viewing distance"));
            }
            else
                sg = mStaticGeometrySmall[ptr.getCell()];
        }
        else
        {
            if( mStaticGeometry.find(ptr.getCell()) == mStaticGeometry.end())
            {

                uniqueID = uniqueID +1;
                sg = mRenderer.getScene()->createStaticGeometry( "sg" + Ogre::StringConverter::toString(uniqueID));
                mStaticGeometry[ptr.getCell()] = sg;
            }
            else
                sg = mStaticGeometry[ptr.getCell()];
        }

        // This specifies the size of a single batch region.
        // If it is set too high:
        //  - there will be problems choosing the correct lights
        //  - the culling will be more inefficient
        // If it is set too low:
        //  - there will be too many batches.
        sg->setRegionDimensions(Ogre::Vector3(2500,2500,2500));

        sg->setVisibilityFlags(small ? RV_StaticsSmall : RV_Statics);

        sg->setCastShadows(true);

        sg->setRenderQueueGroup(RQG_Main);

        std::vector<Ogre::Entity*>::reverse_iterator iter = entities.mEntities.rbegin();
        while(iter != entities.mEntities.rend())
        {
            Ogre::Node *node = (*iter)->getParentNode();
            sg->addEntity(*iter, node->_getDerivedPosition(), node->_getDerivedOrientation(), node->_getDerivedScale());

            (*iter)->detachFromParent();
            mRenderer.getScene()->destroyEntity(*iter);
            iter++;
        }
    }

    if (light)
    {
        insertLight(ptr, entities.mSkelBase, bounds.getCenter() - insert->_getDerivedPosition());
    }
}

void Objects::insertLight (const MWWorld::Ptr& ptr, Ogre::Entity* skelBase, Ogre::Vector3 fallbackCenter)
{
    Ogre::SceneNode* insert = mRenderer.getScene()->getSceneNode(ptr.getRefData().getHandle());
    assert(insert);

    MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

    const int color = ref->mBase->mData.mColor;
    const float r = ((color >> 0) & 0xFF) / 255.0f;
    const float g = ((color >> 8) & 0xFF) / 255.0f;
    const float b = ((color >> 16) & 0xFF) / 255.0f;
    const float radius = float (ref->mBase->mData.mRadius);

    Ogre::Light *light = mRenderer.getScene()->createLight();
    light->setDiffuseColour (r, g, b);

    LightInfo info;
    info.name = light->getName();
    info.radius = radius;
    info.colour = Ogre::ColourValue(r, g, b);

    if (ref->mBase->mData.mFlags & ESM::Light::Negative)
        info.colour *= -1;

    info.interior = !ptr.getCell()->mCell->isExterior();

    if (ref->mBase->mData.mFlags & ESM::Light::Flicker)
        info.type = LT_Flicker;
    else if (ref->mBase->mData.mFlags & ESM::Light::FlickerSlow)
        info.type = LT_FlickerSlow;
    else if (ref->mBase->mData.mFlags & ESM::Light::Pulse)
        info.type = LT_Pulse;
    else if (ref->mBase->mData.mFlags & ESM::Light::PulseSlow)
        info.type = LT_PulseSlow;
    else
        info.type = LT_Normal;

    // randomize lights animations
    info.time = Ogre::Math::RangeRandom(-500, +500);
    info.phase = Ogre::Math::RangeRandom(-500, +500);

    // changed to linear to look like morrowind
    bool quadratic = false;
    /*
    if (!lightOutQuadInLin)
        quadratic = lightQuadratic;
    else
    {
        quadratic = !info.interior;
    }
    */

    if (!quadratic)
    {
        float r = radius * lightLinearRadiusMult();
        float attenuation = lightLinearValue() / r;
        light->setAttenuation(r*10, 0, attenuation, 0);
    }
    else
    {
        float r = radius * lightQuadraticRadiusMult();
        float attenuation = lightQuadraticValue() / pow(r, 2);
        light->setAttenuation(r*10, 0, 0, attenuation);
    }

    // If there's an AttachLight bone, attach the light to that, otherwise attach it to the base scene node
    if (skelBase && skelBase->getSkeleton ()->hasBone ("AttachLight"))
    {
        skelBase->attachObjectToBone ("AttachLight", light);
    }
    else
    {
        Ogre::SceneNode* childNode = insert->createChildSceneNode (fallbackCenter);
        childNode->attachObject(light);
    }

    mLights.push_back(info);
}

bool Objects::deleteObject (const MWWorld::Ptr& ptr)
{
    if (Ogre::SceneNode *base = ptr.getRefData().getBaseNode())
    {
        Ogre::SceneNode *parent = base->getParentSceneNode();

        for (std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *>::const_iterator iter (
            mCellSceneNodes.begin()); iter!=mCellSceneNodes.end(); ++iter)
            if (iter->second==parent)
            {
                clearSceneNode (base);
                base->removeAndDestroyAllChildren();
                mRenderer.getScene()->destroySceneNode (base);
                ptr.getRefData().setBaseNode (0);
                return true;
            }

        return false;
    }

    return true;
}

void Objects::removeCell(MWWorld::Ptr::CellStore* store)
{
    if(mCellSceneNodes.find(store) != mCellSceneNodes.end())
    {
        Ogre::SceneNode* base = mCellSceneNodes[store];

        for (int i=0; i<base->numChildren(); ++i)
            clearSceneNode (static_cast<Ogre::SceneNode *> (base->getChild (i)));

        base->removeAndDestroyAllChildren();
        mCellSceneNodes.erase(store);
        mRenderer.getScene()->destroySceneNode(base);
        base = 0;
    }

    if(mStaticGeometry.find(store) != mStaticGeometry.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometry[store];
        mStaticGeometry.erase(store);
        mRenderer.getScene()->destroyStaticGeometry (sg);
        sg = 0;
    }
    if(mStaticGeometrySmall.find(store) != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometrySmall[store];
        mStaticGeometrySmall.erase(store);
        mRenderer.getScene()->destroyStaticGeometry (sg);
        sg = 0;
    }

    if(mBounds.find(store) != mBounds.end())
        mBounds.erase(store);
}

void Objects::buildStaticGeometry(MWWorld::Ptr::CellStore& cell)
{
    if(mStaticGeometry.find(&cell) != mStaticGeometry.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometry[&cell];
        sg->build();
    }
    if(mStaticGeometrySmall.find(&cell) != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometrySmall[&cell];
        sg->build();
    }
}

Ogre::AxisAlignedBox Objects::getDimensions(MWWorld::Ptr::CellStore* cell)
{
    return mBounds[cell];
}

void Objects::enableLights()
{
    std::vector<LightInfo>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mRootNode->getCreator()->hasLight(it->name))
        {
            mRootNode->getCreator()->getLight(it->name)->setVisible(true);
            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

void Objects::disableLights()
{
    std::vector<LightInfo>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mRootNode->getCreator()->hasLight(it->name))
        {
            mRootNode->getCreator()->getLight(it->name)->setVisible(false);
            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

namespace MWRender
{
    namespace Pulse
    {
        static float amplitude (float phase)
        {
            return sin (phase);
        }
    }

    namespace Flicker
    {
        static const float fa = 0.785398f;
        static const float fb = 1.17024f;

        static const float tdo = 0.94f;
        static const float tdm = 2.48f;

        static const float f [3] = { 1.5708f,   4.18774f, 5.19934f };
        static const float o [3] = { 0.804248f, 2.11115f, 3.46832f };
        static const float m [3] = { 1.0f,      0.785f,   0.876f   };
        static const float s = 0.394f;

        static const float phase_wavelength = 120.0f * 3.14159265359f / fa;

        static float frequency (float x)
        {
            return tdo + tdm * sin (fa * x);
        }

        static float amplitude (float x)
        {
            float v = 0.0f;
            for (int i = 0; i < 3; ++i)
                v += sin (fb*x*f[i] + o[1])*m[i];
            return v * s;
        }
    }
}

void Objects::update(const float dt)
{
    std::vector<LightInfo>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mRootNode->getCreator()->hasLight(it->name))
        {
            Ogre::Light* light = mRootNode->getCreator()->getLight(it->name);

            float brightness;
            float cycle_time;
            float time_distortion;

            if ((it->type == LT_Pulse) && (it->type == LT_PulseSlow))
            {
                cycle_time = 2 * Ogre::Math::PI;
                time_distortion = 20.0f;
            }
            else
            {
                cycle_time = 500.0f;
                it->phase = fmod (it->phase + dt, Flicker::phase_wavelength);
                time_distortion = Flicker::frequency (it->phase);
            }

            it->time += it->dir*dt*time_distortion;
            if (it->dir > 0 && it->time > +cycle_time)
            {
                it->dir = -1.0f;
                it->time = +2*cycle_time - it->time;
            }
            if (it->dir < 0 && it->time < -cycle_time)
            {
                it->dir = +1.0f;
                it->time = -2*cycle_time - it->time;
            }

            static const float fast = 4.0f/1.0f;
            static const float slow = 1.0f/1.0f;

            // These formulas are just guesswork, but they work pretty well
            if (it->type == LT_Normal)
            {
                // Less than 1/255 light modifier for a constant light:
                brightness = (const float)(1.0 + Flicker::amplitude(it->time*slow) / 255.0 );
            }
            else if (it->type == LT_Flicker)
            {
                brightness = (const float)(0.75 + Flicker::amplitude(it->time*fast) * 0.25);
            }
            else if (it->type == LT_FlickerSlow)
            {
                brightness = (const float)(0.75 + Flicker::amplitude(it->time*slow) * 0.25);
            }
            else if (it->type == LT_Pulse)
            {
                brightness = (const float)(1.0 + Pulse::amplitude (it->time*fast) * 0.25);
            }
            else if (it->type == LT_PulseSlow)
            {
                brightness = (const float)(1.0 + Pulse::amplitude (it->time*slow) * 0.25);
            }
            else
                assert(0 && "Invalid light type");

            light->setDiffuseColour(it->colour * brightness);

            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

void Objects::rebuildStaticGeometry()
{
    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometry.begin(); it != mStaticGeometry.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }

    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometrySmall.begin(); it != mStaticGeometrySmall.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }
}

void Objects::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    Ogre::SceneNode *node;
    MWWorld::CellStore *newCell = cur.getCell();

    if(mCellSceneNodes.find(newCell) == mCellSceneNodes.end()) {
        node = mRootNode->createChildSceneNode();
        mCellSceneNodes[newCell] = node;
    } else {
        node = mCellSceneNodes[newCell];
    }
    node->addChild(cur.getRefData().getBaseNode());
}

