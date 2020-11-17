#ifndef OPENMW_COMPONENTS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SHADERMANAGER_H

#include <string>
#include <map>
#include <mutex>

#include <osg/ref_ptr>

#include <osg/Shader>

#include <osgViewer/Viewer>

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
        osg::ref_ptr<osg::Shader> getShader(const std::string& templateName, const DefineMap& defines, osg::Shader::Type shaderType);

        osg::ref_ptr<osg::Program> getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader);

        /// Get (a copy of) the DefineMap used to construct all shaders
        DefineMap getGlobalDefines();

        /// Set the DefineMap used to construct all shaders
        /// @param defines The DefineMap to use
        /// @note This will change the source code for any shaders already created, potentially causing problems if they're being used to render a frame. It is recommended that any associated Viewers have their threading stopped while this function is running if any shaders are in use.
        void setGlobalDefines(DefineMap & globalDefines);

        void releaseGLObjects(osg::State* state);

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

        std::mutex mMutex;
    };

    bool parseFors(std::string& source, const std::string& templateName);

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines,
        const ShaderManager::DefineMap& globalDefines, const std::string& templateName);
}

#endif
