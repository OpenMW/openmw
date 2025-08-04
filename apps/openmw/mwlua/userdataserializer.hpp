#ifndef MWLUA_USERDATASERIALIZER_H
#define MWLUA_USERDATASERIALIZER_H

#include <map>
#include <memory>

namespace LuaUtil
{
    class UserdataSerializer;
}

namespace MWLua
{
    // UserdataSerializer is an extension for components/lua/serialization.hpp
    // Needed to serialize references to objects.
    // If local=true, then during deserialization creates LObject, otherwise creates GObject.
    // contentFileMapping is used only for deserialization. Needed to fix references if the order
    // of content files was changed.
    std::unique_ptr<LuaUtil::UserdataSerializer> createUserdataSerializer(
        bool local, std::map<int, int>* contentFileMapping = nullptr);
}

#endif // MWLUA_USERDATASERIALIZER_H
