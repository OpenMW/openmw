#include "objects.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSubEntity.h>
#include <OgreStaticGeometry.h>

#include <components/nifogre/ogre_nif_loader.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"

#include "renderconst.hpp"

using namespace MWRender;

// These are the Morrowind.ini defaults
float Objects::lightLinearValue = 3;
float Objects::lightLinearRadiusMult = 1;

float Objects::lightQuadraticValue = 16;
float Objects::lightQuadraticRadiusMult = 1;

bool Objects::lightOutQuadInLin = true;
bool Objects::lightQuadratic = false;

int Objects::uniqueID = 0;

void Objects::clearSceneNode (Ogre::SceneNode *node)
{
    /// \todo This should probably be moved into OpenEngine at some point.
    for (int i=node->numAttachedObjects()-1; i>=0; --i)
    {
        Ogre::MovableObject *object = node->getAttachedObject (i);
        node->detachObject (object);
        mRenderer.getScene()->destroyMovableObject (object);
    }
}

void Objects::setMwRoot(Ogre::SceneNode* root)
{
    mMwRoot = root;
}

void Objects::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_)
{
    Ogre::SceneNode* root = mMwRoot;
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
    insert->setScale(ptr.getCellRef().scale, ptr.getCellRef().scale, ptr.getCellRef().scale);


    // Convert MW rotation to a quaternion:
    f = ptr.getCellRef().pos.rot;

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

void Objects::insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh)
{
    Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
    assert(insert);

    Ogre::AxisAlignedBox bounds = Ogre::AxisAlignedBox::BOX_NULL;
    NifOgre::EntityList entities = NifOgre::NIFLoader::createEntities(insert, NULL, mesh);
    for(size_t i = 0;i < entities.mEntities.size();i++)
    {
        const Ogre::AxisAlignedBox &tmp = entities.mEntities[i]->getBoundingBox();
        bounds.merge(Ogre::AxisAlignedBox(insert->_getDerivedPosition() + tmp.getMinimum(),
                                          insert->_getDerivedPosition() + tmp.getMaximum())
        );
    }
    Ogre::Vector3 extents = bounds.getSize();
    extents *= insert->getScale();
    float size = std::max(std::max(extents.x, extents.y), extents.z);

    bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) && Settings::Manager::getBool("limit small object distance", "Objects");

    // do not fade out doors. that will cause holes and look stupid
    if (ptr.getTypeName().find("Door") != std::string::npos)
        small = false;

    if (mBounds.find(ptr.getCell()) == mBounds.end())
        mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;
    mBounds[ptr.getCell()].merge(bounds);

    bool transparent = false;
    for(size_t i = 0;i < entities.mEntities.size();i++)
    {
        Ogre::Entity *ent = entities.mEntities[i];
        for (unsigned int i=0; i<ent->getNumSubEntities(); ++i)
        {
            Ogre::MaterialPtr mat = ent->getSubEntity(i)->getMaterial();
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
    }

    if(!mIsStatic || !Settings::Manager::getBool("use static geometry", "Objects") || transparent)
    {
        for(size_t i = 0;i < entities.mEntities.size();i++)
        {
            Ogre::Entity *ent = entities.mEntities[i];

            ent->setRenderingDistance(small ? Settings::Manager::getInt("small object distance", "Viewing distance") : 0);
            ent->setVisibilityFlags(mIsStatic ? (small ? RV_StaticsSmall : RV_Statics) : RV_Misc);
            ent->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);
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

        sg->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);

        for(size_t i = 0;i < entities.mEntities.size();i++)
        {
            Ogre::Entity *ent = entities.mEntities[i];
            insert->detachObject(ent);
            sg->addEntity(ent,insert->_getDerivedPosition(),insert->_getDerivedOrientation(),insert->_getDerivedScale());

            mRenderer.getScene()->destroyEntity(ent);
        }
    }
}

