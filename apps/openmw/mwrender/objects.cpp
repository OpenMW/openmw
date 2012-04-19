#include "objects.hpp"

#include <OgreSceneNode.h>

#include <components/nifogre/ogre_nif_loader.hpp>
#include <components/settings/settings.hpp>
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

    NifOgre::NIFLoader::load(mesh);
    Ogre::Entity *ent = mRenderer.getScene()->createEntity(mesh);


    Ogre::Vector3 extents = ent->getBoundingBox().getSize();
    extents *= insert->getScale();
    float size = std::max(std::max(extents.x, extents.y), extents.z);

    bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) && Settings::Manager::getBool("limit small object distance", "Objects");

    // do not fade out doors. that will cause holes and look stupid
    if (ptr.getTypeName().find("Door") != std::string::npos)
        small = false;

    if (mBounds.find(ptr.getCell()) == mBounds.end())
        mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;

    Ogre::AxisAlignedBox bounds = ent->getBoundingBox();
    bounds = Ogre::AxisAlignedBox(
        insert->_getDerivedPosition() + bounds.getMinimum(),
        insert->_getDerivedPosition() + bounds.getMaximum()
    );

    bounds.scale(insert->getScale());
    mBounds[ptr.getCell()].merge(bounds);

    bool transparent = false;
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

    if(!mIsStatic || !Settings::Manager::getBool("use static geometry", "Objects") || transparent)
    {
        insert->attachObject(ent);

        ent->setRenderingDistance(small ? Settings::Manager::getInt("small object distance", "Viewing distance") : 0);
        ent->setVisibilityFlags(mIsStatic ? (small ? RV_StaticsSmall : RV_Statics) : RV_Misc);
        ent->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);
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

        sg->addEntity(ent,insert->_getDerivedPosition(),insert->_getDerivedOrientation(),insert->_getDerivedScale());

        sg->setVisibilityFlags(small ? RV_StaticsSmall : RV_Statics);

        sg->setCastShadows(true);

        sg->setRenderQueueGroup(transparent ? RQG_Alpha : RQG_Main);

        mRenderer.getScene()->destroyEntity(ent);
    }
}

void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius)
{
    Ogre::SceneNode* insert = mRenderer.getScene()->getSceneNode(ptr.getRefData().getHandle());
    assert(insert);
    Ogre::Light *light = mRenderer.getScene()->createLight();
    light->setDiffuseColour (r, g, b);

    LightInfo info;
    info.name = light->getName();
    info.radius = radius;
    info.colour = Ogre::ColourValue(r, g, b);
    mLights.push_back(info);


    bool quadratic = false;
    if (!lightOutQuadInLin)
        quadratic = lightQuadratic;
    else
    {
        quadratic = !mInterior;
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

void Objects::buildStaticGeometry(ESMS::CellStore<MWWorld::RefData>& cell)
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

void Objects::setInterior(const bool interior)
{
    mInterior = interior;
}

void Objects::update(const float dt)
{
    // adjust the lights depending if we're in an interior or exterior cell
    // quadratic means the light intensity falls off quite fast, resulting in a
    // dark, atmospheric environment (perfect for exteriors)
    // for interiors, we want more "warm" lights, so use linear attenuation.
    std::vector<LightInfo>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mMwRoot->getCreator()->hasLight(it->name))
        {
            Ogre::Light* light = mMwRoot->getCreator()->getLight(it->name);

            bool quadratic = false;
            if (!lightOutQuadInLin)
                quadratic = lightQuadratic;
            else
            {
                quadratic = !mInterior;
            }

            if (!quadratic)
            {
                float radius = it->radius * lightLinearRadiusMult;
                float attenuation = lightLinearValue / it->radius;
                light->setAttenuation(radius*10, 0, attenuation, 0);
            }
            else
            {
                float radius = it->radius * lightQuadraticRadiusMult;
                float attenuation = lightQuadraticValue / pow(it->radius, 2);
                light->setAttenuation(radius*10, 0, 0, attenuation);
            }

            ++it;
        }
        else
            it = mLights.erase(it);
    }
}
