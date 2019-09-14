#include "node.hpp"

#include "../util.hpp"

#include "pointer.hpp"

#include <osg/Node>

#include <components/sceneutil/visitor.hpp>

namespace MWLua
{
    void bindNINode()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for osg::Node.
        {
            auto usertypeDefinition = state.create_simple_usertype<osg::Node>();

            usertypeDefinition.set("new", []()
            {
                return Pointer<osg::Node>(new osg::Node());
            });

            // Basic property binding.
            usertypeDefinition.set("name", sol::property(
                [](osg::Node& self) { return self.getName(); },
                [](osg::Node& self, const char * name) { self.setName(name); }
            ));
            usertypeDefinition.set("refCount", sol::readonly_property(&osg::Node::referenceCount));

            usertypeDefinition.set("parent", sol::readonly_property([](osg::Node& self) -> sol::object
            {
                // Technically, there can be several parents, but it is an invalid state in OpenMW.
                if (self.getNumParents() <= 0) return sol::nil;
                return makeLuaNiPointer(self.getParent(0));
            }));

            state.set_usertype("niObject", usertypeDefinition);
        }

        // Binding for osg::Group.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<osg::Group>();

            usertypeDefinition.set("new", []()
            {
                return Pointer<osg::Group>(new osg::Group());
            });

            // Define inheritance structures. These must be defined in order from top to bottom. The complete chain must be defined.
            usertypeDefinition.set(sol::base_classes, sol::bases<osg::Node>());

            // Functions that need their results wrapped.
            usertypeDefinition.set("getObjectByName", [](osg::Group& self, const char* name)
            {
                SceneUtil::FindByNameVisitor findVisitor (name);
                self.accept(findVisitor);
                return makeLuaNiPointer(findVisitor.mFoundNode);
            });
            //usertypeDefinition.set("getProperty", [](osg::Group& self, int type) { return makeLuaNiPointer(self.getProperty(NI::PropertyType(type))); });

            // Basic function binding.
            usertypeDefinition.set("attachChild", [](osg::Group& self, osg::Group* child)
            {
                if (child == nullptr)
                    return;

                self.addChild(child);
            });
            usertypeDefinition.set("detachChild", [](osg::Group& self, osg::Group* child) -> sol::object
            {
                if (child == nullptr)
                    return sol::nil;

                self.removeChild(child);
                return makeLuaNiPointer(child);
            });
            usertypeDefinition.set("detachChildAt", [](osg::Group& self, unsigned int index) -> sol::object
            {
                if (index < 1 || index > self.getNumChildren())
                {
                    return sol::nil;
                }

                osg::Node* child = self.getChild(index - 1);
                sol::object ptr = makeLuaNiPointer(child);
                self.removeChild(child);
                return ptr;
            });
            usertypeDefinition.set("children", sol::readonly_property([](osg::Group& self)
            {
                auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                sol::table result = state.create_table();

                for (unsigned int i = 0; i < self.getNumChildren(); i++)
                {
                    result[i + 1] = makeLuaObject(self.getChild(i));
                }
                return result;
            }));
            //usertypeDefinition.set("children", sol::readonly_property(&NI::Node::children));

            /*
            usertypeDefinition.set("properties", sol::readonly_property(&NI::AVObject::propertyNode));
            usertypeDefinition.set("flags", &NI::AVObject::flags);
            usertypeDefinition.set("scale", &NI::AVObject::localScale);
            usertypeDefinition.set("translation", &NI::AVObject::localTranslate);
            usertypeDefinition.set("worldBoundOrigin", &NI::AVObject::worldBoundOrigin);
            usertypeDefinition.set("worldBoundRadius", &NI::AVObject::worldBoundRadius);
            usertypeDefinition.set("worldTransform", &NI::AVObject::worldTransform);
            usertypeDefinition.set("controller", sol::readonly_property(&NI::ObjectNET::controllers));

            // Basic function binding.
            usertypeDefinition.set("prependController", &NI::ObjectNET::prependController);
            usertypeDefinition.set("removeController", &NI::ObjectNET::removeController);
            usertypeDefinition.set("removeAllControllers", &NI::ObjectNET::removeAllControllers);

            // Ensure that rotation calls the required functions.
            usertypeDefinition.set("rotation", sol::property(
                [](NI::AVObject& self) { return self.localRotation; },
                [](NI::AVObject& self, TES3::Matrix33* matrix) { self.setLocalRotationMatrix(matrix); }
            ));

            // Basic function binding.
            usertypeDefinition.set("attachProperty", &NI::AVObject::attachProperty);
            usertypeDefinition.set("clearTransforms", &NI::AVObject::clearTransforms);
            usertypeDefinition.set("propagatePositionChange", [](NI::AVObject& self) { self.update(); });
            usertypeDefinition.set("updateEffects", &NI::AVObject::updateEffects);
            usertypeDefinition.set("updateProperties", &NI::AVObject::updateProperties);
            usertypeDefinition.set("clone", [](NI::Object& self) { return makeLuaNiPointer(self.createClone()); });
            usertypeDefinition.set("isOfType", static_cast<bool (__thiscall NI::Object::*)(uintptr_t)>(&NI::Object::isOfType)); //isSameKindAs
            usertypeDefinition.set("isInstanceOfType", static_cast<bool (__thiscall NI::Object::*)(uintptr_t)>(&NI::Object::isInstanceOfType));

            // Make remove property a bit more friendly.
            usertypeDefinition.set("detachProperty", [](NI::AVObject& self, int type) {
                NI::Pointer<NI::Property> prop;
                self.detachProperty(&prop, NI::PropertyType(type));
                return makeLuaNiPointer(prop);
            });

            // Update function with table arguments.
            usertypeDefinition.set("update", [](NI::AVObject& self, sol::optional<sol::table> args) {
                if (args) {
                    auto values = args.value();
                    float time = values.get_or("time", 0.0f);
                    bool updateControllers = values.get_or("controllers", false);
                    bool updateBounds = values.get_or("bounds", true);

                    self.update(time, updateControllers, updateBounds);
                }
                else {
                    self.update();
                }
            });

            // Basic property binding.
            usertypeDefinition.set("effectList", sol::readonly_property(&NI::Node::effectList));

            // Functions that need their results wrapped.
            usertypeDefinition.set("getEffect", [](NI::Node& self, int type) { return makeLuaNiPointer(self.getEffect(type)); });

            // Friendly access to flags.
            usertypeDefinition.set("appCulled", sol::property(&NI::AVObject::getAppCulled, &NI::AVObject::setAppCulled));

            // Legacy access. TODO: Remove.
            usertypeDefinition.set("propegatePositionChange", [](NI::AVObject& self) { self.update(); });
            usertypeDefinition.set("updateNodeEffects", &NI::AVObject::updateEffects);
            */

            // Finish up our usertype.
            state.set_usertype("niNode", usertypeDefinition);
        }
    }
}

namespace sol
{
    template <typename T>
    struct unique_usertype_traits<osg::ref_ptr<T>>
    {
        typedef T type;
        typedef osg::ref_ptr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& value)
        {
            return value == nullptr;
        }

        static type* get(const actual_type& p)
        {
            return p.get();
        }
    };
}