void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius)
{
    Ogre::SceneNode* insert = mRenderer.getScene()->getSceneNode(ptr.getRefData().getHandle());
    assert(insert);
    Ogre::Light *light = mRenderer.getScene()->createLight();
    light->setDiffuseColour (r, g, b);

    MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

    LightInfo info;
    info.name = light->getName();
    info.radius = radius;
    info.colour = Ogre::ColourValue(r, g, b);

    if (ref->base->data.flags & ESM::Light::Negative)
        info.colour *= -1;

    info.interior = (ptr.getCell()->cell->data.flags & ESM::Cell::Interior);

    if (ref->base->data.flags & ESM::Light::Flicker)
        info.type = LT_Flicker;
    else if (ref->base->data.flags & ESM::Light::FlickerSlow)
        info.type = LT_FlickerSlow;
    else if (ref->base->data.flags & ESM::Light::Pulse)
        info.type = LT_Pulse;
    else if (ref->base->data.flags & ESM::Light::PulseSlow)
        info.type = LT_PulseSlow;
    else
        info.type = LT_Normal;

    // random starting phase for the animation
    info.time = Ogre::Math::RangeRandom(0, 2 * Ogre::Math::PI);

    // adjust the lights depending if we're in an interior or exterior cell
    // quadratic means the light intensity falls off quite fast, resulting in a
    // dark, atmospheric environment (perfect for exteriors)
    // for interiors, we want more "warm" lights, so use linear attenuation.
    bool quadratic = false;
    if (!lightOutQuadInLin)
        quadratic = lightQuadratic;
    else
    {
        quadratic = !info.interior;
    }

    if (!quadratic)
    {
        float r = radius * lightLinearRadiusMult;
        float attenuation = lightLinearValue / r;
        light->setAttenuation(r*10, 0, attenuation, 0);
    }
    else
    {
        float r = radius * lightQuadraticRadiusMult;
        float attenuation = lightQuadraticValue / pow(r, 2);
        light->setAttenuation(r*10, 0, 0, attenuation);
    }

    insert->attachObject(light);
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
        if (mMwRoot->getCreator()->hasLight(it->name))
        {
            mMwRoot->getCreator()->getLight(it->name)->setVisible(true);
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
        if (mMwRoot->getCreator()->hasLight(it->name))
        {
            mMwRoot->getCreator()->getLight(it->name)->setVisible(false);
            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

void Objects::update(const float dt)
{
    std::vector<LightInfo>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mMwRoot->getCreator()->hasLight(it->name))
        {
            Ogre::Light* light = mMwRoot->getCreator()->getLight(it->name);

            // Light animation (pulse & flicker)
            it->time += dt;
            const float phase = std::fmod(static_cast<double> (it->time), static_cast<double>(32 * 2 * Ogre::Math::PI)) * 20;
            float pulseConstant;

            // These formulas are just guesswork, but they work pretty well
            if (it->type == LT_Normal)
            {
                // Less than 1/255 light modifier for a constant light:
                pulseConstant = (const float)(1.0 + sin(phase) / 255.0 );
            }
            else if (it->type == LT_Flicker)
            {
                // Let's do a 50% -> 100% sine wave pulse over 1 second:
                // This is 75% +/- 25%
                pulseConstant = (const float)(0.75 + sin(phase) * 0.25);

                // Then add a 25% flicker variation:
                it->resetTime -= dt;
                if (it->resetTime < 0)
                {
                    it->flickerVariation = (rand() % 1000) / 1000 * 0.25;
                    it->resetTime = 0.5;
                }
                if (it->resetTime > 0.25)
                {
                    pulseConstant = (pulseConstant+it->flickerVariation) * (1-it->resetTime * 2.0f) + pulseConstant * it->resetTime * 2.0f;
                }
                else
                {
                    pulseConstant = (pulseConstant+it->flickerVariation) * (it->resetTime * 2.0f) + pulseConstant * (1-it->resetTime * 2.0f);
                }
            }
            else if (it->type == LT_FlickerSlow)
            {
                // Let's do a 50% -> 100% sine wave pulse over 1 second:
                // This is 75% +/- 25%
                pulseConstant = (const float)(0.75 + sin(phase / 4.0) * 0.25);

                // Then add a 25% flicker variation:
                it->resetTime -= dt;
                if (it->resetTime < 0)
                {
                    it->flickerVariation = (rand() % 1000) / 1000 * 0.25;
                    it->resetTime = 0.5;
                }
                if (it->resetTime > 0.5)
                {
                    pulseConstant = (pulseConstant+it->flickerVariation) * (1-it->resetTime) + pulseConstant * it->resetTime;
                }
                else
                {
                    pulseConstant = (pulseConstant+it->flickerVariation) * (it->resetTime) + pulseConstant * (1-it->resetTime);
                }
            }
            else if (it->type == LT_Pulse)
            {
                // Let's do a 75% -> 125% sine wave pulse over 1 second:
                // This is 100% +/- 25%
                pulseConstant = (const float)(1.0 + sin(phase) * 0.25);
            }
            else if (it->type == LT_PulseSlow)
            {
                // Let's do a 75% -> 125% sine wave pulse over 1 second:
                // This is 100% +/- 25%
                pulseConstant = (const float)(1.0 + sin(phase / 4.0) * 0.25);
            }
            else
                assert(0 && "Invalid light type");

            light->setDiffuseColour( it->colour * pulseConstant );

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
