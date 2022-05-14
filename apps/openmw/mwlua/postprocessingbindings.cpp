#include "luabindings.hpp"

#include "../mwbase/environment.hpp"
#include "../mwrender/postprocessor.hpp"

#include "luamanagerimp.hpp"

namespace
{
    template <class T>
    class SetUniformShaderAction final : public MWLua::LuaManager::Action
    {
    public:
        SetUniformShaderAction(LuaUtil::LuaState* state, std::shared_ptr<fx::Technique> shader, const std::string& name, const T& value)
            : MWLua::LuaManager::Action(state), mShader(std::move(shader)), mName(name), mValue(value) {}

        void apply(MWLua::WorldView&) const override
        {
            MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(mShader, mName, mValue);
        }

        std::string toString() const override
        {
            return  std::string("SetUniformShaderAction shader=") + (mShader ? mShader->getName() : "nil") +
                    std::string("uniform=") + (mShader ? mName : "nil");
        }

    private:
        std::shared_ptr<fx::Technique> mShader;
        std::string mName;
        T mValue;
    };
}

namespace MWLua
{
    struct Shader;
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::Shader> : std::false_type {};
}

namespace MWLua
{
    struct Shader
    {
        std::shared_ptr<fx::Technique> mShader;

        Shader(std::shared_ptr<fx::Technique> shader) : mShader(std::move(shader)) {}

        std::string toString() const
        {
            if (!mShader)
                return "Shader(nil)";

            return Misc::StringUtils::format("Shader(%s, %s)", mShader->getName(), mShader->getFileName());
        }

        bool mQueuedAction = false;
    };

    sol::table initPostprocessingPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        sol::usertype<Shader> shader = context.mLua->sol().new_usertype<Shader>("Shader");
        shader[sol::meta_function::to_string] = [](const Shader& shader) { return shader.toString(); };

        shader["enable"] = [context](Shader& shader, sol::optional<int> optPos)
        {
            std::optional<int> pos = std::nullopt;
            if (optPos)
                pos = optPos.value();

            if (shader.mShader && shader.mShader->isValid())
                shader.mQueuedAction = true;

            context.mLuaManager->addAction(
                [&] { MWBase::Environment::get().getWorld()->getPostProcessor()->enableTechnique(shader.mShader, pos); },
                "Enable shader " + (shader.mShader ? shader.mShader->getName() : "nil")
            );
        };

        shader["disable"] = [context](Shader& shader)
        {
            shader.mQueuedAction = false;

            context.mLuaManager->addAction(
                [&] { MWBase::Environment::get().getWorld()->getPostProcessor()->disableTechnique(shader.mShader); },
                "Disable shader " + (shader.mShader ? shader.mShader->getName() : "nil")
            );
        };

        shader["isEnabled"] = [](const Shader& shader)
        {
            return shader.mQueuedAction;
        };

        shader["setBool"] = [context](const Shader& shader, const std::string& name, bool value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<bool>>(context.mLua, shader.mShader, name, value));
        };

        shader["setFloat"] = [context](const Shader& shader, const std::string& name, float value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<float>>(context.mLua, shader.mShader, name, value));
        };

        shader["setInt"] = [context](const Shader& shader, const std::string& name, int value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<int>>(context.mLua, shader.mShader, name, value));
        };

        shader["setVector2"] = [context](const Shader& shader, const std::string& name, const osg::Vec2f& value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec2f>>(context.mLua, shader.mShader, name, value));
        };

        shader["setVector3"] = [context](const Shader& shader, const std::string& name, const osg::Vec3f& value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec3f>>(context.mLua, shader.mShader, name, value));
        };

        shader["setVector4"] = [context](const Shader& shader, const std::string& name, const osg::Vec4f& value)
        {
            context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec4f>>(context.mLua, shader.mShader, name, value));
        };

        api["load"] = [](const std::string& name)
        {
            return Shader(MWBase::Environment::get().getWorld()->getPostProcessor()->loadTechnique(name, false));
        };

        return LuaUtil::makeReadOnly(api);
    }

}
