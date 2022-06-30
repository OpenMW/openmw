#ifndef OPENMW_COMPONENTS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SHADERMANAGER_H

#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <array>

#include <osg/ref_ptr>

#include <osg/Shader>
#include <osg/Program>

namespace Shader
{

    /// @brief Reads shader template files and turns them into a concrete shader, based on a list of define's.
    /// @par Shader templates can get the value of a define with the syntax @define.
    class ShaderManager
    {
    public:

        ShaderManager();

        void setShaderPath(const std::string& path);

        typedef std::map<std::string, std::string> DefineMap;

        /// Create or retrieve a shader instance.
        /// @param shaderTemplate The filename of the shader template.
        /// @param defines Define values that can be retrieved by the shader template.
        /// @param shaderType The type of shader (usually vertex or fragment shader).
        /// @note May return nullptr on failure.
        /// @note Thread safe.
        osg::ref_ptr<osg::Shader> getShader(const std::string& templateName, const DefineMap& defines, osg::Shader::Type shaderType);

        osg::ref_ptr<osg::Program> getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader, const osg::Program* programTemplate=nullptr);

        const osg::Program* getProgramTemplate() const { return mProgramTemplate; }
        void setProgramTemplate(const osg::Program* program) { mProgramTemplate = program; }

        /// Clone an osg::Program including bindUniformBlocks that osg::Program::clone does not copy for some reason.
        static osg::ref_ptr<osg::Program> cloneProgram(const osg::Program*);

        /// Get (a copy of) the DefineMap used to construct all shaders
        DefineMap getGlobalDefines();

        /// Set the DefineMap used to construct all shaders
        /// @param defines The DefineMap to use
        /// @note This will change the source code for any shaders already created, potentially causing problems if they're being used to render a frame. It is recommended that any associated Viewers have their threading stopped while this function is running if any shaders are in use.
        void setGlobalDefines(DefineMap & globalDefines);

        void releaseGLObjects(osg::State* state);

        bool createSourceFromTemplate(std::string& source, std::vector<std::string>& linkedShaderTemplateNames, const std::string& templateName, const ShaderManager::DefineMap& defines);

        void setMaxTextureUnits(int maxTextureUnits) { mMaxTextureUnits = maxTextureUnits; }
        int getMaxTextureUnits() const { return mMaxTextureUnits; }
        int getAvailableTextureUnits() const { return mMaxTextureUnits - mReservedTextureUnits; }

        enum class Slot
        {
            OpaqueDepthTexture,
            SkyTexture,
        };

        int reserveGlobalTextureUnits(Slot slot);

    private:
        void getLinkedShaders(osg::ref_ptr<osg::Shader> shader, const std::vector<std::string>& linkedShaderNames, const DefineMap& defines);
        void addLinkedShaders(osg::ref_ptr<osg::Shader> shader, osg::ref_ptr<osg::Program> program);

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

        typedef std::vector<osg::ref_ptr<osg::Shader> > ShaderList;
        typedef std::map<osg::ref_ptr<osg::Shader>, ShaderList> LinkedShadersMap;
        LinkedShadersMap mLinkedShaders;

        std::mutex mMutex;

        osg::ref_ptr<const osg::Program> mProgramTemplate;

        int mMaxTextureUnits = 0;
        int mReservedTextureUnits = 0;

        std::array<int, 2> mReservedTextureUnitsBySlot = {-1, -1};
    };

    bool parseForeachDirective(std::string& source, const std::string& templateName, size_t foundPos);
    bool parseLinkDirective(std::string& source, std::string& linkTarget, const std::string& templateName, size_t foundPos);

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines,
        const ShaderManager::DefineMap& globalDefines, const std::string& templateName);

    bool parseDirectives(std::string& source, std::vector<std::string>& linkedShaderTemplateNames, const ShaderManager::DefineMap& defines,
        const ShaderManager::DefineMap& globalDefines, const std::string& templateName);
}

#endif
