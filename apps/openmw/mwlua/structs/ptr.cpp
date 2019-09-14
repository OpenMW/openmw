#include "ptr.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/esmstore.hpp"

#include <components/sceneutil/positionattitudetransform.hpp>

namespace MWLua
{
    void bindTES3Reference()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for MWWorld::Ptr
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<MWWorld::Ptr>();

            usertypeDefinition.set("new", sol::no_constructor);
            usertypeDefinition.set("sceneNode", sol::readonly_property([](MWWorld::Ptr& self) { return makeLuaNiPointer(self.getRefData().getBaseNode()->getChild(0)); }));

            // TODO: check for empty

            // Access to other objects that need to be packaged.
            /*
            usertypeDefinition.set("cell", sol::readonly_property([](TES3::Reference& self) -> sol::object {
                // Handle case for the player.
                if (TES3::WorldController::get()->getMobilePlayer()->reference == &self) {
                    return makeLuaObject(TES3::DataHandler::get()->currentCell);
                }

                if (self.owningCollection.asReferenceList == nullptr) {
                    return sol::nil;
                }

                return makeLuaObject(self.owningCollection.asReferenceList->cell);
            }));
            */
            usertypeDefinition.set("object", sol::readonly_property([](MWWorld::Ptr& self) -> sol::object
            {
                if (self.getTypeName() == typeid(ESM::Activator).name())
                    return makeLuaObject(self.get<ESM::Activator>()->mBase);
                if (self.getTypeName() == typeid(ESM::Apparatus).name())
                    return makeLuaObject(self.get<ESM::Apparatus>()->mBase);
                if (self.getTypeName() == typeid(ESM::Armor).name())
                    return makeLuaObject(self.get<ESM::Armor>()->mBase);
                if (self.getTypeName() == typeid(ESM::Book).name())
                    return makeLuaObject(self.get<ESM::Book>()->mBase);
                if (self.getTypeName() == typeid(ESM::Clothing).name())
                    return makeLuaObject(self.get<ESM::Clothing>()->mBase);
                if (self.getTypeName() == typeid(ESM::Container).name())
                    return makeLuaObject(self.get<ESM::Container>()->mBase);
                if (self.getTypeName() == typeid(ESM::Creature).name())
                    return makeLuaObject(self.get<ESM::Creature>()->mBase);
                if (self.getTypeName() == typeid(ESM::Door).name())
                    return makeLuaObject(self.get<ESM::Door>()->mBase);
                if (self.getTypeName() == typeid(ESM::Ingredient).name())
                    return makeLuaObject(self.get<ESM::Ingredient>()->mBase);
                if (self.getTypeName() == typeid(ESM::Light).name())
                    return makeLuaObject(self.get<ESM::Light>()->mBase);
                if (self.getTypeName() == typeid(ESM::Lockpick).name())
                    return makeLuaObject(self.get<ESM::Lockpick>()->mBase);
                if (self.getTypeName() == typeid(ESM::Miscellaneous).name())
                    return makeLuaObject(self.get<ESM::Miscellaneous>()->mBase);
                if (self.getTypeName() == typeid(ESM::NPC).name())
                    return makeLuaObject(self.get<ESM::NPC>()->mBase);
                if (self.getTypeName() == typeid(ESM::Potion).name())
                    return makeLuaObject(self.get<ESM::Potion>()->mBase);
                if (self.getTypeName() == typeid(ESM::Probe).name())
                    return makeLuaObject(self.get<ESM::Probe>()->mBase);
                if (self.getTypeName() == typeid(ESM::Repair).name())
                    return makeLuaObject(self.get<ESM::Repair>()->mBase);
                if (self.getTypeName() == typeid(ESM::Static).name())
                    return makeLuaObject(self.get<ESM::Static>()->mBase);
                if (self.getTypeName() == typeid(ESM::Weapon).name())
                    return makeLuaObject(self.get<ESM::Weapon>()->mBase);

                return sol::nil;
            }));

            // Basic function binding.
            usertypeDefinition.set("activate", [](MWWorld::Ptr& self, MWWorld::Ptr& target) { MWBase::Environment::get().getWorld()->activate(target, self); });

