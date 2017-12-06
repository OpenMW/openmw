#include "shadermanager.hpp"

#include <iostream>
#include <regex>
#include <fstream>

#include <osg/Program>

#include <experimental/filesystem>

namespace Shader
{

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    bool parseIncludes(std::experimental::filesystem::path shaderPath, std::string& source)
    {
        std::string temp = "";
        std::regex to_replace("\r\n");
        std::regex_replace(std::back_inserter(temp), source.begin(), source.end(), to_replace, "\n");
        source = temp;

        std::set<std::experimental::filesystem::path> includedFiles;
        size_t foundPos = 0;
        int fileNumber = 1;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size()-1)
            {
                std::cerr << "Invalid #include " << std::endl;
                return false;
            }
            size_t end = source.find('"', start+1);
            if (end == std::string::npos)
            {
                std::cerr << "Invalid #include " << std::endl;
                return false;
            }
            std::string includeFilename = source.substr(start+1, end-(start+1));
            std::experimental::filesystem::path includePath = shaderPath / includeFilename;
            std::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                std::cerr << "Failed to open " << includePath.string() << std::endl;
                return false;
            }

            std::stringstream buffer;
            buffer << includeFstream.rdbuf();

            // insert #line directives so we get correct line numbers in compiler errors
            int includedFileNumber = fileNumber++;

            int lineNumber = std::count(source.begin(), source.begin() + foundPos, '\n');

            std::stringstream toInsert;
            toInsert << "#line 0 " << includedFileNumber << "\n" << buffer.str() << "\n#line " << lineNumber << " 0\n";

            source.replace(foundPos, (end-foundPos+1), toInsert.str());

            if (includedFiles.insert(includePath).second == false)
            {
                std::cerr << "Detected cyclic #includes" << std::endl;
                return false;
            }
        }
        return true;
    }

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines)
    {
        const char escapeCharacter = '@';
        size_t foundPos = 0;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;", foundPos);
            if (endPos == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string define = source.substr(foundPos+1, endPos - (foundPos+1));
            ShaderManager::DefineMap::const_iterator defineFound = defines.find(define);
            if (defineFound == defines.end())
            {
                std::cerr << "Undefined " << define << std::endl;
                return false;
            }
            else
            {
                source.replace(foundPos, endPos-foundPos, defineFound->second);
            }
        }
        return true;
    }

    osg::ref_ptr<osg::Shader> ShaderManager::getShader(const std::string &shaderTemplate, const ShaderManager::DefineMap &defines, osg::Shader::Type shaderType)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

        // read the template if we haven't already
        TemplateMap::iterator templateIt = mShaderTemplates.find(shaderTemplate);
        if (templateIt == mShaderTemplates.end())
        {
            std::experimental::filesystem::path p = (std::experimental::filesystem::path(mPath) / shaderTemplate);
            std::ifstream stream;
            stream.open(p);
            if (stream.fail())
            {
                std::cerr << "Failed to open " << p.string() << std::endl;
                return NULL;
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            // parse includes
            std::string source = buffer.str();
            if (!parseIncludes(std::experimental::filesystem::path(mPath), source))
                return NULL;

            templateIt = mShaderTemplates.insert(std::make_pair(shaderTemplate, source)).first;
        }

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(shaderTemplate, defines));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = templateIt->second;
            if (!parseDefines(shaderSource, defines))
            {
                // Add to the cache anyway to avoid logging the same error over and over.
                mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), nullptr));
                return NULL;
            }

            osg::ref_ptr<osg::Shader> shader (new osg::Shader(shaderType));
            shader->setShaderSource(shaderSource);
            // Assign a unique name to allow the SharedStateManager to compare shaders efficiently
            static unsigned int counter = 0;
            shader->setName(std::to_string(counter++));

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), shader)).first;
        }
        return shaderIt->second;
    }

    osg::ref_ptr<osg::Program> ShaderManager::getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        ProgramMap::iterator found = mPrograms.find(std::make_pair(vertexShader, fragmentShader));
        if (found == mPrograms.end())
        {
            osg::ref_ptr<osg::Program> program (new osg::Program);
            program->addShader(vertexShader);
            program->addShader(fragmentShader);
            found = mPrograms.insert(std::make_pair(std::make_pair(vertexShader, fragmentShader), program)).first;
        }
        return found->second;
    }

    void ShaderManager::releaseGLObjects(osg::State *state)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        for (auto shader : mShaders)
            shader.second->releaseGLObjects(state);
        for (auto program : mPrograms)
            program.second->releaseGLObjects(state);
    }

}
