#ifndef MWLUA_USERDATASERIALIZER_H
#define MWLUA_USERDATASERIALIZER_H

#include "object.hpp"

namespace LuaUtil
{
    class UserdataSerializer;
}

namespace MWLua
{
    // UserdataSerializer is an extension for components/lua/serialization.hpp
    // Needed to serialize references to objects.
    // If local=true, then during deserialization creates LObject, otherwise creates GObject.
    std::unique_ptr<LuaUtil::UserdataSerializer> createUserdataSerializer(bool local, ObjectRegistry* registry);
}

#endif // MWLUA_USERDATASERIALIZER_H
