#include "postprocessingbindings.hpp"

#include "MyGUI_LanguageManager.h"

#include <components/lua/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwrender/postprocessor.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"

namespace
{
    std::string getLocalizedMyGUIString(std::string_view unlocalized)
    {
        return MyGUI::LanguageManager::getInstance().replaceTags(std::string(unlocalized)).asUTF8();
    }
}

namespace MWLua
{
    struct Shader;
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::Shader> : std::false_type
    {
    };
}

namespace MWLua
{
    struct Shader
    {
        std::shared_ptr<Fx::Technique> mShader;

        Shader(std::shared_ptr<Fx::Technique> shader)
            : mShader(std::move(shader))
        {
        }

        std::string toString() const
        {
            if (!mShader)
                return "Shader(nil)";

            return Misc::StringUtils::format("Shader(%s, %s)", mShader->getName(), mShader->getFileName().value());
        }

        enum
        {
            Action_None,
            Action_Enable,
            Action_Disable
        } mQueuedAction
            = Action_None;
    };

    template <class T>
    auto getSetter(const Context& context)
    {
        return [context](const Shader& shader, const std::string& name, const T& value) {
            context.mLuaManager->addAction(
                [=] {
                    MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(shader.mShader, name, value);
                },
                "SetUniformShaderAction");
        };
    }

    template <class T>
    auto getArraySetter(const Context& context)
    {
        return [context](const Shader& shader, const std::string& name, const sol::table& table) {
            auto targetSize
                = MWBase::Environment::get().getWorld()->getPostProcessor()->getUniformSize(shader.mShader, name);

            if (!targetSize.has_value())
                throw std::runtime_error(Misc::StringUtils::format("Failed setting uniform array '%s'", name));

            if (*targetSize != table.size())
                throw std::runtime_error(Misc::StringUtils::format(
                    "Mismatching uniform array size, got %zu expected %zu", table.size(), *targetSize));

            std::vector<T> values;
            values.reserve(*targetSize);

            for (size_t i = 0; i < *targetSize; ++i)
            {
                sol::object obj = table[LuaUtil::toLuaIndex(i)];
                if (!obj.is<T>())
                    throw std::runtime_error("Invalid type for uniform array");
                values.push_back(obj.as<T>());
            }

            context.mLuaManager->addAction(
                [shader, name, values = std::move(values)] {
                    MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(shader.mShader, name, values);
                },
                "SetUniformShaderAction");
        };
    }

    sol::table initPostprocessingPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        sol::usertype<Shader> shader = lua.new_usertype<Shader>("Shader");
        shader[sol::meta_function::to_string] = [](const Shader& shader) { return shader.toString(); };

        shader["enable"] = [context](Shader& shader, sol::optional<int> optPos) {
            std::optional<int> pos = std::nullopt;
            if (optPos)
                pos = optPos.value();

            if (shader.mShader && shader.mShader->isValid())
                shader.mQueuedAction = Shader::Action_Enable;

            context.mLuaManager->addAction([=, &shader] {
                shader.mQueuedAction = Shader::Action_None;

                if (MWBase::Environment::get().getWorld()->getPostProcessor()->enableTechnique(shader.mShader, pos)
                    == MWRender::PostProcessor::Status_Error)
                    throw std::runtime_error("Failed enabling shader '" + shader.mShader->getName() + "'");
            });
        };

        shader["disable"] = [context](Shader& shader) {
            shader.mQueuedAction = Shader::Action_Disable;

            context.mLuaManager->addAction([&] {
                shader.mQueuedAction = Shader::Action_None;

                if (MWBase::Environment::get().getWorld()->getPostProcessor()->disableTechnique(shader.mShader)
                    == MWRender::PostProcessor::Status_Error)
                    throw std::runtime_error("Failed disabling shader '" + shader.mShader->getName() + "'");
            });
        };

        shader["isEnabled"] = [](const Shader& shader) {
            if (shader.mQueuedAction == Shader::Action_Enable)
                return true;
            else if (shader.mQueuedAction == Shader::Action_Disable)
                return false;
            return MWBase::Environment::get().getWorld()->getPostProcessor()->isTechniqueEnabled(shader.mShader);
        };

        shader["name"] = sol::readonly_property(
            [](const Shader& shader) { return getLocalizedMyGUIString(shader.mShader->getName()); });
        shader["author"] = sol::readonly_property(
            [](const Shader& shader) { return getLocalizedMyGUIString(shader.mShader->getAuthor()); });
        shader["description"] = sol::readonly_property(
            [](const Shader& shader) { return getLocalizedMyGUIString(shader.mShader->getDescription()); });
        shader["version"] = sol::readonly_property(
            [](const Shader& shader) { return getLocalizedMyGUIString(shader.mShader->getVersion()); });

        shader["setBool"] = getSetter<bool>(context);
        shader["setFloat"] = getSetter<float>(context);
        shader["setInt"] = getSetter<int>(context);
        shader["setVector2"] = getSetter<osg::Vec2f>(context);
        shader["setVector3"] = getSetter<osg::Vec3f>(context);
        shader["setVector4"] = getSetter<osg::Vec4f>(context);

        shader["setFloatArray"] = getArraySetter<float>(context);
        shader["setIntArray"] = getArraySetter<int>(context);
        shader["setVector2Array"] = getArraySetter<osg::Vec2f>(context);
        shader["setVector3Array"] = getArraySetter<osg::Vec3f>(context);
        shader["setVector4Array"] = getArraySetter<osg::Vec4f>(context);

        api["load"] = [](const std::string& name) {
            Shader shader{ MWBase::Environment::get().getWorld()->getPostProcessor()->loadTechnique(name, false) };

            if (!shader.mShader || !shader.mShader->isValid())
                throw std::runtime_error(Misc::StringUtils::format("Failed loading shader '%s'", name));

            return shader;
        };

        api["getChain"] = [context]() {
            sol::table chain(context.sol(), sol::create);

            for (const auto& shader : MWBase::Environment::get().getWorld()->getPostProcessor()->getChain())
            {
                // Don't expose internal shaders to the API, they should be invisible to the user
                if (shader->getInternal())
                    continue;
                chain.add(Shader(shader));
            }

            return chain;
        };

        return LuaUtil::makeReadOnly(api);
    }

}
