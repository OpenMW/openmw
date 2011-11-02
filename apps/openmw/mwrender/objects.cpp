#include "objects.hpp"
#include <OgreSceneNode.h>

using namespace MWRender;

void Objects::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_){
    ptr.getRefData().setBaseNode(mBase);
	assert (!mInsert);
	isStatic = static_;

      // Create and place scene node for this object
     mInsert = mBase->createChildSceneNode();
}
void Objects::insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh){

}
void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius){

}

