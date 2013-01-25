#include "actors.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

#include "renderconst.hpp"


namespace MWRender
{

Actors::~Actors(){

    PtrAnimationMap::iterator it = mAllActors.begin();
    for(;it != mAllActors.end();++it)
    {
        delete it->second;
        it->second = NULL;
    }
}

void Actors::setMwRoot(Ogre::SceneNode* root)
{ mMwRoot = root; }

void Actors::insertNPC(const MWWorld::Ptr &ptr, MWWorld::InventoryStore &inv)
{
    insertBegin(ptr, true, true);
    NpcAnimation* anim = new MWRender::NpcAnimation(ptr, ptr.getRefData ().getBaseNode (), inv, RV_Actors);

    mAllActors[ptr] = anim;
}

void Actors::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_)
{
    Ogre::SceneNode* cellnode;
    CellSceneNodeMap::const_iterator celliter = mCellSceneNodes.find(ptr.getCell());
    if(celliter != mCellSceneNodes.end())
        cellnode = celliter->second;
    else
    {
        //Create the scenenode and put it in the map
        cellnode = mMwRoot->createChildSceneNode();
        mCellSceneNodes[ptr.getCell()] = cellnode;
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


}
void Actors::insertCreature (const MWWorld::Ptr& ptr){

    insertBegin(ptr, true, true);
    CreatureAnimation* anim = new MWRender::CreatureAnimation(ptr);

    delete mAllActors[ptr];
    mAllActors[ptr] = anim;
}

bool Actors::deleteObject (const MWWorld::Ptr& ptr)
{
    delete mAllActors[ptr];
    mAllActors.erase(ptr);

    if(Ogre::SceneNode *base=ptr.getRefData().getBaseNode())
    {
        Ogre::SceneNode *parent = base->getParentSceneNode();
        CellSceneNodeMap::const_iterator iter(mCellSceneNodes.begin());
        for(;iter != mCellSceneNodes.end();++iter)
        {
            if(iter->second == parent)
            {
                base->removeAndDestroyAllChildren();
                mRend.getScene()->destroySceneNode (base);
                ptr.getRefData().setBaseNode (0);
                return true;
            }
        }

        return false;
    }

    return true;
}

void Actors::removeCell(MWWorld::Ptr::CellStore* store)
{
    for(PtrAnimationMap::iterator iter = mAllActors.begin();iter != mAllActors.end();)
    {
        if(iter->first.getCell() == store)
        {
            delete iter->second;
            mAllActors.erase(iter++);
        }
        else
            ++iter;
    }
    CellSceneNodeMap::iterator celliter = mCellSceneNodes.find(store);
    if(celliter != mCellSceneNodes.end())
    {
        Ogre::SceneNode *base = celliter->second;
        base->removeAndDestroyAllChildren();
        mRend.getScene()->destroySceneNode(base);
        mCellSceneNodes.erase(celliter);
    }
}

void Actors::playAnimationGroup (const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
{
    PtrAnimationMap::const_iterator iter = mAllActors.find(ptr);
    if(iter != mAllActors.end())
        iter->second->playGroup(groupName, mode, number);
}
void Actors::skipAnimation (const MWWorld::Ptr& ptr)
{
    PtrAnimationMap::const_iterator iter = mAllActors.find(ptr);
    if(iter != mAllActors.end())
        iter->second->skipAnim();
}
void Actors::update (float duration)
{
    for(PtrAnimationMap::const_iterator iter = mAllActors.begin();iter != mAllActors.end();iter++)
        iter->second->runAnimation(duration);
}

void Actors::updateObjectCell(const MWWorld::Ptr &ptr)
{
    Ogre::SceneNode *node;
    MWWorld::CellStore *newCell = ptr.getCell();

    CellSceneNodeMap::const_iterator celliter = mCellSceneNodes.find(newCell);
    if(celliter != mCellSceneNodes.end())
        node = celliter->second;
    else
    {
        node = mMwRoot->createChildSceneNode();
        mCellSceneNodes[newCell] = node;
    }
    node->addChild(ptr.getRefData().getBaseNode());

    PtrAnimationMap::iterator iter = mAllActors.find(ptr);
    if(iter != mAllActors.end())
    {
        /// \note Update key (Ptr's are compared only with refdata so mCell
        /// on key is outdated), maybe redundant
        Animation *anim = iter->second;
        mAllActors.erase(iter);
        mAllActors[ptr] = anim;
    }
}

}
