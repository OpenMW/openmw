#ifndef OPENMW_COMPONENTS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SHADERMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>

#include <osg/Shader>

#include <osgViewer/Viewer>

#include <OpenThreads/Mutex>

namespace Shader
{

    /// @brief Reads shader template files and turns them into a concrete shader, based on a list of define's.
    /// @par Shader templates can get the value of a define with the syntax @define.
    class ShaderManager
    {
    public:
        void setShaderPath(const std::string& path);

        typedef std::map<std::string, std::string> DefineMap;

        /// Create or retrieve a shader instance.
        /// @param shaderTemplate The filename of the shader template.
        /// @param defines Define values that can be retrieved by the shader template.
        /// @param shaderType The type of shader (usually vertex or fragment shader).
        /// @note May return nullptr on failure.
        /// @note Thread safe.
        osg::ref_ptr<osg::Shader> getShader(const std::string& shaderTemplate, const DefineMap& defines, osg::Shader::Type shaderType);

        osg::ref_ptr<osg::Program> getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader);

        /// Get (a copy of) the DefineMap used to construct all shaders
        DefineMap getGlobalDefines();

        /// Set the DefineMap used to construct all shaders
        /// @param defines The DefineMap to use
        /// @note This will change the source code for any shaders already created, potentially causing problems if they're being used to render a frame. It is recommended that any associated Viewers have their threading stopped while this function is running if any shaders are in use.
        void setGlobalDefines(DefineMap & globalDefines);

        void releaseGLObjects(osg::State* state);

        const osg::ref_ptr<osg::Uniform> getShadowMapAlphaTestEnableUniform();
        const osg::ref_ptr<osg::Uniform> getShadowMapAlphaTestDisableUniform();

    private:
        std::string mPath;

        DefineMap mGlobalDefines;

        // <name, code>
        typedef std::map<std::string, std::string> TemplateMap;
        TemplateMap mShaderTemplates;

        typedef std::pair<std::string, DefineMap> MapKey;
        typedef std::map<MapKey, osg::ref_ptr<osg::Shader> > ShaderMap;
        ShaderMap mShaders;

        typedef std::map<std::pair<osg::ref_ptr<osg::Shader>, osg::ref_ptr<osg::Shader> >, osg::ref_ptr<osg::Program> > ProgramMap;
        ProgramMap mPrograms;

        OpenThreads::Mutex mMutex;

        const osg::ref_ptr<osg::Uniform> mShadowMapAlphaTestEnableUniform = new osg::Uniform();
        const osg::ref_ptr<osg::Uniform> mShadowMapAlphaTestDisableUniform = new osg::Uniform();
    };

}

#endif
