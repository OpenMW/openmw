#include "objects.hpp"

#include <OgreSceneNode.h>

#include <components/nifogre/ogre_nif_loader.hpp>

using namespace MWRender;

bool Objects::lightConst = false;
float Objects::lightConstValue = 0.0f;

bool Objects::lightLinear = true;
int Objects::lightLinearMethod = 1;
float Objects::lightLinearValue = 3;
float Objects::lightLinearRadiusMult = 1;

bool Objects::lightQuadratic = false;
int Objects::lightQuadraticMethod = 2;
float Objects::lightQuadraticValue = 16;
float Objects::lightQuadraticRadiusMult = 1;

bool Objects::lightOutQuadInLin = false;

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

/*
    Ogre::Vector3 extents = ent->getBoundingBox().getSize();
    extents *= insert->getScale();
//    float size = std::max(std::max(extents.x, extents.y), extents.z);

    bool small = (size < 250); /// \todo config value

    // do not fade out doors. that will cause holes and look stupid
    if (ptr.getTypeName().find("Door") != std::string::npos)
        small = false;
*/
    const bool small = false;

    if (mBounds.find(ptr.getCell()) == mBounds.end())
        mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;

    Ogre::AxisAlignedBox bounds = ent->getBoundingBox();
    bounds = Ogre::AxisAlignedBox(
        insert->_getDerivedPosition() + bounds.getMinimum(),
        insert->_getDerivedPosition() + bounds.getMaximum()
    );

    bounds.scale(insert->getScale());
    mBounds[ptr.getCell()].merge(bounds);

    if(!mIsStatic)
    {
        insert->attachObject(ent);

        ent->setRenderingDistance(small ? 2500 : 0); /// \todo config value
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

                sg->setRenderingDistance(2500); /// \todo config value
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

        mRenderer.getScene()->destroyEntity(ent);
    }
}

void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius)
{
    Ogre::SceneNode* insert = mRenderer.getScene()->getSceneNode(ptr.getRefData().getHandle());
    assert(insert);
    Ogre::Light *light = mRenderer.getScene()->createLight();
    light->setDiffuseColour (r, g, b);
    mLights.push_back(light->getName());

    float cval=0.0f, lval=0.0f, qval=0.0f;

    if(lightConst)
         cval = lightConstValue;

    if(!lightOutQuadInLin)
    {
        if(lightLinear)
            radius *= lightLinearRadiusMult;
        if(lightQuadratic)
            radius *= lightQuadraticRadiusMult;

        if(lightLinear)
            lval = lightLinearValue / pow(radius, lightLinearMethod);
        if(lightQuadratic)
            qval = lightQuadraticValue / pow(radius, lightQuadraticMethod);
    }
    else
    {
        // FIXME:
        // Do quadratic or linear, depending if we're in an exterior or interior
        // cell, respectively. Ignore lightLinear and lightQuadratic.
    }

    light->setAttenuation(10*radius, cval, lval, qval);

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
    std::vector<std::string>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mMwRoot->getCreator()->hasLight(*it))
        {
            mMwRoot->getCreator()->getLight(*it)->setVisible(true);
            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

void Objects::disableLights()
{
    std::vector<std::string>::iterator it = mLights.begin();
    while (it != mLights.end())
    {
        if (mMwRoot->getCreator()->hasLight(*it))
        {
            mMwRoot->getCreator()->getLight(*it)->setVisible(false);
            ++it;
        }
        else
            it = mLights.erase(it);
    }
}

