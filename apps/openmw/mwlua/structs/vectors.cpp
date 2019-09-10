#include "vectors.hpp"

#include <osg/Vec2i>
#include <osg/Vec4f>

#include <iomanip>

#include "../sol.hpp"
#include "../luamanager.hpp"

namespace mwse
{
    namespace lua
    {
        void bindTES3Vectors()
        {
            // FIXME: only the "tes3vector3.func(vec, vec2)" or vec.func(vec, vec2) syntax works. The vec.func(vec2) causes crashes because of null argument.

            // Get our lua state.
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Binding for osg::Vec2i.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<osg::Vec2i>();
                usertypeDefinition.set("new", sol::constructors<osg::Vec2i(), osg::Vec2i(int, int)>());

                // Basic property bindings.
                usertypeDefinition.set("min", sol::property(
                    [](osg::Vec2i& self) { return self.x(); },
                    [](osg::Vec2i& self, int value) { self[0] = value; }
                ));
                usertypeDefinition.set("max", sol::property(
                    [](osg::Vec2i& self) { return self.y(); },
                    [](osg::Vec2i& self, int value) { self[1] = value; }
                ));

                // Basic function binding.
                usertypeDefinition.set("copy", [](osg::Vec2i& self) { return osg::Vec2i(self); });

                // Finish up our usertype.
                state.set_usertype("tes3rangeInt", usertypeDefinition);
            }

            // Binding for osg::Vec2f.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<osg::Vec2f>();
                usertypeDefinition.set("new", sol::constructors<osg::Vec2f(), osg::Vec2f(float, float)>());

                // Operator overloading.
                usertypeDefinition.set(sol::meta_function::addition, sol::overload(
                    [](osg::Vec2f& self, osg::Vec2f& target) { return self + target; },
                    [](osg::Vec2f& self, float value) { return osg::Vec2f(self.x() + value, self.y()); }
                ));

                usertypeDefinition.set(sol::meta_function::subtraction, sol::overload(
                    [](osg::Vec2f& self, osg::Vec2f& target) { return self - target; },
                    [](osg::Vec2f& self, float value) { return osg::Vec2f(self.x() - value, self.y() - value); }
                ));

                usertypeDefinition.set(sol::meta_function::multiplication, sol::overload(
                    [](osg::Vec2f& self, osg::Vec2f& target) { return osg::Vec2f(self.x() * target.x(), self.y() * target.y()); },
                    [](osg::Vec2f& self, float multiplier) { return self * multiplier; }
                ));

                usertypeDefinition.set(sol::meta_function::length, &osg::Vec2f::length);
                usertypeDefinition.set(sol::meta_function::to_string, [](osg::Vec2f& self)
                {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << "<" << self.x() << ", " << self.y() << ">";
                    return ss.str();
                });

                // Allow objects to be serialized to json using their ID.
                usertypeDefinition.set("__tojson", [](osg::Vec4f& self, sol::table jsonState)
                {
                    std::ostringstream ss;
                    ss << "[\"x\":" << self.x() << ",\"y\":" << self.y() << "]";
                    return ss.str();
                });