            /*
            usertypeDefinition.set("clearActionFlag", &TES3::Reference::clearActionFlag);
            usertypeDefinition.set("clone", &TES3::Reference::clone);
            usertypeDefinition.set("deleteDynamicLightAttachment", &TES3::Reference::deleteDynamicLightAttachment);
            usertypeDefinition.set("detachDynamicLightFromAffectedNodes", &TES3::Reference::detachDynamicLightFromAffectedNodes);
            */
            usertypeDefinition.set("disable", [](MWWorld::Ptr& self) { MWBase::Environment::get().getWorld()->disable(self); });
            usertypeDefinition.set("enable", [](MWWorld::Ptr& self) { MWBase::Environment::get().getWorld()->disable(self); });

            /*
            usertypeDefinition.set("getAttachedDynamicLight", &TES3::Reference::getAttachedDynamicLight);
            usertypeDefinition.set("getOrCreateAttachedDynamicLight", &TES3::Reference::getOrCreateAttachedDynamicLight);
            usertypeDefinition.set("setActionFlag", &TES3::Reference::setActionFlag);
            usertypeDefinition.set("testActionFlag", &TES3::Reference::testActionFlag);
            */

            //usertypeDefinition.set("updateEquipment", &TES3::Reference::updateBipedParts);

            // Functions exposed as properties.
            //usertypeDefinition.set("activationReference", sol::property(&TES3::Reference::getActionReference, &TES3::Reference::setActionReference));
            //usertypeDefinition.set("attachments", sol::readonly_property(&TES3::Reference::getAttachments));
            //usertypeDefinition.set("context", sol::readonly_property(&getContext));
            //usertypeDefinition.set("data", sol::readonly_property(&TES3::Reference::getLuaTable));
            //usertypeDefinition.set("isEmpty", sol::property(&TES3::Reference::getEmptyInventoryFlag, &TES3::Reference::setEmptyInventoryFlag));
            //usertypeDefinition.set("isRespawn", sol::readonly_property(&TES3::Reference::isRespawn));
            usertypeDefinition.set("orientation", sol::property(
                [](MWWorld::Ptr& self)
                {
                    ESM::Position position = self.getRefData().getPosition();
                    return osg::Vec3f(position.rot[0], position.rot[1], position.rot[2]);
                },
                [](MWWorld::Ptr& self, osg::Vec3f& value)
                {
                    ESM::Position position = self.getRefData().getPosition();
                    position.rot[0] = value[0];
                    position.rot[1] = value[1];
                    position.rot[2] = value[2];
                    self.getRefData().setPosition(position);
                }
            ));
            usertypeDefinition.set("position", sol::property(
                [](MWWorld::Ptr& self)
                {
                    ESM::Position position = self.getRefData().getPosition();
                    return osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]);
                },
                [](MWWorld::Ptr& self, osg::Vec3f& value)
                {
                    MWBase::Environment::get().getWorld()->moveObject(self, value.x(), value.y(), value.z());
                    self.getClass().adjustPosition(self, false);
                }
            ));
            usertypeDefinition.set("originalOrientation", sol::property(
                [](MWWorld::Ptr& self)
                {
                    ESM::Position position = self.getCellRef().getPosition();
                    return osg::Vec3f(position.rot[0], position.rot[1], position.rot[2]);
                },
                [](MWWorld::Ptr& self, osg::Vec3f& value)
                {
                    ESM::Position position = self.getCellRef().getPosition();
                    position.rot[0] = value[0];
                    position.rot[1] = value[1];
                    position.rot[2] = value[2];
                    self.getCellRef().setPosition(position);
                }
            ));
            usertypeDefinition.set("originalPosition", sol::property(
                [](MWWorld::Ptr& self)
                {
                    ESM::Position position = self.getCellRef().getPosition();
                    return osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]);
                },
                [](MWWorld::Ptr& self, osg::Vec3f& value)
                {
                    ESM::Position position = self.getCellRef().getPosition();
                    position.pos[0] = value[0];
                    position.pos[1] = value[1];
                    position.pos[2] = value[2];
                    self.getCellRef().setPosition(position);
                }
            ));

            /*
            // Functions for manually syncing the scene graph, for if orientation or position is manually modified.
            usertypeDefinition.set("updateSceneGraph", [](TES3::Reference& self) {
                TES3::Matrix33 tempOutArg;
                self.sceneNode->setLocalRotationMatrix(self.updateSceneMatrix(&tempOutArg));
                self.sceneNode->update();
                self.setObjectModified(true);
            });
            */


            // Finish up our usertype.
            state.set_usertype("tes3reference", usertypeDefinition);
        }
    }
}
