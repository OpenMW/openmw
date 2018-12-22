#ifndef OPENMW_COMPONENTS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SHADERMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>

#include <osg/Shader>

#include <OpenThreads/Mutex>

namespace Shader
{

    /// @brief Reads shader template files and turns them into a concrete shader, based on a list of define's.
    /// @par Shader templates can get the value of a define with the syntax @define.
    class ShaderManager
    {
    public:
        ShaderManager(): _lockglobaldefines(false) {}
        void setShaderPath(const std::string& path);

        typedef std::map<std::string, std::string> DefineMap;
        const DefineMap& getGlobalDefines() { return _globaldefines; }
        void setGlobalDefines(DefineMap&m) { if(!_lockglobaldefines) _globaldefines=m;else OSG_WARN<<"forbidden call to ShaderManager::setGlobalDefines during main loop"<<std::endl; }
        void lockGlobalDefines() { _lockglobaldefines=true; }
        /// Create or retrieve a shader instance.
        /// @param shaderTemplate The filename of the shader template.
        /// @param defines Define values that can be retrieved by the shader template.
        /// @param shaderType The type of shader (usually vertex or fragment shader).
        /// @note May return nullptr on failure.
        /// @note Thread safe.
        osg::ref_ptr<osg::Shader> getShader(const std::string& shaderTemplate, const DefineMap& defines, osg::Shader::Type shaderType);

        osg::ref_ptr<osg::Program> getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader);

        void releaseGLObjects(osg::State* state);

    private:
        std::string mPath;

        // <name, code>
        typedef std::map<std::string, std::string> TemplateMap;
        TemplateMap mShaderTemplates;

        typedef std::pair<std::string, DefineMap> MapKey;
        typedef std::map<MapKey, osg::ref_ptr<osg::Shader> > ShaderMap;
        ShaderMap mShaders;

        typedef std::map<std::pair<osg::ref_ptr<osg::Shader>, osg::ref_ptr<osg::Shader> >, osg::ref_ptr<osg::Program> > ProgramMap;
        ProgramMap mPrograms;

        OpenThreads::Mutex mMutex;

        DefineMap _globaldefines;
        bool _lockglobaldefines;
    };

}

#endif