                // Basic property bindings.
                usertypeDefinition.set("x", sol::property(
                    [](osg::Vec2f& self) { return self.x(); },
                    [](osg::Vec2f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("y", sol::property(
                    [](osg::Vec2f& self) { return self.y(); },
                    [](osg::Vec2f& self, float value) { self[1] = value; }
                ));

                // These can also be used for float ranges.
                usertypeDefinition.set("min", sol::property(
                    [](osg::Vec2f& self) { return self.x(); },
                    [](osg::Vec2f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("max", sol::property(
                    [](osg::Vec2f& self) { return self.y(); },
                    [](osg::Vec2f& self, float value) { self[1] = value; }
                ));

                // Basic function binding.
                usertypeDefinition.set("copy", [](osg::Vec2f& self) { return osg::Vec2f(self); });

                usertypeDefinition.set("length", &osg::Vec2f::length);
                usertypeDefinition.set("length2", &osg::Vec2f::length2);
                usertypeDefinition.set("distance", [](osg::Vec2f& self, osg::Vec2f& target) { return (self - target).length(); });
                usertypeDefinition.set("distance2", [](osg::Vec2f& self, osg::Vec2f& target) { return (self - target).length2(); });
                usertypeDefinition.set("dot", [](osg::Vec2f& self, osg::Vec2f& target) { return self * target; });

                usertypeDefinition.set("negate", [](osg::Vec2f& self) { return -self; });
                usertypeDefinition.set("normalize", [](osg::Vec2f& self)
                {
                    if (self.length2() == 0.f)
                        return false;
                    self.normalize();
                    return true;
                });
                usertypeDefinition.set("normalized", [](osg::Vec2f& self)
                {
                    osg::Vec2f copy(self);
                    copy.normalize();
                    return copy;
                });

                // Finish up our usertype.
                state.set_usertype("tes3vector2", usertypeDefinition);
            }

            // Binding for osg::Vec3f.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<osg::Vec3f>();
                usertypeDefinition.set("new", sol::constructors<osg::Vec3f(), osg::Vec3f(float, float, float)>());

                // Operator overloading.
                usertypeDefinition.set(sol::meta_function::addition, sol::overload(
                    [](osg::Vec3f& self, osg::Vec3f& target) { return self + target; },
                    [](osg::Vec3f& self, float value) { return osg::Vec3f(self.x() + value, self.y() + value, self.z() + value); }
                ));

                usertypeDefinition.set(sol::meta_function::subtraction, sol::overload(
                    [](osg::Vec3f& self, osg::Vec3f& target) { return self - target; },
                    [](osg::Vec3f& self, float value) { return osg::Vec3f(self.x() - value, self.y() - value, self.z() - value); }
                ));

                usertypeDefinition.set(sol::meta_function::multiplication, sol::overload(
                    [](osg::Vec3f& self, osg::Vec3f& target) { return osg::Vec3f(self.x() * target.x(), self.y() * target.y(), self.z() * target.z()); },
                    [](osg::Vec3f& self, float multiplier) { return self * multiplier; }
                ));

                usertypeDefinition.set(sol::meta_function::length, &osg::Vec3f::length);
                usertypeDefinition.set(sol::meta_function::to_string, [](osg::Vec3f& self)
                {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << "<" << self.x() << ", " << self.y() << ", " << self.z() << ">";
                    return ss.str();
                });

                // Allow objects to be serialized to json using their ID.
                usertypeDefinition.set("__tojson", [](osg::Vec3f& self, sol::table jsonState)
                {
                    std::ostringstream ss;
                    ss << "[\"x\":" << self.x() << ",\"y\":" << self.y() << ",\"z\":" << self.z() << "]";
                    return ss.str();
                });

                // Basic property bindings.
                usertypeDefinition.set("x", sol::property(
                    [](osg::Vec3f& self) { return self.x(); },
                    [](osg::Vec3f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("y", sol::property(
                    [](osg::Vec3f& self) { return self.y(); },
                    [](osg::Vec3f& self, float value) { self[1] = value; }
                ));
                usertypeDefinition.set("z", sol::property(
                    [](osg::Vec3f& self) { return self.z(); },
                    [](osg::Vec3f& self, float value) { self[2] = value; }
                ));

                // These can also be used for RGB.
                usertypeDefinition.set("r", sol::property(
                    [](osg::Vec3f& self) { return self.x(); },
                    [](osg::Vec3f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("g", sol::property(
                    [](osg::Vec3f& self) { return self.y(); },
                    [](osg::Vec3f& self, float value) { self[1] = value; }
                ));
                usertypeDefinition.set("b", sol::property(
                    [](osg::Vec3f& self) { return self.z(); },
                    [](osg::Vec3f& self, float value) { self[2] = value; }
                ));

                // Basic function binding.
                usertypeDefinition.set("copy", [](osg::Vec3f& self) { return osg::Vec3f(self); });

                usertypeDefinition.set("cross", &osg::Vec3f::operator^);
                usertypeDefinition.set("length", &osg::Vec3f::length);
                usertypeDefinition.set("length2", &osg::Vec3f::length2);
                usertypeDefinition.set("distance", [](osg::Vec3f& self, osg::Vec3f& target) { return (self - target).length(); });
                usertypeDefinition.set("distance2", [](osg::Vec3f& self, osg::Vec3f& target) { return (self - target).length2(); });
                usertypeDefinition.set("dot", [](osg::Vec3f& self, osg::Vec3f& target) { return osg::Vec3f(self.x() * target.x(), self.y() * target.y(), self.z() * target.z()); });
                //usertypeDefinition.set("outerProduct", &TES3::Vector3::outerProduct);
                usertypeDefinition.set("heightDifference", [](osg::Vec3f& self, osg::Vec3f& target) { return fabs(target.z() - self.z()); });

                usertypeDefinition.set("negate", [](osg::Vec3f& self) { return -self; });
                usertypeDefinition.set("normalize", [](osg::Vec3f& self)
                {
                    if (self.length2() == 0.f)
                        return false;
                    self.normalize();
                    return true;
                });
                usertypeDefinition.set("normalized", [](osg::Vec3f& self)
                {
                    osg::Vec3f copy(self);
                    copy.normalize();
                    return copy;
                });

                // Finish up our usertype.
                state.set_usertype("tes3vector3", usertypeDefinition);
            }

            // Binding for osg::Vec4f
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<osg::Vec4f>();
                usertypeDefinition.set("new", sol::constructors<osg::Vec4f(), osg::Vec3f(float, float, float, float)>());

                // Operator overloading.
                usertypeDefinition.set(sol::meta_function::addition, sol::overload(
                    [](osg::Vec4f& self, osg::Vec4f& target) { return self + target; },
                    [](osg::Vec4f& self, float value) { return osg::Vec4f(self.x() + value, self.y() + value, self.z() + value, self.w() + value); }
                ));

                usertypeDefinition.set(sol::meta_function::subtraction, sol::overload(
                    [](osg::Vec4f& self, osg::Vec4f& target) { return self - target; },
                    [](osg::Vec4f& self, float value) { return osg::Vec4f(self.x() - value, self.y() - value, self.z() - value, self.w() - value); }
                ));

                usertypeDefinition.set(sol::meta_function::multiplication, sol::overload(
                    [](osg::Vec4f& self, osg::Vec4f& target) { return osg::Vec4f(self.x() * target.x(), self.y() * target.y(), self.z() * target.z(), self.w() * target.w()); },
                    [](osg::Vec4f& self, float multiplier) { return self * multiplier; }
                ));

                usertypeDefinition.set(sol::meta_function::length, &osg::Vec4f::length);
                usertypeDefinition.set(sol::meta_function::to_string, [](osg::Vec4f& self)
                {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << "<" << self.x() << ", " << self.y() << ", " << self.z() << ", " << self.w() << ">";
                    return ss.str();
                });

                // Allow objects to be serialized to json using their ID.
                usertypeDefinition.set("__tojson", [](osg::Vec4f& self, sol::table jsonState)
                {
                    std::ostringstream ss;
                    ss << "[\"x\":" << self.x() << ",\"y\":" << self.y() << ",\"z\":" << self.z() << ",\"w\":" << self.w()<< "]";
                    return ss.str();
                });

                // Basic property bindings.
                usertypeDefinition.set("x", sol::property(
                    [](osg::Vec4f& self) { return self.x(); },
                    [](osg::Vec4f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("y", sol::property(
                    [](osg::Vec4f& self) { return self.y(); },
                    [](osg::Vec4f& self, float value) { self[1] = value; }
                ));
                usertypeDefinition.set("z", sol::property(
                    [](osg::Vec4f& self) { return self.z(); },
                    [](osg::Vec4f& self, float value) { self[2] = value; }
                ));
                usertypeDefinition.set("w", sol::property(
                    [](osg::Vec4f& self) { return self.w(); },
                    [](osg::Vec4f& self, float value) { self[3] = value; }
                ));

                // These can also be used for RGBA.
                usertypeDefinition.set("r", sol::property(
                    [](osg::Vec4f& self) { return self.x(); },
                    [](osg::Vec4f& self, float value) { self[0] = value; }
                ));
                usertypeDefinition.set("g", sol::property(
                    [](osg::Vec4f& self) { return self.y(); },
                    [](osg::Vec4f& self, float value) { self[1] = value; }
                ));
                usertypeDefinition.set("b", sol::property(
                    [](osg::Vec4f& self) { return self.z(); },
                    [](osg::Vec4f& self, float value) { self[2] = value; }
                ));
                usertypeDefinition.set("a", sol::property(
                    [](osg::Vec4f& self) { return self.w(); },
                    [](osg::Vec4f& self, float value) { self[3] = value; }
                ));

                // Basic function binding.
                usertypeDefinition.set("copy", [](osg::Vec4f& self) { return osg::Vec4f(self); });

                usertypeDefinition.set("length", &osg::Vec4f::length);
                usertypeDefinition.set("length2", &osg::Vec4f::length2);
                usertypeDefinition.set("distance", [](osg::Vec4f& self, osg::Vec4f& target) { return (self - target).length(); });
                usertypeDefinition.set("distance2", [](osg::Vec4f& self, osg::Vec4f& target) { return (self - target).length2(); });
                usertypeDefinition.set("dot", [](osg::Vec4f& self, osg::Vec4f& target) { return self * target; });

                usertypeDefinition.set("negate", [](osg::Vec4f& self) { return -self; });
                usertypeDefinition.set("normalize", [](osg::Vec4f& self)
                {
                    if (self.length2() == 0.f)
                        return false;
                    self.normalize();
                    return true;
                });
                usertypeDefinition.set("normalized", [](osg::Vec4f& self)
                {
                    osg::Vec4f copy(self);
                    copy.normalize();
                    return copy;
                });

                // Finish up our usertype.
                state.set_usertype("tes3vector4", usertypeDefinition);
            }

            /*
            // Binding for TES3::Matrix33.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<TES3::Matrix33>();
                usertypeDefinition.set("new", sol::constructors<
                        TES3::Matrix33(),
                        TES3::Matrix33(TES3::Vector3*, TES3::Vector3*, TES3::Vector3*),
                        TES3::Matrix33(float, float, float, float, float, float, float, float, float)
                >());

                // Operator overloading.
                usertypeDefinition.set(sol::meta_function::addition, &TES3::Matrix33::operator+);
                usertypeDefinition.set(sol::meta_function::subtraction, &TES3::Matrix33::operator-);
                usertypeDefinition.set(sol::meta_function::equal_to, &TES3::Matrix33::operator==);
                usertypeDefinition.set(sol::meta_function::multiplication, sol::overload(
                    sol::resolve<TES3::Matrix33(const float)>(&TES3::Matrix33::operator*),
                    sol::resolve<TES3::Vector3(const TES3::Vector3&)>(&TES3::Matrix33::operator*),
                    sol::resolve<TES3::Matrix33(const TES3::Matrix33&)>(&TES3::Matrix33::operator*)
                ));

                // Operator overloading.
                usertypeDefinition.set(sol::meta_function::to_string, [](TES3::Matrix33& self) {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << std::dec
                        << "<<<" << self.m0.x << ", " << self.m0.y << ", " << self.m0.z << ">,\n"
                        << "  <" << self.m1.x << ", " << self.m1.y << ", " << self.m1.z << ">,\n"
                        << "  <" << self.m2.x << ", " << self.m2.y << ", " << self.m2.z << ">>>";
                    return ss.str();
                });

                // Basic property bindings.
                usertypeDefinition.set("x", sol::readonly_property([](TES3::Matrix33& self) { return self.m0; }));
                usertypeDefinition.set("y", sol::readonly_property([](TES3::Matrix33& self) { return self.m1; }));
                usertypeDefinition.set("z", sol::readonly_property([](TES3::Matrix33& self) { return self.m2; }));

                // Basic function binding.
                usertypeDefinition.set("copy", [](TES3::Matrix33& self) { return TES3::Matrix33(self); });
                usertypeDefinition.set("fromEulerXYZ", &TES3::Matrix33::fromEulerXYZ);
                usertypeDefinition.set("reorthogonalize", &TES3::Matrix33::reorthogonalize);
                usertypeDefinition.set("toIdentity", &TES3::Matrix33::toIdentity);
                usertypeDefinition.set("toRotation", &TES3::Matrix33::toRotation);
                usertypeDefinition.set("toRotationX", &TES3::Matrix33::toRotationX);
                usertypeDefinition.set("toRotationY", &TES3::Matrix33::toRotationY);
                usertypeDefinition.set("toRotationZ", &TES3::Matrix33::toRotationZ);
                usertypeDefinition.set("toZero", &TES3::Matrix33::toZero);
                usertypeDefinition.set("transpose", &TES3::Matrix33::transpose);

                // Handle functions with out values.
                usertypeDefinition.set("invert", [](TES3::Matrix33& self) {
                    TES3::Matrix33 matrix;
                    bool valid = self.invert(&matrix);
                    return std::make_tuple(matrix, valid);
                });
                usertypeDefinition.set("toEulerXYZ", [](TES3::Matrix33& self) {
                    float x, y, z;
                    bool isUnique = self.toEulerXYZ(&x, &y, &z);
                    return std::make_tuple(TES3::Vector3(x, y, z), isUnique);
                });
                usertypeDefinition.set("toEulerZYX", [](TES3::Matrix33& self) {
                    float x, y, z;
                    bool isUnique = self.toEulerZYX(&x, &y, &z);
                    return std::make_tuple(TES3::Vector3(x, y, z), isUnique);
                });

                // Finish up our usertype.
                state.set_usertype("tes3matrix33", usertypeDefinition);
            }

            // FIXME: There is not "Transform" in OpenMW. Use the PAT?
            // Binding for TES3::Transform.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<TES3::Transform>();
                usertypeDefinition.set("new", sol::no_constructor);

                // Basic property bindings.
                usertypeDefinition.set("rotation", sol::readonly_property([](TES3::Transform& self) { return self.rotation; }));
                usertypeDefinition.set("translation", sol::readonly_property([](TES3::Transform& self) { return self.translation; }));
                usertypeDefinition.set("scale", sol::readonly_property([](TES3::Transform& self) { return self.scale; }));

                // Basic function binding.
                usertypeDefinition.set("copy", [](TES3::Transform& self) { return TES3::Transform(self); });

                // Finish up our usertype.
                state.set_usertype("tes3transform", usertypeDefinition);
            }
            */
        }
    }
}
