#include "shadermanager.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

#include <osg/Program>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include <components/debug/debuglog.hpp>

namespace Shader
{

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    bool parseIncludes(boost::filesystem::path shaderPath, std::string& source)
    {
        boost::replace_all(source, "\r\n", "\n");

        std::set<boost::filesystem::path> includedFiles;
        size_t foundPos = 0;
        int fileNumber = 1;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size()-1)
            {
                Log(Debug::Error) << "Invalid #include";
                return false;
            }
            size_t end = source.find('"', start+1);
            if (end == std::string::npos)
            {
                Log(Debug::Error) << "Invalid #include";
                return false;
            }
            std::string includeFilename = source.substr(start+1, end-(start+1));
            boost::filesystem::path includePath = shaderPath / includeFilename;
            boost::filesystem::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                Log(Debug::Error) << "Failed to open " << includePath.string();
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
                Log(Debug::Error) << "Detected cyclic #includes";
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
                Log(Debug::Error) << "Unexpected EOF";
                return false;
            }
            std::string define = source.substr(foundPos+1, endPos - (foundPos+1));
            ShaderManager::DefineMap::const_iterator defineFound = defines.find(define);
            if (defineFound == defines.end())
            {
                Log(Debug::Error) << "Undefined " << define;
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
            boost::filesystem::path p = (boost::filesystem::path(mPath) / shaderTemplate);
            boost::filesystem::ifstream stream;
            stream.open(p);
            if (stream.fail())
            {
                Log(Debug::Error) << "Failed to open " << p.string();
                return nullptr;
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            // parse includes
            std::string source = buffer.str();
            if (!parseIncludes(boost::filesystem::path(mPath), source))
                return nullptr;

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
                return nullptr;
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
